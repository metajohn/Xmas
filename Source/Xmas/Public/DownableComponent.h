#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameInteractable.h"
#include "DownableComponent.generated.h"

class UHealthComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDownedStateChangedSignature, bool, bIsDowned);

// Drop-in ActorComponent giving its owning actor a server-authoritative "downed" flag, replicated
// via OnRep. State only for now (no bleed-out timer, no movement/incapacitation) — that behavior
// belongs wherever the owner already handles movement/animation, once this flag exists to drive it.
//
// If the owner also has a UHealthComponent, downing happens automatically on that component's
// OnHealthDepleted. TriggerDown()/Revive() are also exposed directly for owners without health
// (or for a scripted/manual down).
//
// Implements IGameInteractable directly so a downed actor is revivable through the same interact
// trace (AXmasCharacter::PerformInteractionCheck) already used for every other interactable —
// no separate revive input/system.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XMAS_API UDownableComponent : public UActorComponent, public IGameInteractable
{
	GENERATED_BODY()

public:
	UDownableComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Server-only: puts the owner into the downed state. No effect without authority or if already downed.
	UFUNCTION(BlueprintCallable, Category = "Downed")
	void TriggerDown();

	// Server-only: clears the downed state. No effect without authority or if not downed. Does not
	// touch health — pair with a UHealthComponent::Heal call wherever revive is triggered if the
	// revived actor shouldn't be left at 0 health.
	UFUNCTION(BlueprintCallable, Category = "Downed")
	void Revive();

	UFUNCTION(BlueprintPure, Category = "Downed")
	bool IsDowned() const { return bIsDowned; }

	virtual void Interact_Implementation(AActor* Interactor) override;

	UPROPERTY(BlueprintAssignable, Category = "Downed")
	FOnDownedStateChangedSignature OnDownedStateChanged;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_IsDowned, BlueprintReadOnly, Category = "Downed")
	bool bIsDowned = false;

	UFUNCTION()
	void OnRep_IsDowned();

	UFUNCTION()
	void HandleHealthDepleted(AActor* DamageInstigator);
	
	UPROPERTY()
	UHealthComponent* HealthComponent;
};