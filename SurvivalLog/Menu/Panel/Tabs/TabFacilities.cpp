#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 设施（mod facilities：门窗耐久） ----------
void TabFacilities()
{
            ImGui::Text((const char *)u8"设施");
            ImGui::Separator();
            // 设施耐久（mod 门窗：门=8 窗=9）
            ImGui::Text((const char *)u8"设施耐久");
            int32_t door_cnt = 0, door_min = -1, door_max = -1;
            int32_t win_cnt = 0, win_min = -1, win_max = -1;
            SLSDK_GetHomeDurabilitySummary(8, &door_cnt, &door_min, &door_max);
            SLSDK_GetHomeDurabilitySummary(9, &win_cnt, &win_min, &win_max);
            ImGui::Text((const char *)u8"门: %d 扇 (最低 %d / 上限 %d)", door_cnt, door_min, door_max);
            ImGui::Text((const char *)u8"窗: %d 扇 (最低 %d / 上限 %d)", win_cnt, win_min, win_max);
            static int dur_value = 100;
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"耐久值", &dur_value);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"应用到门"))
            {
                int32_t updated = 0;
                SLSDK_SetHomeDurability(8, dur_value, &updated);
            }
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"应用到窗"))
            {
                int32_t updated = 0;
                SLSDK_SetHomeDurability(9, dur_value, &updated);
            }
            return;
}
