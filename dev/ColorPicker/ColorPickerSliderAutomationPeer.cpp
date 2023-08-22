// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ColorPickerSliderAutomationPeer.h"

#include "ColorPicker.h"
#include "ResourceAccessor.h"
#include "Utils.h"

#include "ColorPickerSliderAutomationPeer.properties.cpp"

ColorPickerSliderAutomationPeer::ColorPickerSliderAutomationPeer(winrt::ColorPickerSlider const& owner)
    : ReferenceTracker(owner)
{
}

winrt::IInspectable ColorPickerSliderAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    // If this slider is handling the alpha channel, then we don't want to do anything special for it -
    // in that case, we'll just return the base SliderAutomationPeer.
    if (Owner().as<winrt::ColorPickerSlider>().ColorChannel() != winrt::ColorPickerHsvChannel::Alpha && patternInterface == winrt::PatternInterface::Value)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

bool ColorPickerSliderAutomationPeer::IsReadOnly()
{
    return false;
}

winrt::hstring ColorPickerSliderAutomationPeer::Value()
{
    winrt::ColorPickerSlider owner = Owner().as<winrt::ColorPickerSlider>();
    winrt::DependencyObject currentObject = owner;
    winrt::ColorPicker owningColorPicker{ nullptr };

    while (currentObject && !(owningColorPicker = currentObject.try_as<winrt::ColorPicker>()))
    {
        currentObject = winrt::VisualTreeHelper::GetParent(currentObject);
    }

    if (owningColorPicker)
    {
        const winrt::Color color = owningColorPicker.Color();
        const double sliderValue = owner.Value();

        return GetValueString(color, static_cast<int>(round(sliderValue)));
    }
    else
    {
        return L"";
    }
}

void ColorPickerSliderAutomationPeer::SetValue(winrt::hstring const& /*value*/)
{
    MUX_ASSERT(false); // Not implemented.
}

void ColorPickerSliderAutomationPeer::RaisePropertyChangedEvent(winrt::Color oldColor, winrt::Color newColor, int oldValue, int newValue)
{
    winrt::hstring oldValueString = GetValueString(oldColor, oldValue);
    winrt::hstring newValueString = GetValueString(newColor, newValue);

    __super::RaisePropertyChangedEvent(winrt::ValuePatternIdentifiers::ValueProperty(), box_value(oldValueString), box_value(newValueString));
}

winrt::hstring ColorPickerSliderAutomationPeer::GetValueString(winrt::Color color, int value)
{
    if (DownlevelHelper::ToDisplayNameExists())
    {
        winrt::hstring resourceStringWithName;
        switch (Owner().as<winrt::ColorPickerSlider>().ColorChannel())
        {
        case winrt::ColorPickerHsvChannel::Hue:
            resourceStringWithName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringHueSliderWithColorName);
             break;
        case winrt::ColorPickerHsvChannel::Saturation:
            resourceStringWithName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringSaturationSliderWithColorName);
            break;
        case winrt::ColorPickerHsvChannel::Value:
            resourceStringWithName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringValueSliderWithColorName);
            break;
        default:
            return L"";
        }

        return StringUtil::FormatString(
            resourceStringWithName,
            value,
            winrt::ColorHelper::ToDisplayName(color).data());
    }
    else
    {
        winrt::hstring resourceStringWithoutName;
        switch (Owner().as<winrt::ColorPickerSlider>().ColorChannel())
        {
        case winrt::ColorPickerHsvChannel::Hue:
            resourceStringWithoutName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringHueSliderWithoutColorName);
            break;
        case winrt::ColorPickerHsvChannel::Saturation:
            resourceStringWithoutName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringSaturationSliderWithoutColorName);
            break;
        case winrt::ColorPickerHsvChannel::Value:
            resourceStringWithoutName = ResourceAccessor::GetLocalizedStringResource(SR_ValueStringValueSliderWithoutColorName);
            break;
        default:
            return L"";
        }

        return StringUtil::FormatString(
            resourceStringWithoutName,
            value);
    }
}
