#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Prepare Phase (mod prepare: gold + time) ----------
void TabPrepare()
{
            ImGui::Text((const char *)u8"Prepare Phase");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }

            // Diagnostic info (used to locate instance-chain break points)
            //static char sdk_diag[1024] = {};
            //SLSDK_DebugInfo(sdk_diag, sizeof(sdk_diag));
            //ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            //ImGui::TextWrapped("%s", sdk_diag);
            //ImGui::PopStyleColor();
            //ImGui::Separator();

            // Gold (mod: shows CurrentGold + AddGold to increase)
            int curGold = SLSDK_GetGold();
            ImGui::Text((const char *)u8"Current Gold: %d", curGold);
            static int gold_add = 0;
            ImGui::InputInt((const char *)u8"Amount to Add", &gold_add);
            static int gold_add_state = 0; // 0=no action 1=success 2=failed
            if (ImGui::Button((const char *)u8"Add Gold"))
                gold_add_state = SLSDK_AddGold(gold_add) ? 1 : 2;
            ImGui::SameLine();
            ImGui::Text(gold_add_state == 1 ? (const char *)u8"[Added]" : gold_add_state == 2 ? (const char *)u8"[Failed]" : "");
            ImGui::Separator();


            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }
            int day = SLSDK_GameDay();
            int hour = SLSDK_GameHour();
            int total = SLSDK_GameTotalSeconds();
            float remain = SLSDK_RemainCountdownHour();
            bool frozen = SLSDK_IsClockFrozen();
            ImGui::Text((const char *)u8"Day %d", day);
            ImGui::Text((const char *)u8"Current Time: %02d:%02d", hour, (total >= 0 && hour >= 0) ? (total % 3600) / 60 : 0);
            ImGui::Text((const char *)u8"Total Seconds: %d", total);
            ImGui::Text((const char *)u8"Remaining Countdown: %.2f hours", remain);
            ImGui::Text((const char *)u8"Time Frozen: %s", frozen ? (const char *)u8"Yes" : (const char *)u8"No");
            ImGui::Separator();

            static int extend_hours = 1;
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"Extend Hours", &extend_hours);
            if (extend_hours < 1)
                extend_hours = 1;
            static int ext_state = 0; // 0=no action 1=success 2=failed
            if (ImGui::Button((const char *)u8"Extend Countdown"))
                ext_state = SLSDK_ExtendCountdown(extend_hours) ? 1 : 2;
            ImGui::SameLine();
            ImGui::Text(ext_state == 1 ? (const char *)u8"[Extended]" : ext_state == 2 ? (const char *)u8"[Failed, only available during Prepare Phase]" : "");
            ImGui::Separator();

            static bool time_freeze = false;
            if (ImGui::Checkbox((const char *)u8"Freeze Time", &time_freeze))
                SLSDK_SetTimeFrozen(time_freeze);
            // Per-frame freeze upkeep is handled centrally by PanelUpdateLocks (reads the SDK global FrozenOverride, persists across tabs)
            return;
}
