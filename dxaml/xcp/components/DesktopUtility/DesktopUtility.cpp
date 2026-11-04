// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "paltypes.h"
#include "DesktopUtility.h"
#include <winternl.h>

// The user-mode SDK header does not declare this documented ntdll export.
extern "C" NTSYSAPI NTSTATUS NTAPI RtlGetVersion(PRTL_OSVERSIONINFOW versionInformation);

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
        RTL_OSVERSIONINFOW version = { sizeof(version) };
        const NTSTATUS status = ::RtlGetVersion(&version);
        if (status != 0)
        {
            TRACE_HR_NORETURN(HRESULT_FROM_NT(status));
            return false;
        }

        // Windows 11 retains major/minor version 10.0 and starts at build 22000.
        // AppCompat shims can make a newer OS appear older to this process, so a
        // true result may enable the workaround on Windows 11. On query failure,
        // assume a newer OS and leave the workaround off.
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
