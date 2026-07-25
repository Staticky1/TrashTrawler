// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NetItem.generated.h"

class UStaticMeshComponent;

UCLASS()
class TRASHTRAWLER_API ANetItem : public AActor
{
	GENERATED_BODY()
	
public:
	ANetItem();

	// Called by the net when it scoops this item up.
	void OnCollected(USceneComponent* AttachTo);

	int32 GetPointValue() const { return PointsWorth; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> ItemMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "NetItem", meta = (AllowPrivateAccess = "true"))
	int32 PointsWorth = 1; // how many points this item is worth when collected

};
