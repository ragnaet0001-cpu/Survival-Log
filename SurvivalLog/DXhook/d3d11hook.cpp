#include "d3d11hook.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#include <d3d11.h>
#include "../Menu/Panel/panel.h"

// IDXGISwapChain vtable: Present = 8, ResizeBuffers = 13
static constexpr int VTABLE_PRESENT_INDEX = 8;
static constexpr int VTABLE_RESIZE_INDEX = 13;

// ======================= D3D11 overlay 资源 =======================
static ID3D11Device *g_pd3dDevice = nullptr;
static ID3D11DeviceContext *g_pContext = nullptr;
static ID3D11RenderTargetView *g_mainRenderTargetView = nullptr;
static HWND g_window = nullptr;
static WNDPROC g_oWndProc = nullptr;
static bool g_initialized = false;

// imgui 1.91.x 的 imgui_impl_win32.h 把 WndProcHandler 声明放在 #if 0 块里，需自行 extern 声明
// （定义在 imgui_impl_win32.cpp，Kuro 原版同款做法）
extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProcHook(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
        return 1;

    return CallWindowProc(g_oWndProc, hwnd, uMsg, wParam, lParam);
}

static bool CreateRenderTarget(IDXGISwapChain *pSwapChain)
{
    if (!pSwapChain || !g_pd3dDevice)
        return false;

    ID3D11Texture2D *pBackBuffer = nullptr;
    if (FAILED(pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer))))
    {
        LOG_ERROR("D3D11: Failed to get back buffer");
        return false;
    }

    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();

    if (!g_mainRenderTargetView)
    {
        LOG_ERROR("D3D11: Failed to create render target view");
        return false;
    }
    return true;
}

static void InitImGui(HWND window)
{
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    // 字体加载带兜底：msyhl -> msyh -> msyhbd -> simhei -> 内置默认字体
    static const char *fontCandidates[] = {
        "C:\\Windows\\Fonts\\msyhl.ttc",
        "C:\\Windows\\Fonts\\msyh.ttc",
        "C:\\Windows\\Fonts\\msyhbd.ttc",
        "C:\\Windows\\Fonts\\simhei.ttf",
    };
    bool fontLoaded = false;
    for (const char *fontPath : fontCandidates)
    {
        if (GetFileAttributesA(fontPath) != INVALID_FILE_ATTRIBUTES)
        {
            if (io.Fonts->AddFontFromFileTTF(fontPath, 16.0f, NULL, io.Fonts->GetGlyphRangesChineseFull()))
            {
                LOG_INFO("D3D11: Font loaded: %s", fontPath);
                fontLoaded = true;
                break;
            }
        }
    }
    if (!fontLoaded)
    {
        LOG_WARN("D3D11: No CJK font found, using ImGui default font");
        io.Fonts->AddFontDefault();
    }

    io.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();

    g_oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProcHook);

    ImGui_ImplWin32_Init(window);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pContext);
}

// ======================= 获取 D3D11 swapchain vtable (dummy 创建，原神同款) =======================
static bool GetD3D11SwapChainVTable(void **vTable, size_t size)
{
    constexpr wchar_t DUMMY_WINDOW_CLASS[] = L"SurvivalLogD3D11Dummy";

    // 注册 dummy 窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DefWindowProcW;
    wc.lpszClassName = DUMMY_WINDOW_CLASS;

    if (!RegisterClassExW(&wc))
    {
        LOG_ERROR("D3D11: Failed to register dummy window class.");
        return false;
    }

    // 创建 dummy 窗口
    HWND hWnd = CreateWindowExW(0, DUMMY_WINDOW_CLASS, L"", WS_DISABLED,
                                0, 0, 1, 1, nullptr, nullptr, nullptr, nullptr);
    if (!hWnd)
    {
        UnregisterClassW(DUMMY_WINDOW_CLASS, GetModuleHandleW(nullptr));
        LOG_ERROR("D3D11: Failed to create dummy window.");
        return false;
    }

    // 创建 D3D11 设备 + swapchain
    DXGI_SWAP_CHAIN_DESC swapDesc = {};
    swapDesc.BufferCount = 1;
    swapDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapDesc.OutputWindow = hWnd;
    swapDesc.SampleDesc.Count = 1;
    swapDesc.Windowed = TRUE;
    swapDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain *pSwapChain = nullptr;
    ID3D11Device *pDevice = nullptr;
    ID3D11DeviceContext *pContext = nullptr;
    D3D_FEATURE_LEVEL featureLevel;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        0,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &swapDesc,
        &pSwapChain,
        &pDevice,
        &featureLevel,
        &pContext
    );

    if (FAILED(hr))
    {
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr,
            D3D_DRIVER_TYPE_WARP,
            nullptr,
            0,
            nullptr,
            0,
            D3D11_SDK_VERSION,
            &swapDesc,
            &pSwapChain,
            &pDevice,
            &featureLevel,
            &pContext
        );
    }

    // 清理窗口
    if (hWnd)
    {
        DestroyWindow(hWnd);
        UnregisterClassW(DUMMY_WINDOW_CLASS, GetModuleHandleW(nullptr));
    }

    if (FAILED(hr) || !pSwapChain)
    {
        LOG_ERROR("D3D11: Failed to create D3D11 device and swap chain. HRESULT: 0x%X", (UINT)hr);
        return false;
    }

    // 复制 vtable
    void **swapChainVTable = *reinterpret_cast<void ***>(pSwapChain);
    memcpy_s(vTable, size, swapChainVTable, size);

    LOG_INFO("D3D11 swapchain vtable: Present=0x%p ResizeBuffers=0x%p",
             vTable[VTABLE_PRESENT_INDEX], vTable[VTABLE_RESIZE_INDEX]);

    // 释放资源
    if (pDevice) pDevice->Release();
    if (pContext) pContext->Release();
    if (pSwapChain) pSwapChain->Release();

    return true;
}

// ======================= Present / ResizeBuffers hook =======================
static HRESULT WINAPI Present_Hook(IDXGISwapChain *pSwapChain, UINT SyncInterval, UINT Flags)
{
    if (!g_initialized)
    {
        HRESULT hr = pSwapChain->GetDevice(__uuidof(ID3D11Device), (void **)&g_pd3dDevice);
        if (FAILED(hr) || !g_pd3dDevice)
        {
            LOG_ERROR("D3D11: GetDevice failed: hr=0x%08X", (UINT)hr);
            return CALL_ORIGIN(Present_Hook, pSwapChain, SyncInterval, Flags);
        }

        g_pd3dDevice->GetImmediateContext(&g_pContext);
        if (!g_pContext)
        {
            LOG_ERROR("D3D11: GetImmediateContext failed");
            return CALL_ORIGIN(Present_Hook, pSwapChain, SyncInterval, Flags);
        }

        DXGI_SWAP_CHAIN_DESC desc;
        pSwapChain->GetDesc(&desc);
        g_window = desc.OutputWindow;

        if (!CreateRenderTarget(pSwapChain))
        {
            LOG_ERROR("D3D11: CreateRenderTarget failed");
            return CALL_ORIGIN(Present_Hook, pSwapChain, SyncInterval, Flags);
        }

        InitImGui(g_window);
        g_initialized = true;
        LOG_INFO("D3D11 overlay initialized: window=0x%p", g_window);
    }

    if (g_initialized)
    {
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        RenderPanel();

        g_pContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    }

    return CALL_ORIGIN(Present_Hook, pSwapChain, SyncInterval, Flags);
}

static HRESULT WINAPI ResizeBuffers_Hook(IDXGISwapChain *pSwapChain, UINT BufferCount, UINT Width, UINT Height, DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    if (g_mainRenderTargetView)
    {
        if (g_pContext)
            g_pContext->OMSetRenderTargets(0, nullptr, nullptr);
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }

    HRESULT hr = CALL_ORIGIN(ResizeBuffers_Hook, pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);
    if (SUCCEEDED(hr) && g_initialized)
    {
        CreateRenderTarget(pSwapChain);
    }
    return hr;
}

// ======================= 初始化 / 释放 =======================
bool InitD3D11Hook()
{
    LOG_INFO("Waiting for process initialization...");

    HANDLE d3d11Module = nullptr;
    HANDLE dxgiModule = nullptr;

    while (true)
    {
        d3d11Module = GetModuleHandleA("d3d11.dll");
        dxgiModule = GetModuleHandleA("dxgi.dll");

        if (d3d11Module && dxgiModule)
            break;

        if (WaitForSingleObject(GetCurrentProcess(), 1000) != WAIT_TIMEOUT)
        {
            LOG_ERROR("Process terminated while waiting for DirectX");
            return false;
        }

        LOG_INFO("Waiting for DirectX modules...");
    }

    LOG_INFO("DirectX modules found, initializing hooks...");

    // 获取 swapchain vtable
    void *d3d11SwapChainVTable[40] = {};
    if (!GetD3D11SwapChainVTable(d3d11SwapChainVTable, sizeof(d3d11SwapChainVTable)))
    {
        LOG_ERROR("D3D11: Failed to get D3D11 swap chain virtual table.");
        return false;
    }

    try
    {
        // 安装 hooks（原神同款：直接 HookFunction）
        HookManager::HookFunction(d3d11SwapChainVTable[VTABLE_PRESENT_INDEX], Present_Hook);
        HookManager::HookFunction(d3d11SwapChainVTable[VTABLE_RESIZE_INDEX], ResizeBuffers_Hook);

        if (!HookManager::getOrigin(Present_Hook) || !HookManager::getOrigin(ResizeBuffers_Hook))
        {
            LOG_ERROR("D3D11: Failed to resolve original functions");
            HookManager::detachAll();
            return false;
        }

        LOG_INFO("D3D11 successfully hooked using HookManager (Detours)");
        return true;
    }
    catch (...)
    {
        LOG_ERROR("Exception during hook initialization");
        HookManager::detachAll();
        return false;
    }
}

void ReleaseD3D11Hook()
{
    // 卸载所有 detours hooks
    HookManager::detachAll();

    if (g_pd3dDevice)
    {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
    }

    if (g_mainRenderTargetView)
    {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }

    if (g_oWndProc && g_window)
    {
        SetWindowLongPtr(g_window, GWLP_WNDPROC, (LONG_PTR)g_oWndProc);
        g_oWndProc = nullptr;
    }

    if (g_pContext)
    {
        g_pContext->Release();
        g_pContext = nullptr;
    }

    if (g_pd3dDevice)
    {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }

    g_window = nullptr;
    g_initialized = false;
}
