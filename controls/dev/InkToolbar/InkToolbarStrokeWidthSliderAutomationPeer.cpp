// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "InkToolbarStrokeWidthSliderAutomationPeer.h"

#include <algorithm>
#include <cmath>
#include <string>

#include "ResourceAccessor.h"
#include "Utils.h"

#include "InkToolbarStrokeWidthSliderAutomationPeer.properties.cpp"

InkToolbarStrokeWidthSliderAutomationPeer::InkToolbarStrokeWidthSliderAutomationPeer(winrt::InkToolbarStrokeWidthSlider const& owner)
    : ReferenceTracker(owner)
{
    // Resolve here (peer creation, UI thread) and cache; the same lookup from the GetHelpTextCore UIA
    // callback can escape as a fail-fast.
    try { m_rangeFormat = ResourceAccessor::GetLocalizedStringResource(SR_InkToolbarStrokeWidthSliderRangeFormat); }
    catch (...) { m_rangeFormat = L"Minimum %1!s!, maximum %2!s!"; }
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

winrt::hstring InkToolbarStrokeWidthSliderAutomationPeer::GetNameCore()
{
    // Narrator reads the name on every focus but does not reliably speak HelpText or the RangeValue
    // bounds for a Value-pattern slider, so fold the absolute minimum and maximum stroke width into
    // the name to guarantee they are announced.
    auto baseName = __super::GetNameCore();
    if (auto owner = Owner().try_as<winrt::Microsoft::UI::Xaml::Controls::Primitives::RangeBase>())
    {
        auto range = StringUtil::FormatString(
            m_rangeFormat, ValueToString(owner.Minimum()).c_str(), ValueToString(owner.Maximum()).c_str());
        if (!range.empty())
        {
            if (baseName.empty())
            {
                return range;
            }
            return winrt::hstring{ std::wstring{ baseName.c_str() } + L", " + std::wstring{ range.c_str() } };
        }
    }

    return baseName;
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
