// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "enumdefs.g.h"

namespace Focus { namespace XYFocusPrivate {

    double CalculatePrimaryAxisDistance(
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& candidateBounds);

    double CalculateSecondaryAxisDistance(
        _In_ const DirectUI::FocusNavigationDirection direction,
        _In_ const XRECTF_RB& bounds,
        _In_ const XRECTF_RB& candidateBounds);

    double CalculatePercentInShadow(
        _In_ const std::pair<double, double>& referenceManifold,
        _In_ const std::pair<double, double>& potentialManifold);

    static bool IsLeft(_In_ const DirectUI::FocusNavigationDirection direction) { return direction == DirectUI::FocusNavigationDirection::Left; }
    static bool IsRight(_In_ const DirectUI::FocusNavigationDirection direction) { return direction == DirectUI::FocusNavigationDirection::Right; }
    static bool IsUp(_In_ const DirectUI::FocusNavigationDirection direction) { return direction == DirectUI::FocusNavigationDirection::Up; }
    static bool IsDown(_In_ const DirectUI::FocusNavigationDirection direction) { return direction == DirectUI::FocusNavigationDirection::Down; }
}}