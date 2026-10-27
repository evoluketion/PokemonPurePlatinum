DESCRIPTION="Route 201: pick Chimchar from the briefcase, then the first battle"
SAVE="route201-briefcase.sav"
SECONDS_TO_RUN=150
DUMP_EVERY=90
# As the briefcase scenario up to confirming Chimchar, then A every 40 frames
# through the dialogue into the battle with the wild Starly and its menus.
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1:0:900,40:0:0x1:3800:999999"
    SSTEST_KEYS="1500:6:0x1,2100:6:0x2,2500:6:0x10,2800:6:0x1,3400:6:0x1"
)
