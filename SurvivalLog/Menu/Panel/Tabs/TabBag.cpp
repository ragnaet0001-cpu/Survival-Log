#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 背包（mod bag） ----------
void TabBag()
{
            ImGui::Text((const char *)u8"背包");
            ImGui::Separator();
            // 背包列表（每帧刷新，最多 256 条）
            static SLItemView items_buf[256];
            int n = SLSDK_ListBackpackItems(items_buf, 256);
            ImGui::Text((const char *)u8"背包物品: %d 种", n);
            if (ImGui::BeginChild((const char *)u8"##itemslist", ImVec2(0, 240), true))
            {
                for (int i = 0; i < n && i < 256; ++i)
                {
                    char line[160];
                    snprintf(line, sizeof(line), "[%d] %s cfg=%d x%d (id=%lld)", i, SLSDK_GetItemName(items_buf[i].ConfigId), items_buf[i].ConfigId, items_buf[i].Count, (long long)items_buf[i].InstanceId);
                    ImGui::Text("%s", line);
                    char b1[32], b2[32];
                    snprintf(b1, sizeof(b1), (const char *)u8"删##%d", i);
                    snprintf(b2, sizeof(b2), (const char *)u8"复##%d", i);
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

            // 设置单个物品数量（超堆叠上限自动开新堆）
            static int64_t set_id = 0;
            static int set_cnt = 1;
            ImGui::SetNextItemWidth(100);
            ImGui::InputScalar((const char *)u8"物品 InstanceId", ImGuiDataType_S64, &set_id);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"数量##set", &set_cnt);
            if (set_cnt < 1)
                set_cnt = 1;
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"设置数量"))
                SLSDK_SetBackpackItemCount(set_id, set_cnt);
            ImGui::Separator();

            // 背包尺寸 / 负重（mod BackpackManager）
            SLBagInfo baginfo = {};
            if (SLSDK_GetBagInfo(&baginfo))
            {
                ImGui::Text((const char *)u8"背包: %dx%d  负重 %d", baginfo.Columns, baginfo.Rows, baginfo.MaxBurden);
                static int bag_cols = 8, bag_rows = 6;
                if (baginfo.Columns > 0 && bag_cols <= 0)
                    bag_cols = baginfo.Columns;
                if (baginfo.Rows > 0 && bag_rows <= 0)
                    bag_rows = baginfo.Rows;
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt((const char *)u8"列##bc", &bag_cols);
                ImGui::SameLine();
                ImGui::SetNextItemWidth(80);
                ImGui::InputInt((const char *)u8"行##br", &bag_rows);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"应用尺寸"))
                {
                    if (SLSDK_SetBagSize(bag_cols, bag_rows))
                    {
                        bag_cols = baginfo.Columns;
                        bag_rows = baginfo.Rows;
                    }
                }
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"还原尺寸"))
                    SLSDK_ResetBagSize();
                static int max_burden = 0;
                if (max_burden <= 0)
                    max_burden = baginfo.MaxBurden > 0 ? baginfo.MaxBurden : 100;
                ImGui::SetNextItemWidth(100);
                ImGui::InputInt((const char *)u8"最大负重", &max_burden);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"应用负重"))
                    SLSDK_SetMaxBurden(max_burden);
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"还原负重"))
                    SLSDK_ResetMaxBurden();
            }
            else
            {
                ImGui::Text((const char *)u8"背包信息不可用（请先进入游戏存档）");
            }
            return;
}
