// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsMovementComponent.h"
#include "GameFramework/Character.h"

UPhysicsMovementComponent::UPhysicsMovementComponent()
{
	// Ensure mass isn't zero to avoid division by zero
	CharacterMass = FMath::Max(1.0f, CharacterMass);
}

void UPhysicsMovementComponent::CalcVelocity(float DeltaTime, float Friction, bool bFluid, float BrakingDeceleration)
{
	if (!bUseMassBasedPhysics)
	{
		Super::CalcVelocity(DeltaTime, Friction, bFluid, BrakingDeceleration);
		return;
	}

	// 1. GET USER INPUT (Check actual controller keys pressed)
	FVector InputDir = GetLastInputVector().GetSafeNormal();
	float InputMagnitude = GetLastInputVector().Size();
	bool bHasPlayerInput = !InputDir.IsNearlyZero();

	// 2. INPUT FORCE ACCELERATION (F = ma)
	if (bHasPlayerInput)
	{
		// Align input direction to slope if grounded
		if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
		{
			FVector FloorNormal = CurrentFloor.HitResult.Normal;
			InputDir = FVector::VectorPlaneProject(InputDir, FloorNormal).GetSafeNormal();
		}

		FVector InputForce = InputDir * InputMagnitude * BaseMovementForce;
		float CurrentSpeedInInputDir = FVector::DotProduct(Velocity, InputDir);
		float MaxSprintSpeed = 1000.0f; // Human top speed cap

		if (CurrentSpeedInInputDir < MaxSprintSpeed)
		{
			float ForceScale = FMath::Clamp(1.0f - (CurrentSpeedInInputDir / MaxSprintSpeed), 0.1f, 1.0f);
			FVector PhysicalAcceleration = (InputForce * ForceScale) / CharacterMass;
			Velocity += PhysicalAcceleration * DeltaTime;
		}
	}

	// 3. SLOPE PHYSICS (Grounded)
	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		FVector FloorNormal = CurrentFloor.HitResult.Normal;

		// Re-project velocity parallel to the slope face
		FVector SurfaceVelocity = FVector::VectorPlaneProject(Velocity, FloorNormal);
		if (!SurfaceVelocity.IsNearlyZero())
		{
			Velocity = SurfaceVelocity.GetSafeNormal() * Velocity.Size();
		}

		// Downhill gravity push
		FVector GravityDir = FVector(0.f, 0.f, -1.f);
		FVector SlopeDirection = FVector::VectorPlaneProject(GravityDir, FloorNormal).GetSafeNormal();
		float SlopeSteepness = 1.0f - FloorNormal.Z;

		if (SlopeSteepness > 0.03f)
		{
			Velocity += SlopeDirection * FMath::Abs(GetGravityZ()) * SlopeSteepness * DeltaTime;
		}
	}
}

void UPhysicsMovementComponent::ApplyVelocityBraking(float DeltaTime, float Friction, float BrakingDeceleration)
{
	if (!bUseMassBasedPhysics || !UpdatedComponent)
	{
		Super::ApplyVelocityBraking(DeltaTime, Friction, BrakingDeceleration);
		return;
	}

	bool bHasPlayerInput = !GetLastInputVector().IsNearlyZero();

	// FIX FOR INFINITE SLIDING: Hard-stop when moving slow and no keys are pressed
	if (!bHasPlayerInput && Velocity.SizeSquared() < 2500.0f) // Below ~50 u/s
	{
		Velocity = FVector::ZeroVector;
		return;
	}

	if (Velocity.IsNearlyZero()) return;

	FVector ForwardDir = UpdatedComponent->GetForwardVector();
	FVector RightDir = UpdatedComponent->GetRightVector();

	float ForwardSpeed = FVector::DotProduct(Velocity, ForwardDir);
	float SidewaysSpeed = FVector::DotProduct(Velocity, RightDir);

	// High stopping drag when keys released, low drag while holding keys
	float ActiveForwardDrag = bHasPlayerInput ? ForwardDrag : ActiveBrakingForce;
	float ActiveLateralGrip = bHasPlayerInput ? LateralGrip : (ActiveBrakingForce * 2.0f);

	ForwardSpeed *= FMath::Clamp(1.0f - (ActiveForwardDrag * DeltaTime), 0.0f, 1.0f);
	SidewaysSpeed *= FMath::Clamp(1.0f - (ActiveLateralGrip * DeltaTime), 0.0f, 1.0f);

	Velocity = (ForwardDir * ForwardSpeed) + (RightDir * SidewaysSpeed);
}
