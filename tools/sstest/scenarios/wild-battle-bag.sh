DESCRIPTION="Route 201 by Sandgem: walk in the grass until a wild battle, then tap BAG"
SAVE="route201-grass.sav"
SECONDS_TO_RUN=80
DUMP_EVERY=60
# Down two tiles, up two tiles, over and over until something jumps out.
WALK=""
for start in $(seq 1500 80 6000); do
    WALK+="$start:32:0x80,$((start + 40)):32:0x40,"
done
# Nothing in the field answers the touch screen yet, so these taps only do
# something once the battle's command menu is up (tap BAG in its compact layout). B now and then
# backs out again, so both ways in and out get seen.
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1:0:1400,300:150:0x2:1500:999999"
    SSTEST_KEYS="${WALK%,}"
    SSTEST_TOUCH="120:0:188:120:1500:999999"
)
