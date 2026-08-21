#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 准备阶段（mod prepare：金币 + 时间） ----------
void TabPrepare()
{
            ImGui::Text((const char *)u8"准备阶段");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }

            // 诊断信息（定位实例链断点）
            static char sdk_diag[1024] = {};
            SLSDK_DebugInfo(sdk_diag, sizeof(sdk_diag));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
            ImGui::TextWrapped("%s", sdk_diag);
            ImGui::PopStyleColor();
            ImGui::Separator();

            // 金币（mod：CurrentGold 显示 + AddGold 增加）
            int curGold = SLSDK_GetGold();
            ImGui::Text((const char *)u8"当前金币: %d", curGold);
            static int gold_add = 0;
            ImGui::InputInt((const char *)u8"增加数量", &gold_add);
            static int gold_add_state = 0; // 0=未操作 1=成功 2=失败
            if (ImGui::Button((const char *)u8"增加金币"))
                gold_add_state = SLSDK_AddGold(gold_add) ? 1 : 2;
            ImGui::SameLine();
            ImGui::Text(gold_add_state == 1 ? (const char *)u8"[已增加]" : gold_add_state == 2 ? (const char *)u8"[失败]" : "");
            ImGui::Separator();


            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }
            int day = SLSDK_GameDay();
            int hour = SLSDK_GameHour();
            int total = SLSDK_GameTotalSeconds();
            float remain = SLSDK_RemainCountdownHour();
            bool frozen = SLSDK_IsClockFrozen();
            ImGui::Text((const char *)u8"第 %d 天", day);
            ImGui::Text((const char *)u8"当前时刻: %02d:%02d", hour, (total >= 0 && hour >= 0) ? (total % 3600) / 60 : 0);
            ImGui::Text((const char *)u8"总秒: %d", total);
            ImGui::Text((const char *)u8"剩余倒计时: %.2f 小时", remain);
            ImGui::Text((const char *)u8"时间冻结: %s", frozen ? (const char *)u8"是" : (const char *)u8"否");
            ImGui::Separator();

            static int extend_hours = 1;
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"延长小时", &extend_hours);
            if (extend_hours < 1)
                extend_hours = 1;
            static int ext_state = 0; // 0=未操作 1=成功 2=失败
            if (ImGui::Button((const char *)u8"延长倒计时"))
                ext_state = SLSDK_ExtendCountdown(extend_hours) ? 1 : 2;
            ImGui::SameLine();
            ImGui::Text(ext_state == 1 ? (const char *)u8"[已延长]" : ext_state == 2 ? (const char *)u8"[失败，仅准备阶段可用]" : "");
            ImGui::Separator();

            static bool time_freeze = false;
            if (ImGui::Checkbox((const char *)u8"冻结时间", &time_freeze))
                SLSDK_SetTimeFrozen(time_freeze);
            if (time_freeze)
                SLSDK_ApplyFrozenOverride(); // 每帧保持冻结（mod FrozenOverride 同款）
            return;
}
