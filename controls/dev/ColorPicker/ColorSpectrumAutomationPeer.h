// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorSpectrum.h"

#include "ColorSpectrumAutomationPeer.g.h"

class ColorSpectrumAutomationPeer :
    public ReferenceTracker<ColorSpectrumAutomationPeer, winrt::implementation::ColorSpectrumAutomationPeerT, winrt::IValueProvider>
{
public:
    ColorSpectrumAutomationPeer(winrt::ColorSpectrum const& owner);

    // IAutomationPeerOverrides 
    winrt::IInspectable GetPatternCore(winrt::PatternInterface const& patternInterface);
    winrt::AutomationControlType GetAutomationControlTypeCore();
    winrt::hstring GetLocalizedControlTypeCore();
    winrt::hstring GetNameCore();
    winrt::hstring GetClassNameCore();
    winrt::hstring GetHelpTextCore();
    winrt::Rect GetBoundingRectangleCore();
    winrt::Point GetClickablePointCore();

    // IValueProvider properties and methods
    bool IsReadOnly();
    winrt::hstring Value();
    void SetValue(winrt::hstring const& value);

    void RaisePropertyChangedEvent(winrt::Color oldColor, winrt::Color newColor, winrt::float4 oldHsvColor, winrt::float4 newHsvColor);

private:
    static winrt::hstring GetValueString(winrt::Color color, winrt::float4 hsvColor);
};
