# Leo the Wizard Cat

Leo the Wizard Cat is a C++/SFML 2D platformer built around a custom Entity Component System (ECS) engine. The player controls Leo, a wizard cat who fights slimes with elemental spells, activates campfire checkpoints, moves through layered parallax maps, and switches between fire, water, wind, and rock abilities.

This project was originally developed as a two-person course project by **Peter Davidson** and **Eric Jiang**. The current CMake executable target is named `ecsp1`, but the game itself is **Leo the Wizard Cat**.

## Table of Contents

- [Features](#features)
- [Controls](#controls)
- [Project Structure](#project-structure)
- [Architecture Overview](#architecture-overview)
- [Requirements](#requirements)
- [Build and Run](#build-and-run)
- [Generating Documentation](#generating-documentation)
- [Debugging Notes](#debugging-notes)
- [Known Notes](#known-notes)
- [Future Improvements](#future-improvements)
- [Credits](#credits)

## Features

- 2D platformer gameplay built with **C++20** and **SFML 3**
- Custom ECS engine with:
  - reusable entity IDs
  - vector-backed component storage
  - entity-to-component index mapping
  - ordered system execution
  - scene stack support for menu, gameplay, and pause states
- Main menu, gameplay scene, and pause overlay
- Map-driven world loading from `.map` files
- Atlas-based sprite loading and rendering
- Parallax background layers for visual depth
- GLSL fragment shaders for hurt flashes and rock-shield effects
- Player movement with acceleration, deceleration, jumping, fast-fall, wall interactions, and spell-based movement modifiers
- Four spell elements:
  - **Fire**: offensive projectile and campfire/checkpoint interaction
  - **Water**: water interaction and healing behavior
  - **Wind**: double-jump ability
  - **Rock**: wall-jump support and one-hit shield behavior
- Projectile lifecycle management with spell-specific animations, sounds, collision checks, explosions, and lifetime cleanup
- Slime enemies with movement, HP, damage flash, death behavior, resistances, and sound effects
- HP system with invulnerability timers, death state, shield behavior, and heart UI sprites
- Checkpoint/campfire system for respawning after death
- Animation system for player, enemies, spells, explosions, campfires, and UI sprites
- Non-blocking sound playback through a `SoundManager`
- Doxygen-generated API documentation in `docs/html/`

## Controls

### Menu and Pause

| Key | Action |
|---|---|
| `Enter` | Start the game from the main menu |
| `Esc` | Pause during gameplay |
| `Enter` or `Esc` | Resume from pause |

### Gameplay

| Key | Action |
|---|---|
| `A` | Move left |
| `D` | Move right |
| `W` or `Space` | Jump |
| `S` | Fast-fall while falling |
| `E` | Cast the selected spell |
| `1` | Select Fire |
| `2` | Select Water |
| `3` | Select Wind |
| `4` | Select Rock |

## Project Structure

```text
.
├── CMakeLists.txt              # Root CMake configuration
├── README.md                   # Original classroom README
├── ABOUT.txt                   # Project design notes and team contribution summary
├── doxygen.txt                 # Doxygen configuration
├── docs/html/                  # Generated Doxygen HTML documentation
├── extern/SFML-3.0.0/          # Optional bundled SFML fallback, if included in the repo/zip
├── assets/
│   ├── maps/                   # Map files loaded by main.cpp
│   ├── sprites/                # Player, enemy, spell, background, tile, and UI sprites
│   ├── sounds/                 # Movement, spell, hit, death, and UI sounds
│   ├── fonts/                  # Font used by menu/pause text
│   ├── hurt.frag               # Hurt-flash shader
│   ├── rock_shield.frag        # Rock-shield shader
│   └── main.frag               # Shader asset from earlier project work
└── engine/
    ├── main.cpp                # Program entry point; creates engine, scenes, systems, maps, assets
    ├── ECSEngine.h             # Window ownership, scene stack, and main loop
    ├── Scene.h                 # Scene-level managers, delta time, and system execution
    ├── EntityManager.h         # Entity IDs and type-safe component attachment/access
    ├── ComponentStorage.h      # Vector-backed component storage with free-list reuse
    ├── SystemManager.h         # Ordered system ownership and execution
    ├── SpriteManager.*         # Sprite/texture atlas management
    ├── SoundManager.*          # Sound buffer and playback management
    ├── ShaderManager.h         # GLSL fragment shader loading/cache
    ├── WindowManager.*         # Window and camera/view helpers
    ├── *Component.h            # ECS component definitions
    └── *System.h               # ECS systems for input, collision, rendering, spells, enemies, HP, etc.
```

## Architecture Overview

### ECS Engine

The engine is built around a template-based ECS design:

- `EntityManager<Components...>` owns entities and component storage.
- `ComponentStorage<T>` stores components in vectors and maps entity IDs to component indices.
- `SystemManager<Components...>` owns systems and runs them in the order they are registered.
- Each system receives the current `Scene`, giving it access to entities, sprites, sounds, shaders, timing, and the window.

Entity IDs are reused after removal to reduce unnecessary allocation. Component references are valid until the component or entity is removed.

### Scene Stack

`ECSEngine` owns the SFML window and a stack of scenes. Only the top scene runs each frame:

1. The gameplay scene is created and loaded.
2. The gameplay scene is pushed onto the stack.
3. The main menu scene is pushed on top, so it runs first.
4. Pressing `Enter` completes the menu and returns to gameplay.
5. Pressing `Esc` during gameplay pushes a pause scene.
6. Completing the pause scene pops it and resumes gameplay.

The engine performs one final `display()` call per frame after the active scene runs.

### Main Gameplay Systems

The gameplay scene registers systems in this order:

1. `ProcessEventsSystem`
2. `CollisionUpdateSystem`
3. `InputSystem`
4. `SpellSystem`
5. `GravitySystem`
6. `MovementSystem`
7. `CollisionSystem`
8. `ProjectileSystem`
9. `EnemySystem`
10. `HpSystem`
11. `CheckpointSystem`
12. `ScoreSystem`
13. `CameraSystem`
14. `AnimationSystem`
15. `SpriteSystem`
16. `SpawnSystem`

This ordering is important because input, movement, collision, damage, animation, camera, and rendering depend on state produced by earlier systems.

### Collision and Movement

The platforming logic uses translated AABB bounds stored in collision components. The collision pipeline separates collision-bound updates from collision resolution:

- `CollisionUpdateSystem` updates translated AABBs and prepares collision state.
- `CollisionSystem` resolves collisions, identifies contact sides, applies damage/knockback rules, detects water/campfire/goal interactions, and triggers effects.

Movement is velocity-based and uses acceleration/deceleration. Gravity is modified for wall-slide behavior, and some movement abilities depend on the selected element.

### Rendering and Shaders

`SpriteManager` loads textures and finalizes a sprite atlas after map load. `SpriteSystem` renders sprites in world or screen space, supports flipping, layer ordering, parallax factors, animation frame updates, and shader selection.

`ShaderManager` loads fragment shaders by name. Current shader effects include:

- `hurt`: damage flash effect
- `rock_shield`: shield visual effect

### Audio

`SoundManager` stores sound buffers and `sf::Sound` instances in maps. Sounds are registered at startup and played by name from gameplay systems. Playback is non-blocking, allowing overlapping spell, hit, jump, and enemy sounds.

## Requirements

- CMake 3.10+
- A C++20-compatible compiler
- SFML 3.x with these modules:
  - Graphics
  - Window
  - System
  - Audio
- Optional: Doxygen for regenerating API docs
- Optional: Valgrind for memory/debug checks on Linux

The project uses SFML 3 APIs, so SFML 2.x is not expected to compile without code changes.

## Build and Run

### 1. Clone the repository

```bash
git clone <repo-url>
cd project-4-platformer
```

If you renamed the public repository to `Leo-the-wizard-cat`, use that folder name instead.

### 2. Install or expose SFML 3

The CMake configuration first tries to find a system-wide SFML 3 installation.

On macOS with Homebrew:

```bash
brew install cmake sfml
```

On Linux, install CMake and SFML 3 through your package manager if available, or build/install SFML 3 manually.

If SFML 3 is installed in a custom location, pass it to CMake:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/SFML-3.0.0
```

The submitted project may include a bundled SFML folder at:

```text
extern/SFML-3.0.0/
```

If CMake cannot find SFML automatically but the bundled folder exists, configure with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/extern/SFML-3.0.0"
```

### 3. Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable target is currently named `ecsp1` and is usually generated under `build/engine/`.

### 4. Run

The game loads assets through a resource path. By default, `main.cpp` uses:

```text
../../assets/
```

That default works when running from inside the generated `build/engine/` directory:

```bash
cd build/engine
./ecsp1
```

Alternatively, from the repository root, pass the assets path explicitly:

```bash
./build/engine/ecsp1 -path assets/
```

The trailing slash in `assets/` is important because the code appends subpaths such as `sprites/...`, `sounds/...`, and `maps/...`.

### 5. Common run commands

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/engine/ecsp1 -path assets/
```

From the build output directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cd build/engine
./ecsp1
```

### 6. If dynamic libraries are not found

If your OS cannot locate SFML dynamic libraries at runtime, set the library path before running.

macOS:

```bash
export DYLD_LIBRARY_PATH="$PWD/extern/SFML-3.0.0/lib:$DYLD_LIBRARY_PATH"
./build/engine/ecsp1 -path assets/
```

Linux:

```bash
export LD_LIBRARY_PATH="$PWD/extern/SFML-3.0.0/lib:$LD_LIBRARY_PATH"
./build/engine/ecsp1 -path assets/
```

If you installed SFML system-wide, you usually do not need these variables.

## Generating Documentation

The repository includes a Doxygen configuration file:

```text
doxygen.txt
```

To regenerate the API documentation:

```bash
doxygen doxygen.txt
```

Then open:

```text
docs/html/index.html
```

The Doxygen input is configured to document the `engine/` directory.

## Debugging Notes

### Debug build

```bash
cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build-debug
./build-debug/engine/ecsp1 -path assets/
```

### Valgrind

On Linux, you can run:

```bash
valgrind --leak-check=full --track-origins=yes ./build-debug/engine/ecsp1 -path assets/
```

SFML/OpenGL/audio libraries may report external allocations depending on the platform and driver. Focus first on invalid reads/writes and leaks that point into this project’s `engine/` code.

### Generated atlas

At startup, the game finalizes the sprite atlas and writes an `atlas.png` file to the current working directory. This is a debug artifact and can be deleted.

## Known Notes

- The game is a **2D SFML platformer**, not a 3D renderer.
- The executable target is currently named `ecsp1`; renaming the target would make the project easier to understand for future readers.
- Some files and comments still reference earlier course project names such as `Project 4` or prior ECS assignments.
- The root `README.md` is the original classroom starter README; this document is intended as the practical project README.
- The project uses a fixed/limited timestep approach: the engine targets 60 FPS, while scenes clamp large delta-time spikes to avoid physics instability.
- `ScoreSystem` exists but the final project notes say enemy-score integration was not completed.
- The goal/next-level portal was noted as pending future work in the project notes.
- The repository/zip may contain generated build files or generated Doxygen HTML. In a cleaned public repo, `build/`, `.DS_Store`, and other generated artifacts should usually be excluded.

## Future Improvements

- Rename the executable target from `ecsp1` to something like `leo_the_wizard_cat`.
- Add a short gameplay GIF or screenshots to the README.
- Add a release build or packaged binary for non-developer reviewers.
- Move hardcoded paths and system tuning constants into configuration files.
- Add automated tests for entity/component storage, collision side detection, projectile lifetime, and checkpoint respawn behavior.
- Complete score integration for defeated enemies.
- Add a portal/level-transition system for progressing beyond the current maps.
- Replace any remaining debug `std::cout` logs with a small logging utility or debug flag.
- Document asset sources and licenses in a dedicated `CREDITS.md` file.

## Credits

Developed by **Peter Davidson** and **Eric Jiang**:

- Peter focused on collision, gravity, sprite rendering, projectiles, enemies, HP, checkpoints, animations, shader/sound/system management, entity management, and ECS engine integration.
- Eric focused on process events, input, movement, spawning, score, camera/lookahead logic, spell selection/casting logic, sprite/window/math/component storage, and ECS engine integration.

Original project context: CMPUT 350 Project Assignment 2 Part 2.

Built with:

- [C++](https://isocpp.org/)
- [SFML](https://www.sfml-dev.org/)
- [CMake](https://cmake.org/)
- [Doxygen](https://www.doxygen.nl/)
