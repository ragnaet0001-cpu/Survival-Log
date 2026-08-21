#pragma once
#include <Windows.h>
#include <vector>
#include <iostream>
#include <thread>
#include <chrono>
// own
#include "DXhook/dev/logger.h"

// HookManager (Detours)
#include "Hook/HookManager.h"

// ImGui
#include "DXhook/imgui/imgui.h"
#include "DXhook/imgui/imgui_impl_win32.h"
#include "DXhook/imgui/imgui_impl_dx11.h"

// DirectX
#include <d3d11.h>
#include <dxgi1_4.h>
