# FestiBrawl (Unity)

A 2D fighting game with 8 holiday mascot fighters. Best of 3, single-script Unity port.

> **Heads up:** Unity needs you to do 3 manual editor steps that I can't pre-author into the project, because Unity scenes/meta files use GUIDs that break if I author them by hand. The setup below takes about 60 seconds.

## Setup

### 1. Create a new project
- Open **Unity Hub**
- Click **New Project**
- Pick the **2D Core** template (or any 2D template)
- Unity version: **2022.3 LTS or newer** (also works on Unity 6)
- Name it whatever, click **Create Project**

### 2. Add the script
- Once the editor opens, in the **Project** window, navigate into `Assets`
- Drag the `Assets/Scripts/FestiBrawl.cs` file from this download into the Unity `Assets/` folder (or create an `Assets/Scripts/` subfolder and drop it there)
- Wait for Unity to finish compiling (bottom-right spinner)

### 3. Attach the script and run
- Select **Main Camera** in the **Hierarchy** window (left side)
- In the **Inspector** (right side), scroll to the bottom and click **Add Component**
- Type `FestiBrawl` and pick it from the list
- (Recommended but optional) Set the camera's **Background** to solid black: in the Inspector, under Camera → Environment → Background, click the swatch and pick black
- (Recommended but optional) Set the **Game** view aspect ratio: at the top of the Game window, change **Free Aspect** to **16:9 Aspect** or add a custom 960x540 resolution
- Press **Play** (the ▶ button at the top of the editor)

That's it. The game runs in the Game view.

## Controls

### Player 1
| Action | Key |
|--------|-----|
| Move | **A / D** |
| Jump | **W** |
| Crouch / Block | **S** |
| Punch | **F** |
| Kick | **G** |
| Special | **H** |

### Player 2
| Action | Key |
|--------|-----|
| Move | **← / →** |
| Jump | **↑** |
| Crouch / Block | **↓** |
| Punch | **J** |
| Kick | **K** |
| Special | **L** |

### Menu
- **Enter** or **Space** — confirm
- **Escape** — back

## The 8 Fighters

| # | Name | Holiday | Special |
|---|------|---------|---------|
| 1 | SAN T. CLAWS | Christmas | Hurls a sack of gifts |
| 2 | ESTHER BUNNY | Easter | Throws explosive eggs |
| 3 | JACK O'LANTERN | Halloween | Vanishes in spirit smoke (teleport) |
| 4 | TOM GOBBLESON | Thanksgiving | Rising feather twister |
| 5 | CU PID | Valentine's | Long-range heart arrow |
| 6 | PATTY O'LUCKY | St. Patrick's | Lucky-charm rush strike (dash) |
| 7 | UNCLE SAM | 4th of July | Patriot rocket projectile |
| 8 | MAC A. BEE | Hanukkah | Spinning dreidel hex (poison DoT) |

## Architecture

A single 2,400-line C# `MonoBehaviour`. Key choices:

- **`OnPostRender` + `GL`** for shape rendering (rectangles, triangles, circles, lines). This is why the script must be on a Camera — `OnPostRender` only fires for `MonoBehaviour`s attached to one.
- **`OnGUI`** for all text. The GUI matrix is scaled so the virtual 960x540 viewport fits the actual screen.
- **Procedural sprites** via local-space transforms (the `LT` struct) — each fighter body part is drawn in local coords and transformed to world coords per-vertex.
- **Procedural audio** via `AudioClip.Create` + `SetData` — square/saw/sine waves with falloff envelopes. No audio assets.
- **Legacy `Input` system** (`Input.GetKey`, `Input.GetKeyDown`). No package install needed.
- **Coroutines** for delayed projectile spawning during specials.

## Known caveats — read this first

I built this without being able to run Unity, so expect 1-3 small fixups on first launch. Most likely suspects:

1. **`Hidden/Internal-Colored` shader missing.** This shader ships with Unity's built-in render pipeline. If you used the **URP** or **HDRP** template instead of **2D Core / Built-in**, you'll see no shapes (just text overlay). Fix: use the Built-in / 2D Core template, or add a custom unlit material and assign it manually.

2. **Text font.** I'm using Unity's default `GUI.skin` font for all text. The arcade aesthetic from the HTML version (Press Start 2P) is lost. To restore it: download Press Start 2P TTF, drop it in `Assets/Fonts/`, and assign it to the `GUIStyle` in `Awake()` via `textStyle.font = yourFont;`.

3. **Camera background.** If you skip the "set background to black" step, you'll see the default skybox showing through unintended areas. Cheapest fix: select Main Camera, set Camera → Environment → Background → Solid Color → black.

4. **Game window aspect.** If you leave it on Free Aspect with a tall window, the GUI text will scale to fit but shapes drawn in pixel-space will not. Best is to lock the Game window to 16:9 or 960x540.

5. **Camera projection.** The default 2D template gives an Orthographic camera, which is what we want. If your camera is Perspective, switch it: Camera → Projection → Orthographic.

6. **Audio sample rate.** I'm using 22050 Hz. If any platform complains about that, change `sampleRate` to `44100` in `MakeSfx` and `MakeNoise`. Modern desktops should be fine.

7. **`OnPostRender` not firing.** This callback only runs when the script is on a Camera AND that camera renders something. If you stripped the camera or attached the script to an empty GameObject, no shapes will draw. Fix: re-attach to Main Camera.

8. **Coordinate convention.** `GL.LoadPixelMatrix(0, W, H, 0)` gives top-left origin Y-down to match the canvas/Godot version. If something draws upside-down, the alternative is `GL.LoadPixelMatrix(0, W, 0, H)` and you'll need to flip Y in every draw call. The current setup should be correct.

## What's not here (vs. the HTML5 version)

- Mobile touch controls
- The CRT scanline post-processing effect
- High scores, settings, save data
- Real sprites (everything is procedural shapes)
- Music

## Why no `.unity` scene file is included

Unity scenes use auto-generated GUIDs in `.meta` files that link assets together. If I authored a scene file by hand, it would either reference GUIDs that don't exist on your machine (broken script reference) or it would be safe but still need you to re-link the script. Either way, the manual "attach script to camera" step is unavoidable. So: no scene file, just the script.

## Tweaking

Want to change a fighter's stats? Edit the `ROSTER` array near the top of `FestiBrawl.cs`. Each entry has `hp`, `speed`, `jump`, `power`, `weight`, `build`, and `special` fields.

Want to add a new fighter? Add an entry to `ROSTER`, then add a `case` in `DrawCharacterFace` for the head, optionally one in `DrawChestDetail` for the torso, and one in `AuraColor` for the special-move glow color.

Want to swap procedural sprites for real Unity sprites? Replace the entire `DrawFighterSprite` body with a `SpriteRenderer.sprite = ...` lookup. The `Fighter` class already tracks `state`, `stateTimer`, `walkPhase`, and `facing` — those map cleanly to Unity `Animator` parameters.

## Files

```
festibrawl_unity/
└── Assets/
    └── Scripts/
        └── FestiBrawl.cs    # The whole game (~2,400 lines)
README.md                    # This file
```
