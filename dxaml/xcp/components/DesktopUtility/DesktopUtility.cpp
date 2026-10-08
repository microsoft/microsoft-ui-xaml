// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "paltypes.h"
#include "DesktopUtility.h"
#include <winternl.h>

namespace DesktopUtility {

bool shouldReturnCachedIsOnDesktopValue = false;

bool IsOnDesktop()
{
    static bool isOnDesktopResult = false;

    if (!shouldReturnCachedIsOnDesktopValue)
    {
        ULONG platformId = 0;
        RtlGetDeviceFamilyInfoEnum(NULL, &platformId, NULL);
        shouldReturnCachedIsOnDesktopValue = true;

        isOnDesktopResult = platformId == DEVICEFAMILYINFOENUM_DESKTOP ||
                            platformId == DEVICEFAMILYINFOENUM_SERVER;
    }
    return isOnDesktopResult;
}

bool IsPriorToWindows11() noexcept
{
    static const bool isPriorToWindows11Result = []()
    {
        const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        if (!ntdll)
        {
            TRACE_HR_NORETURN(HRESULT_FROM_WIN32(GetLastError()));
            return false;
        }

        using RtlGetVersionFn = NTSTATUS(WINAPI*)(PRTL_OSVERSIONINFOW);
        const auto rtlGetVersion = reinterpret_cast<RtlGetVersionFn>(GetProcAddress(ntdll, "RtlGetVersion"));
        if (!rtlGetVersion)
        {
            TRACE_HR_NORETURN(HRESULT_FROM_WIN32(GetLastError()));
            return false;
        }

        RTL_OSVERSIONINFOW version = { sizeof(version) };
        const NTSTATUS status = rtlGetVersion(&version);
        if (status != 0)
        {
            TRACE_HR_NORETURN(HRESULT_FROM_NT(status));
            return false;
        }

        // Compatibility shims can report an older OS, so this may apply the workaround
        // on Windows 11. If the query fails, assume the latest OS and skip the workaround.
        return version.dwMajorVersion < 10 ||
            (version.dwMajorVersion == 10 && version.dwMinorVersion == 0 && version.dwBuildNumber < 22000);
    }();
    return isPriorToWindows11Result;
}

void DeleteIsOnDesktopCache()
{
    shouldReturnCachedIsOnDesktopValue = false;
}

} // namespace
