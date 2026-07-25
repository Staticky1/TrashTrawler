// Fill out your copyright notice in the Description page of Project Settings.


#include "Enviroment/TTOcean.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/BoxComponent.h"

// Sets default values
ATTOcean::ATTOcean()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	// 1. Create the Dynamic Mesh Component
	OceanMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OceanDiscMesh"));
	SetRootComponent(OceanMeshComponent);
	OceanMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("UnderwaterBoundsBox"));
	BoundsBox->SetBoxExtent(FVector(5000, 5000, 10000));
	BoundsBox->SetupAttachment(RootComponent);
	BoundsBox->AddLocalOffset(FVector(0, 0, -10001));
	
	PostProcessComp = CreateDefaultSubobject<UPostProcessComponent>(TEXT("PostProcessComp"));
	PostProcessComp->SetupAttachment(BoundsBox);
	PostProcessComp->bUnbound = false;
	
}

// Called when the game starts or when spawned
void ATTOcean::BeginPlay()
{
	Super::BeginPlay();

}

// Called every frame
void ATTOcean::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    const APlayerCameraManager* CM =
        UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!CM) return;

    const FVector Cam = CM->GetCameraLocation();
    float SeaLevelZ = GetActorLocation().Z;

    SetActorLocation(FVector(Cam.X,Cam.Y,SeaLevelZ));

}

