// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <tip/tip.h>
#include <WexTestClass.h>

namespace TableViewTipTestHelpers
{
    inline bool HasNativeAbi() noexcept
    {
#if TABLEVIEW_TIP_REAL_LIBRARY
        auto const module = tip_details_GetKernelBaseModuleHandle();
        return module &&
            GetProcAddress(module, "TestCreate") &&
            GetProcAddress(module, "TestQueryData") &&
            GetProcAddress(module, "TestUnlockData") &&
            GetProcAddress(module, "TestClose") &&
            GetProcAddress(module, "TestReport") &&
            GetProcAddress(module, "TestControlReporting");
#else
        return false;
#endif
    }

    inline bool RequireNativeAbi() noexcept
    {
        if (HasNativeAbi()) { return true; }
        WEX::Logging::Log::Result(WEX::Logging::TestResults::Skipped,
            L"The selected real TiP runtime ABI is unavailable on this host.");
        return false;
    }
}
