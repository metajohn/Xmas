# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`Xmas` is an Unreal Engine 5.5 first-person/third-person C++ co-op game project. There is no separate build tool, package manager, or test runner outside of Unreal's own toolchain — this is a game codebase, not an app/service.

- Engine version: 5.5 (`EngineAssociation` in `Xmas.uproject`)
- Primary game module: `Source/Xmas` (target names `Xmas` and `XmasEditor`, see `Source/Xmas.Target.cs` / `Source/XmasEditor.Target.cs`)
- IDE: developed with JetBrains Rider (`.idea/`, `Plugins/Developer/RiderLink`); Visual Studio solution (`Xmas.sln`) is also generated. `Plugins/Developer/RiderLink/` is Rider's own editor-integration plugin — not game code.

## Build / run commands

There is no CLI test suite. Development happens by compiling the C++ module and iterating in the Unreal Editor.

- **Generate project files**: right-click `Xmas.uproject` → "Generate Visual Studio project files" (or use Rider, which regenerates automatically when opening the `.uproject`). Only needed when module/file *structure* changes (new files, new `.Build.cs` dependencies) — not needed for ordinary edits within existing files.
- **Build the editor target** (from a Visual Studio Developer prompt or Rider), using UnrealBuildTool directly:
  ```
  <UE_ROOT>\Engine\Build\BatchFiles\Build.bat XmasEditor Win64 Development -project="E:\Xmas\Xmas.uproject"
  ```
- **Open in editor**: launch `Xmas.uproject` (prompts to build if binaries are stale), or open `Xmas.sln` and run/debug the `XmasEditor` target.
- **Iterate**: Live Coding (Ctrl+Alt+F11 in editor, or hot-reload from the IDE) is fine for small, non-structural C++ changes while the editor is open. For anything touching a class's parent, its constructor's subobject setup, or component structure, do a full editor close + clean rebuild instead — hot reload has caused stale-CDO issues on exactly this kind of change during development so far.
- No unit/automation test targets are currently set up in this repo.

## Architecture

### Player character
`AXmasCharacter` (`XmasCharacter.h/.cpp`) is the single player character class, inheriting from `APBPlayerCharacter` (the `PBCharacterMovement` plugin's character class) for HL2-style movement (bunnyhopping, surfing, ramp sliding, etc.).

- A second, duplicate character class and a custom `UPhysicsMovementComponent` force/mass-based movement override used to exist. Both are obsolete: `AXmasCharacter` was reparented from plain `ACharacter` to `APBPlayerCharacter`, and the `SetDefaultSubobjectClass<UPhysicsMovementComponent>` override was removed from its constructor (not a legal override for PB's `UPBPlayerMovement`). If remnants of the second character class or `UPhysicsMovementComponent` are still in the tree, they're migration debris — confirm nothing references them (Class Viewer / Reference Viewer) before deleting, don't assume they're already gone.
- See `Plugins/PBCharacterMovement/README.md` for tuning cvars (`move.Pogo`, `move.Bunnyhopping`, `move.JumpBoost`, `move.AlwaysApplyFriction`, gravity/physics-material setup) before changing movement feel.
- `PBCharacterMovement` is vendored third-party code (declared as a dependency in `Xmas.uproject` and `Xmas.Build.cs`) — treat as external, extend via `AXmasCharacter` rather than modifying directly.

### Content / Blueprints
`Content/Xmas` — Blueprint assets. Enhanced Input assets (`UInputMappingContext`/`UInputAction` references) and gameplay-tunable properties are wired up on Blueprint subclasses (e.g. `BP_XmasCharacter`), not in C++ defaults — when tracing input/gameplay bugs, check the Blueprint, not just the C++ class. `GlobalDefaultGameMode` is `BP_XmasGameMode` (`Content/Xmas/BP_XmasGameMode.uasset`), configured in `Config/DefaultEngine.ini`.

### Interaction system
- `IGameInteractable` (`GameInteractable.h`) — `UINTERFACE` with one `BlueprintNativeEvent`: `Interact(AActor* Interactor)`.
- `UInteractableComponent` — drop-in `ActorComponent` implementation of `IGameInteractable` that broadcasts an `OnInteracted` Blueprint-assignable delegate. Attach to any actor to make it interactable without writing an actor subclass.
- Character interaction flow (`PerformInteractionCheck`): line-trace along the camera's view (`InteractionDistance`), check if the hit actor itself implements `IGameInteractable`, and if not, look for a `UInteractableComponent` on it instead.

### Placement system (current, prop-level)
`TogglePlacement` / `PlacementPreviewTick` / `HandPrimary`:
1. `TogglePlacement` spawns a preview `AXmasActor` from `PropToSpawnClass` and starts a 0.033s (~30Hz) timer.
2. `PlacementPreviewTick` line-traces from the camera and moves the preview actor to the hit point each tick.
3. `HandPrimary` (bound to the primary-action input), while in placement mode, calls `AXmasActor::PlaceProp()` to switch the actor's mesh collision from query-only/overlap to full query-and-physics/block, "locking it in," and exits placement mode.

`AXmasActor` is the base placeable prop actor (`PropMeshComponent` as root `UStaticMeshComponent`). Not yet networked, no ghost/ownership-lock behavior, no surface constraints — treat as a single-player prototype of the interaction, not a finished system.

### Enhanced Input
All input is bound via the Enhanced Input plugin in `SetupPlayerInputComponent`. Input assets (`UInputMappingContext`, `UInputAction*` for move/look/jump/interact/toggle-placement/primary) are `EditAnywhere` properties left null in C++ and assigned per-Blueprint-subclass in the editor — a missing binding is usually a Blueprint content issue, not a C++ one.

## Conventions

- No data-oriented design / ECS. Standard Actors/Components throughout — don't introduce Mass Entity, HISM-based batching, or similar unless explicitly asked; current scope doesn't need it.
- For any looping/ongoing state (ambient effects, zone tracking, anything with a "start" and a "stop"), prefer polling/re-deriving current state on a tick or timer over mutating state via paired Begin/End events. Paired-event state has caused bugs in past projects where an End event got dropped and state leaked indefinitely.
- Networking is not yet implemented. When it is, replication needs to be designed in from the start for whatever's being built — flag this rather than building single-player-only logic if a task looks like it'll need multiplayer support soon.