// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <vector>
#include <utility>
#include "enumdefs.g.h"
#include "minxcptypes.h"
#include "KeyTip.h"

namespace KeytipAutomaticPlacementAlgorithm
{
    // If enableMonitorDetection is true, a non-null CXamlIslandRoot must be given.  Otherwise, we fall back to just
    // using "screenBounds" and we don't check monitor bounds.
    void PositionKeyTips(
        _Inout_ std::vector<KeyTip>& objectBounds,
        _In_ const XRECTF_RB& screenBounds,
        _In_ std::vector<XRECTF_RB>& focusableElementBounds,
        _In_opt_ CXamlIslandRoot* xamlIslandRoot,
        bool enableMonitorDetection
    );
};
