#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- 关于（mod about） ----------
void TabAbout()
{
            ImGui::Text((const char *)u8"关于");
            ImGui::Separator();
            if (ImGui::Button((const char *)u8"关闭菜单"))
                show_window = false;
            return;
}
