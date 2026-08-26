#pragma once
#include "../../includes.h"

void RenderPanel();

// ---------- 全局锁定状态（勾选只改状态；每帧由 PanelUpdateLocks 统一生效，对齐 mod CheatGUI.Update） ----------
extern bool g_lock_hp, g_lock_sta, g_lock_sat, g_lock_mor;
extern bool g_lock_dur_slots[10]; // 10 个槽位类型的锁状态（1小 2中 3大 4挂壁 5中央 6桌上 7床 8门 9窗 10塔防装置）
extern int32_t g_lock_dur_value;
void PanelUpdateLocks();
// 每帧无条件执行的系统逻辑（SDK 延迟初始化 + hook 安装 + 锁定生效；与菜单显隐无关，d3d11hook 每帧调用）
void PanelFrameUpdate();

// ---------- 各 tab 内容模块（对齐原神项目 Gui 模块化风格，Tabs\TabXxx.cpp） ----------
void TabPrepare();
void TabItems();
void TabBag();
void TabMisc();
void TabAttributes();
void TabProficiency();
void TabFacilities();
void TabBuffs();
void TabAbout();
void TabZombie();

extern inline bool show_window = true;
