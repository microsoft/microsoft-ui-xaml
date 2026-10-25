// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
#include "WinUIProfilerHooks.h"
#include <cstdio>

DWORD WINAPI HoldRegistry(void*)
{
    wchar_t name[100]{};
    swprintf_s(name, L"Local\\WinUIProfiler-Termination-%lu", GetCurrentProcessId());
    HANDLE ready = OpenEventW(EVENT_MODIFY_STATE, FALSE, name);
    if (!ready) return 1;
    int owner = 0;
    WinUIProfiler::Hooks::Born(WinUIProfiler::Domain::XamlCore, &owner);
    WinUIProfiler::Hooks::HoldRegistryForTerminationTest(ready);
    return 0;
}

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD reason, void* reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        WinUIProfiler::Hooks::InitializeRuntime();
        HANDLE thread = CreateThread(nullptr, 0, HoldRegistry, nullptr, 0, nullptr);
        if (!thread) return FALSE;
        CloseHandle(thread);
    }
    if (reason == DLL_PROCESS_DETACH)
    {
        wchar_t name[120]{};
        GetEnvironmentVariableW(L"WINUI_PROFILER_TERMINATION_STATE", name, ARRAYSIZE(name));
        HANDLE mapping = OpenFileMappingW(FILE_MAP_WRITE, FALSE, name);
        auto state = static_cast<LONG*>(MapViewOfFile(mapping, FILE_MAP_WRITE, 0, 0, sizeof(LONG)));
        if (state) *state = 1;
        WinUIProfiler::Hooks::ShutdownRuntime(reserved != nullptr);
        if (!WinUIProfiler::IsEnabled(WinUIProfiler::Event::ObjectLifetime))
        {
            if (state) *state = 3;
            return TRUE;
        }
        int releasedPeer = 0;
        WinUIProfiler::Hooks::Destroyed(WinUIProfiler::Domain::XamlPeer, &releasedPeer);
        WinUIProfiler::Hooks::Born(WinUIProfiler::Domain::XamlCore, &releasedPeer);
        WinUIProfiler::Hooks::InitializeRuntime();
        WinUIProfiler::Hooks::ShutdownRuntime();
        if (state) *state = 2;
    }
    return TRUE;
}
