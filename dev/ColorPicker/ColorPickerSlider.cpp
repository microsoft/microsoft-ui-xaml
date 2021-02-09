// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ColorPickerSlider.h"
#include "ColorPickerSliderAutomationPeer.h"

#include "ColorPicker.h"
#include "ResourceAccessor.h"
#include "Utils.h"

ColorPickerSlider::ColorPickerSlider()
{
    // We want the ColorPickerSlider to pick up everything for its default style from the Slider's default style,
    // since its purpose is just to turn off keyboarding.  So we'll give it Slider's control name as its default style key
    // instead of ColorPickerSlider.
    winrt::IControlProtected controlProtected = *this;
    auto propertyValue = box_value(L"Windows.UI.Xaml.Controls.Slider");
    controlProtected.DefaultStyleKey(propertyValue);

    ValueChanged({ this, &ColorPickerSlider::OnValueChangedEvent });
}

winrt::AutomationPeer ColorPickerSlider::OnCreateAutomationPeer()
{
    return winrt::make<ColorPickerSliderAutomationPeer>(*this);
}

void ColorPickerSlider::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    winrt::IControlProtected thisAsControlProtected = *this;

    m_toolTip.set(GetTemplateChildT<winrt::ToolTip>(L"ToolTip", thisAsControlProtected));

    if (auto toolTip = m_toolTip.get())
    {
        toolTip.Content(box_value(GetToolTipString()));
    }
}

void ColorPickerSlider::OnKeyDown(winrt::KeyRoutedEventArgs const& args)
{
    if ((Orientation() == winrt::Orientation::Horizontal &&
            args.Key() != winrt::VirtualKey::Left &&
            args.Key() != winrt::VirtualKey::Right) ||
        (Orientation() == winrt::Orientation::Vertical &&
            args.Key() != winrt::VirtualKey::Up &&
            args.Key() != winrt::VirtualKey::Down))
    {
        __super::OnKeyDown(args);
        return;
    }

    winrt::ColorPicker parentColorPicker = GetParentColorPicker();

    if (!parentColorPicker)
    {
        return;
    }

    bool isControlDown = (winrt::Window::Current().CoreWindow().GetKeyState(winrt::VirtualKey::Control) & winrt::CoreVirtualKeyStates::Down) == winrt::CoreVirtualKeyStates::Down;

    double minBound = 0;
    double maxBound = 0;

    Hsv currentHsv = winrt::get_self<ColorPicker>(parentColorPicker)->GetCurrentHsv();
    double currentAlpha = 0;

    switch (ColorChannel())
    {
    case winrt::ColorPickerHsvChannel::Hue:
        minBound = parentColorPicker.MinHue();
        maxBound = parentColorPicker.MaxHue();
        currentHsv.h = Value();
        break;

    case winrt::ColorPickerHsvChannel::Saturation:
        minBound = parentColorPicker.MinSaturation();
        maxBound = parentColorPicker.MaxSaturation();
        currentHsv.s = Value() / 100;
        break;

    case winrt::ColorPickerHsvChannel::Value:
        minBound = parentColorPicker.MinValue();
        maxBound = parentColorPicker.MaxValue();
        currentHsv.v = Value() / 100;
        break;

    case winrt::ColorPickerHsvChannel::Alpha:
        minBound = 0;
        maxBound = 100;
        currentAlpha = Value() / 100;
        break;

    default:
        throw winrt::hresult_error(E_FAIL);
    }

    const bool shouldInvertHorizontalDirection = FlowDirection() == winrt::FlowDirection::RightToLeft && !IsDirectionReversed();

    const IncrementDirection direction =
        ((args.Key() == winrt::VirtualKey::Left && !shouldInvertHorizontalDirection) ||
            (args.Key() == winrt::VirtualKey::Right && shouldInvertHorizontalDirection) ||
            args.Key() == winrt::VirtualKey::Up) ?
        IncrementDirection::Lower :
        IncrementDirection::Higher;

    const IncrementAmount amount = isControlDown ? IncrementAmount::Large : IncrementAmount::Small;

    if (ColorChannel() != winrt::ColorPickerHsvChannel::Alpha)
    {
        currentHsv = IncrementColorChannel(currentHsv, ColorChannel(), direction, amount, false /* shouldWrap */, minBound, maxBound);
    }
    else
    {
        currentAlpha = IncrementAlphaChannel(currentAlpha, direction, amount, false /* shouldWrap */, minBound, maxBound);
    }

    switch (ColorChannel())
    {
    case winrt::ColorPickerHsvChannel::Hue:
        Value(currentHsv.h);
        break;

    case winrt::ColorPickerHsvChannel::Saturation:
        Value(currentHsv.s * 100);
        break;

    case winrt::ColorPickerHsvChannel::Value:
        Value(currentHsv.v * 100);
        break;

    case winrt::ColorPickerHsvChannel::Alpha:
        Value(currentAlpha * 100);
        break;

    default:
        MUX_ASSERT(false);
    }

    args.Handled(true);
}

void ColorPickerSlider::OnGotFocus(winrt::RoutedEventArgs const& /*e*/)
{
    if (auto toolTip = m_toolTip.get())
    {
        toolTip.Content(box_value(GetToolTipString()));
        toolTip.IsEnabled(true);
        toolTip.IsOpen(true);
    }
}

void ColorPickerSlider::OnLostFocus(winrt::RoutedEventArgs const& /*e*/)
{
    if (auto toolTip = m_toolTip.get())
    {
        toolTip.IsOpen(false);
    }
}

void ColorPickerSlider::OnValueChangedEvent(winrt::IInspectable const& /*sender*/, winrt::RangeBaseValueChangedEventArgs const& args)
{
    if (auto toolTip = m_toolTip.get())
    {
        toolTip.Content(box_value(GetToolTipString()));

        // ToolTip doesn't currently provide any way to re-run its placement logic if its placement target moves,
        // so toggling IsEnabled induces it to do that without incurring any visual glitches.
        toolTip.IsEnabled(false);
        toolTip.IsEnabled(true);
    }

    winrt::DependencyObject currentObject = *this;
    winrt::ColorPicker owningColorPicker{ nullptr };

    while (currentObject && (owningColorPicker = currentObject.try_as<winrt::ColorPicker>()) == nullptr)
    {
        currentObject = winrt::VisualTreeHelper::GetParent(currentObject);
    }

    if (owningColorPicker)
    {
        const winrt::Color oldColor = owningColorPicker.Color();
        Hsv hsv = RgbToHsv(RgbFromColor(oldColor));
        hsv.v = args.NewValue() / 100.0;
        const winrt::Color newColor = ColorFromRgba(HsvToRgb(hsv));

        winrt::ColorPickerSliderAutomationPeer peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).as<winrt::ColorPickerSliderAutomationPeer>();
        winrt::get_self<ColorPickerSliderAutomationPeer>(peer)->RaisePropertyChangedEvent(oldColor, newColor, static_cast<int>(round(args.OldValue())), static_cast<int>(round(args.NewValue())));
    }
}

winrt::ColorPicker ColorPickerSlider::GetParentColorPicker()
{
    winrt::ColorPicker parentColorPicker{ nullptr };
    winrt::DependencyObject currentObject = *this;

    while (currentObject && !(parentColorPicker = currentObject.try_as<winrt::ColorPicker>()))
    {
        currentObject = winrt::VisualTreeHelper::GetParent(currentObject);
    }

    return parentColorPicker;
}

winrt::hstring ColorPickerSlider::GetToolTipString()
{
    const unsigned int sliderValue = static_cast<unsigned int>(round(Value()));

    if (ColorChannel() == winrt::ColorPickerHsvChannel::Alpha)
    {
        return StringUtil::FormatString(
            ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringAlphaSlider),
            sliderValue);
    }
    else
    {
        winrt::ColorPicker parentColorPicker = GetParentColorPicker();
        if (parentColorPicker && DownlevelHelper::ToDisplayNameExists())
        {
            Hsv currentHsv = winrt::get_self<ColorPicker>(parentColorPicker)->GetCurrentHsv();
            winrt::hstring localizedString;

            switch (ColorChannel())
            {
            case winrt::ColorPickerHsvChannel::Hue:
                currentHsv.h = Value();
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringHueSliderWithColorName);
                break;

            case winrt::ColorPickerHsvChannel::Saturation:
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringSaturationSliderWithColorName);
                currentHsv.s = Value() / 100;
                break;

            case winrt::ColorPickerHsvChannel::Value:
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringValueSliderWithColorName);
                currentHsv.v = Value() / 100;
                break;
            default:
                throw winrt::hresult_error(E_FAIL);
            }

            return StringUtil::FormatString(
                localizedString,
                sliderValue,
                winrt::ColorHelper::ToDisplayName(ColorFromRgba(HsvToRgb(currentHsv))).data());
        }
        else
        {
            winrt::hstring localizedString;
            switch (ColorChannel())
            {
            case winrt::ColorPickerHsvChannel::Hue:
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringHueSliderWithoutColorName);
                break;
            case winrt::ColorPickerHsvChannel::Saturation:
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringSaturationSliderWithoutColorName);
                break;
            case winrt::ColorPickerHsvChannel::Value:
                localizedString = ResourceAccessor::GetLocalizedStringResource(SR_ToolTipStringValueSliderWithoutColorName);
                break;
            default:
                throw winrt::hresult_error(E_FAIL);
            }

            return StringUtil::FormatString(
                localizedString,
                sliderValue);
        }
    }
}
