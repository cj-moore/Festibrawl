# FestiBrawl (Godot 4)

A 2D fighting game with 8 holiday mascot fighters. Best of 3, single-file GDScript port.

## Setup

1. Install **Godot 4.3 or newer** (download from https://godotengine.org/download).
2. Open Godot and click **Import**.
3. Select the `project.godot` file in this folder.
4. Once the project opens, press **F5** (or the Play button) to run.

The first time you run it, Godot may ask you to set a main scene — pick `Main.tscn`. The project file already specifies it, but if Godot prompts you anyway, that's why.

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

This is a single-file port. Everything is in `main.gd`:

- **State machine** — title, character select, fight, round-over, match-over
- **Procedural rendering** — fighters, projectiles, backgrounds all drawn via `_draw()` calls. No sprite assets needed.
- **Procedural audio** — SFX generated as `AudioStreamWAV` at startup using square/saw/noise waveforms.
- **AI** — finite-state behavior tree for CPU opponent (approach / retreat / attack / block / jump / special).
- **4 backgrounds** — workshop, graveyard, wonderland, fireworks — randomly chosen per match.

## Known caveats (read this first)

I built this without being able to run Godot, so there's a real chance you'll hit one or two small issues on first launch. The most likely suspects:

1. **Font rendering** — I use `ThemeDB.fallback_font` which is the engine default. If text doesn't appear or sizes look off, that's the place to look. You can drop in a custom font (e.g. Press Start 2P) and replace the `ui_font` reference in `_ready()`.

2. **`Color(hex_string)` constructor** — Godot 4 accepts `Color("#ff4040")` syntax. If your version complains, change the `H()` helper at the bottom of the file to use `Color.html(hex)` instead.

3. **Lambda captures in `_start_special()`** — projectile spawning uses `get_tree().create_timer().timeout.connect(func(): ...)`. This pattern works in 4.2+ but if it errors, replace with a manual frame-counter approach (track pending spawns in an array, decrement each `_process`).

4. **`draw_set_transform_matrix()`** — used to apply translation+rotation+scale to fighters. If your Godot version uses a different signature, the alternative is `draw_set_transform(position, rotation, scale)`.

5. **`AudioStreamWAV` data format** — I write little-endian 16-bit PCM. If audio sounds garbled, check the byte-packing loop in `_make_sfx()`.

6. **No CRT scanline shader** — the original HTML5 version had one; porting it would need a `ShaderMaterial` on a `ColorRect` overlay. Skipped for simplicity.

7. **No background music** — the canvas version didn't have any, so this doesn't either. Drop an `AudioStreamPlayer` with a `.ogg` file as a child of Main if you want some.

## What's not here (vs. the HTML5 version)

- Mobile touch controls (Godot has its own input remapping UI you can build out)
- The CRT scanline post-processing effect
- Any kind of save/load (high scores, settings)

## Tweaking

Want to change a fighter's stats? Edit the `ROSTER` constant at the top of `main.gd`. Each entry has `hp`, `speed`, `jump`, `power`, `weight`, and `special` fields.

Want to add a new fighter? Add an entry to `ROSTER`, add a `match` case in `_draw_character_face()` for the head, optionally one in `_draw_chest_detail()` for the torso, and one in `_aura_color_for()` for the special-move glow color.

Want to swap procedural sprites for real ones? Replace the entire `_draw_fighter_sprite()` body with a `Sprite2D` lookup. The fighter dict already tracks `state`, `state_timer`, `walk_phase`, `facing` — those map cleanly to an `AnimationPlayer`.

## Files

```
festibrawl/
├── project.godot   # Engine config + input map
├── Main.tscn        # Single-node scene
├── main.gd          # The whole game (~1700 lines)
├── icon.svg         # Project icon
└── README.md        # This file
```
