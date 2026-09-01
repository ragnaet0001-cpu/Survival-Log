#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Attributes (mod attributes) ----------
void TabAttributes()
{
            ImGui::Text((const char *)u8"Attributes");
            ImGui::Separator();
            // Attributes (mod full version: current value + max + lock + move speed; value is the actual value, e.g. 20 = 20.0)
            static const int attr_keys[5] = { 1, 2, 3, 5, 4 }; // Satiety/Morale/Stamina/Vitality/Health
            static const int attr_max_keys[5] = { 101, 102, 103, 105, 104 };
            static const char *attr_names[5] = {
                (const char *)u8"Satiety",
                (const char *)u8"Morale",
                (const char *)u8"Stamina",
                (const char *)u8"Vitality",
                (const char *)u8"Health",
            };
            static float attr_vals[5] = { 0, 0, 0, 0, 0 };
            for (int i = 0; i < 5; ++i)
            {
                int cur = SLSDK_GetAttr(attr_keys[i]);
                int mx = SLSDK_GetAttrMax(attr_max_keys[i]);
                if (cur < 0)
                    continue;
                float disp = (float)cur; // mod semantics: actual value from GetTotalValue_Float
                bool changed = ImGui::SliderFloat(attr_names[i], &attr_vals[i], 0.0f, 100.0f, "%.1f");
                if (!ImGui::IsItemActive())
                    attr_vals[i] = disp; // follow the game's value while the slider isn't active
                if (changed)
                    SLSDK_SetAttr(attr_keys[i], (int)(attr_vals[i] + 0.5f));
                ImGui::SameLine();
                ImGui::Text((const char *)u8"Max:%d", mx >= 0 ? mx : -1);
            }
            // Set max value
            static int max_sel = 0;
            static int max_edit = 100;
            ImGui::SetNextItemWidth(120);
            ImGui::Combo((const char *)u8"##maxsel", &max_sel, attr_names, 5);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"Max Value", &max_edit);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Apply Max"))
                SLSDK_SetAttrMax(attr_max_keys[max_sel], max_edit);
            ImGui::Separator();

            // Attribute locks (checkbox only changes global state; refilled every frame by PanelUpdateLocks, persists across tabs)
            ImGui::Checkbox((const char *)u8"Lock HP", &g_lock_hp);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"Lock Stamina", &g_lock_sta);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"Lock Satiety", &g_lock_sat);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"Lock Morale", &g_lock_mor);
            ImGui::Separator();

            // Move speed multiplier (mod SetMoveSpeedMultiplier: 0.5-5.0)
            static float speed_mult = 1.0f;
            float spd_cur = -1.0f, spd_orig = -1.0f, spd_mult = 1.0f;
            if (SLSDK_GetMoveSpeed(&spd_cur, &spd_orig, &spd_mult))
            {
                ImGui::Text((const char *)u8"Current Speed: %.1f  Original: %.1f", spd_cur, spd_orig);
                if (ImGui::SliderFloat((const char *)u8"Speed Multiplier", &speed_mult, 0.5f, 5.0f, "%.2f"))
                    SLSDK_SetMoveSpeedMultiplier(speed_mult);
                if (!ImGui::IsItemActive() && spd_mult != speed_mult)
                    speed_mult = spd_mult;
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Reset Speed"))
                {
                    SLSDK_ResetMoveSpeed();
                    speed_mult = 1.0f;
                }
            }
            else
            {
                speed_mult = 1.0f;
                ImGui::Text((const char *)u8"Speed unavailable (please load a save first)");
            }
            return;
}
