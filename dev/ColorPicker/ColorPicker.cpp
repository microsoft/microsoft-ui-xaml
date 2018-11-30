// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SharedHelpers.h"
#include "ColorPicker.h"
#include "ColorSpectrum.h"

#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"

using namespace std;

ColorPicker::ColorPicker()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_ColorPicker);

    SetDefaultStyleKey(this);

    Unloaded({ this, &ColorPicker::OnUnloaded });
}

void ColorPicker::OnApplyTemplate()
{
    winrt::IControlProtected thisAsControlProtected = *this;

    m_colorSpectrum.set(GetTemplateChildT<winrt::ColorSpectrum>(L"ColorSpectrum", thisAsControlProtected));

    m_colorPreviewRectangleGrid = GetTemplateChildT<winrt::Grid>(L"ColorPreviewRectangleGrid", thisAsControlProtected);
    m_colorPreviewRectangle = GetTemplateChildT<winrt::Rectangle>(L"ColorPreviewRectangle", thisAsControlProtected);
    m_previousColorRectangle = GetTemplateChildT<winrt::Rectangle>(L"PreviousColorRectangle", thisAsControlProtected);
    m_colorPreviewRectangleCheckeredBackgroundImageBrush = GetTemplateChildT<winrt::ImageBrush>(L"ColorPreviewRectangleCheckeredBackgroundImageBrush", thisAsControlProtected);

    m_thirdDimensionSlider = GetTemplateChildT<winrt::ColorPickerSlider>(L"ThirdDimensionSlider", thisAsControlProtected);
    m_thirdDimensionSliderGradientBrush = GetTemplateChildT<winrt::LinearGradientBrush>(L"ThirdDimensionSliderGradientBrush", thisAsControlProtected);

    m_alphaSlider = GetTemplateChildT<winrt::ColorPickerSlider>(L"AlphaSlider", thisAsControlProtected);
    m_alphaSliderGradientBrush = GetTemplateChildT<winrt::LinearGradientBrush>(L"AlphaSliderGradientBrush", thisAsControlProtected);
    m_alphaSliderBackgroundRectangle = GetTemplateChildT<winrt::Rectangle>(L"AlphaSliderBackgroundRectangle", thisAsControlProtected);
    m_alphaSliderCheckeredBackgroundImageBrush = GetTemplateChildT<winrt::ImageBrush>(L"AlphaSliderCheckeredBackgroundImageBrush", thisAsControlProtected);

    m_moreButton = GetTemplateChildT<winrt::ButtonBase>(L"MoreButton", thisAsControlProtected);
    
    m_colorRepresentationComboBox = GetTemplateChildT<winrt::ComboBox>(L"ColorRepresentationComboBox", thisAsControlProtected);
    m_redTextBox = GetTemplateChildT<winrt::TextBox>(L"RedTextBox", thisAsControlProtected);
    m_greenTextBox = GetTemplateChildT<winrt::TextBox>(L"GreenTextBox", thisAsControlProtected);
    m_blueTextBox = GetTemplateChildT<winrt::TextBox>(L"BlueTextBox", thisAsControlProtected);
    m_hueTextBox = GetTemplateChildT<winrt::TextBox>(L"HueTextBox", thisAsControlProtected);
    m_saturationTextBox = GetTemplateChildT<winrt::TextBox>(L"SaturationTextBox", thisAsControlProtected);
    m_valueTextBox = GetTemplateChildT<winrt::TextBox>(L"ValueTextBox", thisAsControlProtected);
    m_alphaTextBox = GetTemplateChildT<winrt::TextBox>(L"AlphaTextBox", thisAsControlProtected);
    m_hexTextBox = GetTemplateChildT<winrt::TextBox>(L"HexTextBox", thisAsControlProtected);

    m_RgbComboBoxItem = GetTemplateChildT<winrt::ComboBoxItem>(L"RGBComboBoxItem", thisAsControlProtected);
    m_HsvComboBoxItem = GetTemplateChildT<winrt::ComboBoxItem>(L"HSVComboBoxItem", thisAsControlProtected);
    m_redLabel = GetTemplateChildT<winrt::TextBlock>(L"RedLabel", thisAsControlProtected);
    m_greenLabel = GetTemplateChildT<winrt::TextBlock>(L"GreenLabel", thisAsControlProtected);
    m_blueLabel = GetTemplateChildT<winrt::TextBlock>(L"BlueLabel", thisAsControlProtected);
    m_hueLabel = GetTemplateChildT<winrt::TextBlock>(L"HueLabel", thisAsControlProtected);
    m_saturationLabel = GetTemplateChildT<winrt::TextBlock>(L"SaturationLabel", thisAsControlProtected);
    m_valueLabel = GetTemplateChildT<winrt::TextBlock>(L"ValueLabel", thisAsControlProtected);
    m_alphaLabel = GetTemplateChildT<winrt::TextBlock>(L"AlphaLabel", thisAsControlProtected);

    m_checkerColorBrush = GetTemplateChildT<winrt::SolidColorBrush>(L"CheckerColorBrush", thisAsControlProtected);

    if (auto colorSpectrum = m_colorSpectrum.get())
    {
        colorSpectrum.ColorChanged({ this, &ColorPicker::OnColorSpectrumColorChanged });
        colorSpectrum.SizeChanged({ this, &ColorPicker::OnColorSpectrumSizeChanged });

        winrt::AutomationProperties::SetName(colorSpectrum, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameColorSpectrum));
    }

    if (m_colorPreviewRectangleGrid)
    {
        m_colorPreviewRectangleGrid.SizeChanged({ this, &ColorPicker::OnColorPreviewRectangleGridSizeChanged });
    }

    if (m_thirdDimensionSlider)
    {
        m_thirdDimensionSlider.ValueChanged({ this, &ColorPicker::OnThirdDimensionSliderValueChanged });
        SetThirdDimensionSliderChannel();
    }

    if (m_alphaSlider)
    {
        m_alphaSlider.ValueChanged({ this, &ColorPicker::OnAlphaSliderValueChanged });
        m_alphaSlider.ColorChannel(winrt::ColorPickerHsvChannel::Alpha);

        winrt::AutomationProperties::SetName(m_alphaSlider, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameAlphaSlider));
    }

    if (m_alphaSliderBackgroundRectangle)
    {
        m_alphaSliderBackgroundRectangle.SizeChanged({ this, &ColorPicker::OnAlphaSliderBackgroundRectangleSizeChanged });
    }

    if (m_moreButton)
    {
        if (auto moreButtonAsToggleButton = m_moreButton.try_as<winrt::ToggleButton>())
        {
            moreButtonAsToggleButton.Checked({ this, &ColorPicker::OnMoreButtonChecked });
            moreButtonAsToggleButton.Unchecked({ this, &ColorPicker::OnMoreButtonUnchecked });
        }
        else
        {
            m_moreButton.Click({ this, &ColorPicker::OnMoreButtonClicked });
        }

        winrt::AutomationProperties::SetName(m_moreButton, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameMoreButtonCollapsed));
        winrt::AutomationProperties::SetHelpText(m_moreButton, ResourceAccessor::GetLocalizedStringResource(SR_HelpTextMoreButton));

        m_moreButtonLabel = GetTemplateChildT<winrt::TextBlock>(L"MoreButtonLabel", thisAsControlProtected);

        if (m_moreButtonLabel)
        {
            m_moreButtonLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextMoreButtonLabelCollapsed));
        }
    }

    if (m_colorRepresentationComboBox)
    {
        m_colorRepresentationComboBox.SelectionChanged({ this, &ColorPicker::OnColorRepresentationComboBoxSelectionChanged });

        winrt::AutomationProperties::SetName(m_colorRepresentationComboBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameColorModelComboBox));
    }

    if (m_redTextBox)
    {
        m_redTextBox.TextChanging({ this, &ColorPicker::OnRgbTextChanging });
        m_redTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_redTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_redTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameRedTextBox));
    }

    if (m_greenTextBox)
    {
        m_greenTextBox.TextChanging({ this, &ColorPicker::OnRgbTextChanging });
        m_greenTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_greenTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_greenTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameGreenTextBox));
    }

    if (m_blueTextBox)
    {
        m_blueTextBox.TextChanging({ this, &ColorPicker::OnRgbTextChanging });
        m_blueTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_blueTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_blueTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameBlueTextBox));
    }

    if (m_hueTextBox)
    {
        m_hueTextBox.TextChanging({ this, &ColorPicker::OnHueTextChanging });
        m_hueTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_hueTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_hueTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameHueTextBox));
    }

    if (m_saturationTextBox)
    {
        m_saturationTextBox.TextChanging({ this, &ColorPicker::OnSaturationTextChanging });
        m_saturationTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_saturationTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_saturationTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameSaturationTextBox));
    }

    if (m_valueTextBox)
    {
        m_valueTextBox.TextChanging({ this, &ColorPicker::OnValueTextChanging });
        m_valueTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_valueTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_valueTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameValueTextBox));
    }

    if (m_alphaTextBox)
    {
        m_alphaTextBox.TextChanging({ this, &ColorPicker::OnAlphaTextChanging });
        m_alphaTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_alphaTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_alphaTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameAlphaTextBox));
    }

    if (m_hexTextBox)
    {
        m_hexTextBox.TextChanging({ this, &ColorPicker::OnHexTextChanging });
        m_hexTextBox.GotFocus({ this, &ColorPicker::OnTextBoxGotFocus });
        m_hexTextBox.LostFocus({ this, &ColorPicker::OnTextBoxLostFocus });

        winrt::AutomationProperties::SetName(m_hexTextBox, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameHexTextBox));
    }

    if (m_RgbComboBoxItem)
    {
        m_RgbComboBoxItem.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_ContentRGBComboBoxItem)));
    }

    if (m_HsvComboBoxItem)
    {
        m_HsvComboBoxItem.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_ContentHSVComboBoxItem)));
    }

    if (m_redLabel)
    {
        m_redLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextRedLabel));
    }

    if (m_greenLabel)
    {
        m_greenLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextGreenLabel));
    }

    if (m_blueLabel)
    {
        m_blueLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextBlueLabel));
    }

    if (m_hueLabel)
    {
        m_hueLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextHueLabel));
    }

    if (m_saturationLabel)
    {
        m_saturationLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextSaturationLabel));
    }

    if (m_valueLabel)
    {
        m_valueLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextValueLabel));
    }

    if (m_alphaLabel)
    {
        m_alphaLabel.Text(ResourceAccessor::GetLocalizedStringResource(SR_TextAlphaLabel));
    }

    if (m_checkerColorBrush)
    {
        m_checkerColorBrush.RegisterPropertyChangedCallback(winrt::SolidColorBrush::ColorProperty(), { this, &ColorPicker::OnCheckerColorChanged });
    }

    CreateColorPreviewCheckeredBackground();
    CreateAlphaSliderCheckeredBackground();
    UpdateVisualState(false /* useTransitions */);
    InitializeColor();
    UpdatePreviousColorRectangle();
}

void ColorPicker::OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ColorProperty)
    {
        OnColorChanged(args);
    }
    else if (property == s_PreviousColorProperty)
    {
        OnPreviousColorChanged(args);
    }
    else if (property == s_IsAlphaEnabledProperty)
    {
        OnIsAlphaEnabledChanged(args);
    }
    else if (property == s_IsColorSpectrumVisibleProperty ||
        property == s_IsColorPreviewVisibleProperty ||
        property == s_IsColorSliderVisibleProperty ||
        property == s_IsAlphaSliderVisibleProperty ||
        property == s_IsMoreButtonVisibleProperty ||
        property == s_IsColorChannelTextInputVisibleProperty ||
        property == s_IsAlphaTextInputVisibleProperty ||
        property == s_IsHexInputVisibleProperty)
    {
        OnPartVisibilityChanged(args);
    }
    else if (property == s_MinHueProperty ||
        property == s_MaxHueProperty)
    {
        OnMinMaxHueChanged(args);
    }
    else if (property == s_MinSaturationProperty ||
        property == s_MaxSaturationProperty)
    {
        OnMinMaxSaturationChanged(args);
    }
    else if (property == s_MinValueProperty ||
        property == s_MaxValueProperty)
    {
        OnMinMaxValueChanged(args);
    }
    else if (property == s_ColorSpectrumComponentsProperty)
    {
        OnColorSpectrumComponentsChanged(args);
    }
}

void ColorPicker::OnColorChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    // If we're in the process of internally updating the color, then we don't want to respond to the Color property changing,
    // aside from raising the ColorChanged event.
    if (!m_updatingColor)
    {
        winrt::Color color = Color();

        m_currentRgb = Rgb(color.R / 255.0, color.G / 255.0, color.B / 255.0);
        m_currentAlpha = color.A / 255.0;
        m_currentHsv = RgbToHsv(m_currentRgb);
        m_currentHex = GetCurrentHexValue();

        UpdateColorControls(ColorUpdateReason::ColorPropertyChanged);
    }

    winrt::Color oldColor = unbox_value<winrt::Color>(args.OldValue());
    winrt::Color newColor = unbox_value<winrt::Color>(args.NewValue());

    if (oldColor.A != newColor.A ||
        oldColor.R != newColor.R ||
        oldColor.G != newColor.G ||
        oldColor.B != newColor.B)
    {
        auto colorChangedEventArgs = winrt::make_self<ColorChangedEventArgs>();

        colorChangedEventArgs->OldColor(oldColor);
        colorChangedEventArgs->NewColor(newColor);

        m_colorChangedEventSource(*this, *colorChangedEventArgs);
    }
}

void ColorPicker::OnPreviousColorChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    UpdatePreviousColorRectangle();
    UpdateVisualState(true /* useTransitions */);
}

void ColorPicker::OnIsAlphaEnabledChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    m_currentHex = GetCurrentHexValue();

    if (m_hexTextBox)
    {
        m_updatingControls = true;
        m_hexTextBox.Text(m_currentHex);
        m_updatingControls = false;
    }

    OnPartVisibilityChanged(args);
}

void ColorPicker::OnPartVisibilityChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    UpdateVisualState(true /* useTransitions */);
}

void ColorPicker::OnMinMaxHueChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minHue = MinHue();
    int maxHue = MaxHue();

    if (minHue < 0 || minHue > 359)
    {
        throw winrt::hresult_invalid_argument(L"MinHue must be between 0 and 359.");
    }
    else if (maxHue < 0 || maxHue > 359)
    {
        throw winrt::hresult_invalid_argument(L"MaxHue must be between 0 and 359.");
    }

    m_currentHsv.h = max(static_cast<double>(minHue), min(m_currentHsv.h, static_cast<double>(maxHue)));

    UpdateColor(m_currentHsv, ColorUpdateReason::ColorPropertyChanged);
    UpdateThirdDimensionSlider();
}

void ColorPicker::OnMinMaxSaturationChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minSaturation = MinSaturation();
    int maxSaturation = MaxSaturation();

    if (minSaturation < 0 || minSaturation > 100)
    {
        throw winrt::hresult_invalid_argument(L"MinSaturation must be between 0 and 100.");
    }
    else if (maxSaturation < 0 || maxSaturation > 100)
    {
        throw winrt::hresult_invalid_argument(L"MaxSaturation must be between 0 and 100.");
    }

    m_currentHsv.s = max(static_cast<double>(minSaturation) / 100, min(m_currentHsv.s, static_cast<double>(maxSaturation) / 100));

    UpdateColor(m_currentHsv, ColorUpdateReason::ColorPropertyChanged);
    UpdateThirdDimensionSlider();
}

void ColorPicker::OnMinMaxValueChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    int minValue = MinValue();
    int maxValue = MaxValue();

    if (minValue < 0 || minValue > 100)
    {
        throw winrt::hresult_invalid_argument(L"MinValue must be between 0 and 100.");
    }
    else if (maxValue < 0 || maxValue > 100)
    {
        throw winrt::hresult_invalid_argument(L"MaxValue must be between 0 and 100.");
    }

    m_currentHsv.v = max(static_cast<double>(minValue) / 100, min(m_currentHsv.v, static_cast<double>(maxValue) / 100));

    UpdateColor(m_currentHsv, ColorUpdateReason::ColorPropertyChanged);
    UpdateThirdDimensionSlider();
}

void ColorPicker::OnColorSpectrumComponentsChanged(winrt::DependencyPropertyChangedEventArgs const& args)
{
    UpdateThirdDimensionSlider();
    SetThirdDimensionSliderChannel();
}

void ColorPicker::UpdateVisualState(bool useTransitions)
{
    winrt::IReference<winrt::Color> previousColor = PreviousColor();
    bool isAlphaEnabled = IsAlphaEnabled();
    bool isColorSpectrumVisible = IsColorSpectrumVisible();

    const wchar_t *previousColorStateName;

    if (isColorSpectrumVisible)
    {
        previousColorStateName = previousColor ? L"PreviousColorVisibleVertical" : L"PreviousColorCollapsedVertical";
    }
    else
    {
        previousColorStateName = previousColor ? L"PreviousColorVisibleHorizontal" : L"PreviousColorCollapsedHorizontal";
    }

    winrt::VisualStateManager::GoToState(*this, isColorSpectrumVisible ? L"ColorSpectrumVisible" : L"ColorSpectrumCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, previousColorStateName, useTransitions);
    winrt::VisualStateManager::GoToState(*this, IsColorPreviewVisible() ? L"ColorPreviewVisible" : L"ColorPreviewCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, IsColorSliderVisible() ? L"ThirdDimensionSliderVisible" : L"ThirdDimensionSliderCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, isAlphaEnabled && IsAlphaSliderVisible() ? L"AlphaSliderVisible" : L"AlphaSliderCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, IsMoreButtonVisible() ? L"MoreButtonVisible" : L"MoreButtonCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, !IsMoreButtonVisible() || m_textEntryGridOpened ? L"TextEntryGridVisible" : L"TextEntryGridCollapsed", useTransitions);

    if (m_colorRepresentationComboBox)
    {
        winrt::VisualStateManager::GoToState(*this, m_colorRepresentationComboBox.SelectedIndex() == 0 ? L"RgbSelected" : L"HsvSelected", useTransitions);
    }

    winrt::VisualStateManager::GoToState(*this, IsColorChannelTextInputVisible() ? L"ColorChannelTextInputVisible" : L"ColorChannelTextInputCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, isAlphaEnabled && IsAlphaTextInputVisible() ? L"AlphaTextInputVisible" : L"AlphaTextInputCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, IsHexInputVisible() ? L"HexInputVisible" : L"HexInputCollapsed", useTransitions);
    winrt::VisualStateManager::GoToState(*this, isAlphaEnabled ? L"AlphaEnabled" : L"AlphaDisabled", useTransitions);
}

void ColorPicker::InitializeColor()
{
    auto color = Color();

    m_currentRgb = Rgb(color.R / 255.0, color.G / 255.0, color.B / 255.0);
    m_currentHsv = RgbToHsv(m_currentRgb);
    m_currentAlpha = color.A / 255.0;
    m_currentHex = GetCurrentHexValue();

    SetColorAndUpdateControls(ColorUpdateReason::InitializingColor);
}

void ColorPicker::UpdateColor(const Rgb &rgb, ColorUpdateReason reason)
{
    m_currentRgb = rgb;
    m_currentHsv = RgbToHsv(m_currentRgb);
    m_currentHex = GetCurrentHexValue();

    SetColorAndUpdateControls(reason);
}

void ColorPicker::UpdateColor(const Hsv &hsv, ColorUpdateReason reason)
{
    m_currentHsv = hsv;
    m_currentRgb = HsvToRgb(hsv);
    m_currentHex = GetCurrentHexValue();

    SetColorAndUpdateControls(reason);
}

void ColorPicker::UpdateColor(double alpha, ColorUpdateReason reason)
{
    m_currentAlpha = alpha;
    m_currentHex = GetCurrentHexValue();

    SetColorAndUpdateControls(reason);
}

void ColorPicker::SetColorAndUpdateControls(ColorUpdateReason reason)
{
    m_updatingColor = true;

    Color(ColorFromRgba(m_currentRgb, m_currentAlpha));
    UpdateColorControls(reason);

    m_updatingColor = false;
}

void ColorPicker::UpdatePreviousColorRectangle()
{
    if (!m_previousColorRectangle)
    {
        return;
    }

    winrt::IReference<winrt::Color> previousColor = PreviousColor();

    if (previousColor)
    {
        m_previousColorRectangle.Fill(winrt::SolidColorBrush(previousColor.Value()));
    }
    else
    {
        m_previousColorRectangle.Fill(nullptr);
    }
}

void ColorPicker::UpdateColorControls(ColorUpdateReason reason)
{
    // If we're updating the controls internally, we don't want to execute any of the controls'
    // event handlers, because that would then update the color, which would update the color controls,
    // and then we'd be in an infinite loop.
    m_updatingControls = true;

    // We pass in the reason why we're updating the color controls because
    // we don't want to re-update any control that was the cause of this update.
    // For example, if a user selected a color on the ColorSpectrum, then we
    // don't want to update the ColorSpectrum's color based on this change.
    if (reason != ColorUpdateReason::ColorSpectrumColorChanged && m_colorSpectrum)
    {
        m_colorSpectrum.get().HsvColor(winrt::float4{ static_cast<float>(m_currentHsv.h), static_cast<float>(m_currentHsv.s), static_cast<float>(m_currentHsv.v), static_cast<float>(m_currentAlpha) });
    }

    if (m_colorPreviewRectangle)
    {
        auto color = Color();

        m_colorPreviewRectangle.Fill(winrt::SolidColorBrush(color));
    }

    if (reason != ColorUpdateReason::ThirdDimensionSliderChanged && m_thirdDimensionSlider)
    {
        UpdateThirdDimensionSlider();
    }

    if (reason != ColorUpdateReason::AlphaSliderChanged && m_alphaSlider)
    {
        UpdateAlphaSlider();
    }

    auto strongThis = get_strong();
    auto updateTextBoxes = [strongThis, reason]()
    {
        if (reason != ColorUpdateReason::RgbTextBoxChanged)
        {
            if (strongThis->m_redTextBox)
            {
                strongThis->m_redTextBox.Text(to_wstring(static_cast<::byte>(round(strongThis->m_currentRgb.r * 255))));
            }

            if (strongThis->m_greenTextBox)
            {
                strongThis->m_greenTextBox.Text(to_wstring(static_cast<::byte>(round(strongThis->m_currentRgb.g * 255))));
            }

            if (strongThis->m_blueTextBox)
            {
                strongThis->m_blueTextBox.Text(to_wstring(static_cast<::byte>(round(strongThis->m_currentRgb.b * 255))));
            }
        }

        if (reason != ColorUpdateReason::HsvTextBoxChanged)
        {
            if (strongThis->m_hueTextBox)
            {
                strongThis->m_hueTextBox.Text(to_wstring(static_cast<int>(round(strongThis->m_currentHsv.h))));
            }

            if (strongThis->m_saturationTextBox)
            {
                strongThis->m_saturationTextBox.Text(to_wstring(static_cast<int>(round(strongThis->m_currentHsv.s * 100))));
            }

            if (strongThis->m_valueTextBox)
            {
                strongThis->m_valueTextBox.Text(to_wstring(static_cast<int>(round(strongThis->m_currentHsv.v * 100))));
            }
        }

        if (reason != ColorUpdateReason::AlphaTextBoxChanged && strongThis->m_alphaTextBox)
        {
            strongThis->m_alphaTextBox.Text(to_wstring(static_cast<int>(round(strongThis->m_currentAlpha * 100))) + wstring(L"%"));
        }

        if (reason != ColorUpdateReason::HexTextBoxChanged && strongThis->m_hexTextBox)
        {
            strongThis->m_hexTextBox.Text(strongThis->m_currentHex);
        }
    };

    if (SharedHelpers::IsRS2OrHigher())
    {
        // A reentrancy bug with setting TextBox.Text was fixed in RS2,
        // so we can just directly set the TextBoxes' Text property there.
        updateTextBoxes();
    }
    else if (!SharedHelpers::IsInDesignMode())
    {
        // Otherwise, we need to post this to the dispatcher to avoid that reentrancy bug.
        m_dispatcherHelper.RunAsync([strongThis, updateTextBoxes]()
        {
            strongThis->m_updatingControls = true;
            updateTextBoxes();
            strongThis->m_updatingControls = false;
        });
    }

    m_updatingControls = false;
}

void ColorPicker::OnColorSpectrumColorChanged(const winrt::ColorSpectrum& sender, const winrt::ColorChangedEventArgs& /*args*/)
{
    // If we're updating controls, then this is being raised in response to that,
    // so we'll ignore it.
    if (m_updatingControls)
    {
        return;
    }

    auto hsvColor = sender.HsvColor();
    UpdateColor(Hsv(hsv::GetHue(hsvColor), hsv::GetSaturation(hsvColor), hsv::GetValue(hsvColor)), ColorUpdateReason::ColorSpectrumColorChanged);
}

void ColorPicker::OnColorSpectrumSizeChanged(const winrt::IInspectable& /*sender*/, const winrt::SizeChangedEventArgs& args)
{
    // Since the ColorPicker is arranged vertically, the ColorSpectrum's height can be whatever we want it to be -
    // the width is the limiting factor.  Since we want it to always be a square, we'll set its height to whatever its width is.
    if (args.NewSize().Width != args.PreviousSize().Width)
    {
        m_colorSpectrum.get().Height(args.NewSize().Width);
    }
}

void ColorPicker::OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // If we're in the middle of creating image bitmaps while being unloaded,
    // we'll want to synchronously cancel it so we don't have any asynchronous actions
    // lingering beyond our lifetime.
    CancelAsyncAction(m_createColorPreviewRectangleCheckeredBackgroundBitmapAction);
    CancelAsyncAction(m_alphaSliderCheckeredBackgroundBitmapAction);
}

void ColorPicker::OnCheckerColorChanged(winrt::DependencyObject const& o, winrt::DependencyProperty const& p)
{
    CreateColorPreviewCheckeredBackground();
    CreateAlphaSliderCheckeredBackground();
}

void ColorPicker::OnColorPreviewRectangleGridSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& /*args*/)
{
    CreateColorPreviewCheckeredBackground();
}

void ColorPicker::OnAlphaSliderBackgroundRectangleSizeChanged(winrt::IInspectable const& /*sender*/, winrt::SizeChangedEventArgs const& /*args*/)
{
    CreateAlphaSliderCheckeredBackground();
}

void ColorPicker::OnThirdDimensionSliderValueChanged(winrt::IInspectable const& /*sender*/, winrt::RangeBaseValueChangedEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    winrt::ColorSpectrumComponents components = ColorSpectrumComponents();

    double h = m_currentHsv.h;
    double s = m_currentHsv.s;
    double v = m_currentHsv.v;

    switch (components)
    {
    case winrt::ColorSpectrumComponents::HueValue:
    case winrt::ColorSpectrumComponents::ValueHue:
        s = m_thirdDimensionSlider.Value() / 100.0;
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
    case winrt::ColorSpectrumComponents::SaturationHue:
        v = m_thirdDimensionSlider.Value() / 100.0;
        break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
    case winrt::ColorSpectrumComponents::SaturationValue:
        h = m_thirdDimensionSlider.Value();
        break;
    }

    UpdateColor(Hsv(h, s, v), ColorUpdateReason::ThirdDimensionSliderChanged);
}

void ColorPicker::OnAlphaSliderValueChanged(winrt::IInspectable const& /*sender*/, winrt::RangeBaseValueChangedEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    UpdateColor(m_alphaSlider.Value() / 100.0, ColorUpdateReason::AlphaSliderChanged);
}

void ColorPicker::OnMoreButtonClicked(winrt::IInspectable const& /*sender*/, winrt::RoutedEventArgs const& /*args*/)
{
    m_textEntryGridOpened = !m_textEntryGridOpened;
    UpdateMoreButton();
}

void ColorPicker::OnMoreButtonChecked(winrt::IInspectable const& /*sender*/, winrt::RoutedEventArgs const& /*args*/)
{
    m_textEntryGridOpened = true;
    UpdateMoreButton();
}

void ColorPicker::OnMoreButtonUnchecked(winrt::IInspectable const& /*sender*/, winrt::RoutedEventArgs const& /*args*/)
{
    m_textEntryGridOpened = false;
    UpdateMoreButton();
}

void ColorPicker::UpdateMoreButton()
{
    if (m_moreButton)
    {
        winrt::AutomationProperties::SetName(m_moreButton, ResourceAccessor::GetLocalizedStringResource(m_textEntryGridOpened ? SR_AutomationNameMoreButtonExpanded : SR_AutomationNameMoreButtonCollapsed));
    }

    if (m_moreButtonLabel)
    {
        m_moreButtonLabel.Text(ResourceAccessor::GetLocalizedStringResource(m_textEntryGridOpened ? SR_TextMoreButtonLabelExpanded : SR_TextMoreButtonLabelCollapsed));
    }

    UpdateVisualState(true /* useTransitions */);
}

void ColorPicker::OnColorRepresentationComboBoxSelectionChanged(winrt::IInspectable const& /*sender*/, winrt::SelectionChangedEventArgs const& /*args*/)
{
    UpdateVisualState(true /* useTransitions */);
}

void ColorPicker::OnTextBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& /*args*/)
{
    winrt::TextBox textBox = sender.as<winrt::TextBox>();

    m_isFocusedTextBoxValid = true;
    m_previousString = textBox.Text();
}

void ColorPicker::OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& /*args*/)
{
    winrt::TextBox textBox = sender.as<winrt::TextBox>();

    // When a text box loses focus, we want to check whether its contents were valid.
    // If they weren't, then we'll roll back its contents to their last valid value.
    if (!m_isFocusedTextBoxValid)
    {
        textBox.Text(m_previousString);
    }

    // Now that we know that no text box is currently being edited, we'll update all of the color controls
    // in order to clear away any invalid values currently in any text box.
    UpdateColorControls(ColorUpdateReason::ColorPropertyChanged);
}

void ColorPicker::OnRgbTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    unsigned long componentValue;

    if (TryParseInt(sender.Text(), &componentValue) == false || componentValue < 0 || componentValue > 255)
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(ApplyConstraintsToRgbColor(GetRgbColorFromTextBoxes()), ColorUpdateReason::RgbTextBoxChanged);
    }
}

void ColorPicker::OnHueTextChanging(winrt::TextBox const& /*sender*/, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    unsigned long hueValue;

    int minHue = MinHue();
    int maxHue = MaxHue();

    if (TryParseInt(m_hueTextBox.Text(), &hueValue) == false || hueValue < static_cast<unsigned long>(minHue) || hueValue > static_cast<unsigned long>(maxHue))
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(GetHsvColorFromTextBoxes(), ColorUpdateReason::HsvTextBoxChanged);
    }
}

void ColorPicker::OnSaturationTextChanging(winrt::TextBox const& /*sender*/, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    unsigned long saturationValue;

    int minSaturation = MinSaturation();
    int maxSaturation = MaxSaturation();

    if (TryParseInt(m_saturationTextBox.Text(), &saturationValue) == false || saturationValue < static_cast<unsigned long>(minSaturation) || saturationValue > static_cast<unsigned long>(maxSaturation))
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(GetHsvColorFromTextBoxes(), ColorUpdateReason::HsvTextBoxChanged);
    }
}

void ColorPicker::OnValueTextChanging(winrt::TextBox const& /*sender*/, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    unsigned long valueValue;

    int minValue = MinValue();
    int maxValue = MaxValue();

    if (TryParseInt(m_valueTextBox.Text(), &valueValue) == false || valueValue < static_cast<unsigned long>(minValue) || valueValue > static_cast<unsigned long>(maxValue))
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(GetHsvColorFromTextBoxes(), ColorUpdateReason::HsvTextBoxChanged);
    }
}

void ColorPicker::OnAlphaTextChanging(winrt::TextBox const& /*sender*/, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // If the user hasn't entered a %, we'll do that for them, keeping the cursor
    // where it was before.
    int cursorPosition = m_alphaTextBox.SelectionStart() + m_alphaTextBox.SelectionLength();

    const wchar_t* s = m_alphaTextBox.Text().begin() + m_alphaTextBox.Text().size() - 1;

    if (*s != '%')
    {
        m_alphaTextBox.Text(static_cast<wstring>(m_alphaTextBox.Text()) + wstring(L"%"));
        m_alphaTextBox.SelectionStart(cursorPosition);
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    unsigned long alphaValue;
    winrt::hstring alphaString{ static_cast<wstring>(m_alphaTextBox.Text()).substr(0, m_alphaTextBox.Text().size() - 1) };

    if (TryParseInt(alphaString, &alphaValue) == false || alphaValue < 0 || alphaValue > 100)
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(alphaValue / 100.0, ColorUpdateReason::AlphaTextBoxChanged);
    }
}

void ColorPicker::OnHexTextChanging(winrt::TextBox const& /*sender*/, winrt::TextBoxTextChangingEventArgs const& /*args*/)
{
    // If we're in the process of updating controls in response to a color change,
    // then we don't want to do anything in response to a control being updated,
    // since otherwise we'll get into an infinite loop of updating.
    if (m_updatingControls)
    {
        return;
    }

    // If the user hasn't entered a #, we'll do that for them, keeping the cursor
    // where it was before.
    if (m_hexTextBox.Text().begin()[0] != '#')
    {
        m_hexTextBox.Text(wstring(L"#") + static_cast<wstring>(m_hexTextBox.Text()));
        m_hexTextBox.SelectionStart(m_hexTextBox.Text().size());
    }

    // We'll respond to the text change if the user has entered a valid value.
    // Otherwise, we'll do nothing except mark the text box's contents as invalid.
    bool isAlphaEnabled = IsAlphaEnabled();
    Rgb rgbValue;
    double alphaValue = 1.0;

    if (isAlphaEnabled)
    {
        HexToRgba(m_hexTextBox.Text(), &rgbValue, &alphaValue);
    }
    else
    {
        rgbValue = HexToRgb(m_hexTextBox.Text());
    }

    if ((rgbValue.r == -1 && rgbValue.g == -1 && rgbValue.b == -1 && alphaValue == -1) || alphaValue < 0 || alphaValue > 1)
    {
        m_isFocusedTextBoxValid = false;
    }
    else
    {
        m_isFocusedTextBoxValid = true;
        UpdateColor(ApplyConstraintsToRgbColor(rgbValue), ColorUpdateReason::HexTextBoxChanged);
        UpdateColor(alphaValue, ColorUpdateReason::HexTextBoxChanged);
    }
}

Rgb ColorPicker::GetRgbColorFromTextBoxes()
{
    return Rgb(_wtoi(m_redTextBox.Text().data()) / 255.0, _wtoi(m_greenTextBox.Text().data()) / 255.0, _wtoi(m_blueTextBox.Text().data()) / 255.0);
}

Hsv ColorPicker::GetHsvColorFromTextBoxes()
{
    return Hsv(_wtoi(m_hueTextBox.Text().data()), _wtoi(m_saturationTextBox.Text().data()) / 100.0, _wtoi(m_valueTextBox.Text().data()) / 100.0);
}

winrt::hstring ColorPicker::GetCurrentHexValue()
{
    return IsAlphaEnabled() ? RgbaToHex(m_currentRgb, m_currentAlpha) : RgbToHex(m_currentRgb);
}

Rgb ColorPicker::ApplyConstraintsToRgbColor(const Rgb &rgb)
{
    double minHue = MinHue();
    double maxHue = MaxHue();
    double minSaturation = MinSaturation() / 100.0;
    double maxSaturation = MaxSaturation() / 100.0;
    double minValue = MinValue() / 100.0;
    double maxValue = MaxValue() / 100.0;

    Hsv hsv = RgbToHsv(rgb);

    hsv.h = min(max(hsv.h, minHue), maxHue);
    hsv.s = min(max(hsv.s, minSaturation), maxSaturation);
    hsv.v = min(max(hsv.v, minValue), maxValue);

    return HsvToRgb(hsv);
}

void ColorPicker::UpdateThirdDimensionSlider()
{
    if (!m_thirdDimensionSlider || !m_thirdDimensionSliderGradientBrush)
    {
        return;
    }

    // Since the slider changes only one color dimension, we can use a LinearGradientBrush
    // for its background instead of needing to manually set pixels ourselves.
    // We'll have the gradient go between the minimum and maximum values in the case where
    // the slider handles saturation or value, or in the case where it handles hue,
    // we'll have it go between red, yellow, green, cyan, blue, and purple, in that order.
    m_thirdDimensionSliderGradientBrush.GradientStops().Clear();

    switch (ColorSpectrumComponents())
    {
    case winrt::ColorSpectrumComponents::HueValue:
    case winrt::ColorSpectrumComponents::ValueHue:
    {
        int minSaturation = MinSaturation();
        int maxSaturation = MaxSaturation();

        m_thirdDimensionSlider.Minimum(minSaturation);
        m_thirdDimensionSlider.Maximum(maxSaturation);
        m_thirdDimensionSlider.Value(m_currentHsv.s * 100);

        // If MinSaturation >= MaxSaturation, then by convention MinSaturation is the only value
        // that the slider can take.
        if (minSaturation >= maxSaturation)
        {
            maxSaturation = minSaturation;
        }

        AddGradientStop(m_thirdDimensionSliderGradientBrush, 0.0, { m_currentHsv.h, minSaturation / 100.0, 1.0 }, 1.0);
        AddGradientStop(m_thirdDimensionSliderGradientBrush, 1.0, { m_currentHsv.h, maxSaturation / 100.0, 1.0 }, 1.0);
    }
    break;

    case winrt::ColorSpectrumComponents::HueSaturation:
    case winrt::ColorSpectrumComponents::SaturationHue:
    {
        int minValue = MinValue();
        int maxValue = MaxValue();

        m_thirdDimensionSlider.Minimum(minValue);
        m_thirdDimensionSlider.Maximum(maxValue);
        m_thirdDimensionSlider.Value(m_currentHsv.v * 100);

        // If MinValue >= MaxValue, then by convention MinValue is the only value
        // that the slider can take.
        if (minValue >= maxValue)
        {
            maxValue = minValue;
        }

        AddGradientStop(m_thirdDimensionSliderGradientBrush, 0.0, { m_currentHsv.h, m_currentHsv.s, minValue / 100.0 }, 1.0);
        AddGradientStop(m_thirdDimensionSliderGradientBrush, 1.0, { m_currentHsv.h, m_currentHsv.s, maxValue / 100.0 }, 1.0);
    }
    break;

    case winrt::ColorSpectrumComponents::ValueSaturation:
    case winrt::ColorSpectrumComponents::SaturationValue:
    {
        int minHue = MinHue();
        int maxHue = MaxHue();

        m_thirdDimensionSlider.Minimum(minHue);
        m_thirdDimensionSlider.Maximum(maxHue);
        m_thirdDimensionSlider.Value(m_currentHsv.h);

        // If MinHue >= MaxHue, then by convention MinHue is the only value
        // that the slider can take.
        if (minHue >= maxHue)
        {
            maxHue = minHue;
        }

        double minOffset = minHue / 359.0;
        double maxOffset = maxHue / 359.0;

        // With unclamped hue values, we have six different gradient stops, corresponding to red, yellow, green, cyan, blue, and purple.
        // However, with clamped hue values, we may not need all of those gradient stops.
        // We know we need a gradient stop at the start and end corresponding to the min and max values for hue,
        // and then in the middle, we'll add any gradient stops corresponding to the hue of those six pure colors that exist
        // between the min and max hue.
        AddGradientStop(m_thirdDimensionSliderGradientBrush, 0.0, { static_cast<double>(minHue), 1.0, 1.0 }, 1.0);

        for (int sextant = 1; sextant <= 5; sextant++)
        {
            double offset = sextant / 6.0;

            if (minOffset < offset && maxOffset > offset)
            {
                AddGradientStop(m_thirdDimensionSliderGradientBrush, (offset - minOffset) / (maxOffset - minOffset), { 60.0 * sextant, 1.0, 1.0 }, 1.0);
            }
        }

        AddGradientStop(m_thirdDimensionSliderGradientBrush, 1.0, { static_cast<double>(maxHue), 1.0, 1.0 }, 1.0);
    }
    break;
    }
}

void ColorPicker::SetThirdDimensionSliderChannel()
{
    if (!m_thirdDimensionSlider)
    {
        return;
    }

    switch (ColorSpectrumComponents())
    {
    case winrt::ColorSpectrumComponents::ValueSaturation:
    case winrt::ColorSpectrumComponents::SaturationValue:
        m_thirdDimensionSlider.ColorChannel(winrt::ColorPickerHsvChannel::Hue);
        winrt::AutomationProperties::SetName(m_thirdDimensionSlider, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameHueSlider));
        break;

    case winrt::ColorSpectrumComponents::HueValue:
    case winrt::ColorSpectrumComponents::ValueHue:
        m_thirdDimensionSlider.ColorChannel(winrt::ColorPickerHsvChannel::Saturation);
        winrt::AutomationProperties::SetName(m_thirdDimensionSlider, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameSaturationSlider));
        break;

    case winrt::ColorSpectrumComponents::HueSaturation:
    case winrt::ColorSpectrumComponents::SaturationHue:
        m_thirdDimensionSlider.ColorChannel(winrt::ColorPickerHsvChannel::Value);
        winrt::AutomationProperties::SetName(m_thirdDimensionSlider, ResourceAccessor::GetLocalizedStringResource(SR_AutomationNameValueSlider));
        break;
    }
}

void ColorPicker::UpdateAlphaSlider()
{
    if (!m_alphaSlider || !m_alphaSliderGradientBrush)
    {
        return;
    }

    // Since the slider changes only one color dimension, we can use a LinearGradientBrush
    // for its background instead of needing to manually set pixels ourselves.
    // We'll have the gradient go between the minimum and maximum values in the case where
    // the slider handles saturation or value, or in the case where it handles hue,
    // we'll have it go between red, yellow, green, cyan, blue, and purple, in that order.
    m_alphaSliderGradientBrush.GradientStops().Clear();

    m_alphaSlider.Minimum(0);
    m_alphaSlider.Maximum(100);
    m_alphaSlider.Value(m_currentAlpha * 100);

    AddGradientStop(m_alphaSliderGradientBrush, 0.0, m_currentHsv, 0.0);
    AddGradientStop(m_alphaSliderGradientBrush, 1.0, m_currentHsv, 1.0);
}

void ColorPicker::CreateColorPreviewCheckeredBackground()
{
    if (SharedHelpers::IsInDesignMode()) return;

    if (m_colorPreviewRectangleGrid &&
        m_colorPreviewRectangleCheckeredBackgroundImageBrush)
    {
        int width = static_cast<int>(round(m_colorPreviewRectangleGrid.ActualWidth()));
        int height = static_cast<int>(round(m_colorPreviewRectangleGrid.ActualHeight()));
        shared_ptr<vector<::byte>> bgraCheckeredPixelData = make_shared<vector<::byte>>();
        auto strongThis = get_strong();

        CreateCheckeredBackgroundAsync(
            width,
            height,
            GetCheckerColor(),
            bgraCheckeredPixelData,
            m_createColorPreviewRectangleCheckeredBackgroundBitmapAction,
            m_dispatcherHelper,
            [strongThis](winrt::WriteableBitmap checkeredBackgroundSoftwareBitmap)
        {
            strongThis->m_colorPreviewRectangleCheckeredBackgroundImageBrush.ImageSource(checkeredBackgroundSoftwareBitmap);
        });
    }
}

void ColorPicker::CreateAlphaSliderCheckeredBackground()
{
    if (SharedHelpers::IsInDesignMode()) return;

    if (m_alphaSliderBackgroundRectangle &&
        m_alphaSliderCheckeredBackgroundImageBrush)
    {
        int width = static_cast<int>(round(m_alphaSliderBackgroundRectangle.ActualWidth()));
        int height = static_cast<int>(round(m_alphaSliderBackgroundRectangle.ActualHeight()));
        shared_ptr<vector<::byte>> bgraCheckeredPixelData = make_shared<vector<::byte>>();
        auto strongThis = get_strong();

        CreateCheckeredBackgroundAsync(
            width,
            height,
            GetCheckerColor(),
            bgraCheckeredPixelData,
            m_alphaSliderCheckeredBackgroundBitmapAction,
            m_dispatcherHelper,
            [strongThis](winrt::WriteableBitmap checkeredBackgroundSoftwareBitmap)
        {
            strongThis->m_alphaSliderCheckeredBackgroundImageBrush.ImageSource(checkeredBackgroundSoftwareBitmap);
        });
    }
}

void ColorPicker::AddGradientStop(winrt::LinearGradientBrush brush, double offset, Hsv hsvColor, double alpha)
{
    winrt::GradientStop stop;

    Rgb rgbColor = HsvToRgb(hsvColor);

    stop.Color(winrt::ColorHelper::FromArgb(
        static_cast<unsigned char>(round(alpha * 255)),
        static_cast<unsigned char>(round(rgbColor.r * 255)),
        static_cast<unsigned char>(round(rgbColor.g * 255)),
        static_cast<unsigned char>(round(rgbColor.b * 255))));
    stop.Offset(offset);

    brush.GradientStops().Append(stop);
}

winrt::Color ColorPicker::GetCheckerColor()
{
    winrt::Color checkerColor;

    if (m_checkerColorBrush)
    {
        checkerColor = m_checkerColorBrush.Color();
    }
    else
    {
        winrt::IInspectable checkerColorAsI = winrt::Application::Current().Resources().Lookup(box_value(L"SystemListLowColor"));
        checkerColor = unbox_value<winrt::Color>(checkerColorAsI);
    }

    return checkerColor;
}