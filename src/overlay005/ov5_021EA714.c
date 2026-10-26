#include "overlay005/ov5_021EA714.h"

#include <nitro.h>
#include <string.h>

#include <simulator/sim.h>

#include "applications/poketch/unavailable/graphics.h"
#include "field/field_system.h"
#include "field/field_system_sub2_t.h"

#include "game_overlay.h"
#include "poketch.h"
#include "render_oam.h"
#include "system.h"
#include "system_flags.h"
#include "vars_flags.h"

FS_EXTERN_OVERLAY(poketch_unavailable);
FS_EXTERN_OVERLAY(poketch);

// fieldSystem->bottomScreen values for the two screens set up here: the Poketch
// (or its unavailable wallpaper) on the overworld, and the wallpaper alone.
#define FIELD_BOTTOM_SCREEN_POKETCH     1
#define FIELD_BOTTOM_SCREEN_UNAVAILABLE 4

// Under main_only single-screen mode the overworld fills the window and the
// bottom screen is shown, if at all, as a panel over it. The Poketch sits in the
// top-right corner, opened and minimised with R - which the overworld otherwise
// ignores - and tucked away whenever the player isn't free to move: dialogue,
// the start menu and cutscenes all draw over the right and bottom of the map,
// and that's when the DS puts the Poketch to sleep anyway. The wallpaper shown
// before the Poketch is obtained has nothing on it worth a panel. Clicks on the
// panel reach the Poketch through the placement.
static BOOL sPoketchPanelOpen = FALSE;

static const SIM_MainOnlyPlacement sPoketchPanelFraming = {
    .centerX = 0.77f,
    .centerY = 0.765f,
    .width = 0.42f,
    .height = 0.42f,
    .srcCenterX = 0.5f,
    .srcCenterY = 0.5f,
    .srcWidth = 1.0f,
    .srcHeight = 1.0f,
    .cutout = FALSE,
    .defringe = FALSE,
    .hidden = FALSE,
};

static const SIM_MainOnlyPlacement sBottomScreenHiddenFraming = {
    .centerX = 0.5f,
    .centerY = 0.5f,
    .width = 1.0f,
    .height = 1.0f,
    .srcCenterX = 0.5f,
    .srcCenterY = 0.5f,
    .srcWidth = 1.0f,
    .srcHeight = 1.0f,
    .cutout = TRUE,
    .defringe = FALSE,
    .hidden = TRUE,
};

static BOOL FieldSystem_IsMainOnly(void)
{
    return SIM_GetConfigPtr()->screenLayout == SIM_CONFIG_SCREEN_LAYOUT_MAIN_ONLY;
}

static BOOL FieldSystem_HasPoketchBottomScreen(FieldSystem *fieldSystem)
{
    return fieldSystem->bottomScreen == FIELD_BOTTOM_SCREEN_POKETCH
        && fieldSystem->unk_04 != NULL
        && fieldSystem->unk_04->poketchSys != NULL;
}

void FieldSystem_UpdateBottomScreenFraming(FieldSystem *fieldSystem, BOOL playerCanMove)
{
    if (!FieldSystem_IsMainOnly() || !fieldSystem->runningFieldMap) {
        return;
    }

    // The Underground and the other bottom screens aren't converted yet.
    if (fieldSystem->bottomScreen != FIELD_BOTTOM_SCREEN_POKETCH
        && fieldSystem->bottomScreen != FIELD_BOTTOM_SCREEN_UNAVAILABLE) {
        return;
    }

    BOOL hasPoketch = FieldSystem_HasPoketchBottomScreen(fieldSystem);

    if (hasPoketch && playerCanMove && (gSystem.pressedKeys & PAD_BUTTON_R)) {
        sPoketchPanelOpen = !sPoketchPanelOpen;
    }

    // Restated every frame rather than at transitions, so whatever an
    // application run from the field left behind is corrected on return.
    SIM_MainOnlyPlacement mainFraming = SIM_MAIN_ONLY_PLACEMENT_BASE;
    SIM_SetMainOnlyPlacement(FALSE, &mainFraming);

    if (hasPoketch && playerCanMove && sPoketchPanelOpen) {
        SIM_SetMainOnlyPlacement(TRUE, &sPoketchPanelFraming);
    } else {
        SIM_SetMainOnlyPlacement(TRUE, &sBottomScreenHiddenFraming);
    }
}

// The bottom screen is being torn down: keep it hidden while it fades out, and
// hand the default framing back once it's gone so whatever's next starts clean.
static void FieldSystem_HideBottomScreenFraming(void)
{
    if (FieldSystem_IsMainOnly()) {
        SIM_SetMainOnlyPlacement(TRUE, &sBottomScreenHiddenFraming);
    }
}

void FieldSystem_SendPoketchEvent(FieldSystem *fieldSystem, enum PoketchEventID eventID, u32 dummy)
{
    if (fieldSystem->unk_04 != NULL && fieldSystem->unk_04->poketchSys != NULL) {
        PoketchSystem_SendEvent(fieldSystem->unk_04->poketchSys, eventID, dummy);
    }
}

// Whether ov5_021EA728 set up the Poketch rather than its unavailable
// wallpaper. Tearing down has to match what was set up.
static BOOL sPoketchSetUp;

static BOOL FieldSystem_ShouldShowPoketch(FieldSystem *fieldSystem)
{
    Poketch *poketch = SaveData_GetPoketch(fieldSystem->saveData);
    VarsFlags *varsFlags = SaveData_GetVarsFlags(fieldSystem->saveData);
    return Poketch_IsEnabled(poketch) && (SystemFlag_CheckPoketchHidden(varsFlags) == 0);
}

void ov5_021EA728(FieldSystem *fieldSystem)
{
    sPoketchSetUp = FieldSystem_ShouldShowPoketch(fieldSystem);

    if (sPoketchSetUp) {
        Overlay_LoadByID(FS_OVERLAY_ID(poketch), 2);
        PoketchSystem_Create(fieldSystem, &fieldSystem->unk_04->poketchSys, fieldSystem->saveData, fieldSystem->bgConfig, RenderOam_GetScreenOam(1));
    } else {
        Overlay_LoadByID(FS_OVERLAY_ID(poketch_unavailable), 2);
        PoketchUnavailableScreen_Init(fieldSystem->bgConfig);
    }
}

void ov5_021EA790(FieldSystem *fieldSystem)
{
    FieldSystem_HideBottomScreenFraming();

    if (sPoketchSetUp) {
        PoketchSystem_StartShutdown(fieldSystem->unk_04->poketchSys);
    } else {
        PoketchUnavailableScreen_Exit(fieldSystem->bgConfig);
    }
}

u8 ov5_021EA7CC(FieldSystem *fieldSystem)
{
    if (sPoketchSetUp) {
        if (PoketchSystem_IsSystemShutdown(fieldSystem->unk_04->poketchSys)) {
            fieldSystem->unk_04->poketchSys = NULL;
            Overlay_UnloadByID(FS_OVERLAY_ID(poketch));
            SIM_ResetMainOnlyPlacements();
            return 1;
        }
    } else {
        if (PoketchUnavailableScreen_IsDone(fieldSystem->bgConfig)) {
            Overlay_UnloadByID(FS_OVERLAY_ID(poketch_unavailable));
            SIM_ResetMainOnlyPlacements();
            return 1;
        }
    }

    return 0;
}

void ov5_021EA830(FieldSystem *fieldSystem)
{
    Overlay_LoadByID(FS_OVERLAY_ID(poketch_unavailable), 2);
    PoketchUnavailableScreen_Init(fieldSystem->bgConfig);
}

void ov5_021EA848(FieldSystem *fieldSystem)
{
    FieldSystem_HideBottomScreenFraming();
    PoketchUnavailableScreen_Exit(fieldSystem->bgConfig);
}

BOOL ov5_021EA854(FieldSystem *fieldSystem)
{
    if (PoketchUnavailableScreen_IsDone(fieldSystem->bgConfig)) {
        Overlay_UnloadByID(FS_OVERLAY_ID(poketch_unavailable));
        SIM_ResetMainOnlyPlacements();
        return 1;
    }

    return 0;
}
