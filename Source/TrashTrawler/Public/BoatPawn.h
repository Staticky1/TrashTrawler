// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "BoatPawn.generated.h"

class UBoxComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ANetPawn;

UCLASS()
class TRASHTRAWLER_API ABoatPawn : public APawn
{
	GENERATED_BODY()

public:
	// Sets default values for this pawn's properties
	ABoatPawn();


	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoatHullMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoatCraneMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoatRopeMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoatPropMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BoatRudderMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	// Enhanced Input
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> BoatMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> ThrottleAction; // 1D axis: W = +1, S = -1

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> SteerAction;    // 1D axis: D = +1, A = -1

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction; // 2D: mouse XY / right stick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (AllowPrivateAccess = "true"))
	float LookYawSpeed = 100.f;   // deg/s at full input

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (AllowPrivateAccess = "true"))
	float LookPitchSpeed = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (AllowPrivateAccess = "true"))
	float MinLookPitch = -70.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Camera", meta = (AllowPrivateAccess = "true"))
	float MaxLookPitch = 20.f;

	// Tuning (cm/s, deg, deg/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Movement", meta = (AllowPrivateAccess = "true"))
	float MaxForwardSpeed = 1600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Movement", meta = (AllowPrivateAccess = "true"))
	float MaxReverseSpeed = 500.f;

	// How aggressively speed chases the throttle target.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Movement", meta = (AllowPrivateAccess = "true"))
	float ThrottleAcceleration = 350.f;

	// Passive water drag pulling speed to zero when off-throttle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Movement", meta = (AllowPrivateAccess = "true"))
	float WaterDrag = 250.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (AllowPrivateAccess = "true"))
	float MaxRudderAngle = 35.f;

	// How fast the rudder swings toward its target angle.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (AllowPrivateAccess = "true"))
	float RudderSlewSpeed = 90.f;

	// Yaw rate (deg/s) at full rudder AND full forward speed.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Steering", meta = (AllowPrivateAccess = "true"))
	float MaxTurnRate = 28.f;

	// Locked ocean surface height.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|World", meta = (AllowPrivateAccess = "true"))
	float OceanZ = 0.f;

	// Visual prop spin speed multiplier (deg per cm travelled).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Visual", meta = (AllowPrivateAccess = "true"))
	float PropSpinRate = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Lean", meta = (AllowPrivateAccess = "true"))
	float MaxTurnRoll = 12.f;    // deg of bank at full turn

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Lean", meta = (AllowPrivateAccess = "true"))
	float MaxAccelPitch = 6.f;   // deg bow-up under throttle

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Lean", meta = (AllowPrivateAccess = "true"))
	float LeanInterpSpeed = 3.f; // how fast roll/pitch chase their targets

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Bob", meta = (AllowPrivateAccess = "true"))
	float BobPitchAmplitude = 1.5f;  // deg

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Bob", meta = (AllowPrivateAccess = "true"))
	float BobRollAmplitude = 2.0f;   // deg

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Bob", meta = (AllowPrivateAccess = "true"))
	float BobHeaveAmplitude = 8.0f;  // cm of vertical rise/fall

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boat|Bob", meta = (AllowPrivateAccess = "true"))
	float BobSpeed = 1.2f;           // overall rate

	// ---- Runtime state ----
	float ThrottleInput = 0.f;   // -1..1, from input
	float SteerInput = 0.f;      // -1..1, from input
	float CurrentSpeed = 0.f;    // signed cm/s
	float CurrentRudderAngle = 0.f; // deg, smoothed
	float PropSpinAngle = 0.f;   // accumulated deg
	float CurrentRoll = 0.f;
	float CurrentPitch = 0.f;
	float PrevSpeed = 0.f; // for accel this frame
	float BobTime = 0.f;
	float LookYaw = 0.f;
	float LookPitch = -15.f; // starting downward tilt

	// ---- Input handlers ----
	void OnThrottle(const FInputActionValue& Value);
	void OnThrottleReleased(const FInputActionValue& Value);
	void OnSteer(const FInputActionValue& Value);
	void OnSteerReleased(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);


	//Net

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> NetDropPoint; // where the net launches from / returns to

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Net", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<ANetPawn> NetPawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> DeployNetAction;

	UPROPERTY(Transient)
	TObjectPtr<ANetPawn> ActiveNet;

	void OnDeployNet();
public:
	// Called by the net so it knows where to come home to, and to clean up.
	FVector GetNetDockLocation() const { return NetDropPoint->GetComponentLocation(); }
	void OnNetReturned() { ActiveNet = nullptr; }

	virtual void NotifyControllerChanged() override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


};
