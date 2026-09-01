#include "panel.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstdint>
#include "../../SDK/SurvivalLogSDK.h"

// ---------- Global lock state (tab checkboxes only change state; PanelUpdateLocks called each frame in RenderPanel for unified effect) ----------
bool g_lock_hp = false, g_lock_sta = false, g_lock_sat = false, g_lock_mor = false;
bool g_lock_dur_slots[10] = {}; // Slot type locks (index 0=id1 small ... 9=id10 tower defense device)
int32_t g_lock_dur_value = 10000;

// Unified lock application each frame (aligned with mod CheatGUI.Update: locks don't depend on currently open tab)
void PanelUpdateLocks()
{
    // Attribute locks (mod ApplyAttrLocks: replenish each frame; internally returns directly when all disabled)
    if (g_lock_hp || g_lock_sta || g_lock_sat || g_lock_mor)
        SLSDK_ApplyAttrLocks(g_lock_hp, g_lock_sta, g_lock_sat, g_lock_mor);
    // Facility durability locks (each slot type maintained each frame; TabFacilities checkbox only changes state, aligned with attribute locks)
    for (int i = 0; i < 10; i++)
    {
        if (g_lock_dur_slots[i])
            SLSDK_ApplyDurabilityLockSlot(i + 1, g_lock_dur_value);
    }
    // Anti-exploration exposure reset each frame (switch maintained by TabMisc -> SLSDK_SetNoExploreExposure, internally reads SDK global switch)
    SLSDK_ApplyNoExploreExposure();
    // Time freeze maintained each frame (switch maintained by TabPrepare -> SLSDK_SetTimeFrozen, internally reads SDK global FrozenOverride)
    SLSDK_ApplyFrozenOverride();
    // dexter.sl multiplier types maintained each frame (switch set by each tab, internally checks on/off; aligned with mod CheatGUI.Update)
    // Note: action speed doesn't use per-frame application (during gameplay, action treats During as countdown decrement; per-frame override would stutter actions;
    //       automatically applied to new actions via Hook_GetConfigAction)
    SLSDK_ApplyExposureRate();
}

// Style setup from Genshin Impact project (SetStyle)
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
    styles.ColorButtonPosition = ImGuiDir_Right; // Original value 1, new version is ImGuiDir enum
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
    styles.WindowMenuButtonPosition = ImGuiDir_Left; // Original value 0, new version is ImGuiDir enum
    styles.WindowMinSize = ImVec2(32.0, 32.0);
    styles.WindowPadding = ImVec2(8.0, 8.0);
    styles.WindowRounding = 7.0f;
    styles.WindowTitleAlign = ImVec2(0.5, 0.5);
}

// System logic executed unconditionally each frame (SDK lazy initialization + hook installation + lock application).
// Independent of menu visibility: D3D11Hook Present_Hook calls this each frame (RenderPanel doesn't execute when menu is hidden, but this must run)
void PanelFrameUpdate()
{
    // Attach render thread to il2cpp domain (idempotent): unattached threads can occasionally crash when calling managed APIs during game GC
    SLSDK_EnsureThreadAttached();
    // SDK lazy initialization: HotUpdate.dll not loaded during early game startup, retry every 2 seconds
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
        // Install Detours hooks after SDK is ready (infinite food/time freeze/backpack size, etc.; idempotent)
        SLSDK_InstallHooks();
        // Batch 6 hooks (cooking time/exposure events, idempotent; dexter.sl features)
        SLSDK_InstallBatch6Hooks();
        // Batch 8 hooks (power generation/power storage multiplier, idempotent; mod PowerManagerPatch)
        SLSDK_InstallPowerHooks();
    }

    // Lock features applied uniformly each frame (independent of currently open tab, aligned with mod CheatGUI.Update)
    PanelUpdateLocks();
}

void RenderPanel()
{
    // Genshin Impact style: set style only once
    static bool is_style_set = false;
    if (!is_style_set)
    {
        SetStyle();
        is_style_set = true;
    }

    ImGui::SetNextWindowBgAlpha(1.0f);
    ImGui::SetNextWindowSize(ImVec2(620, 440), ImGuiCond_FirstUseEver);
    ImGui::Begin("SurvivalLog", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

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
        // Same as mod MenuType (9 tabs, no wallhack/aimbot): Resources = Preparation/Items/Backpack/Misc; Survival = Attributes/Proficiency/Facilities/Buffs; Other = About
        "Preparation",
        "Items",
        "Backpack",
        "Miscellaneous",
        "Attributes",
        "Proficiency",
        "Facilities",
        "Buffs",
        "Zombies",
        "About",
    };

    ImGui::BeginChild("SurvivalLog_Button", {150.0f, Windowsize.y - 40.0f}, true);
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
    // Content area dispatcher: aligned with Genshin Impact project CheatManagerBase::DrawTabFeatures style (tab name -> independent module function)
    static void (*const kTabRenderers[])(void) = {
        TabPrepare,     // 0 Preparation (coins + time)
        TabItems,       // 1 Items
        TabBag,         // 2 Backpack
        TabMisc,        // 3 Miscellaneous
        TabAttributes,  // 4 Attributes
        TabProficiency, // 5 Proficiency
        TabFacilities,  // 6 Facilities
        TabBuffs,       // 7 Buffs
        TabZombie,      // 8 Zombies
        TabAbout,       // 9 About
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
