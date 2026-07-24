// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"


#include "TTOcean.generated.h"

class UDynamicMeshComponent;

UCLASS()
class TRASHTRAWLER_API ATTOcean : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATTOcean();

	// The Dynamic Mesh Component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> OceanMesh;

	/** Verts per side. 256 => ~131k tris. */
	UPROPERTY(EditAnywhere) int32 Resolution = 256;
	/** Half-extent of the sheet in cm. 5,000,000 = 50 km. */
	UPROPERTY(EditAnywhere) float Extent = 5000000.f;
	/** >1 biases density toward the camera. 3-4 works well. */
	UPROPERTY(EditAnywhere) float FalloffExponent = 3.5f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void BuildMesh();
	float CenterCellSize() const;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
