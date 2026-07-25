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

UENUM()
enum class ENetState : uint8 { Descending, Returning, Docked };

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

private:
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USphereComponent> NetRoot;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> NetMesh;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	UPROPERTY(VisibleAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputMappingContext> NetMappingContext;
	UPROPERTY(EditAnywhere, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UInputAction> MoveAction; // 2D: steer while sinking

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Net", meta = (AllowPrivateAccess = "true"))
	float SinkSpeed = 400.f;
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

	ENetState NetState = ENetState::Docked;
	UPROPERTY(Transient) TObjectPtr<ABoatPawn> OwningBoat;
	FVector2D SteerInput = FVector2D::ZeroVector;
	FVector HorizontalVel = FVector::ZeroVector;

	void OnMove(const FInputActionValue& Value);
	void OnMoveReleased(const FInputActionValue& Value);
	void TickDescending(float Dt);
	void TickReturning(float Dt);
	void Dock();
};