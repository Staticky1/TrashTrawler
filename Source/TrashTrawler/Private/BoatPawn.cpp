// Fill out your copyright notice in the Description page of Project Settings.


#include "BoatPawn.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "NetPawn.h"
#include "NetItem.h"

// Sets default values
ABoatPawn::ABoatPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	BoatHullMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatHullMesh"));
	SetRootComponent(BoatHullMesh);
	BoatHullMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // kinematic → query-only
	BoatHullMesh->SetCollisionObjectType(ECC_Pawn);
	BoatHullMesh->SetCollisionResponseToAllChannels(ECR_Overlap);
	BoatHullMesh->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BoatHullMesh->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BoatHullMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);

	BoatCraneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatCraneMesh"));
	BoatCraneMesh->SetupAttachment(BoatHullMesh);
	BoatRopeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatRopeMesh"));
	BoatRopeMesh->SetupAttachment(BoatHullMesh);
	BoatPropMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatPropMesh"));
	BoatPropMesh->SetupAttachment(BoatHullMesh);
	BoatRudderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoatRudderMesh"));
	BoatRudderMesh->SetupAttachment(BoatHullMesh);

	NetDropPoint = CreateDefaultSubobject<USceneComponent>(TEXT("NetDropPoint"));
	NetDropPoint->SetupAttachment(BoatCraneMesh);

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1800.f;
	CameraBoom->SetRelativeRotation(FRotator(-15.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = true;

	CameraBoom->bEnableCameraLag = true;
	CameraBoom->bEnableCameraRotationLag = true;
	CameraBoom->CameraLagSpeed = 3.f;
	CameraBoom->CameraRotationLagSpeed = 4.f;
	CameraBoom->SetUsingAbsoluteRotation(true);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

}

// Called when the game starts or when spawned
void ABoatPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABoatPawn::OnNetReturned()
{
	if (ActiveNet)
	{
		const TArray<ANetItem*> CaughtItems = ActiveNet->GetCaughtItems();
		for (ANetItem* CaughtItem : CaughtItems)
		{
			if (CaughtItem)
			{
				AddPoints(CaughtItem->GetPointValue());
			}
		}
	}

	ActiveNet = nullptr;


}

void ABoatPawn::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();
	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			if (BoatMappingContext) Sub->AddMappingContext(BoatMappingContext, 0);
		}
	}
}

// Called every frame
void ABoatPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// --- 1. Rudder: slew smoothly toward the input-driven target angle ---
	const float TargetRudder = SteerInput * MaxRudderAngle;
	CurrentRudderAngle = FMath::FInterpConstantTo(
		CurrentRudderAngle, TargetRudder, DeltaTime, RudderSlewSpeed);

	// --- 2. Speed: integrate toward throttle target, with passive drag ---
	const float TargetSpeed = (ThrottleInput >= 0.f)
		? ThrottleInput * MaxForwardSpeed
		: ThrottleInput * MaxReverseSpeed;

	if (!FMath::IsNearlyZero(ThrottleInput))
	{
		CurrentSpeed = FMath::FInterpConstantTo(
			CurrentSpeed, TargetSpeed, DeltaTime, ThrottleAcceleration);
	}
	else
	{
		// Off-throttle: bleed speed toward zero.
		CurrentSpeed = FMath::FInterpConstantTo(
			CurrentSpeed, 0.f, DeltaTime, WaterDrag);
	}

	// --- 3. Yaw: rudder authority scales with (signed) speed ---
	// Signed speed makes steering invert automatically in reverse.
	const float SpeedRatio = CurrentSpeed / MaxForwardSpeed; // signed
	const float RudderRatio = (MaxRudderAngle > 0.f) ? (CurrentRudderAngle / MaxRudderAngle) : 0.f;
	const float YawThisFrame = RudderRatio * SpeedRatio * MaxTurnRate * DeltaTime;

	if (!FMath::IsNearlyZero(YawThisFrame))
	{
		AddActorWorldRotation(FRotator(0.f, YawThisFrame, 0.f));
	}

	// --- 4. Translate along forward vector, then pin to ocean height ---
	const float Yaw = GetActorRotation().Yaw;
	const FVector FlatForward = FRotator(0.f, Yaw, 0.f).Vector();
	const FVector Delta = FlatForward * CurrentSpeed * DeltaTime;

	FHitResult Hit;
	AddActorWorldOffset(Delta, true, &Hit);

	if (Hit.bBlockingHit)
	{
		// Slide the remaining distance along the surface instead of dead-stopping.
		const FVector Remaining = Delta * (1.f - Hit.Time);
		const FVector Slide = FVector::VectorPlaneProject(Remaining, Hit.Normal);
		AddActorWorldOffset(Slide, true);

		// Bleed speed by how head-on the hit was: glancing scrape keeps momentum,
		// square-on impact kills most of it.
		const float HeadOn = FVector::DotProduct(GetActorForwardVector(), -Hit.Normal);
		CurrentSpeed *= FMath::Clamp(1.f - HeadOn, 0.1f, 1.f);
	}

	// --- 5. Cosmetic mesh animation ---
	if (BoatRudderMesh)
	{
		BoatRudderMesh->SetRelativeRotation(FRotator(0.f, -CurrentRudderAngle, 0.f));
	}
	if (BoatPropMesh)
	{
		PropSpinAngle = FMath::Fmod(PropSpinAngle + CurrentSpeed * PropSpinRate * DeltaTime, 360.f);
		BoatPropMesh->SetRelativeRotation(FRotator(0.f, 0.f, PropSpinAngle));
	}

	// --- 7. Cosmetic lean (unchanged) ---
	const float Accel = (CurrentSpeed - PrevSpeed) / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);
	PrevSpeed = CurrentSpeed;

	const float YawRate = (DeltaTime > 0.f) ? (YawThisFrame / DeltaTime) : 0.f;
	const float TargetRoll = FMath::Clamp(YawRate / MaxTurnRate, -1.f, 1.f) * MaxTurnRoll;
	const float TargetPitch = FMath::Clamp(Accel / ThrottleAcceleration, -1.f, 1.f) * MaxAccelPitch;

	CurrentRoll = FMath::FInterpTo(CurrentRoll, TargetRoll, DeltaTime, LeanInterpSpeed);
	CurrentPitch = FMath::FInterpTo(CurrentPitch, TargetPitch, DeltaTime, LeanInterpSpeed / 4);

	// --- 8. Idle bob: layered sines, faded out as the boat speeds up ---
	BobTime += DeltaTime * BobSpeed;

	// 1 when stationary → 0 near full speed, so bob yields to real motion.
	const float BobFade = 1.f - FMath::Clamp(FMath::Abs(CurrentSpeed) / MaxForwardSpeed, 0.f, 1.f);

	// Different frequencies/phases per axis so it never looks obviously periodic.
	const float BobPitch = FMath::Sin(BobTime) * BobPitchAmplitude * BobFade;
	const float BobRoll = FMath::Sin(BobTime * 0.7f + 1.3f) * BobRollAmplitude * BobFade;
	const float BobHeave = FMath::Sin(BobTime * 0.9f + 0.5f) * BobHeaveAmplitude * BobFade;

	// --- 9. Compose final transform: lean + bob layered together ---
	SetActorRotation(FRotator(CurrentPitch + BobPitch, Yaw, CurrentRoll + BobRoll));

	FVector Loc = GetActorLocation();
	Loc.Z = OceanZ + BobHeave; // heave rides on top of the ocean pin
	SetActorLocation(Loc);

	const float BoatYaw = GetActorRotation().Yaw;
	CameraBoom->SetWorldRotation(FRotator(LookPitch, BoatYaw + LookYaw, 0.f));


}

// Called to bind functionality to input
void ABoatPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (ThrottleAction)
		{
			EIC->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ABoatPawn::OnThrottle);
			EIC->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ABoatPawn::OnThrottleReleased);
		}
		if (SteerAction)
		{
			EIC->BindAction(SteerAction, ETriggerEvent::Triggered, this, &ABoatPawn::OnSteer);
			EIC->BindAction(SteerAction, ETriggerEvent::Completed, this, &ABoatPawn::OnSteerReleased);
		}
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABoatPawn::OnLook);
		}
		if (DeployNetAction)
		{
			EIC->BindAction(DeployNetAction, ETriggerEvent::Started, this, &ABoatPawn::OnDeployNet);
		}
	}

}

void ABoatPawn::OnDeployNet()
{
	if (ActiveNet || !NetPawnClass) return; // already out, or misconfigured

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC) return;

	ActiveNet = GetWorld()->SpawnActor<ANetPawn>(
		NetPawnClass, NetDropPoint->GetComponentTransform());
	if (!ActiveNet) { return; }

	ActiveNet->InitializeNet(this);

	// Hand off: drop the boat's context so it can't fight the net's, then possess.
	if (auto* Sub = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
	{
		if (BoatMappingContext) Sub->RemoveMappingContext(BoatMappingContext);
	}
	PC->Possess(ActiveNet); // fires ANetPawn::NotifyControllerChanged → adds the net's context

}

void ABoatPawn::OnThrottle(const FInputActionValue& Value)
{
	ThrottleInput = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
}

void ABoatPawn::OnThrottleReleased(const FInputActionValue& Value)
{
	ThrottleInput = 0.f; // coast; drag brings it down
}

void ABoatPawn::OnSteer(const FInputActionValue& Value)
{
	SteerInput = FMath::Clamp(Value.Get<float>(), -1.f, 1.f);
}

void ABoatPawn::OnSteerReleased(const FInputActionValue& Value)
{
	SteerInput = 0.f; // rudder returns to center
}

void ABoatPawn::OnLook(const FInputActionValue& Value)
{
	const FVector2D Axis = Value.Get<FVector2D>();
	const float Dt = GetWorld()->GetDeltaSeconds();
	LookYaw += Axis.X * LookYawSpeed * Dt;
	LookPitch = FMath::Clamp(LookPitch + Axis.Y * LookPitchSpeed * Dt, MinLookPitch, MaxLookPitch);

}