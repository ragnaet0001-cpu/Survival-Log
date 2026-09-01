#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Facilities (mod facilities: slot durability, one row per type + per-row lock) ----------
void TabFacilities()
{
            ImGui::Text((const char *)u8"Facilities");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }

            // Slot types (Config_SlotType, verified at runtime: 1 Small 2 Medium 3 Large 4 Wall-mounted 5 Central
            // 6 Tabletop 7 Bed 8 Door 9 Window 10 Tower defense device)
            static const char* SLOT_NAMES[10] = {
                (const char *)u8"Small", (const char *)u8"Medium", (const char *)u8"Large", (const char *)u8"Wall-mounted", (const char *)u8"Central",
                (const char *)u8"Tabletop", (const char *)u8"Bed", (const char *)u8"Door", (const char *)u8"Window", (const char *)u8"Tower Defense Device",
            };

            // Lock durability value (shared across all types; checked types are kept every frame by PanelUpdateLocks)
            ImGui::Text((const char *)u8"Lock Durability Value:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"##durval", &g_lock_dur_value);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Apply to All Checked"))
            {
                for (int i = 0; i < 10; i++)
                {
                    if (g_lock_dur_slots[i])
                        SLSDK_ApplyDurabilityLockSlot(i + 1, g_lock_dur_value);
                }
            }
            ImGui::Separator();

            // One row per type: name (count min~max) + lock checkbox + apply button
            for (int i = 0; i < 10; i++)
            {
                int32_t cnt = 0, mn = -1, mx = -1;
                SLSDK_GetHomeDurabilitySummary(i + 1, &cnt, &mn, &mx);
                char line[160];
                snprintf(line, sizeof(line), (const char *)u8"#%d %s (%d items %d~%d)", i + 1, SLOT_NAMES[i], cnt, mn, mx);
                ImGui::Text("%s", line);
                ImGui::SameLine();
                char lock_label[24], apply_btn[32];
                snprintf(lock_label, sizeof(lock_label), (const char *)u8"Lock##f%d", i);
                snprintf(apply_btn, sizeof(apply_btn), (const char *)u8"Apply##fa%d", i);
                ImGui::Checkbox(lock_label, &g_lock_dur_slots[i]);
                ImGui::SameLine();
                if (ImGui::SmallButton(apply_btn))
                {
                    int32_t updated = 0;
                    SLSDK_SetHomeDurability(i + 1, g_lock_dur_value, &updated);
                }
            }
            return;
}
