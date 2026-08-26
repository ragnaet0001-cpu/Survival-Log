#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 杂项（mod resources） ----------
void TabMisc()
{
            // ---------- 杂项（mod 熟练度/关系/图鉴成就/暴露/生存点/移动热键） ----------
            ImGui::Text((const char *)u8"杂项");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }

            // 关系 / 图鉴 / 成就
            SLRelationshipView rel = {};
            if (SLSDK_GetRelationship(&rel))
            {
                ImGui::Text((const char *)u8"邻居好感: %d/%d %s", rel.Affinity, rel.MaxAffinity, rel.Locked ? (const char *)u8"(未解锁)" : (const char *)u8"(已解锁)");
                static int rel_val = 0;
                if (rel_val <= 0)
                    rel_val = rel.MaxAffinity;
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt((const char *)u8"设置好感", &rel_val);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"应用好感"))
                    SLSDK_SetRelationship(rel_val);
            }
            if (ImGui::Button((const char *)u8"解锁全部图鉴"))
                SLSDK_UnlockAllCodex();
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"解锁全部成就"))
                SLSDK_UnlockAllAchievements();
            ImGui::Separator();

            // 暴露度 / 生存点
            float expCur = 0.0f, expTime = 0.0f, expMove = 0.0f;
            int32_t expMax = 0;
            bool expRun = false;
            if (SLSDK_GetExposure(&expCur, &expMax, &expTime, &expMove, &expRun))
            {
                ImGui::Text((const char *)u8"暴露度: %.0f/%.0f (时间 %.0f 移动 %.0f)", expCur, (float)expMax, expTime, expMove);
                static float exp_set = 0.0f;
                ImGui::SetNextItemWidth(100);
                ImGui::InputFloat((const char *)u8"设置暴露", &exp_set, 1.0f, 10.0f, "%.0f");
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"应用暴露"))
                    SLSDK_SetExposure(exp_set);
                static bool no_exp = false;
                if (ImGui::Checkbox((const char *)u8"防暴露(不增长)", &no_exp))
                    SLSDK_SetNoExploreExposure(no_exp);
                // 每帧清零由 PanelUpdateLocks 统一执行（读 SDK 全局开关，换页也生效）
            }
            else
            {
                ImGui::Text((const char *)u8"暴露度不可用（请进入探索场景）");
            }
            int sp = SLSDK_GetSurvivalPoints();
            ImGui::Text((const char *)u8"生存点: %d", sp);
            static int sp_set = 0;
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"设置生存点", &sp_set);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"应用生存点"))
                SLSDK_SetSurvivalPoints(sp_set);
            ImGui::Separator();

            // 移动 / 热键
            static bool move_block = false;
            if (ImGui::Checkbox((const char *)u8"锁定移动", &move_block))
                SLSDK_SetMovementBlocked(move_block);
            ImGui::SameLine();
            static bool hotkey_off = false;
            if (ImGui::Checkbox((const char *)u8"禁用游戏热键", &hotkey_off))
                SLSDK_SetHotKeyDisabled(hotkey_off);

            // 无限食物（mod SetInfiniteFoodShelfLife；真正无限由 hook 层实现）
            static bool inf_food = false;
            if (ImGui::Checkbox((const char *)u8"无限食物保质期", &inf_food))
            {
                int32_t affected = 0;
                SLSDK_SetInfiniteFoodShelfLife(inf_food, &affected);
            }
            ImGui::SameLine();
            if (SLSDK_InfiniteFoodEnabled())
                ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), (const char *)u8"[已开启]");

            // dexter.sl 倍率（动作速度/烹饪时间/暴露增长）
            ImGui::Separator();
            static int act_speed_mult = 1;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderInt((const char *)u8"动作速度倍率", &act_speed_mult, 1, 20);
            if (ImGui::Button((const char *)u8"应用##actspeed"))
                SLSDK_SetActionSpeedMultiplier(act_speed_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"还原##actspeed"))
                SLSDK_ResetActionSpeedMultiplier();
            static int cook_mult = 10;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderInt((const char *)u8"烹饪时间倍率", &cook_mult, 1, 50);
            if (ImGui::Button((const char *)u8"应用##cook"))
                SLSDK_SetCookingTimeMultiplier(cook_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"还原##cook"))
                SLSDK_ResetCookingTimeMultiplier();
            static float exp_rate = 1.0f;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"暴露增长倍率", &exp_rate, 0.05f, 1.0f, "%.2f");
            if (ImGui::Button((const char *)u8"应用##exprate"))
                SLSDK_SetExposureRate(exp_rate);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"还原##exprate"))
                SLSDK_ResetExposureRate();

            // 电力倍率（mod PowerManagerPatch：发电量/储电；应用=开启, 还原=关闭(倍率=1)）
            ImGui::Separator();
            static float gen_mult = 1.0f;
            static float cap_mult = 1.0f;
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"发电量倍率", &gen_mult, 1.0f, 200.0f, "%.0f");
            ImGui::SetNextItemWidth(110);
            ImGui::SliderFloat((const char *)u8"储电倍率", &cap_mult, 1.0f, 200.0f, "%.0f");
            if (ImGui::Button((const char *)u8"应用##power"))
                SLSDK_SetPowerMultiplier(gen_mult, cap_mult);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"还原##power"))
                SLSDK_ResetPowerMultiplier();

            return;
}
