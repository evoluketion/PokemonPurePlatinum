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
- The screen relies on stylus/touch input tied to the sub-screen's
  coordinate space — `main_only`'s touch handling only covers the mechanical
  "don't crash" case (see `sim_main.c`'s touch-panel bounding box guard);
  it does not remap touch semantics to mouse/keyboard. That's separate,
  per-screen input work, not something the compositing layer solves.

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

## Placing and cleaning up overlay content

Removing an overlay engine's full-screen fill leaves its real content sitting
wherever the DS layout put it — sized and positioned for a screen of its own,
which rarely suits the composited single-screen frame. Two opt-in sim-layer
helpers (both in `simulator/sim.h`, both no-ops outside `main_only`, and both
defaulting to off so no other screen changes behavior) handle the common
cases. A screen calls them for itself; nothing is automatic.

**`SIM_SetMainOnlyOverlayTransform(scale, anchorX, anchorY)`** shrinks the SUB
engine's overlay content and places it. `scale` 1.0 draws it full-size;
`anchorX`/`anchorY` (0..1, default 0.5/0.5 = centered) position the center of
the scaled content in the window. The title screen uses
`(0.5f, 0.235f, 0.55f)` to tuck the logo and "PRESS START" into the dark space
at the left of the Giratina scene. Implemented in `G2SIM_FragmentShader` by
discarding fragments outside the anchored box and remapping the texture
coordinate inside it, so it costs nothing but a coordinate transform.

**`SIM_SetMainOnlyOverlayDefringe(TRUE)`** strips the white matte from art
that was anti-aliased against an opaque light background. Much of this game's
2D art has near-white pixels baked into its silhouette for exactly that
reason — invisible on hardware, where the art has its own light backdrop
behind it, but a conspicuous halo once the art is composited over a dark
scene. The shader reduces alpha by lightness, but only for near-greyscale
pixels, and only near the silhouette: it measures how far the pixel is from
undrawn space (up to three texels) and scales the fade down with distance, so
the matte tapers off instead of a hard bite being taken out of the art.
Genuine art colors — the logo's yellow fill, its blue outline — sit well above
the greyscale threshold and keep full opacity, and interior white (the
"PLATINUM VERSION" lettering) is shielded by its own black outline.

The thresholds in `G2SIM_FragmentShader` are tuned by eye against the title
screen logo; if another screen's art needs it stronger or gentler, the knobs
are the `smoothstep` range (how dark a grey still counts as matte), the
`mx - mn` greyscale cutoff, and the per-distance `strength` values.

Doing this in the shader is deliberate: the alternative, editing the source
art, would change the generated NARC and break the `filesys.sha1` / ROM
matching that the DS build still verifies. Keeping the fix in the
presentation layer leaves the art byte-identical and the authentic dual-screen
layouts untouched.

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

- The overlay order (Engine A = base, Engine B = overlay) is
  currently fixed by draw order. A screen that wants the opposite (2D base,
  3D overlay) isn't supported yet; `swapScreens` no longer affects
  `main_only`'s vertex data since both halves are always present in the
  composite quad; it would need to be repurposed if that flexibility is
  needed later.
- No visual regression testing exists for this — verification so far is
  "builds cleanly, runs without crashing, `main_only` renders `DbgLog.txt`
  with no errors." Actual pixel-level correctness needs to be checked on a
  real display.
