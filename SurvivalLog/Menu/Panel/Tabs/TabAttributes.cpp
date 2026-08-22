#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 属性（mod attributes） ----------
void TabAttributes()
{
            ImGui::Text((const char *)u8"属性");
            ImGui::Separator();
            // 属性（mod 完整版：当前值 + 上限 + 锁 + 移速；值为实际值如 20 = 20.0）
            static const int attr_keys[5] = { 1, 2, 3, 5, 4 }; // 饱腹/心态/精力/生命/Health
            static const int attr_max_keys[5] = { 101, 102, 103, 105, 104 };
            static const char *attr_names[5] = {
                (const char *)u8"饱腹 Satiety",
                (const char *)u8"心态 Morale",
                (const char *)u8"精力 Stamina",
                (const char *)u8"生命 Vitality",
                (const char *)u8"Health",
            };
            static float attr_vals[5] = { 0, 0, 0, 0, 0 };
            for (int i = 0; i < 5; ++i)
            {
                int cur = SLSDK_GetAttr(attr_keys[i]);
                int mx = SLSDK_GetAttrMax(attr_max_keys[i]);
                if (cur < 0)
                    continue;
                float disp = (float)cur; // mod 语义：GetTotalValue_Float 实际值
                bool changed = ImGui::SliderFloat(attr_names[i], &attr_vals[i], 0.0f, 100.0f, "%.1f");
                if (!ImGui::IsItemActive())
                    attr_vals[i] = disp; // 当前 slider 未激活时跟随游戏值
                if (changed)
                    SLSDK_SetAttr(attr_keys[i], (int)(attr_vals[i] + 0.5f));
                ImGui::SameLine();
                ImGui::Text((const char *)u8"上限:%d", mx >= 0 ? mx : -1);
            }
            // 设置上限
            static int max_sel = 0;
            static int max_edit = 100;
            ImGui::SetNextItemWidth(120);
            ImGui::Combo((const char *)u8"##maxsel", &max_sel, attr_names, 5);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(100);
            ImGui::InputInt((const char *)u8"上限值", &max_edit);
            ImGui::SameLine();
            if (ImGui::Button((const char *)u8"应用上限"))
                SLSDK_SetAttrMax(attr_max_keys[max_sel], max_edit);
            ImGui::Separator();

            // 属性锁（勾选只改全局状态；每帧由 PanelUpdateLocks 统一补满，换页也生效）
            ImGui::Checkbox((const char *)u8"锁定生命", &g_lock_hp);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"锁定精力", &g_lock_sta);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"锁定饱腹", &g_lock_sat);
            ImGui::SameLine();
            ImGui::Checkbox((const char *)u8"锁定心态", &g_lock_mor);
            ImGui::Separator();

            // 移速倍率（mod SetMoveSpeedMultiplier：0.5-5.0）
            static float speed_mult = 1.0f;
            float spd_cur = -1.0f, spd_orig = -1.0f, spd_mult = 1.0f;
            if (SLSDK_GetMoveSpeed(&spd_cur, &spd_orig, &spd_mult))
            {
                ImGui::Text((const char *)u8"当前移速: %.1f  原始: %.1f", spd_cur, spd_orig);
                if (ImGui::SliderFloat((const char *)u8"移速倍率", &speed_mult, 0.5f, 5.0f, "%.2f"))
                    SLSDK_SetMoveSpeedMultiplier(speed_mult);
                if (!ImGui::IsItemActive() && spd_mult != speed_mult)
                    speed_mult = spd_mult;
                ImGui::SameLine();
                if (ImGui::Button((const char *)u8"恢复移速"))
                {
                    SLSDK_ResetMoveSpeed();
                    speed_mult = 1.0f;
                }
            }
            else
            {
                speed_mult = 1.0f;
                ImGui::Text((const char *)u8"移速不可用（请先进入游戏存档）");
            }
            return;
}
