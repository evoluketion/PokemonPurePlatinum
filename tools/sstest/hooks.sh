#!/bin/bash
# Apply or remove the tools/sstest test hooks. See tools/sstest/README.md.
# Usage: tools/sstest/hooks.sh on|off|status
set -e

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
HOOKS="$ROOT/tools/sstest/hooks"
LIBNTR="$ROOT/subprojects/libntr"

game_on() { grep -q "SSTEST-HOOK" "$ROOT/src/system.c"; }
libntr_on() { grep -q "SSTEST-HOOK" "$LIBNTR/libraries/sim/src/sim_main.cpp"; }

case "$1" in
on)
    game_on || git -C "$ROOT" apply "$HOOKS/game.patch"
    libntr_on || git -C "$LIBNTR" apply "$HOOKS/libntr.patch"
    echo "Hooks on. Build with 'make linux' before running scenarios."
    echo "Turn them off before committing or regenerating the libntr patch."
    ;;
off)
    game_on && git -C "$ROOT" apply -R "$HOOKS/game.patch"
    libntr_on && git -C "$LIBNTR" apply -R "$HOOKS/libntr.patch"
    echo "Hooks off. Rebuild with 'make linux' to drop them from the game."
    ;;
status)
    game_on && echo "game hooks: on" || echo "game hooks: off"
    libntr_on && echo "libntr hooks: on" || echo "libntr hooks: off"
    game_on || libntr_on
    ;;
*)
    echo "Usage: $0 on|off|status"
    exit 2
    ;;
esac
