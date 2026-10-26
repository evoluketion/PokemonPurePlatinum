# Converting a Screen to Single-Screen Layout

The DS renders every screen across two physical LCDs, driven by two separate
2D engines (Engine A/"main" and Engine B/"sub") plus one 3D engine that can
only composite through Engine A. `libntr`'s simulator (`libraries/sim/` in
the `NitroSDK-4.2.30001` subproject) emulates this by drawing each engine's
output into the game window according to a `ScreenLayout` config value
(`sim_config.ini` → `[General] ScreenLayout=`). `vertical`, `horizontal`, and
`large` all show both engines side by side, DS-style. `main_only` is a
genuine single-screen layout: it shows *both* engines' content composited
into one window, not just one screen cropped away.

This page documents how `main_only` works and what a given screen's game code
needs to do to look right under it. The title screen
(`src/applications/title_screen.c`) is the first screen converted this way.
The compositing mechanism itself needed no per-screen involvement, but the
title screen turned out to need one small, targeted game-code change — see
"Why the title screen needed a small change" below for what it was and why
it wasn't a coincidence-free case after all.

## How `main_only` composites the two engines

Implementation: `sim_screenquads.c` (`s_MainOnlyCompositeQuad`), `g3_shader.c`
(the `transparentBackdrop` uniform in `G2SIM_FragmentShader`), `sim_main.c`
(`DrawEngine` setting that uniform for the SUB pass), `sim_config.h`/
`sim_config.c` (the `SIM_CONFIG_SCREEN_LAYOUT_MAIN_ONLY` enum value and its
`main_only` ini string), `gui_config.c` (dev-panel option).

Each frame, `SIM_Render` draws the MAIN engine (Engine A) first, then the SUB
engine (Engine B), always in that order, into the same full-window quad:

1. **MAIN engine pass** draws normally, filling the entire window with
   opaque content (including its backdrop color). This is the *base* layer.
2. **SUB engine pass** draws as a *cutout overlay*: `DrawEngine` sets the
   fragment shader's `transparentBackdrop` uniform for this pass only, which
   makes any pixel that would otherwise just be filled with the backdrop
   color come out fully transparent (`alpha = 0`) instead. Actual drawn
   content (BG tiles, sprites) still comes out opaque. With the normal
   `SRC_ALPHA`/`ONE_MINUS_SRC_ALPHA` blending already active, this means only
   the SUB engine's real content shows up, layered over the MAIN engine's
   base — its backdrop, whatever color it is, never touches the final image.

An earlier version of this composited by drawing the SUB pass with additive
blending instead, on the assumption its backdrop would be black (so adding
it would be a no-op). That assumption doesn't hold in general — the title
screen's logo backdrop turned out to be a light cream/red-gold pattern, not
black, which blew out the whole image instead of overlaying cleanly. The
transparent-backdrop cutout approach above doesn't depend on backdrop color
at all, so it doesn't have that failure mode.

This composite is driven purely by draw order — Engine A is always the base,
Engine B is always the overlay — and is **independent of `GXLayers_SwapDisplay`
/ `gSystem.whichScreenIs3D`**. That swap only affects which physical LCD each
engine's content would land on in real dual-screen layouts; it has no effect
on `main_only`'s composite, because `main_only` draws both engines' content
into the same full-window quad regardless of which physical screen the
hardware-accurate layouts would route them to.

## What this means for a screen's content

`main_only` will look right, with no code changes, when:

- The engine that should be the **visual background** (typically whichever
  one carries the 3D content, since 3D can only render through Engine A) is
  drawn first — which it always is.
- The engine that should be an **overlay** (logos, HUD text, prompts) only
  covers *part* of the screen, leaving enough of its backdrop untouched for
  the base to show through where intended. Backdrop *color* doesn't matter —
  the cutout is based on which pixels were actually drawn, not on color-
  keying — but if the overlay engine fills its entire screen with solid
  content, the base underneath will be completely hidden (not blended),
  which may or may not be what the screen wants.

It will look wrong, and need either a code change or a different approach,
when:

- Both engines have dense, mutually-obscuring full-screen content (e.g. two
  unrelated gameplay views) where an overlay doesn't make sense at all —
  these screens need an actual UI redesign (repositioning elements onto a
  single engine's layers), not just a compositing trick.
- The screen relies on touch input in a way a mouse can't stand in for.
  Clicking does work under `main_only`: the sim maps the cursor back through
  the touch screen's placement (`SIM_MainOnlyWindowToTouch` in
  `sim_main.cpp`), so a click lands on whatever touch-screen pixel is drawn
  under it, wherever the screen is placed and however it's scaled. What it
  doesn't do is invent keyboard alternatives for touch-only prompts, or make
  sense of a touch screen that's been hidden. That's per-screen input work.

## Why the title screen needed a small change

The title screen splits its content along the lines above in principle:
Giratina's 3D model, its starfield background, and copyright text render on
Engine A; the "Pokémon Platinum" logo and blinking "PRESS START" prompt
render on Engine B. No touch input is involved either, which is why it was
chosen as the pilot screen.

In practice, Engine B also carries a decorative border/fill layer
(`top_screen_border`, loaded onto both `TITLE_SCREEN_LAYER_LOGO_BG` and
`TITLE_SCREEN_LAYER_LOGO_BG_2`) that's meant to fill its own dedicated
physical screen entirely — it's real DS-hardware content, not empty
backdrop. With it enabled, Engine B draws opaque content across effectively
100% of the frame, so the cutout has nothing to cut: Giratina renders
correctly underneath, it's just completely covered. This is the "overlay
engine fills its entire screen" failure case described above, and it's what
first showed up as an all-white/cream screen with no Giratina visible at
all.

The fix, in `title_screen.c`'s `ToggleLogoBgLayer`/`ToggleLogoBg2Layer`
helpers: check `SIM_GetConfigPtr()->screenLayout` (from `simulator/sim.h`,
already used elsewhere in `src/port/`) and force those two border layers
**off** under `SIM_CONFIG_SCREEN_LAYOUT_MAIN_ONLY`. The actual logo art
(`TITLE_SCREEN_LAYER_LOGO`) and "PRESS START" text layer are untouched by this
and still overlay Giratina directly, since they only draw their own art with
transparent margins rather than a full-screen fill.

Note the helpers force `enable = FALSE` and still call through to
`GXLayers_EngineBToggleLayers`, rather than returning early and skipping the
call. That distinction matters and is easy to get wrong:
`GXLayers_DisableEngineBLayers()` only zeroes `gx_layers.c`'s software
`sEngineBLayers` tracking variable — it never writes the visible-plane
register. The register is only updated when a toggle actually calls through.
So merely skipping the "turn it on" call leaves whatever the *previous* screen
put in that register still applied, which showed up as a full-screen wash of
stale layer content with Giratina completely hidden behind it.

This is a real, if small, game-code change — not just a sim-layer trick — and
it's the expected shape of what the "Rolling out" section below describes as
"adjust the backdrop/layer content so it fits the compositing model." Screens
with a similar dedicated full-screen border/fill layer on their overlay
engine will likely need the same kind of targeted force-off.

## Placing engine output: `SIM_MainOnlyPlacement`

Removing an overlay engine's full-screen fill leaves its real content sitting
wherever the DS layout put it — sized and positioned for a screen of its own,
which rarely suits the composited single-screen frame. `simulator/sim.h`
exposes one struct describing how an engine is composited, set per engine:

```c
SIM_MainOnlyPlacement p = SIM_MAIN_ONLY_PLACEMENT_OVERLAY;
p.centerX = 0.235f;  p.centerY = 0.55f;
p.width   = 0.5f;    p.height  = 0.5f;
p.defringe = TRUE;
SIM_SetMainOnlyPlacement(TRUE, &p);   // TRUE = SUB engine
```

`SIM_GetMainOnlyPlacement` reads one back so a caller can change a single field,
and `SIM_ResetMainOnlyPlacements` restores the defaults. A screen calls these for
itself — nothing is automatic — and should reset on the way out so it doesn't
leave its framing applied to whatever runs next. All of it is inert outside
`main_only`.

The fields fall into four groups:

- **Destination rect** (`centerX`/`centerY`/`width`/`height`, defaulting to the
  full window). Where this engine's output lands. Note `centerY` is measured
  **upwards** — the composite quad maps window-bottom to `t=0.5` and window-top
  to `t=1.0` — so *lower* values move content *down* the screen.
- **Source rect** (`srcCenterX`/`srcCenterY`/`srcWidth`/`srcHeight`, defaulting
  to all of it). Which part of the engine's output to show. This exists because
  scenes routinely render a picture smaller than the screen and pad it with
  black; cropping to the picture is what lets two engines sit edge to edge
  without a band of that padding between them. **Aspect ratio is the caller's
  problem**: crop a short wide band and place it in a tall rect and it stretches.
  Size the destination from the crop.
- **`cutout`**. `FALSE` draws the engine opaquely, backdrop included, covering
  what is underneath — a base layer. `TRUE` leaves untouched-*backdrop* pixels
  transparent so only real drawn content appears over the base. Note the
  distinction: a cutout removes backdrop, not drawn content, so a full-screen
  background *layer* still covers everything and has to be toggled off in game
  code (see above). Whichever engine has `cutout = FALSE` is drawn first, so
  either engine can be the base — which the opening cutscene relies on, since it
  hands that role back and forth as it swaps which engine drives the display.
- **`defringe`** strips the white matte from art anti-aliased against an opaque
  light background. Much of this game's 2D art has near-white pixels baked into
  its silhouette for that reason — invisible on hardware, where the art has its
  own light backdrop, but a conspicuous halo once composited over a dark scene.
  Silhouette pixels that are essentially greyscale get their alpha reduced by how
  light they are; genuine art colors keep full opacity. Thresholds are tuned by
  eye in `G2SIM_FragmentShader`.
- **`hidden`** drops the engine from the composite entirely, for content that
  only made sense as a second physical screen.

Doing the de-fringe in the shader is deliberate: editing the source art would
change the generated NARC and break the `filesys.sha1` / ROM matching the DS
build still verifies. Keeping it in the presentation layer leaves the art
byte-identical and the authentic dual-screen layouts untouched.

### Tuning placements live

Framing a scene is a by-eye job — the numbers mean nothing until you see the
composite. The debug overlay (Tab) has a **Placement** panel with sliders for
every field on both engines, plus a reset. Values set there last until the game
writes them again, which scenes do at their own transitions, so an edit sticks
within a scene and is overwritten when the next one starts. It also makes the
state *visible*, which matters because a screen that sets a placement and fails
to put it back looks exactly like a scene that simply draws nothing.

## Worked example: the opening cutscene

`src/game_opening/` is the second thing converted, and it is a better guide than
the title screen because it needs a different framing in almost every scene. The
pattern that emerged: **each scene states the framing it wants at its own
transition**, rather than relying on the previous scene's teardown. Framings are
declared as file-scope constants and applied through a small helper that
no-ops outside `main_only`.

Three things about this cutscene generalise:

1. **Phase boundaries are not scene boundaries.** The copyright text and the
   "GAME FREAK presents" scene are two halves of *one* phase, split at a frame
   number partway through it. Hiding an engine for "the copyright phase" also
   hid the GAME FREAK lettering, which lives on that engine. Scope framing
   changes to the frame the content actually changes, not to the enclosing
   function.
2. **The art is often authored across both screens.** The montage shots are
   single 256x512 images that the game splits by scrolling each engine to its own
   band (`ov77_021D3DC4`). Stacking the engines at half scale reassembles the
   original picture rather than juxtaposing two unrelated ones.
3. **Moving a whole engine moves its background too.** To lift just the GAME
   FREAK logo, the fix was `Bg_SetOffset` on that one BG layer — shifting the
   engine would have dragged the full-screen background and its side borders
   along with it and left a black band.

Extracting the NARCs and rendering the layers to PNG was far more reliable than
reading the layer-loading code, and is worth doing first for any scene: it turns
"which engine has the text" from a guess into a fact.

## Worked example: Professor Rowan's intro

`src/applications/rowan_intro/rowan_intro_app.c` is the third screen converted.
Rowan, his dialogue and the info panels are on the main engine; the sub engine
has a full-screen backdrop on BG3 (the olive Poké Ball pattern, and the blue
panels behind the control info), with its real content (the Yes/No buttons,
the Poké Ball's centre button, Buneary) on BG1/BG2. So it is the title
screen's shape: the main engine is the base, and BG3 is forced off so the rest
overlays Rowan.

The twist is the Poké Ball scene, where the ball itself is drawn into that same
BG3 backdrop along with a cream background. Two things handle it:

- **Transparent background colours.** The backdrop's tiles use colours 1-8 for
  the background and 9 and up for the ball, and those tiles back nothing else.
  Under `main_only` they're loaded with colours 1-8 remapped to 0, the
  transparent colour, so just the ball is left. This happens at load time, so
  the NARC stays byte-identical.
- **A scene framing.** While the ball's backdrop is loaded, BG3 is shown and
  the sub engine is scaled to 60% and raised, so the ball sits in front of Rowan
  above the text box instead of covering it. Clicking the ball works through
  that placement.

The sim also changed for this scene: a master-brightness fade to black on a
`main_only` cutout overlay now fades its content out instead of darkening it.
On hardware the ball fades in as part of the whole bottom screen; in the
composite only the ball is left, and darkening that showed a black disc over
Rowan. When the base fades along with the overlay the result is the same black
either way.

The naming keyboard and the TV segment are separate applications and haven't
been converted.

## Worked example: the overworld and the Pokétch

The overworld map is the main engine and fills the window. Its bottom screen is
one of a few things picked by `fieldSystem->bottomScreen`
(`src/overlay005/ov5_021D5BC0.c`). Two of them are handled in
`src/overlay005/ov5_021EA714.c`:

- **The Pokéball wallpaper** shown before the Pokétch is obtained is hidden
  outright. It has nothing on it worth showing.
- **The Pokétch** is a panel in the top-right corner, opened and minimised with
  R, which the overworld otherwise ignores. It's hidden whenever the player
  can't move (dialogue, the start menu, cutscenes). Those draw over the right
  and bottom of the map, and that's when the DS puts the Pokétch to sleep
  anyway. Clicks on the panel reach the Pokétch through the placement.

`FieldSystem_UpdateBottomScreenFraming` restates the framing every field frame
rather than at transitions, so whatever an application run from the field left
behind is corrected on return. When the bottom screen is torn down, it's hidden
while it fades out and the default framing is restored once it's gone. The
Underground and the other field bottom screens aren't converted.

## Rolling this out to other screens

For each of the remaining `src/applications/` screens:

1. Check what's on each engine (search for `GXLayers_EngineAToggleLayers` /
   `GXLayers_EngineBToggleLayers` calls and the screen's `BgTemplate` setup).
2. If it matches the shape above (background/3D on one engine, black-backed
   overlay content on the other), `main_only` should already render it
   correctly — verify visually, no code change expected.
3. If it doesn't match (non-black backdrop, two dense competing screens, or
   touch-driven UI), that screen needs its own redesign: either adjust the
   backdrop/layer content so it fits the compositing model, or reposition
   elements to live on a single engine's layers, or build real mouse/keyboard
   input handling for it. There's no generic fallback for this by design —
   see the project plan for why touch-overlay shims were explicitly ruled out.

## Known limitations / follow-ups

- Which engine is the base is chosen by the `cutout` flags rather than fixed,
  so either engine can sit underneath. `swapScreens` no longer affects
  `main_only`'s vertex data, since both halves are always present in the
  composite quad; it would need repurposing if it were wanted again.
- Placements are global state written from the game thread and read from the
  render thread. In practice they change only at scene transitions, so a torn
  read would cost at most one frame, but it is unsynchronised.
- No visual regression testing exists for this — verification so far is
  "builds cleanly, runs without crashing, `main_only` renders `DbgLog.txt`
  with no errors." Actual pixel-level correctness needs to be checked on a
  real display.
