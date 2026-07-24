// Fill out your copyright notice in the Description page of Project Settings.


#include "Enviroment/TTOcean.h"
#include "Components/DynamicMeshComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/MeshNormals.h"
#include "Kismet/GameplayStatics.h"

using namespace UE::Geometry;

// Sets default values
ATTOcean::ATTOcean()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostUpdateWork;

	// 1. Create the Dynamic Mesh Component
	OceanMesh = CreateDefaultSubobject<UDynamicMeshComponent>(TEXT("DynamicMeshComponent"));
	SetRootComponent(OceanMesh);
	OceanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	OceanMesh->SetTangentsType(EDynamicMeshComponentTangentsMode::AutoCalculated);
	OceanMesh->SetBoundsScale(10.f); // room for WPO displacement
    OceanMesh->SetMobility(EComponentMobility::Movable);
    OceanMesh->bUseAsOccluder = false;   // it shouldn't occlude anything
    OceanMesh->bNeverDistanceCull = true;


}

// Called when the game starts or when spawned
void ATTOcean::BeginPlay()
{
	Super::BeginPlay();
    UE_LOG(LogTemp, Warning, TEXT("Ocean mobility: %d (2 == Movable)"),
        (int32)OceanMesh->Mobility);
	BuildMesh();


}

static FORCEINLINE float Warp(float T, float Exp, float Ext)
{
	// T in [-1,1] -> warped world offset
	return FMath::Sign(T) * FMath::Pow(FMath::Abs(T), Exp) * Ext;
}

// Called every frame
void ATTOcean::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    const APlayerCameraManager* CM =
        UGameplayStatics::GetPlayerCameraManager(this, 0);
    if (!CM) return;

    const FVector Cam = CM->GetCameraLocation();
    const float Snap = FMath::Max(CenterCellSize(), 1.f);

    float SeaLevelZ = GetActorLocation().Z;

    SetActorLocation(FVector(Cam.X,Cam.Y,SeaLevelZ));

}

void ATTOcean::BuildMesh()
{
    FDynamicMesh3 Mesh;
    Mesh.EnableAttributes();
    Mesh.Attributes()->SetNumUVLayers(1);
    FDynamicMeshUVOverlay* UVs = Mesh.Attributes()->GetUVLayer(0);
    FDynamicMeshNormalOverlay* Normals = Mesh.Attributes()->PrimaryNormals();

    const int32 N = Resolution;
    TArray<int32> VIDs, UVIDs, NIDs;
    VIDs.Reserve((N + 1) * (N + 1));

    for (int32 Y = 0; Y <= N; ++Y)
    {
        const float TY = (2.f * Y / N) - 1.f;
        for (int32 X = 0; X <= N; ++X)
        {
            const float TX = (2.f * X / N) - 1.f;
            const FVector3d P(Warp(TX, FalloffExponent, Extent),
                Warp(TY, FalloffExponent, Extent), 0.0);
            const int32 VID = Mesh.AppendVertex(P);
            VIDs.Add(VID);
            // UVs are placeholders — sample the material from absolute world position instead
            UVIDs.Add(UVs->AppendElement(FVector2f(TX, TY)));
            NIDs.Add(Normals->AppendElement(FVector3f::UnitZ()));
        }
    }

    auto Idx = [N](int32 X, int32 Y) { return Y * (N + 1) + X; };

    for (int32 Y = 0; Y < N; ++Y)
        for (int32 X = 0; X < N; ++X)
        {
            const int32 A = Idx(X, Y), B = Idx(X + 1, Y);
            const int32 C = Idx(X + 1, Y + 1), D = Idx(X, Y + 1);

            const int32 T0 = Mesh.AppendTriangle(VIDs[A], VIDs[B], VIDs[C]);
            const int32 T1 = Mesh.AppendTriangle(VIDs[A], VIDs[C], VIDs[D]);
            if (T0 >= 0) {
                UVs->SetTriangle(T0, FIndex3i(UVIDs[A], UVIDs[B], UVIDs[C]));
                Normals->SetTriangle(T0, FIndex3i(NIDs[A], NIDs[B], NIDs[C]));
            }
            if (T1 >= 0) {
                UVs->SetTriangle(T1, FIndex3i(UVIDs[A], UVIDs[C], UVIDs[D]));
                Normals->SetTriangle(T1, FIndex3i(NIDs[A], NIDs[C], NIDs[D]));
            }
        }
    Mesh.ReverseOrientation(true);
    OceanMesh->SetMesh(MoveTemp(Mesh));
    OceanMesh->NotifyMeshUpdated();
}

float ATTOcean::CenterCellSize() const
{
    const float T = 2.f / Resolution;          // first step away from 0
    return Warp(T, FalloffExponent, Extent);
}

