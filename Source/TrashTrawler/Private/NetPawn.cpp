#include "NetPawn.h"
#include "BoatPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NetItem.h"


ANetPawn::ANetPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	NetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetMesh"));
	SetRootComponent(NetMesh);
	NetMesh->SetSimulatePhysics(false); // enabled in InitializeNet
	NetMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	NetMesh->SetCollisionObjectType(ECC_WorldDynamic);
	NetMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	NetMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block); // only the seabed
	NetMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block); // only the seabed
	NetMesh->SetLinearDamping(NetLinearDamping);
	NetMesh->SetAngularDamping(NetAngularDamping);
	NetMesh->SetEnableGravity(false); // we drive sink with a custom force

	NetBottomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetBottomMesh"));
	NetBottomMesh->SetupAttachment(NetMesh);
	NetBottomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // purely visual
	NetBottomMesh->SetSimulatePhysics(false);
	NetBottomMesh->SetVisibility(false); // only visible when the net is bottomed

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(NetMesh);
	CameraBoom->SetUsingAbsoluteRotation(true); // position follows the net; rotation does NOT tumble
	CameraBoom->TargetArmLength = 900.f;
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 4.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	RopeAttachPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RopeAttachPoint"));
	RopeAttachPoint->SetupAttachment(NetMesh);
	RopeAttachPoint->SetRelativeLocation(FVector(0.f, 0.f, 30.f));
}

void ANetPawn::InitializeNet(ABoatPawn* InBoat)
{
	OwningBoat = InBoat;
	NetState = ENetState::Descending;
	NetBottomMesh->SetVisibility(false);
	LookYaw = GetActorRotation().Yaw; // face the way the boat was pointing
	bHasBeenFalling = false;
	SettleTimer = 0.f;

	NetMesh->SetSimulatePhysics(true);
}

void ANetPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (NetMappingContext) Sub->AddMappingContext(NetMappingContext, 0);
		}
	}
}

void ANetPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	if (auto* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ANetPawn::OnMove);
			EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &ANetPawn::OnMoveReleased);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ANetPawn::OnLook);
		}
	}
}

void ANetPawn::OnMove(const FInputActionValue& Value) { SteerInput = Value.Get<FVector2D>(); }
void ANetPawn::OnMoveReleased(const FInputActionValue& Value) { SteerInput = FVector2D::ZeroVector; }

void ANetPawn::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	LookYaw += Axis.X * LookYawSpeed * Dt;
	LookPitch = FMath::Clamp(LookPitch + Axis.Y * LookPitchSpeed * Dt, MinLookPitch, MaxLookPitch);

}

void ANetPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (NetState)
	{
	case ENetState::Descending: TickDescending(DeltaTime); break;
	case ENetState::Returning:  TickReturning(DeltaTime);  break;
	case ENetState::Bottomed:   TickBottomed(DeltaTime);   break;
	default: break;
	}

	// Camera holds look offset only — no tumble, ever.
	CameraBoom->SetWorldRotation(FRotator(LookPitch, LookYaw, 0.f));
}

void ANetPawn::TickDescending(float Dt)
{
	bool OnSeabed = false;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(HitResult, GetActorLocation(), GetActorLocation() - FVector(0.f, 0.f, 100.f), ECC_WorldStatic, Params))
	{
		//Hit seabed stop movement
		OnSeabed = true;
	}

	if (!OnSeabed)
	{
		// Steer horizontally relative to camera heading (negated — matches the earlier fix).
		const FRotator YawOnly(0.f, FollowCamera->GetComponentRotation().Yaw, 0.f);
		const FVector Fwd = YawOnly.Vector();
		const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
		const FVector Wish = (Fwd * SteerInput.X + Right * SteerInput.Y).GetClampedToMaxSize(1.f);

		// bAccelChange = true → mass-independent, so tuning doesn't depend on the mesh's mass.
		NetMesh->AddForce(Wish * SteerForce, NAME_None, true);
		NetMesh->AddForce(FVector(0.f, 0.f, -SinkAccel), NAME_None, true);
	}

	// Settle test: has it fallen, then gone quiet (both linear & angular) long enough?
	const FVector LinVel = NetMesh->GetPhysicsLinearVelocity();
	const FVector AngVel = NetMesh->GetPhysicsAngularVelocityInDegrees();

	if (LinVel.Z < -50.f) bHasBeenFalling = true; // guard against the spawn-frame false-positive

	if (bHasBeenFalling && LinVel.Size() < SettleLinSpeed && AngVel.Size() < SettleAngSpeed)
		SettleTimer += Dt;
	else
		SettleTimer = 0.f;

	if (SettleTimer >= SettleDwell)
	{
		NetMesh->SetSimulatePhysics(false); // hand back to kinematic for the return
		NetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // don't snag rising
		NetBottomMesh->SetVisibility(true);
		TryCollectItem();
		NetState = ENetState::Bottomed;
	}
}

void ANetPawn::TickReturning(float Dt)
{
	if (!OwningBoat) { Dock(); return; }

	const FVector Target = OwningBoat->GetNetDockLocation();
	const FVector ToTarget = Target - GetActorLocation();

	if (ToTarget.SizeSquared() <= FMath::Square(DockThreshold)) { Dock(); return; }

	AddActorWorldOffset(ToTarget.GetSafeNormal() * ReturnSpeed * Dt, false);

	// Level out from whatever tumbled orientation it settled in.
	const FRotator Upright(0.f, GetActorRotation().Yaw, 0.f);
	SetActorRotation(FMath::RInterpTo(GetActorRotation(), Upright, Dt, 4.f));
}

void ANetPawn::TickBottomed(float Dt)
{
	float BottomTime = TimeOnBottomCounter + Dt;

	if (BottomTime >= TimeOnBottom)
	{
		NetState = ENetState::Returning;
	}
	else
	{
		TimeOnBottomCounter = BottomTime;
	}
}

void ANetPawn::Dock()
{
	NetState = ENetState::Docked;

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		// Drop our context while we're still possessed, then hand control back.
		if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (NetMappingContext) Sub->RemoveMappingContext(NetMappingContext);
		}
		if (OwningBoat) PC->Possess(OwningBoat); // fires boat's NotifyControllerChanged → re-adds boat context
	}

	if (OwningBoat) OwningBoat->OnNetReturned(); // clears its ActiveNet ref
	for (ANetItem* Item : CaughtItems)
	{
		if (Item) Item->Destroy(); // item is now in the boat's inventory
	}
	Destroy();
}

void ANetPawn::TryCollectItem()
{
	if (CaughtItems.Num() > 0) return; // one per drop

	const FVector Center = GetActorLocation();
	const FVector Start = Center + FVector(0.f, 0.f, 50.f);
	const FVector End = Center - FVector(0.f, 0.f, PickupProbeLength);

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	TArray<FHitResult> ItemHits;
	if (GetWorld()->SweepMultiByChannel(ItemHits, Start, End, FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(200.f), Params))
	{
		for (const FHitResult& ItemHit : ItemHits)
		{
			if (ANetItem* Item = Cast<ANetItem>(ItemHit.GetActor()))
			{
				CaughtItems.Add(Item);
				Item->OnCollected(NetMesh); // attaches under the net, rides up on Returning
			}
		}
	}
}