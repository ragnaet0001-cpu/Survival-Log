// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "includes.h"
#include "DXhook/d3d11hook.h"
#include "DXhook/dev/Console.h"

DWORD WINAPI MainThread(HMODULE hModule, LPVOID)
{
    LOG_INFO("SurvivalLog DLL injected");

    if (!InitD3D11Hook())
    {
        LOG_ERROR("Failed to initialize D3D11 hook");
        CleanupConsole();
        FreeLibraryAndExitThread(hModule, 1);
        return 1;
    }

    LOG_INFO("D3D11 hook initialized. Press INS / HOME to open menu");
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        CreateConsole();
        CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)MainThread, hModule, 0, nullptr));
        break;
    case DLL_PROCESS_DETACH:
        ReleaseD3D11Hook();
        CleanupConsole();
        break;
    }
    return TRUE;
}
