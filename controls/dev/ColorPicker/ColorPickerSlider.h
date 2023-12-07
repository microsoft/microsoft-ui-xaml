// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorPickerSlider.g.h"
#include "ColorPickerSlider.properties.h"

class ColorPickerSlider :
    public ReferenceTracker<ColorPickerSlider, winrt::implementation::ColorPickerSliderT>,
    public ColorPickerSliderProperties
{
public:
    ColorPickerSlider();

    // IUIElementOverridesHelper overrides
    winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElementOverrides overrides
    void OnApplyTemplate();

    // IControlOverrides overrides
    void OnKeyDown(winrt::KeyRoutedEventArgs const& e);
    void OnGotFocus(winrt::RoutedEventArgs const& e);
    void OnLostFocus(winrt::RoutedEventArgs const& e);

private:
    void OnValueChangedEvent(winrt::IInspectable const& sender, winrt::RangeBaseValueChangedEventArgs const& args);

    winrt::ColorPicker GetParentColorPicker();
    winrt::hstring GetToolTipString();

    tracker_ref<winrt::ToolTip> m_toolTip{ this };
};