// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ColorSpectrumAutomationPeer.h"

#include "ResourceAccessor.h"
#include "Utils.h"

#include "ColorSpectrumAutomationPeer.properties.cpp"

ColorSpectrumAutomationPeer::ColorSpectrumAutomationPeer(winrt::ColorSpectrum const& owner) :
    ReferenceTracker(owner)
{
}

winrt::IInspectable ColorSpectrumAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (patternInterface == winrt::PatternInterface::Value)
    {
        return *this;
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::AutomationControlType ColorSpectrumAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::Slider;
}

winrt::hstring ColorSpectrumAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_LocalizedControlTypeColorSpectrum);
}

winrt::hstring ColorSpectrumAutomationPeer::GetNameCore()
{
    winrt::hstring nameString = __super::GetNameCore();

    // If a name hasn't been provided by AutomationProperties.Name in markup,
    // then we'll return the default value.
    if (nameString.empty())
    {
        nameString = ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameColorSpectrum);
    }

    return nameString;
}

winrt::hstring ColorSpectrumAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ColorSpectrum>();
}

winrt::hstring ColorSpectrumAutomationPeer::GetHelpTextCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_HelpTextColorSpectrum);
}

winrt::Rect ColorSpectrumAutomationPeer::GetBoundingRectangleCore()
{
    winrt::Rect boundingRectangle = winrt::get_self<ColorSpectrum>(Owner().as<winrt::ColorSpectrum>())->GetBoundingRectangle();
    return boundingRectangle;
}

winrt::Point ColorSpectrumAutomationPeer::GetClickablePointCore()
{
    const auto boundingRect = GetBoundingRectangleCore(); // Call potentially overridden method

    return winrt::Point{ boundingRect.X + boundingRect.Width / 2, boundingRect.Y + boundingRect.Height / 2 };
}

bool ColorSpectrumAutomationPeer::IsReadOnly()
{
    return false;
}

winrt::hstring ColorSpectrumAutomationPeer::Value()
{
    const winrt::ColorSpectrum colorSpectrumOwner = Owner().as<winrt::ColorSpectrum>();
    const winrt::Color color = colorSpectrumOwner.Color();
    const winrt::float4 hsvColor = colorSpectrumOwner.HsvColor();

    return GetValueString(color, hsvColor);
}

void ColorSpectrumAutomationPeer::SetValue(winrt::hstring const& value)
{
    winrt::ColorSpectrum colorSpectrumOwner = Owner().as<winrt::ColorSpectrum>();
    const winrt::Color color = unbox_value<winrt::Color>(winrt::XamlBindingHelper::ConvertValue({ winrt::hstring_name_of<winrt::Color>(), winrt::TypeKind::Metadata }, box_value(value)));

    colorSpectrumOwner.Color(color);

    // Since ColorPicker sets ColorSpectrum.Color and ColorPicker also responds to ColorSpectrum.ColorChanged,
    // we could get into an infinite loop if we always raised ColorSpectrum.ColorChanged when ColorSpectrum.Color changed.
    // Because of that, we'll raise the event manually.
    winrt::get_self<ColorSpectrum>(colorSpectrumOwner)->RaiseColorChanged();
}

void ColorSpectrumAutomationPeer::RaisePropertyChangedEvent(winrt::Color oldColor, winrt::Color newColor, winrt::float4 oldHsvColor, winrt::float4 newHsvColor)
{
    winrt::hstring oldValueString = GetValueString(oldColor, oldHsvColor);
    winrt::hstring newValueString = GetValueString(newColor, newHsvColor);

    __super::RaisePropertyChangedEvent(winrt::ValuePatternIdentifiers::ValueProperty(), box_value(oldValueString), box_value(newValueString));
}

winrt::hstring ColorSpectrumAutomationPeer::GetValueString(winrt::Color color, winrt::float4 hsvColor)
{
    const auto hue = static_cast<unsigned int>(round(hsv::GetHue(hsvColor)));
    const auto saturation = static_cast<unsigned int>(round(hsv::GetSaturation(hsvColor) * 100));
    const auto value = static_cast<unsigned int>(round(hsv::GetValue(hsvColor) * 100));

    if (DownlevelHelper::ToDisplayNameExists())
    {
        return StringUtil::FormatString(
            ResourceAccessor::GetLocalizedStringResource(SR_ValueStringColorSpectrumWithColorName),
            winrt::ColorHelper::ToDisplayName(color).data(),
            hue, saturation, value);
    }
    else
    {
        return StringUtil::FormatString(
            ResourceAccessor::GetLocalizedStringResource(SR_ValueStringColorSpectrumWithoutColorName),
            hue, saturation, value);
    }
}
