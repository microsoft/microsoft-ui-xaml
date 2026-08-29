// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <windows.h>
#include <functional>
#include <fstream>
#include <array>
#include <activation.h>
#include <assert.h>
#include <mindebug.h>

// This provides a mechanism to detour DComp to load mockdcomp.dll while loading Microsoft.UI.Xaml.dll.
// It does this via a script that redirects the XAML activation from HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsRuntime\ActivatableClassId\Microsoft.UI.Xaml.Applicaiton
// It stores the original DllPath in a registry key HKEY_LOCAL_MACHINE\[XAML_ROOT_KEY]\MockDCompInjector which is queried and passes through the
// DllGetActivationFactory calls to instantiate the proper object from the original DLL's.
// However, the main injection functionality is done when the DLL is loaded via DllMain which essentially loads Mock10, dcomp.dll and calls to detours the DComp device
// creation so that when the real XAML is loaded afterwards, it will use the mock device instead of the real device.

// Validation macros
#define VERIFY_HR(expr) { HRESULT hr = (expr); if (FAILED(hr)) { return false; } }
#define VERIFY_BOOL(expr) { if (!(expr)) { return false; } }
#define VERIFY_NOTNULL(expr) { if ((expr) == nullptr) { return false; } }
#define VERIFY_LONGERROR(expr) { if ((expr) != ERROR_SUCCESS) { return false; } }

// Function typedefs
typedef HRESULT (WINAPI* PfnDllGetActivationFactory)(HSTRING, IActivationFactory**);
typedef HRESULT (*PfnStartDetourMockDCompDevice)();
typedef HRESULT (*PfnStopDetourMockDCompDevice)();

// Global State Variables
bool g_shouldInject = true;
bool g_isInjected = false;
HMODULE g_originalXamlHandle = nullptr;
PfnDllGetActivationFactory g_originalDllGetActivationFactory = nullptr;

HMODULE g_mockDCompHandle = nullptr;
PfnStartDetourMockDCompDevice g_startDetourMockDCompDevice = nullptr;
PfnStopDetourMockDCompDevice g_stopDetourMockDCompDevice = nullptr;

HMODULE g_dcompHandle = nullptr;
HMODULE g_mock10Handle = nullptr;

// Helper functions
bool ShouldInject()
{
    // Check if we should load private DLLs, both exe's and dll's can cause exclusion
    std::wifstream excludedModuleList(L"C:\\Windows\\System32\\MockDCompInjector\\excluded.txt");

    if (excludedModuleList.is_open())
    {
        std::array<wchar_t, MAX_PATH> excludedModuleFileName;
        while (excludedModuleList.getline(excludedModuleFileName.data(), excludedModuleFileName.size()))
        {
            if (GetModuleHandle(excludedModuleFileName.data()) != nullptr)
            {
                return false;
            }
        }

        excludedModuleList.close();
    }

    return true;
}

bool HookOriginalXaml()
{
    wchar_t origXamlDllPath[MAX_PATH];
    DWORD origXamlDllPathSize = MAX_PATH;
    DWORD origXamlDllPathType = {};

    // Return may be ERROR_MORE_DATA but it is a path and shouldn't exceed MAX_PATH length.
    if (ERROR_SUCCESS == RegGetValueW(
        HKEY_LOCAL_MACHINE,
        XAML_ROOT_KEY L"\\MockDCompInjector",
        L"DllPath",
        RRF_RT_REG_SZ,
        &origXamlDllPathType,
        origXamlDllPath,
        &origXamlDllPathSize))
    {
        // Load the original XAML and ensure we have activation factory redirection setup.
        VERIFY_NOTNULL(g_originalXamlHandle = LoadLibraryExW(origXamlDllPath, nullptr, 0));
    }
    else
    {
        // Error, so load from the system32 location
        VERIFY_NOTNULL(g_originalXamlHandle = LoadLibraryExW(L"C:\\Windows\\System32\\Microsoft.UI.Xaml.dll", nullptr, 0));
    }

    VERIFY_NOTNULL(g_originalDllGetActivationFactory = reinterpret_cast<PfnDllGetActivationFactory>(GetProcAddress(g_originalXamlHandle, "DllGetActivationFactory")));

    return true;
}

bool InjectMockDComp()
{
    // Load the dll's required for MockDComp injection
    VERIFY_NOTNULL(g_dcompHandle = LoadLibraryExW(L"C:\\Windows\\System32\\dcomp.dll", nullptr, 0));
    VERIFY_NOTNULL(g_mock10Handle = LoadLibraryExW(L"C:\\Windows\\System32\\MockDCompInjector\\mock10.dll", nullptr, 0));

    VERIFY_NOTNULL(g_mockDCompHandle = LoadLibraryExW(L"C:\\Windows\\System32\\MockDCompInjector\\MockDComp.dll", nullptr, 0));
    VERIFY_NOTNULL(g_startDetourMockDCompDevice = reinterpret_cast<PfnStartDetourMockDCompDevice>(GetProcAddress(g_mockDCompHandle, "StartDetourMockDCompDevice")));
    VERIFY_NOTNULL(g_stopDetourMockDCompDevice = reinterpret_cast<PfnStopDetourMockDCompDevice>(GetProcAddress(g_mockDCompHandle, "StopDetourMockDCompDevice")));

    VERIFY_HR(g_startDetourMockDCompDevice());

    return true;
}

bool Initialize()
{
    VERIFY_BOOL(HookOriginalXaml());

    if (ShouldInject())
    {
        g_isInjected = InjectMockDComp();
        VERIFY_BOOL(g_isInjected);
    }

    return true;
}

bool Deinitialize()
{
    bool result = true;

    if (g_isInjected &&
        (g_stopDetourMockDCompDevice != nullptr))
    {
        VERIFY_HR(g_stopDetourMockDCompDevice());
    }

    if (g_originalXamlHandle != nullptr)
    {
        FreeLibrary(g_originalXamlHandle);
        g_originalXamlHandle = nullptr;
    }

    if (g_mock10Handle != nullptr)
    {
        FreeLibrary(g_mock10Handle);
        g_mock10Handle = nullptr;
    }
    

    if (g_dcompHandle != nullptr)
    {
        FreeLibrary(g_dcompHandle);
        g_dcompHandle = nullptr;
    }

    return result;
}

// Dll Exported Functions
BOOL APIENTRY DllMain(HMODULE, DWORD ul_reason_for_call, LPVOID)
{
    BOOL result = true;
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        result = Initialize();
        break;
    case DLL_PROCESS_DETACH:
        result = Deinitialize();
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    assert(result);
    return TRUE;
}

// Passthrough function to allow activation from the original DLL to proceed normally (This and DllMain provide the contract
// expected from the original DLL.
extern "C" HRESULT WINAPI DllGetActivationFactory(_In_ HSTRING hstrAcid, _Outptr_ IActivationFactory** factory)
{
    return g_originalDllGetActivationFactory(hstrAcid, factory);
}
