// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSliderAutomationPeer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "InkToolbarStrokeWidthSliderAutomationPeer.properties.cpp"

InkToolbarStrokeWidthSliderAutomationPeer::InkToolbarStrokeWidthSliderAutomationPeer(winrt::InkToolbarStrokeWidthSlider const& owner)
    : ReferenceTracker(owner)
{
}

winrt::IInspectable InkToolbarStrokeWidthSliderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // Narrator reads a plain Slider's RangeValue as a percentage of (Maximum - Minimum). Exposing a
    // Value pattern makes it announce the absolute stroke width (e.g. "2") instead.
    if (patternInterface == winrt::PatternInterface::Value)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

bool InkToolbarStrokeWidthSliderAutomationPeer::IsReadOnly()
{
    return false;
}

winrt::hstring InkToolbarStrokeWidthSliderAutomationPeer::Value()
{
    if (auto owner = Owner().try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBase>())
    {
        return ValueToString(owner.Value());
    }

    return L"";
}

void InkToolbarStrokeWidthSliderAutomationPeer::SetValue(winrt::hstring const& value)
{
    auto owner = Owner().try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBase>();
    if (!owner)
    {
        return;
    }

    double parsed = 0.0;
    try
    {
        parsed = std::stod(winrt::to_string(value));
    }
    catch (...)
    {
        // UIA SetValue contract: a value that is not a number is rejected.
        throw winrt::hresult_invalid_argument();
    }

    owner.Value(std::clamp(parsed, owner.Minimum(), owner.Maximum()));
}

void InkToolbarStrokeWidthSliderAutomationPeer::RaiseValueChanged(double oldValue, double newValue)
{
    __super::RaisePropertyChangedEvent(
        winrt::ValuePatternIdentifiers::ValueProperty(),
        box_value(ValueToString(oldValue)),
        box_value(ValueToString(newValue)));
}

winrt::hstring InkToolbarStrokeWidthSliderAutomationPeer::ValueToString(double value)
{
    // Report the actual stroke width, not a rounded integer, so fractional widths are announced
    // accurately. Whole widths read as "2" rather than "2.0".
    if (value == std::floor(value))
    {
        return winrt::to_hstring(static_cast<int>(value));
    }

    std::wstring text = std::to_wstring(value);
    text.erase(text.find_last_not_of(L'0') + 1);
    if (!text.empty() && text.back() == L'.')
    {
        text.pop_back();
    }
    return winrt::hstring{ text };
}
