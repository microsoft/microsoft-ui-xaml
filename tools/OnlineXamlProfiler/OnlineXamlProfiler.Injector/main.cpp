// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <windows.h>
#include <objbase.h>
#include <wil/resource.h>
#include <cerrno>
#include <cstdio>
#include <cwchar>
#include "..\OnlineXamlProfilerTap\TapIdentity.h"

namespace
{
    int Fail(const char* operation, HRESULT status)
    {
        fprintf(stderr, "%s (HRESULT 0x%08lX).\n", operation, static_cast<unsigned long>(status));
        return static_cast<int>(status);
    }
}

int wmain(int argc, wchar_t** argv)
{
    if (argc != 6)
    {
        return Fail("Expected PID, matching FrameworkUdk path, WinUI path, TAP path, and pipe name", E_INVALIDARG);
    }
    wchar_t* end{};
    errno = 0;
    const DWORD processId = wcstoul(argv[1], &end, 10);
    if (!processId || !end || *end || errno == ERANGE)
    {
        return Fail("Invalid target PID", E_INVALIDARG);
    }
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized))
    {
        return Fail("COM initialization failed", initialized);
    }
    auto uninitialize = wil::scope_exit([] { CoUninitialize(); });

    // Keep the target's diagnostics protocol and loader dependencies out of the profiler's WinUI process.
    wil::unique_hmodule module(LoadLibraryExW(argv[2], nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32));
    if (!module)
    {
        return Fail("Loading the target's FrameworkUdk diagnostics initializer failed", HRESULT_FROM_WIN32(GetLastError()));
    }
    using Initialize = HRESULT(WINAPI*)(PCWSTR, DWORD, PCWSTR, PCWSTR, CLSID, PCWSTR);
    const auto initialize = reinterpret_cast<Initialize>(GetProcAddress(module.get(), "InitializeXamlDiagnosticsEx"));
    if (!initialize)
    {
        return Fail("The target's FrameworkUdk does not export InitializeXamlDiagnosticsEx", HRESULT_FROM_WIN32(GetLastError()));
    }
    for (UINT32 index = 1; index <= 256; ++index)
    {
        wchar_t endpoint[64]{};
        swprintf_s(endpoint, L"WinUIVisualDiagConnection%u", index);
        const HRESULT status = initialize(endpoint, processId, argv[3], argv[4], OnlineXamlProfiler::TapClsid, argv[5]);
        if (SUCCEEDED(status))
        {
            return 0;
        }
        if (status != HRESULT_FROM_WIN32(ERROR_NOT_FOUND))
        {
            return Fail("Initializing the target's XAML diagnostics failed", status);
        }
    }
    return Fail("No WinUI diagnostics endpoint was found (connections 1 through 256)", HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
}
