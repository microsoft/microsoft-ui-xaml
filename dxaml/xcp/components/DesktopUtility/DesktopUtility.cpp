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

void DeleteIsOnDesktopCache()
{
    shouldReturnCachedIsOnDesktopValue = false;
}

} // namespace

