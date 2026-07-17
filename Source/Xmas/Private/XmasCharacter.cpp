#include "XmasCharacter.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameInteractable.h"
#include "InteractableComponent.h"
#include "XmasActor.h"

AXmasCharacter::AXmasCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create and position the camera
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(RootComponent);
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    FirstPersonCamera->bUsePawnControlRotation = true; // Camera rotates with mouse
}

void AXmasCharacter::BeginPlay()
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

void AXmasCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction,              ETriggerEvent::Triggered, this,     &AXmasCharacter::Move);
        EnhancedInputComponent->BindAction(LookAction,              ETriggerEvent::Triggered, this,     &AXmasCharacter::Look);
        EnhancedInputComponent->BindAction(JumpAction,              ETriggerEvent::Started, this,       &AXmasCharacter::Jump);
        EnhancedInputComponent->BindAction(JumpAction,              ETriggerEvent::Completed,this,      &AXmasCharacter::StopJumping);
        EnhancedInputComponent->BindAction(InteractAction,          ETriggerEvent::Started, this,       &AXmasCharacter::Interact);
        EnhancedInputComponent->BindAction(TogglePlacementAction,   ETriggerEvent::Started, this,       &AXmasCharacter::TogglePlacement);
        EnhancedInputComponent->BindAction(PrimaryAction,           ETriggerEvent::Started, this,       &AXmasCharacter::HandPrimary);
    }
}

void AXmasCharacter::Move(const FInputActionValue& Value)
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
        UE_LOG(LogTemp, Warning, TEXT("Movement Vector: %s"), *MovementVector.ToString());
    }
}

void AXmasCharacter::Look(const FInputActionValue& Value)
{
    FVector2D LookAxisVector = Value.Get<FVector2D>();

    if (Controller != nullptr)
    {
        AddControllerYawInput(LookAxisVector.X);
        AddControllerPitchInput(LookAxisVector.Y);
    }
}

void AXmasCharacter::Interact()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("Interacted!"));
        PerformInteractionCheck();
    }
}

void AXmasCharacter::HandPrimary()
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

void AXmasCharacter::TogglePlacement()
{
    bIsPlacementMode = !bIsPlacementMode;
    if (GEngine)
    {
        FString ModeMessage = bIsPlacementMode ? TEXT("Placement Mode: ACTIVE") : TEXT("Placement Mode: INACTIVE");
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, ModeMessage);

    }
    if (bIsPlacementMode)
    {
        GetWorldTimerManager().SetTimer(PlacementTimerHandle, this, &AXmasCharacter::PlacementPreviewTTick, 0.033f, true);
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

void AXmasCharacter::PlacementPreviewTTick()
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

void AXmasCharacter::PerformInteractionCheck()
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

void AXmasCharacter::Tick(float DeltaTime)
{
}

