#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"
#include <stdio.h>

// ---------- É¥Ê¬£¨Ê¬³±»÷É±£© ----------
void TabZombie()
{
    ImGui::Text((const char *)u8"É¥Ê¬£¨Ê¬³±£©");
    ImGui::Separator();
    if (!SLSDK_Ready())
    {
        ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK Î´¾ÍÐ÷£¨µÈ´ýÓÎÏ·¼ÓÔØ£©...");
        return;
    }


    static int last_kill_all = -1;
    if (ImGui::Button((const char *)u8"»÷É±È«²¿É¥Ê¬"))
        last_kill_all = SLSDK_KillAllZombies();
    if (last_kill_all >= 0)
        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), (const char *)u8"ÒÑ»÷É± %d Ö»", last_kill_all);
    ImGui::Separator();

    // Ã¶¾ÙÁÐ±í
    static SLZombieView zombies[256];
    int32_t n = SLSDK_GetZombies(zombies, 256);
    ImGui::Text((const char *)u8"µ±Ç°É¥Ê¬: %d Ö»", n);
    if (n <= 0)
    {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), (const char *)u8"£¨ÎÞ´æ»îÉ¥Ê¬£©");
        return;
    }
    if (n > 256)
        n = 256;
    for (int32_t i = 0; i < n; ++i)
    {
        char lbl[64];
        snprintf(lbl, sizeof(lbl), (const char*)u8"»÷É±##z%d", i);
        ImGui::Text((const char *)u8"ID %lld   HP %d/%d", (long long)zombies[i].InstanceId, zombies[i].CurrentHP, zombies[i].MaxHP);
        ImGui::SameLine();
        if (ImGui::Button(lbl))
            SLSDK_KillZombie(zombies[i].InstanceId);
    }
}
