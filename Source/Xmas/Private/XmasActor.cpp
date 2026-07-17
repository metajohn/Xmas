// Fill out your copyright notice in the Description page of Project Settings.


#include "XmasActor.h"

// Sets default values
AXmasActor::AXmasActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	PropMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PropMeshComponent"));
	RootComponent = PropMeshComponent;
	PropMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PropMeshComponent->SetCollisionResponseToAllChannels(ECR_Overlap);

}

// Called when the game starts or when spawned
void AXmasActor::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AXmasActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AXmasActor::PlaceProp()
{
	if (PropMeshComponent)
	{
		PropMeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PropMeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	}
}

