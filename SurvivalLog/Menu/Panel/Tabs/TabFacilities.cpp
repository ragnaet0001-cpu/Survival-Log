#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 设施（mod facilities：槽位耐久，一行一个类型 + 每行锁） ----------
void TabFacilities()
{
            ImGui::Text((const char *)u8"设施");
            ImGui::Separator();
            if (!SLSDK_Ready())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), (const char *)u8"SDK 未就绪（等待游戏加载）...");
                return;
            }

            // 槽位类型（Config_SlotType 运行时实测：1小 2中 3大 4挂壁 5中央 6桌上 7床 8门 9窗 10塔防装置）
            static const char* SLOT_NAMES[10] = {
                (const char *)u8"小", (const char *)u8"中", (const char *)u8"大", (const char *)u8"挂壁", (const char *)u8"中央",
                (const char *)u8"桌上", (const char *)u8"床", (const char *)u8"门", (const char *)u8"窗", (const char *)u8"塔防装置",
            };

            // 锁耐久值（所有类型共用；勾选的类型每帧由 PanelUpdateLocks 保持）
            ImGui::Text((const char *)u8"锁耐久值:");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"##durval", &g_lock_dur_value);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"应用到所有勾选"))
            {
                for (int i = 0; i < 10; i++)
                {
                    if (g_lock_dur_slots[i])
                        SLSDK_ApplyDurabilityLockSlot(i + 1, g_lock_dur_value);
                }
            }
            ImGui::Separator();

            // 每行一个类型：名称(个数 最低~上限) + 锁勾选 + 应用按钮
            for (int i = 0; i < 10; i++)
            {
                int32_t cnt = 0, mn = -1, mx = -1;
                SLSDK_GetHomeDurabilitySummary(i + 1, &cnt, &mn, &mx);
                char line[160];
                snprintf(line, sizeof(line), (const char *)u8"#%d %s (%d个 %d~%d)", i + 1, SLOT_NAMES[i], cnt, mn, mx);
                ImGui::Text("%s", line);
                ImGui::SameLine();
                char lock_label[24], apply_btn[32];
                snprintf(lock_label, sizeof(lock_label), (const char *)u8"锁##f%d", i);
                snprintf(apply_btn, sizeof(apply_btn), (const char *)u8"应用##fa%d", i);
                ImGui::Checkbox(lock_label, &g_lock_dur_slots[i]);
                ImGui::SameLine();
                if (ImGui::SmallButton(apply_btn))
                {
                    int32_t updated = 0;
                    SLSDK_SetHomeDurability(i + 1, g_lock_dur_value, &updated);
                }
            }
            return;
}
