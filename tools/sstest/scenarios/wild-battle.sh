DESCRIPTION="Route 201 by Sandgem: walk in the grass until a wild battle, then move the menu cursor"
SAVE="route201-grass.sav"
SECONDS_TO_RUN=80
DUMP_EVERY=60
# Down two tiles, up two tiles, over and over until something jumps out.
WALK=""
for start in $(seq 1500 80 6000); do
    WALK+="$start:32:0x80,$((start + 40)):32:0x40,"
done
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1"
    SSTEST_AUTO_UNTIL=1400
    SSTEST_KEYS="${WALK%,}"
)
