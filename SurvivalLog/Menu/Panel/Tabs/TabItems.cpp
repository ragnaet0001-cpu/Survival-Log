#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Items (mod items) ----------
void TabItems()
{
            // ---------- Items / Backpack (mod AddItem series) ----------
            ImGui::Text((const char *)u8"Items");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK not ready (waiting for game to load)...");
                return;
            }

            // Item catalog (matches mod ItemCatalog: search + click to select + add)
            static bool cat_loaded = false;
            static int cat_count = 0;
            static std::vector<SLItemInfo> catalog;
            if (!cat_loaded)
            {
                cat_count = SLSDK_RefreshItemCatalog();
                if (cat_count > 0)
                {
                    catalog.resize((size_t)cat_count);
                    SLSDK_GetItemCatalog(catalog.data(), cat_count);
                    cat_loaded = true;
                }
            }
            if (ImGui::Button((const char *)u8"Refresh Catalog"))
            {
                cat_count = SLSDK_RefreshItemCatalog();
                catalog.resize(cat_count > 0 ? (size_t)cat_count : 0);
                if (cat_count > 0)
                    SLSDK_GetItemCatalog(catalog.data(), cat_count);
                cat_loaded = cat_count > 0;
            }
            ImGui::SameLine();
            ImGui::Text((const char *)u8"Catalog: %d types", cat_count);
            static char search_buf[96] = {};
            ImGui::SetNextItemWidth(240);
            ImGui::InputText((const char *)u8"Search Name/ID", search_buf, sizeof(search_buf));
            static int sel_id = 0;
            static char sel_name[96] = {};
            if (cat_loaded && cat_count > 0)
            {
                if (ImGui::BeginChild((const char *)u8"##catalog", ImVec2(0, 200), true))
                {
                    int shown = 0;
                    int search_id = atoi(search_buf);
                    for (int i = 0; i < cat_count && shown < 300; ++i)
                    {
                        const SLItemInfo &it = catalog[i];
                        if (search_buf[0])
                        {
                            if (!strstr(it.Name, search_buf) && it.Id != search_id)
                                continue;
                        }
                        char label[200];
                        snprintf(label, sizeof(label), "#%d %s##c%d", it.Id, it.Name, i);
                        if (ImGui::Selectable(label, sel_id == it.Id))
                        {
                            sel_id = it.Id;
                            snprintf(sel_name, sizeof(sel_name), "%s", it.Name);
                        }
                        ++shown;
                    }
                }
                ImGui::EndChild();
            }
            ImGui::Text((const char *)u8"Selected: #%d %s", sel_id, sel_name);
            static int item_cnt = 1;
            ImGui::SetNextItemWidth(120);
            ImGui::InputInt((const char *)u8"Count##add", &item_cnt);
            if (item_cnt < 1)
                item_cnt = 1;
            static int add_state = 0; // 0=no action 1=all added 2=failed 3=partial
            if (ImGui::Button((const char *)u8"Add Selected Item"))
            {
                int added = 0;
                bool ok = SLSDK_AddItem(sel_id, item_cnt, &added);
                add_state = !ok ? 2 : (added >= item_cnt ? 1 : 3);
            }
            ImGui::SameLine();
            ImGui::Text(add_state == 1 ? (const char *)u8"[Added]" : add_state == 2 ? (const char *)u8"[Failed]" : add_state == 3 ? (const char *)u8"[Partially Added]" : "");
            return;
}
