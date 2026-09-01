#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Misc (mod resources) ----------
void TabMisc()
{
            // ---------- Misc (mod proficiency/relationship/codex-achievements/exposure/survival points/movement hotkeys) ----------
            ImGui::Text((const char *)u8"Misc");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }

            // Relationship / codex / achievements
            SLRelationshipView rel = {};
            if (SLSDK_GetRelationship(&rel))
            {
                ImGui::Text((const char *)u8"Neighbor Affinity: %d/%d %s", rel.Affinity, rel.MaxAffinity, rel.Locked ? (const char *)u8"(Locked)" : (const char *)u8"(Unlocked)");
                static int rel_val = 0;
                if (rel_val <= 0)
                    rel_val = rel.MaxAffinity;
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt((const char *)u8"Set Affinity", &rel_val);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Apply Affinity"))
                    SLSDK_SetRelationship(rel_val);
            }
            if (ImGui::Button((const char *)u8"Unlock All Codex"))
                SLSDK_UnlockAllCodex();
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Unlock All Achievements"))
                SLSDK_UnlockAllAchievements();
            ImGui::Separator();

            // Exposure / survival points
            float expCur = 0.0f, expTime = 0.0f, expMove = 0.0f;
            int32_t expMax = 0;
            bool expRun = false;
            if (SLSDK_GetExposure(&expCur, &expMax, &expTime, &expMove, &expRun))
            {
                ImGui::Text((const char *)u8"Exposure: %.0f/%.0f (Time %.0f Movement %.0f)", expCur, (float)expMax, expTime, expMove);
                static float exp_set = 0.0f;
                ImGui::SetNextItemWidth(100);
                ImGui::InputFloat((const char *)u8"Set Exposure", &exp_set, 1.0f, 10.0f, "%.0f");
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Apply Exposure"))
                    SLSDK_SetExposure(exp_set);
                static bool no_exp = false;
                if (ImGui::Checkbox((const char *)u8"Prevent Exposure Growth", &no_exp))
                    SLSDK_SetNoExploreExposure(no_exp);
                // Per-frame reset is handled centrally by PanelUpdateLocks (reads the SDK global flag, persists across tabs)
            }
            else
            {
                ImGui::Text((const char *)u8"Exposure unavailable (please enter an exploration scene)");
            }
            int sp = SLSDK_GetSurvivalPoints();
            ImGui::Text((const char *)u8"Survival Points: %d", sp);
            static int sp_set = 0;
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"Set Survival Points", &sp_set);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Apply Survival Points"))
                SLSDK_SetSurvivalPoints(sp_set);
            ImGui::Separator();

            // Movement / hotkeys
            static bool move_block = false;
            if (ImGui::Checkbox((const char *)u8"Lock Movement", &move_block))
                SLSDK_SetMovementBlocked(move_block);
            ImGui::SameLine();
            static bool hotkey_off = false;
            if (ImGui::Checkbox((const char *)u8"Disable Game Hotkeys", &hotkey_off))
                SLSDK_SetHotKeyDisabled(hotkey_off);

            // Infinite food (mod SetInfiniteFoodShelfLife; the actual infinite behavior is implemented in the hook layer)
            static bool inf_food = false;
            if (ImGui::Checkbox((const char *)u8"Infinite Food Shelf Life", &inf_food))
            {
                int32_t affected = 0;
                SLSDK_SetInfiniteFoodShelfLife(inf_food, &affected);
            }
            ImGui::SameLine();
            if (SLSDK_InfiniteFoodEnabled())
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), (const char *)u8"[Enabled]");

            // dexter.sl multipliers (action speed / cooking time / exposure growth)
            ImGui::Separator();
            static int act_speed_mult = 1;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderInt((const char *)u8"Action Speed Multiplier", &act_speed_mult, 1, 20);
            if (ImGui::Button((const char *)u8"Apply##actspeed"))
                SLSDK_SetActionSpeedMultiplier(act_speed_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Reset##actspeed"))
                SLSDK_ResetActionSpeedMultiplier();
            static int cook_mult = 10;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderInt((const char *)u8"Cooking Time Multiplier", &cook_mult, 1, 50);
            if (ImGui::Button((const char *)u8"Apply##cook"))
                SLSDK_SetCookingTimeMultiplier(cook_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Reset##cook"))
                SLSDK_ResetCookingTimeMultiplier();
            static float exp_rate = 1.0f;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"Exposure Growth Multiplier", &exp_rate, 0.05f, 1.0f, "%.2f");
            if (ImGui::Button((const char *)u8"Apply##exprate"))
                SLSDK_SetExposureRate(exp_rate);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Reset##exprate"))
                SLSDK_ResetExposureRate();

            // Power multipliers (mod PowerManagerPatch: generation/storage; Apply = enable, Reset = disable (multiplier = 1))
            ImGui::Separator();
            static float gen_mult = 1.0f;
            static float cap_mult = 1.0f;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"Power Generation Multiplier", &gen_mult, 1.0f, 200.0f, "%.0f");
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"Power Storage Multiplier", &cap_mult, 1.0f, 200.0f, "%.0f");
            if (ImGui::Button((const char *)u8"Apply##power"))
                SLSDK_SetPowerMultiplier(gen_mult, cap_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Reset##power"))
                SLSDK_ResetPowerMultiplier();

            return;
}
