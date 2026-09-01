#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"
#include <stdio.h>

// ---------- Zombie (Horde Kill) ----------
void TabZombie()
{
    ImGui::Text("Zombie (Horde)");
    ImGui::Separator();
    if (!SLSDK_Ready())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "SDK not ready (waiting for game to load)...");
        return;
    }

    static int last_kill_all = -1;
    if (ImGui::Button("Kill All Zombies"))
        last_kill_all = SLSDK_KillAllZombies();
    if (last_kill_all >= 0)
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Killed %d zombies", last_kill_all);
    ImGui::Separator();

    // Enumerate list
    static SLZombieView zombies[256];
    int32_t n = SLSDK_GetZombies(zombies, 256);
    ImGui::Text("Current zombies: %d", n);
    if (n <= 0)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "(No living zombies)");
        return;
    }
    if (n > 256)
        n = 256;
    for (int32_t i = 0; i < n; ++i)
    {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), "Kill##z%d", i);
        ImGui::Text("ID %lld   HP %d/%d", (long long)zombies[i].InstanceId, zombies[i].CurrentHP, zombies[i].MaxHP);
        ImGui::SameLine();
        if (ImGui::Button(lbl))
            SLSDK_KillZombie(zombies[i].InstanceId);
    }
}
