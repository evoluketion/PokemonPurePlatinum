#!/bin/bash
# Build the Linux PC port and run it.
# Usage: ./play.sh [--no-build] [args for the game...]
set -e

cd "$(dirname "$0")"

BUILD=1
if [ "$1" = "--no-build" ]; then
    BUILD=0
    shift
fi

GAME_DIR=build_linux/pokeplatinum

if [ $BUILD -eq 1 ]; then
    # A running copy locks the executable and the build can't replace it.
    if ls -l /proc/[0-9]*/exe 2>/dev/null | grep -q -- "-> $PWD/$GAME_DIR/main$"; then
        echo "The game is already running - close it first, or use --no-build."
        exit 1
    fi
    make linux
fi

if [ ! -x "$GAME_DIR/main" ]; then
    echo "No build found in $GAME_DIR - run without --no-build first."
    exit 1
fi

# The game reads save.bin and its config files from the current directory.
cd "$GAME_DIR"
exec ./main "$@"
