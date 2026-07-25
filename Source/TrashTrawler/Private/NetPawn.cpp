#include "NetPawn.h"
#include "BoatPawn.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

ANetPawn::ANetPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	NetRoot = CreateDefaultSubobject<USphereComponent>(TEXT("NetRoot"));
	SetRootComponent(NetRoot);
	NetRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision); // floor detected via trace

	NetMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("NetMesh"));
	NetMesh->SetupAttachment(NetRoot);
	NetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(NetRoot);
	CameraBoom->TargetArmLength = 900.f;
	CameraBoom->SetRelativeRotation(FRotator(-40.f, 0.f, 0.f)); // look down-ahead at the net
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 4.f;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
}

void ANetPawn::InitializeNet(ABoatPawn* InBoat)
{
	OwningBoat = InBoat;
	NetState = ENetState::Descending;
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
	}
}

void ANetPawn::OnMove(const FInputActionValue& Value) { SteerInput = Value.Get<FVector2D>(); }
void ANetPawn::OnMoveReleased(const FInputActionValue& Value) { SteerInput = FVector2D::ZeroVector; }

void ANetPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	switch (NetState)
	{
	case ENetState::Descending: TickDescending(DeltaTime); break;
	case ENetState::Returning:  TickReturning(DeltaTime);  break;
	default: break;
	}
}

void ANetPawn::TickDescending(float Dt)
{
	// Steer horizontally, relative to the camera's heading so "up" = away from camera.
	const FRotator YawOnly(0.f, FollowCamera->GetComponentRotation().Yaw, 0.f);
	const FVector Fwd = YawOnly.Vector();
	const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);
	const FVector Wish = (Fwd * SteerInput.Y + Right * SteerInput.X).GetClampedToMaxSize(1.f);

	if (!Wish.IsNearlyZero())
		HorizontalVel = (HorizontalVel + Wish * SteerAccel * Dt).GetClampedToMaxSize(MaxSteerSpeed);
	else
		HorizontalVel = FMath::VInterpConstantTo(HorizontalVel, FVector::ZeroVector, Dt, SteerDrag);

	FVector Delta = HorizontalVel * Dt;
	Delta.Z = -SinkSpeed * Dt; // sink
	AddActorWorldOffset(Delta, false);

	// Probe straight down; when the floor is within reach, land and turn around.
	const FVector Start = GetActorLocation();
	const FVector End = Start - FVector(0.f, 0.f, FloorProbeLength);
	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_WorldStatic, Params))
	{
		SetActorLocation(Hit.ImpactPoint); // rest on the floor
		HorizontalVel = FVector::ZeroVector;
		NetState = ENetState::Returning;
	}
}

void ANetPawn::TickReturning(float Dt)
{
	if (!OwningBoat) { Dock(); return; } // boat gone → bail cleanly

	const FVector Target = OwningBoat->GetNetDockLocation(); // queried live — boat may bob/drift
	const FVector ToTarget = Target - GetActorLocation();

	if (ToTarget.SizeSquared() <= FMath::Square(DockThreshold)) { Dock(); return; }

	const FVector Step = ToTarget.GetSafeNormal() * ReturnSpeed * Dt;
	AddActorWorldOffset(Step, false);
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
	Destroy();
}