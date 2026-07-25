#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputActionValue.h"
#include "NetPawn.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class ABoatPawn;
class ANetItem;

UENUM()
enum class ENetState : uint8 { Descending, Returning, Docked, Bottomed };

UCLASS()
class TRASHTRAWLER_API ANetPawn : public APawn
{
	GENERATED_BODY()

public:
	ANetPawn();
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void NotifyControllerChanged() override;

	void InitializeNet(ABoatPawn* InBoat); // called by boat right after spawn

	const TArray<ANetItem*>& GetCaughtItems() const { return CaughtItems; }



private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NetMesh;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NetBottomMesh;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> NetMappingContext;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction; // 2D: steer while sinking
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> LookAction; // 2D: mouse XY / right stick

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float LookYawSpeed = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float LookPitchSpeed = 100.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float MinLookPitch = -75.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float MaxLookPitch = -5.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> RopeAttachPoint;

public:
	FVector GetRopeAttachLocation() const { return RopeAttachPoint->GetComponentLocation(); }

private:

	// Physics descent tuning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float SinkAccel = 600.f;    // downward accel (cm/s^2); terminal speed ≈ SinkAccel / LinearDamping
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float SteerForce = 900.f;   // horizontal steering accel
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float NetLinearDamping = 1.5f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float NetAngularDamping = 0.6f;

	// Settle detection (net has come to rest on the seabed)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float SettleLinSpeed = 50.f;   // cm/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float SettleAngSpeed = 30.f;   // deg/s
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net|Physics", meta = (AllowPrivateAccess = "true"))
	float SettleDwell = 0.4f;      // must stay quiet this long

	float SettleTimer = 0.f;
	bool  bHasBeenFalling = false;

	// Look now accumulates (camera boom is absolute-rotation so the tumble can't reach it)
	float LookYaw = 0.f;
	float LookPitch = -40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float ReturnSpeed = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float SteerAccel = 900.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float MaxSteerSpeed = 450.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float SteerDrag = 700.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float FloorProbeLength = 60.f; // floor considered "hit" within this distance below
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float DockThreshold = 120.f;   // this close to dock point → docked
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float TimeOnBottom = 1.5f;      // how long to stay on bottom before returning

	float TimeOnBottomCounter = 0.f;

	ENetState NetState = ENetState::Docked;
	UPROPERTY(Transient) TObjectPtr<ABoatPawn> OwningBoat;
	FVector2D SteerInput = FVector2D::ZeroVector;
	FVector HorizontalVel = FVector::ZeroVector;

	void OnMove(const FInputActionValue& Value);
	void OnLook(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value);
	void TickDescending(float Dt);
	void TickReturning(float Dt);
	void TickBottomed(float Dt);
	void Dock();


	UPROPERTY(Transient)
	TArray<TObjectPtr<ANetItem>> CaughtItems;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float PickupProbeLength = 120.f; // how far below the net to look for an item

	void TryCollectItem();
};