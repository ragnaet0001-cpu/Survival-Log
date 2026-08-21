#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 杂项（mod resources） ----------
void TabMisc()
{
            // ---------- 杂项（mod 生存规划/熟练度/关系/图鉴成就/暴露/生存点/移动热键） ----------
            ImGui::Text((const char *)u8"杂项");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }

            // 生存规划
            ImGui::Text((const char *)u8"生存规划（已激活）");
            static SLSurvivalPlanView plans[32];
            int np = SLSDK_GetSurvivalPlans(plans, 32);
            for (int i = 0; i < np && i < 32; ++i)
            {
                ImGui::Text((const char *)u8"  #%d %s (Lv%d)", plans[i].TalentId, plans[i].Name, plans[i].Level);
                char b1[32];
                snprintf(b1, sizeof(b1), (const char *)u8"移除##p%d", i);
                ImGui::SameLine();
                if (ImGui::SmallButton(b1))
                    SLSDK_RemoveSurvivalPlan(plans[i].TalentId);
            }
            static bool pcat_loaded = false;
            static std::vector<SLSurvivalPlanView> pcat;
            if (!pcat_loaded)
            {
                int npc = SLSDK_GetSurvivalPlanCatalog(nullptr, 0);
                if (npc > 0)
                {
                    pcat.resize((size_t)npc);
                    SLSDK_GetSurvivalPlanCatalog(pcat.data(), npc);
                    pcat_loaded = true;
                }
            }
            ImGui::Text((const char *)u8"生存规划目录（点击添加）");
            if (ImGui::Button((const char *)u8"刷新规划目录"))
            {
                int npc = SLSDK_GetSurvivalPlanCatalog(nullptr, 0);
                pcat.resize(npc > 0 ? (size_t)npc : 0);
                if (npc > 0)
                    SLSDK_GetSurvivalPlanCatalog(pcat.data(), npc);
                pcat_loaded = npc > 0;
            }
            if (pcat_loaded && !pcat.empty())
            {
                if (ImGui::BeginChild((const char *)u8"##plancat", ImVec2(0, 110), true))
                {
                    for (size_t i = 0; i < pcat.size(); ++i)
                    {
                        char label[256];
                        snprintf(label, sizeof(label), (const char *)u8"#%d %s %s##pc%d", pcat[i].TalentId, pcat[i].Name, pcat[i].Active ? (const char *)u8"[激活]" : (const char *)u8"[未激活]", (int)i);
                        if (ImGui::Selectable(label, false))
                            SLSDK_AddSurvivalPlan(pcat[i].TalentId);
                    }
                }
                ImGui::EndChild();
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
                if (no_exp)
                    SLSDK_ApplyNoExploreExposure(); // 每帧清零
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
            return;
}
