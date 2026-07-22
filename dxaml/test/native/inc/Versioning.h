// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// IsOSBuildAtLeast below needs the Win32 types (DWORD, WINAPI, GetProcAddress, ...) and the
// RTL_OSVERSIONINFOW type from winternl.h. Include them here so this header is self-contained
// and doesn't rely on includers happening to pull these in first.
#include <windows.h>
#include <winternl.h>

// To help TAEF deploy the customized AppxManifest for compatibility tests

// this file defines the versions for managed test.
// please make a corresponding change to dxaml/test/managed/common/Versions.cs when you update this file.

#define APPXMANIFEST_WINDOWS_VERSION_CURRENT             L"AppXManifest.native.current.xml"     // Windows 19H1
#define APPXMANIFEST_WINDOWS_VERSION_CURRENT_DM          L"AppXManifest.DesignMode.xml"         // Windows 19H1 with DesignMode support
#define APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL  L"AppXManifest.Centennial.xml"         // Windows Centennial Latest

#define WINDOWS_OS_VERSION_20H2                         L"19042"
#define WINDOWS_OS_VERSION_19H1                         L"18362"
#define WINDOWS_OS_VERSION_RS5                          L"17763"
#define WINDOWS_OS_VERSION_RS4                          L"17134"
#define WINDOWS_OS_VERSION_22H2                         L"22621"
#define WINDOWS_OS_VERSION_24H2                         L"26100"
#define WINDOWS_OS_VERSION_25H2                         L"26200"

// Returns true if the current OS build number is >= the given value.
// Uses RtlGetVersion (via GetProcAddress) to bypass GetVersionEx manifest lies.
inline bool IsOSBuildAtLeast(DWORD buildNumber)
{
    typedef LONG(WINAPI* RtlGetVersionFn)(PRTL_OSVERSIONINFOW);
    static DWORD cachedBuild = 0;
    if (cachedBuild == 0)
    {
        OSVERSIONINFOW osvi = { sizeof(osvi) };
        auto fn = reinterpret_cast<RtlGetVersionFn>(
            GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "RtlGetVersion"));
        if (fn && fn(&osvi) == 0)
        {
            cachedBuild = osvi.dwBuildNumber;
        }
    }
    return cachedBuild >= buildNumber;
}
