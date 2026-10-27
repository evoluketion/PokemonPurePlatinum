#ifndef SINGLE_SCREEN_H
#define SINGLE_SCREEN_H

#include <nitro/types.h>

#include <simulator/sim.h>

// Shared framings for main_only single-screen mode, so screens of the same kind
// lay out the same way. See docs/single_screen_conversion.md. Each is inert
// outside main_only, like the placements it sets; a screen that uses one should
// call SIM_ResetMainOnlyPlacements on the way out.

// The layout for any text-input screen: the input screen (a keyboard, a word
// picker) scaled into the top three quarters of the window, over the other
// screen, whose bottom quarter - where the game's message boxes sit, holding
// the prompt - shows below it. Drawn full-window the two overlap and the prompt
// covers the input's bottom row; stacked, nothing is hidden, as on the DS.
//
// inputIsSub says which engine draws the input screen: TRUE for the sub
// engine, FALSE for the main engine (e.g. when the screen has swapped it onto
// the touch screen). The other engine's screen should be clear above its
// message box, since it shows in the bands either side of the input. Clicks
// reach the input screen through its placement.
static inline void SingleScreen_FrameTextInput(BOOL inputIsSub)
{
    SIM_MainOnlyPlacement promptFraming = SIM_MAIN_ONLY_PLACEMENT_BASE;
    SIM_MainOnlyPlacement inputFraming = SIM_MAIN_ONLY_PLACEMENT_OVERLAY;

    inputFraming.centerY = 0.625f;
    inputFraming.width = 0.75f;
    inputFraming.height = 0.75f;

    SIM_SetMainOnlyPlacement(inputIsSub, &inputFraming);
    SIM_SetMainOnlyPlacement(!inputIsSub, &promptFraming);
}

static inline BOOL SingleScreen_IsActive(void)
{
    return SIM_GetConfigPtr()->screenLayout == SIM_CONFIG_SCREEN_LAYOUT_MAIN_ONLY;
}

// Battles: the enemy's nameplate (healthbox) is raised to the top edge of the
// screen, and its Poke Ball row with it, to free the left side between it and
// the player's Pokemon for the command menu.
#define SINGLE_SCREEN_BATTLE_ENEMY_RAISE 20

// The main screen row just below the raised enemy healthbox, where the battle
// menus' top edge goes.
#define SINGLE_SCREEN_BATTLE_MENU_TOP 34

#endif // SINGLE_SCREEN_H
