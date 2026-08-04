// Fill out your copyright notice in the Description page of Project Settings.

#include "XmasPlacementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/PrimitiveComponent.h"

UXmasPlacementComponent::UXmasPlacementComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UXmasPlacementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UXmasPlacementComponent, bPlaced);
}

void UXmasPlacementComponent::PlaceProp()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	bPlaced = true;
	ApplyPlacedState();
}

void UXmasPlacementComponent::OnRep_Placed()
{
	ApplyPlacedState();
}

void UXmasPlacementComponent::ApplyPlacedState()
{
	if (!bPlaced || !GetOwner())
	{
		return;
	}

	if (UPrimitiveComponent* RootPrimitive = Cast<UPrimitiveComponent>(GetOwner()->GetRootComponent()))
	{
		RootPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		RootPrimitive->SetCollisionResponseToAllChannels(ECR_Block);
	}
}