// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSliderAutomationPeer.h"

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

void InkToolbarStrokeWidthSliderAutomationPeer::SetValue(winrt::hstring const& /*value*/)
{
    MUX_ASSERT(false); // Not implemented; the size is adjusted through the RangeValue pattern.
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
    return winrt::to_hstring(static_cast<int>(round(value)));
}
