#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- Bag (mod bag) ----------
void TabBag()
{
            ImGui::Text((const char *)u8"Bag");
            ImGui::Separator();
            // Backpack list (refreshed every frame, up to 256 entries)
            static SLItemView items_buf[256];
            int n = SLSDK_ListBackpackItems(items_buf, 256);
            ImGui::Text((const char *)u8"Backpack items: %d types", n);
            if (ImGui::BeginChild((const char *)u8"##itemslist", ImVec2(0, 240), true))
            {
                for (int i = 0; i < n && i < 256; ++i)
                {
                    char line[160];
                    snprintf(line, sizeof(line), "[%d] %s cfg=%d x%d (id=%lld)", i, SLSDK_GetItemName(items_buf[i].ConfigId), items_buf[i].ConfigId, items_buf[i].Count, (long long)items_buf[i].InstanceId);
                    ImGui::Text("%s", line);
                    char b1[32], b2[32];
                    snprintf(b1, sizeof(b1), (const char *)u8"Del##%d", i);
                    snprintf(b2, sizeof(b2), (const char *)u8"Dup##%d", i);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(b1))
                        SLSDK_RemoveBackpackItem(items_buf[i].InstanceId);
                    ImGui::SameLine();
                    if (ImGui::SmallButton(b2))
                        SLSDK_DuplicateBackpackItem(items_buf[i].InstanceId);
                }
            }
            ImGui::EndChild();
            ImGui::Separator();

            // Set the count of a single item (automatically opens a new stack past the stack limit)
            static int64_t set_id = 0;
            static int set_cnt = 1;
            ImGui::SetNextItemWidth(100);
            ImGui::InputScalar((const char *)u8"Item InstanceId", ImGuiDataType_S64, &set_id);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"Count##set", &set_cnt);
            if (set_cnt < 1)
                set_cnt = 1;
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"Set Count"))
                SLSDK_SetBackpackItemCount(set_id, set_cnt);
            ImGui::Separator();

            // Bag size / carry weight (mod BackpackManager)
            SLBagInfo baginfo = {};
            if (SLSDK_GetBagInfo(&baginfo))
            {
                ImGui::Text((const char *)u8"Bag: %dx%d  Max Weight %d", baginfo.Columns, baginfo.Rows, baginfo.MaxBurden);
                static int bag_cols = 8, bag_rows = 6;
                if (baginfo.Columns > 0 && bag_cols <= 0)
                    bag_cols = baginfo.Columns;
                if (baginfo.Rows > 0 && bag_rows <= 0)
                    bag_rows = baginfo.Rows;
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt((const char *)u8"Cols##bc", &bag_cols);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt((const char *)u8"Rows##br", &bag_rows);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Apply Size"))
                {
                    if (SLSDK_SetBagSize(bag_cols, bag_rows))
                    {
                        bag_cols = baginfo.Columns;
                        bag_rows = baginfo.Rows;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Reset Size"))
                    SLSDK_ResetBagSize();
                static int max_burden = 0;
                if (max_burden <= 0)
                    max_burden = baginfo.MaxBurden > 0 ? baginfo.MaxBurden : 100;
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt((const char *)u8"Max Weight", &max_burden);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Apply Weight"))
                    SLSDK_SetMaxBurden(max_burden);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"Reset Weight"))
                    SLSDK_ResetMaxBurden();
                ImGui::Separator();
                ImGui::Text((const char *)u8"Storage/Shelf Expansion (auto-detects storage racks/shelves/fridges, takes effect on panel reopen)");
                {
                    int32_t cr = 1, cb = 1;
                    SLSDK_GetContainerExpansion(&cr, &cb);
                    static int container_rows_mult = 1, container_burden_mult = 1;
                    if (cr != 1 || cb != 1)
                    {
                        container_rows_mult = cr;
                        container_burden_mult = cb;
                    }
                    ImGui::SetNextItemWidth(90);
                    ImGui::InputInt((const char *)u8"Row Multiplier##crows", &container_rows_mult);
                    ImGui::SameLine();
                    ImGui::SetNextItemWidth(90);
                    ImGui::InputInt((const char *)u8"Weight Multiplier##cburden", &container_burden_mult);
                    if (container_rows_mult < 1)
                        container_rows_mult = 1;
                    if (container_burden_mult < 1)
                        container_burden_mult = 1;
                    ImGui::SameLine();
                    if (ImGui::Button((const char *)u8"Apply Storage Expansion"))
                        SLSDK_SetContainerExpansion(container_rows_mult, container_burden_mult);
                    ImGui::SameLine();
                    if (ImGui::Button((const char *)u8"Reset Storage Expansion"))
                    {
                        SLSDK_SetContainerExpansion(1, 1);
                        container_rows_mult = 1;
                        container_burden_mult = 1;
                    }
                }
            }
            else
            {
                ImGui::Text((const char *)u8"Bag info unavailable (please load a save first)");
            }
            return;
}
