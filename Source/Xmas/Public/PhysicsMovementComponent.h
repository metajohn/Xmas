// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class XMAS_API UPhysicsMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()
public:
	UPhysicsMovementComponent();

	// 1. Overriding the core movement pipeline functions
	virtual void CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration) override;
	virtual void ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration) override;

	// 2. Mass & Force Tuning Parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Mass & Forces")
	float CharacterMass = 80.0f; // Mass in kg (e.g. 80kg human, 400kg Spartan)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Mass & Forces")
	float BaseMovementForce = 500000.0f; // Input force in Newtons

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Momentum & Drag")
	float ForwardDrag = 2.0f; // Low forward resistance for momentum

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Momentum & Drag")
	float LateralGrip = 16.0f; // Sideways drag to prevent sliding like soap

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Momentum & Drag")
	float ActiveBrakingForce = 10.0f; // High stopping force when NO keys are pressed

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Custom Physics|Modes")
	bool bUseMassBasedPhysics = true;

};
