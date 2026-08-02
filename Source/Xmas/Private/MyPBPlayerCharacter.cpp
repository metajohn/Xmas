// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPBPlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

#include "GameInteractable.h"
#include "InteractableComponent.h"
#include "PhysicsMovementComponent.h"
#include "XmasActor.h"

AMyPBPlayerCharacter::AMyPBPlayerCharacter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and position the camera
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(RootComponent);
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    FirstPersonCamera->bUsePawnControlRotation = true; // Camera rotates with mouse

    // Unreal jump() overlaps with pb jumping -> jumping in air effectively toggles auto-jump ON permanently
    // hardcoding this value prevents that
    JumpMaxCount = 1;

    // this may be redundant as it was not obvious what value was capping speedz
    IConsoleVariable* CVarMaxSpeed = IConsoleManager::Get().FindConsoleVariable(TEXT("sv_maxspeed"));
    if (CVarMaxSpeed)
    {
        CVarMaxSpeed->Set(50000.0f, ECVF_SetByCode); // Default is usually 320.0 (in HL units)
    }
}

void AMyPBPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();

    // Register the input context with our controller
    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }
}

void AMyPBPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMyPBPlayerCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMyPBPlayerCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AMyPBPlayerCharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AMyPBPlayerCharacter::StopJumping);
        EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AMyPBPlayerCharacter::Interact);
        EnhancedInputComponent->BindAction(TogglePlacementAction, ETriggerEvent::Started, this, &AMyPBPlayerCharacter::TogglePlacement);
        EnhancedInputComponent->BindAction(PrimaryAction, ETriggerEvent::Started, this, &AMyPBPlayerCharacter::HandPrimary);
    }
}

void AMyPBPlayerCharacter::Move(const FInputActionValue& Value)
{
    FVector2D MovementVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        const FRotator Rotation = Controller->GetControlRotation();
        const FRotator YawRotation(0, Rotation.Yaw, 0);

        const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
        const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

        AddMovementInput(ForwardDirection, MovementVector.Y);
        AddMovementInput(RightDirection, MovementVector.X);
    }
    if (GetCharacterMovement())
    {
        FString DebugMsg = FString::Printf(TEXT("Input: (%.2f, %.2f) | MaxSpeed: %.1f | CurrentVelocity: %.1f"),
            MovementVector.X, MovementVector.Y,
            GetCharacterMovement()->GetMaxSpeed(),
            GetCharacterMovement()->Velocity.Size());

        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Cyan, DebugMsg);
    }
}

void AMyPBPlayerCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AMyPBPlayerCharacter::Interact()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Interacted!"));
        PerformInteractionCheck();
    }
}

void AMyPBPlayerCharacter::HandPrimary()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Primary Action Started"));
    }
    if (bIsPlacementMode && ActivePreviewActor)
    {
        ActivePreviewActor->PlaceProp();
        ActivePreviewActor = nullptr;
        bIsPlacementMode = false;
    }
}

void AMyPBPlayerCharacter::TogglePlacement()
{
    bIsPlacementMode = !bIsPlacementMode;
    if (GEngine)
    {
        FString ModeMessage = bIsPlacementMode ? TEXT("Placement Mode: ACTIVE") : TEXT("Placement Mode: INACTIVE");
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, ModeMessage);

    }
    if (bIsPlacementMode)
    {
        GetWorldTimerManager().SetTimer(PlacementTimerHandle, this, &AMyPBPlayerCharacter::PlacementPreviewTick, 0.033f, true);
        if (GetWorld())
        {
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

            if (PropToSpawnClass)
            {
                ActivePreviewActor = GetWorld()->SpawnActor<AXmasActor>(PropToSpawnClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
            }

        }
    }
    else
    {
        GetWorldTimerManager().ClearTimer(PlacementTimerHandle);
        if (ActivePreviewActor)
        {
            ActivePreviewActor->Destroy();
            ActivePreviewActor = nullptr;
        }
    }
}

void AMyPBPlayerCharacter::PlacementPreviewTick()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->PlayerCameraManager) return;

    FVector StartLocation = PC->PlayerCameraManager->GetCameraLocation();
    FVector ForwardVector = PC->PlayerCameraManager->GetCameraRotation().Vector();

    FVector EndLocation = StartLocation + (ForwardVector * 1000.f);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);
    if (ActivePreviewActor)
    {
        QueryParams.AddIgnoredActor(ActivePreviewActor);
    }

    FHitResult HitResult;
    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        StartLocation,
        EndLocation,
        ECC_Visibility,
        QueryParams
    );

    FColor LineColor = bHit ? FColor::Green : FColor::Red;

    DrawDebugLine(
        GetWorld(),
        StartLocation,
        bHit ? HitResult.ImpactPoint : EndLocation,
        LineColor,
        false,
        0.04f,
        0,
        0.1f
    );

    if (bHit)
    {
        if (ActivePreviewActor)
        {
            ActivePreviewActor->SetActorLocation(HitResult.ImpactPoint);
        }
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 15.f, 8, FColor::Blue, false, 0.04f);
    }
}

void AMyPBPlayerCharacter::PerformInteractionCheck()
{
    if (!GetWorld() || !GetController()) return;

    // get players point of view
    FVector CameraLocation;
    FRotator CameraRotation;
    GetController()->GetPlayerViewPoint(CameraLocation, CameraRotation);

    // Calculate the start and end of raycast
    FVector TraceStart = CameraLocation;
    FVector TraceEnd = TraceStart + (CameraRotation.Vector() * InteractionDistance);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    FHitResult HitResult;

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        TraceStart,
        TraceEnd,
        ECC_Visibility,
        QueryParams
    );

    DrawDebugLine(GetWorld(), TraceStart, TraceEnd, FColor::Green, false, 1.f, 0, 1.f);

    if (bHit && HitResult.GetActor())
    {
        AActor* HitActor = HitResult.GetActor();

        if (HitActor->Implements<UGameInteractable>())
        {
            IGameInteractable::Execute_Interact(HitActor, this);
        }
        else
        {
            UActorComponent* InteractableComp = HitActor->GetComponentByClass(UInteractableComponent::StaticClass());
            if (InteractableComp && InteractableComp->Implements<UGameInteractable>())
            {
                IGameInteractable::Execute_Interact(InteractableComp, this);
            }
        }
    }
}

void AMyPBPlayerCharacter::Tick(float DeltaTime)
{

}

void AMyPBPlayerCharacter::Jump()
{
    // Only allow Jump() to process if we are currently on the ground!
    if (GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround())
    {
        Super::Jump();
    }
}

