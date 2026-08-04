// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PBPlayerCharacter.h"
#include "GameFramework/Character.h"
#include "XmasCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

class AXmasActor;

UCLASS()
class XMAS_API AXmasCharacter : public APBPlayerCharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AXmasCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//movement
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
public:
	virtual void Jump() override;

	// Input Callbacks
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	void Interact();
	void HandPrimary();

	//PLACEMENT
protected:
	void TogglePlacement();
	
	bool bIsPlacementMode = false;

	FTimerHandle PlacementTimerHandle;

	void PlacementPreviewTick();

	// Server-only bookkeeping: the actor currently being previewed/placed by this character.
	// Not replicated — only the server (or a listen-server host, which is also the server for
	// its own pawn) ever needs this pointer. Every other client just sees the actor itself,
	// since it's a real replicated actor rather than a per-client cosmetic.
	UPROPERTY()
	AXmasActor* ActivePreviewActor;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	TSubclassOf<class AXmasActor> PropToSpawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Placement")
	float MaxPlacementDistance = 1000.f;

	// The preview ghost must be visible to every player, so it's spawned and moved by the
	// server rather than as a client-local cosmetic. The owning client drives it by tracing
	// locally (it's the only one with an up-to-date camera) and relaying the result here.
	UFUNCTION(Server, Reliable)
	void ServerBeginPreview();

	UFUNCTION(Server, Reliable)
	void ServerEndPreview();

	UFUNCTION(Server, Unreliable)
	void ServerUpdatePreviewLocation(FVector Location);

	UFUNCTION(Server, Reliable)
	void ServerCommitPlacement();

	// Does the actual trace-and-invoke. Only ever called from ServerInteract_Implementation, so
	// it always runs with authority — trusts the server's own knowledge of this character's
	// position/view rather than anything supplied by the client.
	void PerformInteractionCheck();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionDistance = 250.f;

	// Client calls this to request an interaction; the server performs its own trace rather than
	// trusting a client-reported hit actor, so a client can't force-trigger an interact on an
	// arbitrary actor it doesn't actually have line-of-sight/range on.
	UFUNCTION(Server, Reliable)
	void ServerInteract();

public:	

	// First-Person Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;

//INPUT
public:
	// Enhanced Input Assets (assigned in the Blueprint child later)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* TogglePlacementAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
	UInputAction* PrimaryAction;
};
