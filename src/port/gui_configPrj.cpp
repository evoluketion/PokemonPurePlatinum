#include <nitro.h>
#include <simulator/sim.h>
#include <simulator/sim_gui.hpp>
#include <simulator/imgui/imgui.hpp>

#include "sim_gui_prj.hpp"

#include "port/sim_config_prj.h"
#include "particle_system_debug.h"

namespace SIM::GUI {

// Each column separates a stage that can fail on its own. emitters 0 means the
// effect was never created. particles 0 with emitters live means nothing is
// being emitted. visible 0 with particles live means they are being drawn fully
// transparent. child 0 where childCapable is non-zero means the child effect -
// trails and bursts - is not spawning.
static void ParticleSystemsPanel() {
    ParticleSystemDebugInfo info[8];
    int count = ParticleSystem_DebugSnapshot(info, 8);

    ImGui::Text("Particle systems");

    if(count == 0) {
        ImGui::Text("(none active)");
    }

    for(int i = 0; i < count; i++) {
        ImGui::Text("#%d  emitters %d  particles %d  visible %d",
            info[i].systemIndex, info[i].emitterCount,
            info[i].particleCount, info[i].visibleCount);
        ImGui::Text("     child %d  (childCapable emitters %d)",
            info[i].childParticleCount, info[i].childCapableCount);
    }
}

static void EmittersPanel() {
    // drawType: 0 billboard, 1 directional billboard, 2 polygon,
    // 3 directional polygon, 4 directional polygon (centered).
    static constexpr const char * DrawTypeNames[] = {
        "billboard", "dir-billboard", "polygon", "dir-polygon", "dir-polygon-c"
    };
    EmitterDebugInfo emitters[12];
    int count = ParticleSystem_DebugEmitters(emitters, 12);

    ImGui::Text("Emitters (first particle)");

    if(count == 0) {
        ImGui::Text("(none active)");
    }

    for(int i = 0; i < count; i++) {
        const char * name = (emitters[i].drawType >= 0 && emitters[i].drawType < 5)
            ? DrawTypeNames[emitters[i].drawType] : "?";

        ImGui::Text("#%d %-13s tex %d  n %d  a %d",
            emitters[i].systemIndex, name, emitters[i].textureIndex,
            emitters[i].particleCount, emitters[i].alpha);
        ImGui::Text("    pos %.1f %.1f %.1f  scale %.3f x %.3f",
            emitters[i].posX, emitters[i].posY, emitters[i].posZ,
            emitters[i].baseScale, emitters[i].animScale);
    }
}

static void ConfigCheckBox(const char * label, bool * configVar, bool& configChanged) {
    if(ImGui::Checkbox(label, configVar)) {
        configChanged = true;
    }
}

void ConfigPrjMain(bool * openState) {
    ImGui::Begin("pokeplatinum Config", openState);
    SIM_Config_prj_type * myConfig = SIM_Config_prj_GetConfig();
    bool configChanged = false;

    ConfigCheckBox("60 FPS", (bool*)&myConfig->enable60fps, configChanged);
    ConfigCheckBox("60 FPS Speed Fix", (bool*)&myConfig->enable60fpsSpeedFix, configChanged);
    ConfigCheckBox("Enable GF_Asserts", (bool*)&myConfig->enableAsserts, configChanged);
    ConfigCheckBox("DebugBreak on GF assertion fail", (bool*)&myConfig->breakDebuggerOnGfAssert, configChanged);

    if(configChanged) {
        SIM_Config_prj_SaveConfigFile(myConfig);
    }

    ImGui::Separator();
    ParticleSystemsPanel();
    ImGui::Separator();
    EmittersPanel();

    ImGui::End();
}
}