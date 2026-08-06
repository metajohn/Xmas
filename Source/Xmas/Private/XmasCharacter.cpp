#include "XmasCharacter.h"

#include "DownableComponent.h"
#include "HealthComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "GameInteractable.h"
#include "XmasActor.h"
#include "XmasPlacementComponent.h"

AXmasCharacter::AXmasCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    // Create and position the camera
    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(RootComponent);
    FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
    FirstPersonCamera->bUsePawnControlRotation = true; // Camera rotates with mouse
    
    JumpMaxCount = 1;
    
    HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));
    DownableComponent = CreateDefaultSubobject<UDownableComponent>(TEXT("DownableComponent"));
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

void AXmasCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Prevents an orphaned ghost if the owning player disconnects mid-preview (ActivePreviewActor
    // is server-only bookkeeping, so this only ever does something meaningful on the server).
    if (ActivePreviewActor)
    {
        ActivePreviewActor->Destroy();
        ActivePreviewActor = nullptr;
    }

    Super::EndPlay(EndPlayReason);
}

void AXmasCharacter::Jump()
{
    // you need this in addition to the constructor maxjump = 1 to prevent the pb jumping in air to cause auto-jump
    if (GetCharacterMovement() && GetCharacterMovement()->IsMovingOnGround())
    {
        Super::Jump();
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
    //constant state check so that getting revived always corrects movement
    if (DownableComponent && DownableComponent->IsDowned())
    {
        return;
    }
    
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
    }

    ServerInteract();
}

void AXmasCharacter::ServerInteract_Implementation()
{
    PerformInteractionCheck();
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
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, HitResult.GetActor()->GetName());
        AActor* HitActor = HitResult.GetActor();

        if (HitActor->Implements<UGameInteractable>())
        {
            IGameInteractable::Execute_Interact(HitActor, this);
        }
        else if (UActorComponent* InteractableComp = HitActor->FindComponentByInterface(UGameInteractable::StaticClass()))
        {
            IGameInteractable::Execute_Interact(InteractableComp, this);
        }
    }
}

void AXmasCharacter::HandPrimary()
{
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("Primary Action Started"));
    }
    if (bIsPlacementMode)
    {
        ServerCommitPlacement();

        bIsPlacementMode = false;
        GetWorldTimerManager().ClearTimer(PlacementTimerHandle);
    }
}

void AXmasCharacter::ServerBeginPreview_Implementation()
{
    if (!PropToSpawnClass || !GetWorld() || ActivePreviewActor)
    {
        return;
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParams.Owner = this;

    ActivePreviewActor = GetWorld()->SpawnActor<AXmasActor>(PropToSpawnClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
}

void AXmasCharacter::ServerEndPreview_Implementation()
{
    if (ActivePreviewActor)
    {
        ActivePreviewActor->Destroy();
        ActivePreviewActor = nullptr;
    }
}

void AXmasCharacter::ServerUpdatePreviewLocation_Implementation(FVector Location)
{
    if (!ActivePreviewActor)
    {
        return;
    }

    if (FVector::Dist(GetActorLocation(), Location) > MaxPlacementDistance)
    {
        return;
    }

    ActivePreviewActor->SetActorLocation(Location);
}

void AXmasCharacter::ServerCommitPlacement_Implementation()
{
    if (!ActivePreviewActor)
    {
        return;
    }

    if (UXmasPlacementComponent* PlacementComponent = ActivePreviewActor->FindComponentByClass<UXmasPlacementComponent>())
    {
        PlacementComponent->PlaceProp();
    }

    ActivePreviewActor = nullptr;
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
        GetWorldTimerManager().SetTimer(PlacementTimerHandle, this, &AXmasCharacter::PlacementPreviewTick, 0.033f, true);
        ServerBeginPreview();
    }
    else
    {
        GetWorldTimerManager().ClearTimer(PlacementTimerHandle);
        ServerEndPreview();
    }
}

void AXmasCharacter::PlacementPreviewTick()
{
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC || !PC->PlayerCameraManager) return;

    FVector StartLocation = PC->PlayerCameraManager->GetCameraLocation();
    FVector ForwardVector = PC->PlayerCameraManager->GetCameraRotation().Vector();

    FVector EndLocation = StartLocation + (ForwardVector * MaxPlacementDistance);

    // The preview actor's mesh is Overlap-only (see AXmasActor's default collision setup), and a
    // single-channel trace only reports blocking hits, so the ghost can never hit itself here.
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

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
        ServerUpdatePreviewLocation(HitResult.ImpactPoint);
        DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 15.f, 8, FColor::Blue, false, 0.04f);
    }
}

void AXmasCharacter::Tick(float DeltaTime)
{

}

