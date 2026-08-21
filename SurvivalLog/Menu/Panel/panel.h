#pragma once
#include "../../includes.h"

void RenderPanel();

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

extern inline bool show_window = true;
