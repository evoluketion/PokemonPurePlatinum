# sstest: scripted visual tests

Repeatable runs of the game for checking screens, mainly the single-screen
(`main_only`) conversion: boot from a known save, drive the input from a
script, and save frames to look at afterwards. Each run happens in its own
folder, so the real game folder (`build_linux/pokeplatinum`) and its save are
never touched.

## Quick start

```sh
tools/sstest/hooks.sh on        # apply the test hooks
make linux                      # build with them
tools/sstest/run.sh             # list the scenarios
tools/sstest/run.sh overworld-poketch
tools/sstest/run.sh intro-rowan vertical   # same scenario, another layout
tools/sstest/hooks.sh off       # before committing
```

Runs happen off-screen and silently (SDL's `offscreen` video and `dummy`
audio drivers), so they can go on while you use the machine; set
`SSTEST_SHOW=1` to watch one in a window. Frames land in
`build_linux/sstest/<scenario>/out/`, with a `contact.png`
sheet of all of them if ImageMagick is installed, and the game's output in
`log.txt` there.

## The hooks

The hooks are two patches in `hooks/`, kept out of the source so they can't
be committed by accident: `game.patch` adds scripted input to
`src/system.c`, and `libntr.patch` adds frame dumping to libntr's
`sim_main.cpp` (applied to the `subprojects/libntr` checkout, on top of the
single-screen patch). Every hooked line is marked `SSTEST-HOOK`. A build
with the hooks plays normally - they only act when these environment
variables are set:

| Variable | Meaning |
| --- | --- |
| `SSTEST_AUTO="every:offset:mask[:from:to],..."` | Press `mask` on frames where `frame % every == offset`, optionally only from frame `from` up to `to` |
| `SSTEST_AUTO_UNTIL=frame` | Stop the `SSTEST_AUTO` presses and `SSTEST_TOUCH` taps at this frame |
| `SSTEST_KEYS="start:length:mask,..."` | Hold `mask` from frame `start` for `length` frames |
| `SSTEST_TOUCH="every:offset:x:y[:from:to],..."` | Tap the touch screen at `x,y` periodically |
| `SSTEST_DUMP=N` | Save every Nth frame |
| `SSTEST_DUMP_DIR=dir` | Where to save frames (default `dump`) |

There's one clock: frames drawn since boot, counted by the libntr hook. Saved
frames are named by it (`f001440.png` is frame 1440), so a timing read off
a saved frame can go straight into a script. The game reads input less often
than frames are drawn, so a press fires on the first read at or after its
frame; give `SSTEST_KEYS` holds a length of at least 4 so none are missed.
Touch coordinates are the touch screen's own 256x192 pixels, whatever the
layout. Key masks are the DS's:

| Key | Mask | Key | Mask |
| --- | --- | --- | --- |
| A | `0x1` | Right | `0x10` |
| B | `0x2` | Left | `0x20` |
| Select | `0x4` | Up | `0x40` |
| Start | `0x8` | Down | `0x80` |
| R | `0x100` | X | `0x400` |
| L | `0x200` | Y | `0x800` |

Turn the hooks off before committing, and before regenerating the libntr
patch (`subprojects/packagefiles/libntr_patch`) from the checkout - with them
on, they'd end up in it. `hooks.sh status` says whether they're on.

## Scenarios

A scenario is a small shell file in `scenarios/` that `run.sh` sources:

```sh
DESCRIPTION="One line, shown by run.sh"
SAVE="twinleaf-home.sav"    # from saves/; leave out for a new game
LAYOUT="main_only"          # the default
SECONDS_TO_RUN=50           # the game is killed after this
DUMP_EVERY=100              # frames between saved frames
CHEATS=(AlwaysHavePoketch=true)   # [Cheats] in sim_config_prj.ini
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1"
    SSTEST_AUTO_UNTIL=1400
    SSTEST_KEYS="1700:4:0x100"
)
```

The window settings come from the real game folder's `sim_config.ini`, with
the layout set from the scenario and the window size reset to the default.

Timings are found by running with a small `DUMP_EVERY` and reading the frame
numbers off the saved frames. Runs from the same save with the same input
replay closely but not exactly - loading and when a screen starts taking input
can shift by tens of frames - so leave some slack around the frames a script
depends on. From a save, A every 40 frames until frame 1400 gets
through the title screen and Continue into the field. `intro-rowan` shows
pausing the periodic presses around a menu to choose from it.

## Saves

`saves/` holds saves to start scenarios from. Add one by copying
`build_linux/pokeplatinum/save.bin` after saving somewhere useful, and say
where it is here:

| Save | Where |
| --- | --- |
| `twinleaf-home.sav` | The player's house in Twinleaf, downstairs, before the Pokétch |
