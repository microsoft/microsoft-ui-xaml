// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorPickerSlider.h"

#include "ColorPickerSliderAutomationPeer.g.h"

class ColorPickerSliderAutomationPeer :
    public ReferenceTracker<ColorPickerSliderAutomationPeer, winrt::implementation::ColorPickerSliderAutomationPeerT, winrt::IValueProvider>
{
public:
    ColorPickerSliderAutomationPeer(winrt::ColorPickerSlider const& owner);

    // IAutomationPeerOverrides 
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);

    // IValueProvider properties and methods
    bool IsReadOnly();
    winrt::hstring Value();
    void SetValue(winrt::hstring const& value);

    void RaisePropertyChangedEvent(winrt::Color oldColor, winrt::Color newColor, int oldValue, int newValue);

private:
    winrt::hstring GetValueString(winrt::Color color, int value);
};

