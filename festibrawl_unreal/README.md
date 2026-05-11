# FestiBrawl — Unreal Engine 5 port

A 2D fighting game with 8 holiday mascot fighters. This is a port of the
original HTML5/Canvas implementation to UE5, rendered procedurally via
the HUD canvas (no sprite assets, no Paper2D).

## ⚠️ Read this first

This port is **research/reference quality, not production quality**. There
is roughly a **30–50% chance it compiles cleanly on the first attempt**
on a fresh machine — UE C++ is sensitive to engine version, compiler,
and platform header differences. See the **Caveats** section below for
the specific places that are most likely to fail and how to fix them.

If you want a polished, idiomatic Unreal version of this game, you would
build it with Paper2D sprites, a real GameMode + Pawn architecture, and
Enhanced Input. This port deliberately does none of that — everything
lives in a single `AHUD` subclass to mirror the structure of the HTML5,
Godot, and Unity ports.

## Setup

1. **Install Unreal Engine 5.4 or later** from the Epic Games launcher
   (5.3 may also work; earlier 5.x versions almost certainly won't).
2. **Install Visual Studio 2022** (Windows) or Xcode (macOS) with the
   "Game development with C++" workload / C++ toolchain.
3. Extract this zip somewhere outside `Documents` (long paths cause
   issues on Windows).
4. Right-click `FestiBrawl.uproject` → **Generate Visual Studio project
   files**.
5. Open the generated `FestiBrawl.sln`, set configuration to
   `Development Editor` / `Win64`, and **Build**.
6. Once built, open `FestiBrawl.uproject` in the editor.
7. **Create an empty level**: File → New Level → Empty Level. Save it
   as `FestiBrawlMap` in `Content/`.
8. Edit → Project Settings → Maps & Modes → set `Editor Startup Map` and
   `Game Default Map` to your new level.
9. Press **Play**. The HUD draws the entire game on top of the empty
   level.

## Controls

| Action  | Player 1   | Player 2 |
|---------|------------|----------|
| Move    | A / D      | ← / →    |
| Jump    | W          | ↑        |
| Crouch  | S          | ↓        |
| Punch   | F          | J        |
| Kick    | G          | K        |
| Special | H          | L        |

Menus: **Enter** or **Space** to confirm, **Esc** to go back.

## Roster (8 fighters)

| ID       | Name          | Build | Special    |
|----------|---------------|-------|------------|
| santa    | SAN T. CLAWS  | huge  | gift box   |
| easter   | ESTHER BUNNY  | lean  | egg bomb   |
| jack     | JACK O'LANTERN| lean  | teleport   |
| turkey   | TOM GOBBLESON | broad | tornado    |
| cupid    | CU PID        | small | heart arrow|
| patty    | PATTY O'LUCKY | small | dash       |
| sam      | UNCLE SAM     | broad | rocket     |
| macabee  | MAC A. BEE    | lean  | dreidel    |

## Architecture

Everything lives in `AFBHUD` (`Source/FestiBrawl/Public/FBHUD.h` +
`Private/FBHUD.cpp`).

- **Virtual coordinate space**: the game logic uses a fixed 960×540
  coordinate system. `UpdateUIScale()` measures the actual viewport
  each frame and computes a `UIScale` + `UIOffset` so everything
  letterboxes correctly. `Sxy()` / `Sx()` apply that transform.
- **Procedural rendering**: rectangles via `AHUD::DrawRect`, triangles
  via `FCanvasTriangleItem` + `GWhiteTexture`, circles/ellipses
  approximated as triangle fans (24 segments by default), text via
  `AHUD::DrawText` with the default `GEngine->GetMediumFont()` scaled
  by `UIScale`.
- **State machine**: `State` is a string — `"title"`, `"select"`,
  `"fight"`, `"roundover"`, `"matchover"`. `Tick()` dispatches to the
  matching `Update*()`; `DrawHUD()` dispatches to the matching `Draw*()`.
- **Fighters**: `TArray<FFighter>`, exactly 2 elements during a round.
  Projectiles store `OwnerIdx` (int32) instead of a pointer so the
  array can reallocate without dangling.
- **Deferred spawns**: special moves that spawn a projectile mid-animation
  push to a `PendingSpawns` array with a frame counter, decremented in
  `UpdateFight()`. No timers, no lambdas.
- **Local-space sprite transform**: `FLT` is a tiny 2D transform
  (origin + scale + rotation) used to draw fighter limbs in local
  coordinates, then transformed to screen space. `DrawLocal*` wrappers
  apply the transform per-vertex.
- **Audio**: `PlaySfx()` is a no-op stub. Hook up `USoundCue` assets and
  `UGameplayStatics::PlaySound2D` if you want sound.

## Caveats (most-likely first-compile failures)

These are roughly ordered by likelihood. If the build fails, check
these first.

1. **`FCanvasTriangleItem` constructor signature.** The cpp uses:
   ```cpp
   FCanvasTriangleItem TriItem(A, B, Cc, GWhiteTexture);
   ```
   Some UE 5.x revisions reorder or rename the texture parameter, or
   require a `nullptr` instead of `GWhiteTexture`. Check
   `Engine/Source/Runtime/Engine/Public/CanvasItem.h` for the actual
   constructor you have. If it differs, adjust `DrawTri()`.

2. **`GWhiteTexture` resolution.** `GWhiteTexture` is declared in
   `RenderUtils.h`; if you get an unresolved external symbol, add
   `RenderCore` and `RHI` to `FestiBrawl.Build.cs` (already there) and
   make sure you `#include "RenderUtils.h"` at the top of `FBHUD.cpp`.

3. **`Canvas->Canvas` is the `FCanvas*`.** UCanvas exposes its inner
   FCanvas via a member also named `Canvas`. If your engine version
   renamed it (rare but possible), use `GetCanvas()` or similar.

4. **`UCanvas::StrLen` signature.** Used in `MeasureTextWidth` and
   `DrawText2D`:
   ```cpp
   Canvas->StrLen(HUDFont, Text, TextW, TextH);
   ```
   Some versions add a `bool bDPIAware` trailing parameter. If you get
   a "no matching call" error, just pass `false` at the end.

5. **`GEngine->GetMediumFont()`** — should return a valid `UFont*` in
   the editor and packaged builds. If it returns null, fall back to
   `GEngine->GetSmallFont()` or load a `Font'/Engine/EngineFonts/Roboto.Roboto'`
   asset directly.

6. **Font scaling.** Text size is approximated as
   `(RequestedSize / 14.0f) * UIScale`. The 14.0f baseline is for the
   medium engine font; if your text looks too big or small, change
   that constant in `DrawText2D()`.

7. **Default map.** The project ships without any `.umap` files. You
   must create one in the editor (step 7 above) and assign it in
   Project Settings, otherwise pressing Play does nothing and the HUD
   never ticks.

8. **`PlayerController` setup.** `AFBPlayerController` sets input mode
   to game-only. If a SpawnPawn warning appears, it's harmless —
   `AFBGameMode` sets `DefaultPawnClass = nullptr` deliberately.

9. **No idiomatic UE patterns.** This is a single-file procedural game
   stuffed into AHUD. UnrealHeaderTool may complain about the nested
   `struct FCharDef` being non-USTRUCT (it should not — they're plain
   C++ structs without `UPROPERTY`). If UHT does complain, the fix is
   to add `USTRUCT()` + `GENERATED_BODY()` or move the structs out of
   the class entirely.

## Files

```
FestiBrawl.uproject                 # project descriptor
Config/
  DefaultEngine.ini                 # sets FBGameMode as default
Source/
  FestiBrawl.Target.cs              # game build target
  FestiBrawlEditor.Target.cs        # editor build target
  FestiBrawl/
    FestiBrawl.Build.cs             # module dependencies
    FestiBrawl.h                    # module header
    FestiBrawl.cpp                  # IMPLEMENT_PRIMARY_GAME_MODULE
    Public/
      FBGameMode.h                  # AGameModeBase subclass
      FBPlayerController.h          # APlayerController subclass
      FBHUD.h                       # AHUD subclass — all type defs
    Private/
      FBGameMode.cpp
      FBPlayerController.cpp
      FBHUD.cpp                     # ~2000 lines, all game logic
README.md                           # this file
```

## Why not Paper2D / Sprites / GameMode-with-Pawns?

Three other ports of this game exist (HTML5 Canvas, Godot 4, Unity).
All three render procedurally with no sprite assets. To keep this port
comparable in structure and easy to read alongside the others, this
version follows the same "everything in one class, everything drawn
from primitives" approach. A real Unreal 2D fighter would look very
different — and probably ship in a fraction of the code.

## License

Same as the rest of the FestiBrawl project (whatever you've decided
that is — you wrote it).
