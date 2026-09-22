// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSlider.h"
#include "InkToolbarStrokeWidthSliderAutomationPeer.g.h"

class InkToolbarStrokeWidthSliderAutomationPeer :
    public ReferenceTracker<InkToolbarStrokeWidthSliderAutomationPeer, winrt::implementation::InkToolbarStrokeWidthSliderAutomationPeerT, winrt::IValueProvider>
{
public:
    InkToolbarStrokeWidthSliderAutomationPeer(winrt::InkToolbarStrokeWidthSlider const& owner);

    // IAutomationPeerOverrides
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // IValueProvider
    bool IsReadOnly();
    winrt::hstring Value();
    void SetValue(winrt::hstring const& value);

    void RaiseValueChanged(double oldValue, double newValue);

private:
    static winrt::hstring ValueToString(double value);
};
