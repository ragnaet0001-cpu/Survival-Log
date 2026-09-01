#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Buffs (mod buffs: Current Effects / Survival Planning sub-tabs) ----------
void TabBuffs()
{
            // ---------- Buffs (mod GameApi.Buff series: sub-tabs matching mod buff-subnav) ----------
            ImGui::Text((const char *)u8"Buffs");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }

            // Sub-tabs: Current Effects / Survival Planning (mod buffView: current / planning)
            static int buff_sub = 0;
            ImGuiStyle &Style = ImGui::GetStyle();
            auto Color = Style.Colors;
            ImVec4 subColor0 = buff_sub == 0 ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, subColor0);
            if (ImGui::Button((const char *)u8"Current Effects", ImVec2(110, 0)))
                buff_sub = 0;
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImVec4 subColor1 = buff_sub == 1 ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, subColor1);
            if (ImGui::Button((const char *)u8"Survival Planning", ImVec2(110, 0)))
                buff_sub = 1;
            ImGui::PopStyleColor();
            ImGui::Separator();

            if (buff_sub == 0)
            {
                // ---------- Current Effects: current buff list ----------
                static SLBuffView buffs[64];
                int nb = SLSDK_GetBuffs(buffs, 64);
                ImGui::Text((const char *)u8"Active buffs: %d", nb);
                if (ImGui::BeginChild((const char *)u8"##bufflist", ImVec2(0, 150), true))
                {
                    for (int i = 0; i < nb && i < 64; ++i)
                    {
                        char line[192];
                        snprintf(line, sizeof(line), (const char *)u8"%s x%d (id=%lld)", buffs[i].Name, buffs[i].Layers, (long long)buffs[i].InstanceId);
                        ImGui::Text("%s", line);
                        char b1[32];
                        snprintf(b1, sizeof(b1), (const char *)u8"Remove##%d", i);
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

                // Add buff: search + select from config catalog
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
                if (ImGui::Button((const char *)u8"Refresh Buff Catalog"))
                {
                    bcat_count = SLSDK_GetBuffConfigs(nullptr, 0);
                    bcat.resize(bcat_count > 0 ? (size_t)bcat_count : 0);
                    if (bcat_count > 0)
                        SLSDK_GetBuffConfigs(bcat.data(), bcat_count);
                    bcat_loaded = bcat_count > 0;
                }
                ImGui::SameLine();
                ImGui::Text((const char *)u8"Buff catalog: %d types", bcat_count);
                static char bsearch[64] = {};
                ImGui::SetNextItemWidth(200);
                ImGui::InputText((const char *)u8"Search Buff", bsearch, sizeof(bsearch));
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
                ImGui::Text((const char *)u8"Selected: #%d %s", bsel_id, bsel_name);
                if (ImGui::Button((const char *)u8"Add Selected Buff"))
                    SLSDK_AddBuff(bsel_id);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Clear All Buffs"))
                    SLSDK_ClearAllBuffs();
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Remove Negative Buffs"))
                    SLSDK_RemoveAllNegativeBuffs();
            }
            else
            {
                // ---------- Survival Planning: active list ----------
                ImGui::Text((const char *)u8"Survival Planning (Active)");
                static SLSurvivalPlanView plans[32];
                int np = SLSDK_GetSurvivalPlans(plans, 32);
                if (ImGui::BeginChild((const char *)u8"##plans", ImVec2(0, 90), true))
                {
                    for (int i = 0; i < np && i < 32; ++i)
                    {
                        char line[192];
                        snprintf(line, sizeof(line), (const char *)u8"#%d %s (Lv%d)", plans[i].TalentId, plans[i].Name, plans[i].Level);
                        ImGui::Text("%s", line);
                        char b1[32];
                        snprintf(b1, sizeof(b1), (const char *)u8"Remove##p%d", i);
                        ImGui::SameLine();
                        if (ImGui::SmallButton(b1))
                            SLSDK_RemoveSurvivalPlan(plans[i].TalentId);
                    }
                }
                ImGui::EndChild();
                ImGui::Separator();

                // Survival planning catalog (mod survivalPlanCatalog: list + manual add)
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
                if (ImGui::Button((const char *)u8"Refresh Plan Catalog"))
                {
                    int npc = SLSDK_GetSurvivalPlanCatalog(nullptr, 0);
                    pcat.resize(npc > 0 ? (size_t)npc : 0);
                    if (npc > 0)
                        SLSDK_GetSurvivalPlanCatalog(pcat.data(), npc);
                    pcat_loaded = npc > 0;
                }
                ImGui::SameLine();
                ImGui::Text((const char *)u8"Survival planning catalog: %d types", (int)pcat.size());
                static char psearch[64] = {};
                static int psel_id = 0;
                static char psel_name[96] = {};
                ImGui::SetNextItemWidth(200);
                ImGui::InputText((const char *)u8"Search Plan", psearch, sizeof(psearch));
                if (pcat_loaded && !pcat.empty())
                {
                    if (ImGui::BeginChild((const char *)u8"##plancat", ImVec2(0, 110), true))
                    {
                        int shown = 0;
                        int pid = atoi(psearch);
                        for (size_t i = 0; i < pcat.size() && shown < 150; ++i)
                        {
                            if (psearch[0] && !strstr(pcat[i].Name, psearch) && pcat[i].TalentId != pid)
                                continue;
                            char label[256];
                            snprintf(label, sizeof(label), (const char *)u8"#%d %s %s##pc%d", pcat[i].TalentId, pcat[i].Name, pcat[i].Active ? (const char *)u8"[Active]" : (const char *)u8"[Available]", (int)i);
                            if (ImGui::Selectable(label, psel_id == pcat[i].TalentId))
                            {
                                psel_id = pcat[i].TalentId;
                                snprintf(psel_name, sizeof(psel_name), "%s", pcat[i].Name);
                            }
                            ++shown;
                        }
                    }
                    ImGui::EndChild();
                }
                ImGui::Text((const char *)u8"Selected: #%d %s", psel_id, psel_name);
                static bool p_add_ok = false;
                static bool p_add_tried = false;
                if (ImGui::Button((const char *)u8"Add Selected Plan"))
                {
                    p_add_ok = SLSDK_AddSurvivalPlan(psel_id);
                    p_add_tried = true;
                }
                ImGui::SameLine();
                if (p_add_tried)
                {
                    const char* perr = SLSDK_GetLastPlanError();
                    if (p_add_ok)
                    {
                        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), (const char *)u8"Added successfully");
                        p_add_tried = false;
                    }
                    else if (perr && perr[0])
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), (const char *)u8"Failed: %s", perr);
                    else
                        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), (const char *)u8"Add failed");
                }
            }
            return;
}
