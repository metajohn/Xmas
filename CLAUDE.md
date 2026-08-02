# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`Xmas` is an Unreal Engine 5.5 first-person C++ game project. There is no separate build tool, package manager, or test runner outside of Unreal's own toolchain — this is a game codebase, not an app/service.

- Engine version: 5.5 (`EngineAssociation` in `Xmas.uproject`)
- Primary game module: `Source/Xmas` (target names `Xmas` and `XmasEditor`, see `Source/Xmas.Target.cs` / `Source/XmasEditor.Target.cs`)
- IDE: developed with JetBrains Rider (`.idea/`, `Plugins/Developer/RiderLink`); Visual Studio solution (`Xmas.sln`) is also generated

## Build / run commands

There is no CLI test suite. Development happens by compiling the C++ module and iterating in the Unreal Editor.

- **Generate project files**: right-click `Xmas.uproject` → "Generate Visual Studio project files" (or use Rider, which regenerates automatically when opening the `.uproject`).
- **Build the editor target** (from a Visual Studio Developer prompt or Rider), using UnrealBuildTool directly:
  ```
  <UE_ROOT>\Engine\Build\BatchFiles\Build.bat XmasEditor Win64 Development -project="E:\Xmas\Xmas.uproject"
  ```
- **Open in editor**: launch `Xmas.uproject` (prompts to build if binaries are stale), or open `Xmas.sln` and run/debug the `XmasEditor` target.
- **Iterate**: Unreal's Live Coding (Ctrl+Alt+F11 in editor, or hot-reload from the IDE) is the normal loop for C++ changes while the editor is open.
- No unit/automation test targets are currently set up in this repo.

## Architecture

### Module/plugin layout
- `Source/Xmas` — the game's own gameplay code (see classes below).
- `Plugins/PBCharacterMovement` — vendored third-party plugin ("Project Borealis" movement) providing `APBPlayerCharacter` and HL2-style FPS movement (bunnyhopping, surfing, ramp sliding, etc.). Treat this as external code; see `Plugins/PBCharacterMovement/README.md` for its tuning cvars (`move.Pogo`, `move.Bunnyhopping`, `move.JumpBoost`, `move.AlwaysApplyFriction`, gravity/physics-material setup) before changing movement feel. It's declared as a dependency in `Xmas.uproject` and `Xmas.Build.cs`.
- `Content/Xmas` — Blueprint assets. Enhanced Input assets (`UInputMappingContext`/`UInputAction` references) and gameplay-tunable properties are wired up on Blueprint subclasses (e.g. `BP_XmasCharacter`, `BP_PB`), not in C++ defaults — when tracing input/gameplay bugs, check the Blueprint, not just the C++ class.
- `GlobalDefaultGameMode` is `BP_XmasGameMode` (`Content/Xmas/BP_XmasGameMode.uasset`), configured in `Config/DefaultEngine.ini`.

### Two parallel character implementations
The codebase currently has two player character classes with substantially duplicated logic (movement input binding, interaction trace, placement-mode preview). This is mid-migration, not intentional architecture:
- `AXmasCharacter` (`XmasCharacter.h/.cpp`) — plain `ACharacter` using the project's own `UPhysicsMovementComponent` (mass/force-based custom physics movement, set as the character movement component via `SetDefaultSubobjectClass` in the constructor).
- `AMyPBPlayerCharacter` (`MyPBPlayerCharacter.h/.cpp`) — subclasses `APBPlayerCharacter` from the `PBCharacterMovement` plugin instead, to get HL2-style movement. It overrides `Jump()` to only allow jumping while grounded (Unreal's `Jump()` otherwise conflicts with PB's own jump handling and can permanently toggle auto-jump on).

When fixing a bug in shared behavior (movement input, interaction, placement), check whether it needs to be fixed in both classes, and prefer consolidating logic rather than adding a third copy.

### Custom physics movement (`UPhysicsMovementComponent`)
Overrides `UCharacterMovementComponent::CalcVelocity` / `ApplyVelocityBraking` to implement force-based movement (`F = ma` using `CharacterMass`/`BaseMovementForce`), slope alignment/downhill sliding, and directional drag/braking (`ForwardDrag`, `LateralGrip`, `ActiveBrakingForce`). Only used by `AXmasCharacter`; `AMyPBPlayerCharacter` uses the plugin's own movement component instead. Gated by `bUseMassBasedPhysics` (falls back to stock `Super::` behavior when false).

### Interaction system
- `IGameInteractable` (`GameInteractable.h`) — `UINTERFACE` with one `BlueprintNativeEvent`: `Interact(AActor* Interactor)`.
- `UInteractableComponent` — drop-in `ActorComponent` implementation of `IGameInteractable` that broadcasts a `OnInteracted` Blueprint-assignable delegate. Attach this to any actor to make it interactable without writing an actor subclass.
- Character interaction flow (`PerformInteractionCheck`, duplicated in both character classes): line-trace along the camera's view (`InteractionDistance`), then check if the hit actor itself implements `IGameInteractable`, and if not, look for an `UInteractableComponent` on it and interact with that instead.

### Placement system
`TogglePlacement` / `PlacementPreviewTick` / `HandPrimary` implement a build-mode flow (also duplicated across both character classes):
1. `TogglePlacement` spawns a preview `AXmasActor` from `PropToSpawnClass` and starts a 0.033s (~30Hz) timer.
2. `PlacementPreviewTick` line-traces from the camera and moves the preview actor to the hit point each tick.
3. `HandPrimary` (bound to the primary-action input), while in placement mode, calls `AXmasActor::PlaceProp()` to switch the actor's mesh collision from query-only/overlap to full query-and-physics/block, "locking it in," and exits placement mode.

`AXmasActor` is the base placeable prop actor (`PropMeshComponent` as root `UStaticMeshComponent`).

### Enhanced Input
All input is bound via the Enhanced Input plugin in `SetupPlayerInputComponent`. Input assets (`UInputMappingContext`, `UInputAction*` for move/look/jump/interact/toggle-placement/primary) are `EditAnywhere` properties left null in C++ and assigned per-Blueprint-subclass in the editor — a missing binding is usually a Blueprint content issue, not a C++ one.