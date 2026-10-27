DESCRIPTION="Route 201: choosing a starter from Rowan's briefcase"
SAVE="route201-briefcase.sav"
SECONDS_TO_RUN=60
DUMP_EVERY=60
# A through the title screen and Continue (the briefcase is open by ~1100),
# then: A on the first ball, B to back out, Right to the next ball, A on it,
# and A to confirm it.
SSTEST_ENV=(
    SSTEST_AUTO="40:0:0x1"
    SSTEST_AUTO_UNTIL=900
    SSTEST_KEYS="1500:6:0x1,2100:6:0x2,2500:6:0x10,2800:6:0x1,3400:6:0x1"
)
