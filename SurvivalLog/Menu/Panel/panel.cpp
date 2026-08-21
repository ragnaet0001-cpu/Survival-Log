#include "panel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include "../../SDK/SurvivalLogSDK.h"

// 原神项目同款样式设置（SetStyle）
static void SetStyle()
{
    auto &styles = ImGui::GetStyle();
    styles.AntiAliasedFill = true;
    styles.AntiAliasedLines = true;
    styles.AntiAliasedLinesUseTex = true;
    styles.ButtonTextAlign = ImVec2(0.5, 0.5);
    styles.CellPadding = ImVec2(4.0, 2.0);
    styles.ChildBorderSize = 1.0;
    styles.ChildRounding = 5.0;
    styles.CircleTessellationMaxError = 0.3f;
    styles.ColorButtonPosition = ImGuiDir_Right; // 原值 1，新版是 ImGuiDir 枚举
    styles.ColumnsMinSpacing = 6.0;
    styles.CurveTessellationTol = 1.25;
    styles.DisabledAlpha = 0.6f;
    styles.DisplaySafeAreaPadding = ImVec2(3.0, 3.0);
    styles.DisplayWindowPadding = ImVec2(19.0, 19.0);
    styles.FrameBorderSize = 0.0;
    styles.FramePadding = ImVec2(4.0, 3.0);
    styles.FrameRounding = 4.0;
    styles.GrabMinSize = 10.0;
    styles.GrabRounding = 4.0;
    styles.IndentSpacing = 21.0;
    styles.ItemInnerSpacing = ImVec2(4.0, 4.0);
    styles.ItemSpacing = ImVec2(8.0, 4.0);
    styles.LogSliderDeadzone = 4.0;
    styles.MouseCursorScale = 1.0;
    styles.PopupBorderSize = 1.0;
    styles.PopupRounding = 0.0;
    styles.ScrollbarRounding = 9.0;
    styles.ScrollbarSize = 14.0;
    styles.SelectableTextAlign = ImVec2(0.0, 0.0);
    styles.TabBorderSize = 0.0;
    styles.TabRounding = 4.0;
    styles.TouchExtraPadding = ImVec2(0.0, 0.0);
    styles.WindowBorderSize = 1.0;
    styles.WindowMenuButtonPosition = ImGuiDir_Left; // 原值 0，新版是 ImGuiDir 枚举
    styles.WindowMinSize = ImVec2(32.0, 32.0);
    styles.WindowPadding = ImVec2(8.0, 8.0);
    styles.WindowRounding = 7.0f;
    styles.WindowTitleAlign = ImVec2(0.5, 0.5);
}

void RenderPanel()
{
    // SDK 延迟初始化：游戏启动早期 HotUpdate.dll 未加载，每 2 秒重试
    static ULONGLONG s_last_sdk_try = 0;
    if (!SLSDK_Ready())
    {
        ULONGLONG now = GetTickCount64();
        if (now - s_last_sdk_try >= 2000)
        {
            s_last_sdk_try = now;
            SLSDK_Init();
        }
    }
    else
    {
        // SDK 就绪后安装 Detours hook（无限食物/时间冻结/背包尺寸等，幂等）
        SLSDK_InstallHooks();
    }

    // 原神同款：样式只设置一次
    static bool is_style_set = false;
    if (!is_style_set)
    {
        SetStyle();
        is_style_set = true;
    }

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowSize(ImVec2(620, 440), ImGuiCond_FirstUseEver);
    ImGui::Begin((const char *)u8"SurvivalLog", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    auto Windowpos = ImGui::GetWindowPos();
    auto Windowsize = ImGui::GetWindowSize();
    ImGui::StyleColorsLight();

    ImGui::Columns(2, nullptr, false);
    ImGui::SetColumnWidth(0, 160);
    static int current_tab = 0;
    ImGuiStyle &Style = ImGui::GetStyle();
    auto Color = Style.Colors;
    float MenuButtonHeight = 30.0f;
    static std::vector<std::string> MenuType = {
        // mod MenuType 同款（9 tab，无透视/自瞄）：资源=准备阶段/物品/背包/杂项；生存=属性/熟练度/设施/Buff；其他=关于
        (const char *)u8"准备阶段",
        (const char *)u8"物品",
        (const char *)u8"背包",
        (const char *)u8"杂项",
        (const char *)u8"属性",
        (const char *)u8"熟练度",
        (const char *)u8"设施",
        (const char *)u8"Buff",
        (const char *)u8"关于",
    };

    ImGui::BeginChild((const char *)u8"SurvivalLog_Button", {150.0f, Windowsize.y - 40.0f}, true);
    {
        for (int i = 0; i < (int)MenuType.size(); ++i)
        {
            ImGui::SetCursorPos({5.0f, MenuButtonHeight});

            ImVec4 buttonColor = current_tab == i ? Color[ImGuiCol_Button] : ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);

            if (ImGui::Button(MenuType[i].c_str(), {140.0f, 40.0f}))
            {
                current_tab = i;
            }

            ImGui::PopStyleColor();
            MenuButtonHeight += 50.0f;
        }
    }
    ImGui::EndChild();

    ImGui::NextColumn();
    // 内容区分发：对齐原神项目 CheatManagerBase::DrawTabFeatures 风格（tab 名 -> 独立模块函数）
    static void (*const kTabRenderers[])(void) = {
        TabPrepare,     // 0 准备阶段（金币 + 时间）
        TabItems,       // 1 物品
        TabBag,         // 2 背包
        TabMisc,        // 3 杂项
        TabAttributes,  // 4 属性
        TabProficiency, // 5 熟练度
        TabFacilities,  // 6 设施
        TabBuffs,       // 7 Buff
        TabAbout,       // 8 关于
    };

    ImGui::BeginChild("##SurvivalLog_Content", {Windowsize.x - 180.0f, Windowsize.y - 40.0f}, true);
    {
        int tabCount = (int)(sizeof(kTabRenderers) / sizeof(kTabRenderers[0]));
        if (current_tab >= 0 && current_tab < tabCount)
            kTabRenderers[current_tab]();
    }
    ImGui::EndChild();
    ImGui::End();
}
