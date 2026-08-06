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

	HealthComponent = GetOwner() ? GetOwner()->FindComponentByClass<UHealthComponent>() : nullptr;
	if (HealthComponent)
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
	
	const FString Msg = FString::Printf(TEXT("Downed Player %s"), *GetOwner()->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, *Msg);
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	
}

void UDownableComponent::Revive()
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bIsDowned)
	{
		return;
	}

	bIsDowned = false;
	OnDownedStateChanged.Broadcast(bIsDowned);
	const FString Msg = FString::Printf(TEXT("Revived Player %s"), *GetOwner()->GetName());
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, *Msg);
	UE_LOG(LogTemp, Log, TEXT("%s"), *Msg);
	
	if (HealthComponent)
	{
		HealthComponent->Heal(1.0);
	}
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