DESCRIPTION="Twinleaf house with the Poketch: open it, start menu hides it, close it"
SAVE="twinleaf-home.sav"
SECONDS_TO_RUN=35
DUMP_EVERY=100
CHEATS=(AlwaysHavePoketch=true)
# A through the title screen and Continue (the room is up by frame ~600), then:
# R opens the Poketch panel, X opens the start menu (the panel hides), B closes
# it (the panel comes back), and R minimises it again.
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1"
    SSTEST_AUTO_UNTIL=900
    SSTEST_KEYS="1200:6:0x100,1600:6:0x400,2000:6:0x2,2400:6:0x100"
)
