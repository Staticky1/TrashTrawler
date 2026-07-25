// Fill out your copyright notice in the Description page of Project Settings.


#include "NetItem.h"
#include "Components/StaticMeshComponent.h"

ANetItem::ANetItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	// Query-only, and only visible to the pickup trace channel — nothing physically
	// collides with it, so the net falls straight past it.
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block); // "Pickup"
}

void ANetItem::OnCollected(USceneComponent* AttachTo)
{
	// Stop responding to further traces, then ride up with the net from where it sits.
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	AttachToComponent(AttachTo, FAttachmentTransformRules::KeepWorldTransform);
}