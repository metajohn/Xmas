#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChangedSignature, float, NewHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthDepletedSignature, AActor*, DamageInstigator);

// Drop-in ActorComponent giving its owning actor server-authoritative health: ApplyDamage/Heal
// only ever mutate CurrentHealth on the server, which replicates it to every client via OnRep for
// UI/feedback. OnHealthDepleted fires once, server-only, on the 0-health transition — components
// like UDownableComponent bind to it rather than polling, since it's an edge (took fatal damage),
// not a level of ongoing state.
UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class XMAS_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Health")
	float MaxHealth = 100.f;

	// Server-only: reduces CurrentHealth, clamped to [0, MaxHealth]. No effect without authority,
	// with a non-positive amount, or once already depleted.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount, AActor* DamageInstigator);

	// Server-only: raises CurrentHealth, clamped to [0, MaxHealth]. No effect without authority or
	// with a non-positive amount.
	UFUNCTION(BlueprintCallable, Category = "Health")
	void Heal(float HealAmount);

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDepleted() const { return CurrentHealth <= 0.f; }

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthDepletedSignature OnHealthDepleted;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Health")
	float CurrentHealth = 0.f;

	UFUNCTION()
	void OnRep_CurrentHealth();
};