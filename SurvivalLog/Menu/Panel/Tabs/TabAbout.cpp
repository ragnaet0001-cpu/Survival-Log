#include "../../../SDK/SurvivalLogSDK.h"
#include "../panel.h"

// ---------- About (mod about) ----------
void TabAbout()
{
            ImGui::Text((const char *)u8"About");
            ImGui::Separator();
            if (ImGui::Button((const char *)u8"Close Menu"))
                show_window = false;
            return;
}
