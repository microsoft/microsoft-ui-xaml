// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorHelpers.h"
#include "ColorChangedEventArgs.h"
#include "DispatcherHelper.h"

#include "ColorPicker.g.h"

#include "ColorPicker.properties.h"

class ColorPicker : 
    public ReferenceTracker<ColorPicker, winrt::implementation::ColorPickerT>,
    public ColorPickerProperties
{
public:
    ColorPicker();

    // IFrameworkElementOverrides overrides
    void OnApplyTemplate();

    // Property changed handler.
    void OnPropertyChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    Hsv & GetCurrentHsv() { return m_currentHsv; }

private:
    // DependencyProperty changed event handlers
    void OnColorChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnPreviousColorChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnIsAlphaEnabledChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnPartVisibilityChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxHueChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxSaturationChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnMinMaxValueChanged(winrt::DependencyPropertyChangedEventArgs const& args);
    void OnColorSpectrumComponentsChanged(winrt::DependencyPropertyChangedEventArgs const& args);

    // ColorPicker event handlers
    void OnUnloaded(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnCheckerColorChanged(winrt::DependencyObject const& o, winrt::DependencyProperty const& p);

    // Size changed event handlers
    void OnColorPreviewRectangleGridSizeChanged(winrt::IInspectable const& sender, winrt::SizeChangedEventArgs const& args);
    void OnAlphaSliderBackgroundRectangleSizeChanged(winrt::IInspectable const& sender, winrt::SizeChangedEventArgs const& args);

    // ColorSpectrum event handlers
    void OnColorSpectrumColorChanged(const winrt::ColorSpectrum& sender, const winrt::ColorChangedEventArgs& args);
    void OnColorSpectrumSizeChanged(const winrt::IInspectable& sender, const winrt::SizeChangedEventArgs& args);

    // Slider event handlers
    void OnThirdDimensionSliderValueChanged(winrt::IInspectable const& sender, winrt::RangeBaseValueChangedEventArgs const& args);
    void OnAlphaSliderValueChanged(winrt::IInspectable const& sender, winrt::RangeBaseValueChangedEventArgs const& args);

    // More button event handlers
    void OnMoreButtonClicked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnMoreButtonChecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnMoreButtonUnchecked(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    // Color representation ComboBox event handler
    void OnColorRepresentationComboBoxSelectionChanged(winrt::IInspectable const& sender, winrt::SelectionChangedEventArgs const& args);

    // General TextBox event handlers
    void OnTextBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);

    // RGB TextBox event handlers
    void OnRgbTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);

    // HSV TextBox event handlers
    void OnHueTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);
    void OnSaturationTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);
    void OnValueTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);

    // Alpha TextBox event handler
    void OnAlphaTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);

    // Hex TextBox event handler
    void OnHexTextChanging(winrt::TextBox const& sender, winrt::TextBoxTextChangingEventArgs const& args);

    // Helper functions
    void UpdateVisualState(bool useTransitions);

    static void AddGradientStop(const winrt::LinearGradientBrush& brush, double offset, Hsv hsvColor, double alpha);

    winrt::Color GetCheckerColor();

    enum class ColorUpdateReason
    {
        InitializingColor,
        ColorPropertyChanged,
        ColorSpectrumColorChanged,
        ThirdDimensionSliderChanged,
        AlphaSliderChanged,
        RgbTextBoxChanged,
        HsvTextBoxChanged,
        AlphaTextBoxChanged,
        HexTextBoxChanged,
    };

    void InitializeColor();
    void UpdateColor(const Rgb &rgb, ColorUpdateReason reason);
    void UpdateColor(const Hsv &hsv, ColorUpdateReason reason);
    void UpdateColor(double alpha, ColorUpdateReason reason);
    void SetColorAndUpdateControls(ColorUpdateReason reason);

    void UpdatePreviousColorRectangle();

    void UpdateColorControls(ColorUpdateReason reason);

    void UpdateThirdDimensionSlider();
    void SetThirdDimensionSliderChannel();
    void UpdateAlphaSlider();

    void CreateColorPreviewCheckeredBackground();
    void CreateAlphaSliderCheckeredBackground();

    Rgb GetRgbColorFromTextBoxes();
    Hsv GetHsvColorFromTextBoxes();
    winrt::hstring GetCurrentHexValue();

    Rgb ApplyConstraintsToRgbColor(const Rgb &rgb);

    void UpdateMoreButton();

    bool m_updatingColor{ false };
    bool m_updatingControls{ false };
    Rgb m_currentRgb{ 1.0, 1.0, 1.0 };
    Hsv m_currentHsv{ 0.0, 1.0, 1.0 };
    winrt::hstring m_currentHex{ L"#FFFFFFFF" };
    double m_currentAlpha{ 1.0 };

    winrt::hstring m_previousString{ L"" };
    bool m_isFocusedTextBoxValid{ false };

    bool m_textEntryGridOpened{ false };

    // Template parts
    tracker_ref<winrt::ColorSpectrum> m_colorSpectrum{ this };

    tracker_ref<winrt::Grid> m_colorPreviewRectangleGrid{ this };
    tracker_ref<winrt::Rectangle> m_colorPreviewRectangle{ this };
    tracker_ref<winrt::Rectangle> m_previousColorRectangle{ this };
    tracker_ref<winrt::ImageBrush> m_colorPreviewRectangleCheckeredBackgroundImageBrush{ this };

    winrt::IAsyncAction m_createColorPreviewRectangleCheckeredBackgroundBitmapAction{ nullptr };

    tracker_ref<winrt::ColorPickerSlider> m_thirdDimensionSlider{ this };
    tracker_ref<winrt::LinearGradientBrush> m_thirdDimensionSliderGradientBrush{ this };

    tracker_ref<winrt::ColorPickerSlider> m_alphaSlider{ this };
    tracker_ref<winrt::LinearGradientBrush> m_alphaSliderGradientBrush{ this };
    tracker_ref<winrt::Rectangle> m_alphaSliderBackgroundRectangle{ this };
    tracker_ref<winrt::ImageBrush> m_alphaSliderCheckeredBackgroundImageBrush{ this };

    winrt::IAsyncAction m_alphaSliderCheckeredBackgroundBitmapAction{ nullptr };

    tracker_ref<winrt::ButtonBase> m_moreButton{ this };
    tracker_ref<winrt::TextBlock> m_moreButtonLabel{ this };
    
    tracker_ref<winrt::ComboBox> m_colorRepresentationComboBox{ this };
    tracker_ref<winrt::TextBox> m_redTextBox{ this };
    tracker_ref<winrt::TextBox> m_greenTextBox{ this };
    tracker_ref<winrt::TextBox> m_blueTextBox{ this };
    tracker_ref<winrt::TextBox> m_hueTextBox{ this };
    tracker_ref<winrt::TextBox> m_saturationTextBox{ this };
    tracker_ref<winrt::TextBox> m_valueTextBox{ this };
    tracker_ref<winrt::TextBox> m_alphaTextBox{ this };
    tracker_ref<winrt::TextBox> m_hexTextBox{ this };

    tracker_ref<winrt::ComboBoxItem> m_RgbComboBoxItem{ this };
    tracker_ref<winrt::ComboBoxItem> m_HsvComboBoxItem{ this };
    tracker_ref<winrt::TextBlock> m_redLabel{ this };
    tracker_ref<winrt::TextBlock> m_greenLabel{ this };
    tracker_ref<winrt::TextBlock> m_blueLabel{ this };
    tracker_ref<winrt::TextBlock> m_hueLabel{ this };
    tracker_ref<winrt::TextBlock> m_saturationLabel{ this };
    tracker_ref<winrt::TextBlock> m_valueLabel{ this };
    tracker_ref<winrt::TextBlock> m_alphaLabel{ this };

    tracker_ref<winrt::SolidColorBrush> m_checkerColorBrush{ this };

    DispatcherHelper m_dispatcherHelper{ *this };
};
