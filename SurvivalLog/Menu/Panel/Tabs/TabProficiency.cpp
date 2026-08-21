#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 熟练度（mod proficiency） ----------
void TabProficiency()
{
            ImGui::Separator();
            // 熟练度（一行一个，各自带加经验/加1级）
            static SLProficiencyView profs[6];
            int npf = SLSDK_GetProficiencies(profs, 6);
            // 名称由 SDK 兜底（SystemName 空时填枚举名）
            static int prof_exp[6] = { 100, 100, 100, 100, 100, 100 };
            for (int i = 0; i < npf && i < 6; ++i)
            {
                ImGui::Text((const char *)u8"%d.%s Lv%d Exp%d/%d", profs[i].TypeId, profs[i].Name, profs[i].Level, profs[i].Exp, profs[i].NextLevelExp);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(110);
                char pe_label[24], exp_btn[32], lv_btn[32];
                snprintf(pe_label, sizeof(pe_label), (const char *)u8"##pe%d", i);
                snprintf(exp_btn, sizeof(exp_btn), (const char *)u8"加经验##%d", i);
                snprintf(lv_btn, sizeof(lv_btn), (const char *)u8"加1级##%d", i);
                ImGui::InputInt(pe_label, &prof_exp[i]);
                ImGui::SameLine();
                if (ImGui::SmallButton(exp_btn))
                    SLSDK_AddProficiencyExp(profs[i].TypeId, prof_exp[i]);
                ImGui::SameLine();
                if (ImGui::SmallButton(lv_btn))
                {
                    int32_t applied = 0;
                    SLSDK_AddProficiencyLevels(profs[i].TypeId, 1, &applied);
                }
            }
            if (npf <= 0)
                ImGui::Text((const char *)u8"熟练度数据不可用（请先进入游戏存档）");
            return;
}
