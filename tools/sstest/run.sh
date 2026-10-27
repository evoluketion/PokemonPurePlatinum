#!/bin/bash
# Run a tools/sstest scenario and save its frames. See tools/sstest/README.md.
# Usage: tools/sstest/run.sh <scenario> [layout]
#   scenario  a file in tools/sstest/scenarios, without the .sh
#   layout    overrides the scenario's ScreenLayout (main_only, vertical, ...)
# Runs off-screen and silent; set SSTEST_SHOW=1 to watch it in a window.
set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
SSTEST="$ROOT/tools/sstest"
BUILD="$ROOT/build_linux"
GAME="$BUILD/pokeplatinum"

NAME="$1"
if [ -z "$NAME" ] || [ ! -f "$SSTEST/scenarios/$NAME.sh" ]; then
    echo "Usage: $0 <scenario> [layout]"
    echo "Scenarios:"
    for f in "$SSTEST"/scenarios/*.sh; do
        printf "  %-24s %s\n" "$(basename "$f" .sh)" "$(grep -m1 '^DESCRIPTION=' "$f" | cut -d'"' -f2)"
    done
    exit 2
fi

# Scenario defaults, overridden by the scenario file.
DESCRIPTION=""
SAVE=""          # file in tools/sstest/saves; empty starts a new game
LAYOUT="main_only"
SECONDS_TO_RUN=60
DUMP_EVERY=60    # frames between saved frames
SSTEST_ENV=()    # SSTEST_* variables driving the input - see the README
CHEATS=()        # [Cheats] lines for sim_config_prj.ini, e.g. AlwaysHavePoketch=true
source "$SSTEST/scenarios/$NAME.sh"
[ -n "$2" ] && LAYOUT="$2"

if ! grep -q "SSTEST_DUMP" "$BUILD/main" 2>/dev/null; then
    echo "build_linux/main wasn't built with the test hooks."
    echo "Run: tools/sstest/hooks.sh on && make linux"
    exit 1
fi

if [ ! -d "$GAME" ]; then
    echo "No game data in $GAME - run 'make linux' first."
    exit 1
fi

# A fresh run folder: shared game data is linked, but everything the game
# writes to is a copy, so the real game folder is never touched.
RUN="$BUILD/sstest/$NAME"
rm -rf "$RUN"
mkdir -p "$RUN/out"
for f in "$GAME"/*; do
    case "$(basename "$f")" in
    main | save.bin | sim_config.ini | sim_config_prj.ini | firmware.bin | *.AppImage) ;;
    *) ln -s "$f" "$RUN/" ;;
    esac
done
cp "$BUILD/main" "$RUN/main"
cp "$GAME/firmware.bin" "$RUN/" 2>/dev/null || true

if [ -n "$SAVE" ]; then
    cp "$SSTEST/saves/$SAVE" "$RUN/save.bin"
fi

if [ -f "$GAME/sim_config.ini" ]; then
    cp "$GAME/sim_config.ini" "$RUN/sim_config.ini"
else
    printf "[General]\n" > "$RUN/sim_config.ini"
fi
for setting in "ScreenLayout=$LAYOUT" "WindowWidth=0" "WindowHeight=0" "FullScreen=false"; do
    key="${setting%%=*}"
    if grep -q "^$key=" "$RUN/sim_config.ini"; then
        sed -i "s/^$key=.*/$setting/" "$RUN/sim_config.ini"
    else
        sed -i "/^\[General\]/a $setting" "$RUN/sim_config.ini"
    fi
done

{
    printf "[General]\n60Fps=true\n60FpsSpeedFix=true\nEnableAsserts=false\nBreakDebuggerOnAssert=false\n\n[Cheats]\n"
    for cheat in "${CHEATS[@]}"; do
        printf "%s\n" "$cheat"
    done
} > "$RUN/sim_config_prj.ini"

# Run without a window or sound unless asked, so test runs don't take over
# the desktop or the speakers.
if [ -z "$SSTEST_SHOW" ]; then
    export SDL_VIDEODRIVER=offscreen SDL_AUDIODRIVER=dummy
fi

echo "Running $NAME ($LAYOUT, ${SECONDS_TO_RUN}s): $DESCRIPTION"
(
    cd "$RUN"
    env SSTEST_DUMP="$DUMP_EVERY" SSTEST_DUMP_DIR=out "${SSTEST_ENV[@]}" \
        timeout -s KILL "$SECONDS_TO_RUN" ./main > out/log.txt 2>&1
) 2> /dev/null || true

FRAMES=$(ls "$RUN/out"/*.ppm 2>/dev/null | wc -l)
if command -v magick > /dev/null && [ "$FRAMES" -gt 0 ]; then
    for f in "$RUN/out"/*.ppm; do
        magick "$f" "${f%.ppm}.png" && rm "$f"
    done
    magick montage "$RUN/out"/f*.png -resize 40% -tile 10x -geometry +2+2 "$RUN/out/contact.png"
    echo "$FRAMES frames in $RUN/out, contact sheet at $RUN/out/contact.png"
else
    echo "$FRAMES frames in $RUN/out"
fi
