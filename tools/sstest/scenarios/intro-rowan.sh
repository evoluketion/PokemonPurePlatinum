DESCRIPTION="New game through Professor Rowan's intro into the bedroom"
SECONDS_TO_RUN=130
DUMP_EVERY=120
# A every 45 frames and a tap on the touch screen's Yes button / the Poke
# Ball's centre between them, for the touch-only prompts. Rowan's "anything
# else?" menu opens around frame 1430: the A presses pause there while Down,
# Down, A picks "No info needed", so the control info loop is skipped. START
# between the A presses jumps the naming keyboard's cursor to OK, so each name
# is one letter long; nothing else in the intro reacts to it.
SSTEST_ENV=(
    SSTEST_AUTO="45:0:0x1:0:1420,45:0:0x1:1560:999999,45:30:0x8:1560:999999"
    SSTEST_KEYS="1470:4:0x80,1500:4:0x80,1530:4:0x1"
    SSTEST_TOUCH="45:22:128:88"
)
