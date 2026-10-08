// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "paltypes.h"
#include "DesktopUtility.h"

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

bool IsOnWindows10() noexcept
{
    static const bool isOnWindows10Result = []()
    {
        ULONGLONG version = 0;
        RtlGetDeviceFamilyInfoEnum(&version, nullptr, nullptr);

        // The version packs major.minor.build.revision into four 16-bit fields.
        // Windows 10 and 11 both use major version 10; Windows 11 starts at build 22000.
        constexpr ULONGLONG windows10Version = 10ULL << 48;
        constexpr ULONGLONG windows11Version = windows10Version | (22000ULL << 16);
        return version >= windows10Version && version < windows11Version;
    }();
    return isOnWindows10Result;
}

void DeleteIsOnDesktopCache()
{
    shouldReturnCachedIsOnDesktopValue = false;
}

} // namespace
