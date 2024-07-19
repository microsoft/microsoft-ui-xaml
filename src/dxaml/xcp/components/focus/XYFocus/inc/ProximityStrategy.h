// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

namespace Focus { namespace XYFocusPrivate {
    class ProximityStrategy
    {
    public:
        static double GetScore(
            _In_ const DirectUI::FocusNavigationDirection direction,
            _In_ const XRECTF_RB& bounds,
            _In_ const XRECTF_RB& candidateBounds,
            _In_ const double maxDistance,
            _In_ bool considerSecondaryAxis);
    };
}}