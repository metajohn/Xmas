#include "DownableComponent.h"
#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

UDownableComponent::UDownableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UDownableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UDownableComponent, bIsDowned);
}

void UDownableComponent::BeginPlay()
{
	Super::BeginPlay();

	if (UHealthComponent* HealthComponent = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr)
	{
		HealthComponent->OnHealthDepleted.AddDynamic(this, &UDownableComponent::HandleHealthDepleted);
	}
}

void UDownableComponent::HandleHealthDepleted(AActor* DamageInstigator)
{
	TriggerDown();
}

void UDownableComponent::TriggerDown()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || bIsDowned)
	{
		return;
	}

	bIsDowned = true;
	OnDownedStateChanged.Broadcast(bIsDowned);
}

void UDownableComponent::Revive()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsDowned)
	{
		return;
	}

	bIsDowned = false;
	OnDownedStateChanged.Broadcast(bIsDowned);
}

void UDownableComponent::OnRep_IsDowned()
{
	OnDownedStateChanged.Broadcast(bIsDowned);
}

void UDownableComponent::Interact_Implementation(AActor* Interactor)
{
	if (bIsDowned)
	{
		Revive();
	}
}