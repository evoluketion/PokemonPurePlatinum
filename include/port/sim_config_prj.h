#ifndef SIM_CONFIG_PRJ_H
#define SIM_CONFIG_PRJ_H

#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    BOOL enable60fps; // When true, unlock the framerate to 60fps
    BOOL enable60fpsSpeedFix; // When enabled, fixes things in the game that run at double speed due to being at 60fps
	BOOL enableAsserts; //Should asserts cause a communication error?
    BOOL breakDebuggerOnGfAssert; // When true, break the debugger when a GF_ASSERT fails

	// Cheats section
	BOOL walkThroughWalls; // When enabled, allows the player to walk through walls
    BOOL disableRandomEncounters; // When enabled, all random encounters are turned off
    BOOL runFromTrainerBattles; // When enabled, allows running away from trainer battles
    BOOL alwaysHavePoketch; // When enabled, the Poketch is on the overworld's bottom screen even before it's obtained
} SIM_Config_prj_type;

void SIM_Config_prj_LoadDefaults(SIM_Config_prj_type * aConfig);
BOOL SIM_Config_prj_LoadConfigFile(SIM_Config_prj_type * aConfig);
void SIM_Config_prj_SaveConfigFile(SIM_Config_prj_type * aConfig);
void SIM_Config_prj_init();
SIM_Config_prj_type * SIM_Config_prj_GetConfig();

// Runtime override forcing the DS's original 30Hz logic rate even when the 60
// FPS option is on, for sequences whose timing is written in frames and which
// therefore run at double speed at 60Hz. Set it for the duration of such a
// sequence and clear it afterwards. It is deliberately separate from
// enable60fps so it is never persisted to the config file - it describes what
// the game is currently doing, not what the user asked for.
void SIM_Config_prj_SetForce30FpsLogic(BOOL force);
BOOL SIM_Config_prj_GetForce30FpsLogic(void);

// TRUE when game logic is currently stepping at the 60 FPS option's doubled
// rate and the speed fix is on, so per-step animation and movement deltas need
// halving to keep their original speed. This is the condition those sites should
// test rather than the config flags directly: a sequence that has forced the
// 30Hz rate back on via SIM_Config_prj_SetForce30FpsLogic is already running at
// the original rate, and halving as well would run it at half speed.
BOOL SIM_Config_prj_IsLogicRateDoubled(void);

// Counts main-loop iterations; called once per iteration from main.c.
void SIM_Config_prj_AdvanceLogicTick(void);

// TRUE on every other logic tick while the logic rate is doubled. A task whose
// timing is written in frames - fixed per-step offsets, frame-count delays -
// returns early on these ticks to run at the original 30Hz. All such tasks
// share the same parity, so tasks that move things in step stay in step.
BOOL SIM_Config_prj_IsSkippedLogicTick(void);

#ifdef __cplusplus
}
#endif

#endif
