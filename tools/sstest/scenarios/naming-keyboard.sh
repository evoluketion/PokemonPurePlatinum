DESCRIPTION="New game up to the player's naming keyboard, a letter or two typed, then held"
SECONDS_TO_RUN=70
DUMP_EVERY=30
# The same input as intro-rowan up to "Tell me, what is your name?", whose A
# opens the keyboard around frame 3400. The periodic presses stop just before
# that so nothing gets typed or confirmed by accident. The keyboard ignores
# input for a while after it opens, and how long varies a little between runs
# (an A at frame 3700 is always dropped, one at 3900 sometimes), so the A
# presses come later and type one or two letters; then the screen is left
# alone for checking the prompt against the keys.
# The touch taps stop too: on the keyboard (128,88) is the G key.
SSTEST_ENV=(
    SSTEST_AUTO="45:0:0x1:0:1420,45:0:0x1:1560:3400,45:30:0x8:1560:3380"
    SSTEST_KEYS="1470:4:0x80,1500:4:0x80,1530:4:0x1,3900:4:0x1,4100:4:0x1"
    SSTEST_TOUCH="45:22:128:88:0:3380"
)
