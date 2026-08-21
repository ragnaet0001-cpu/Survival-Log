#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Buff（mod buffs） ----------
void TabBuffs()
{
            // ---------- Buff（mod GameApi.Buff 系列） ----------
            ImGui::Text((const char *)u8"Buff");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }

            // 当前 Buff 列表
            static SLBuffView buffs[64];
            int nb = SLSDK_GetBuffs(buffs, 64);
            ImGui::Text((const char *)u8"当前 Buff: %d 个", nb);
            if (ImGui::BeginChild((const char *)u8"##bufflist", ImVec2(0, 150), true))
            {
                for (int i = 0; i < nb && i < 64; ++i)
                {
                    char line[192];
                    snprintf(line, sizeof(line), (const char *)u8"%s x%d (id=%lld)", buffs[i].Name, buffs[i].Layers, (long long)buffs[i].InstanceId);
                    ImGui::Text("%s", line);
                    char b1[32];
                    snprintf(b1, sizeof(b1), (const char *)u8"移除##%d", i);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(b1))
                    {
                        if (buffs[i].InstanceId > 0)
                            SLSDK_RemoveBuff(buffs[i].InstanceId);
                        else
                            SLSDK_RemoveBuffByConfig(buffs[i].ConfigId);
                    }
                }
            }
            ImGui::EndChild();
            ImGui::Separator();

            // 添加 Buff：配置目录搜索选择
            static bool bcat_loaded = false;
            static int bcat_count = 0;
            static std::vector<SLBuffConfigView> bcat;
            if (!bcat_loaded)
            {
                bcat_count = SLSDK_GetBuffConfigs(nullptr, 0);
                if (bcat_count > 0)
                {
                    bcat.resize((size_t)bcat_count);
                    SLSDK_GetBuffConfigs(bcat.data(), bcat_count);
                    bcat_loaded = true;
                }
            }
            if (ImGui::Button((const char *)u8"刷新Buff目录"))
            {
                bcat_count = SLSDK_GetBuffConfigs(nullptr, 0);
                bcat.resize(bcat_count > 0 ? (size_t)bcat_count : 0);
                if (bcat_count > 0)
                    SLSDK_GetBuffConfigs(bcat.data(), bcat_count);
                bcat_loaded = bcat_count > 0;
            }
            ImGui::SameLine();
            ImGui::Text((const char *)u8"Buff 目录: %d 种", bcat_count);
            static char bsearch[64] = {};
            ImGui::SetNextItemWidth(200);
            ImGui::InputText((const char *)u8"搜索Buff", bsearch, sizeof(bsearch));
            static int bsel_id = 0;
            static char bsel_name[96] = {};
            if (bcat_loaded && bcat_count > 0)
            {
                if (ImGui::BeginChild((const char *)u8"##buffcat", ImVec2(0, 130), true))
                {
                    int shown = 0;
                    int sid = atoi(bsearch);
                    for (int i = 0; i < bcat_count && shown < 150; ++i)
                    {
                        if (bsearch[0] && !strstr(bcat[i].Name, bsearch) && bcat[i].ConfigId != sid)
                            continue;
                        char label[200];
                        snprintf(label, sizeof(label), "#%d %s##bc%d", bcat[i].ConfigId, bcat[i].Name, i);
                        if (ImGui::Selectable(label, bsel_id == bcat[i].ConfigId))
                        {
                            bsel_id = bcat[i].ConfigId;
                            snprintf(bsel_name, sizeof(bsel_name), "%s", bcat[i].Name);
                        }
                        ++shown;
                    }
                }
                ImGui::EndChild();
            }
            ImGui::Text((const char *)u8"选中: #%d %s", bsel_id, bsel_name);
            if (ImGui::Button((const char *)u8"添加选中Buff"))
                SLSDK_AddBuff(bsel_id);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"清空所有Buff"))
                SLSDK_ClearAllBuffs();
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"移除负面Buff"))
                SLSDK_RemoveAllNegativeBuffs();
            return;
}
