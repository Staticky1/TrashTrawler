// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"


#include "TTOcean.generated.h"

class UStaticMeshComponent;
class UPostProcessComponent;
class UBoxComponent;

UCLASS()
class TRASHTRAWLER_API ATTOcean : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATTOcean();


	// The Mesh Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> OceanMeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPostProcessComponent> PostProcessComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Bounds")
	TObjectPtr<UBoxComponent> BoundsBox;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
