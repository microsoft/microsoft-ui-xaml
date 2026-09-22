// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSlider.g.h"

// A plain Slider reports only a RangeValue pattern, which Narrator reads as a percentage of
// (Maximum - Minimum). The pen-flyout size slider needs the absolute stroke width spoken instead,
// so this subclass hands out a peer that also exposes a Value pattern. It reuses the stock Slider
// template and adds no behavior of its own.
class InkToolbarStrokeWidthSlider :
    public ReferenceTracker<InkToolbarStrokeWidthSlider, winrt::implementation::InkToolbarStrokeWidthSliderT>
{
public:
    InkToolbarStrokeWidthSlider();

    // IUIElement overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

private:
    void OnValueChangedEvent(winrt::IInspectable const& sender, winrt::RangeBaseValueChangedEventArgs const& args);
};
