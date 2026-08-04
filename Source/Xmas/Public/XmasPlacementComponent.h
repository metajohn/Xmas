// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "XmasPlacementComponent.generated.h"

// Drop-in ActorComponent that gives its owning actor a server-authoritative "placed" state:
// starts unplaced, and once PlaceProp() is called with authority, locks the owner's root
// primitive into solid, blocking collision and replicates that state to every client via OnRep.
// Attach to whichever actor needs to be lockable by the placement system, mirroring
// UInteractableComponent's drop-in pattern rather than baking this into a specific actor class.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XMAS_API UXmasPlacementComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UXmasPlacementComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only: locks the owning actor's root primitive into place. Has no effect without authority.
	void PlaceProp();

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Placed, BlueprintReadOnly, Category = "Placement")
	bool bPlaced = false;

	UFUNCTION()
	void OnRep_Placed();

	void ApplyPlacedState();
};