// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


namespace DirectUI {
    enum class FocusNavigationDirection : uint8_t;
}

namespace FocusRect {
    class RevealFocusSource;

    namespace RevealMotion {
        DirectUI::FocusNavigationDirection CalculateFrom(const XRECTF& bounds);
        float GetStartX(DirectUI::FocusNavigationDirection direction, const RevealFocusSource& source);
        float GetStartY(DirectUI::FocusNavigationDirection direction, const RevealFocusSource& source);
        float GetDistanceX(const RevealFocusSource& source);
        float GetDistanceY(const RevealFocusSource& source);
    }
}