// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "NumberBox.g.h"
#include "NumberBoxValueChangedEventArgs.g.h"
#include "NumberBox.properties.h"
#include "Windows.Globalization.NumberFormatting.h"

class NumberBoxValueChangedEventArgs :
    public winrt::implementation::NumberBoxValueChangedEventArgsT<NumberBoxValueChangedEventArgs>
{
public:
    NumberBoxValueChangedEventArgs(double oldValue, double newValue) : m_oldValue(oldValue), m_newValue(newValue) {}

    double OldValue() { return m_oldValue; }
    double NewValue() { return m_newValue; }

private:
    double m_oldValue;
    double m_newValue;
};

class NumberBox :
    public ReferenceTracker<NumberBox, winrt::implementation::NumberBoxT>,
    public NumberBoxProperties
{
    
public:

    NumberBox();

    void Value(double value);
    double Value();

    // IUIElement
    //virtual winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();

    void OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHeaderTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSpinButtonPlacementModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnValidationModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSmallChangePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnLargeChangePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnIsWrapEnabledPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnNumberFormatterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void ValidateNumberFormatter(winrt::INumberFormatter2 value);

private:
    winrt::DecimalFormatter GetRegionalSettingsAwareDecimalFormatter();

    void OnSpinDownClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSpinUpClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnNumberBoxKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnNumberBoxKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnNumberBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnNumberBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnNumberBoxScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);
    void OnCornerRadiusPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/);

    void ValidateInput();
    void CoerceMinimum();
    void CoerceMaximum();
    void CoerceValue();
    void UpdateTextToValue();
    void UpdateValueToText();

    void UpdateSpinButtonPlacement();
    void UpdateSpinButtonEnabled();
    void StepValue(double change);

    void UpdateHeaderPresenterState();

    bool IsInBounds(double value);

    bool m_valueUpdating{ false };
    bool m_textUpdating{ false };

    winrt::SignificantDigitsNumberRounder m_displayRounder{};

    tracker_ref<winrt::TextBox> m_textBox{ this };
    tracker_ref<winrt::ContentPresenter> m_headerPresenter{ this };
    tracker_ref<winrt::Popup> m_popup{ this };

    winrt::RepeatButton::Click_revoker m_upButtonClickRevoker{};
    winrt::RepeatButton::Click_revoker m_downButtonClickRevoker{};
    winrt::TextBox::PreviewKeyDown_revoker m_textBoxPreviewKeyDownRevoker{};
    winrt::TextBox::KeyDown_revoker m_textBoxKeyDownRevoker{};
    winrt::TextBox::KeyUp_revoker m_textBoxKeyUpRevoker{};
    winrt::RepeatButton::Click_revoker m_popupUpButtonClickRevoker{};
    winrt::RepeatButton::Click_revoker m_popupDownButtonClickRevoker{};

    PropertyChanged_revoker m_cornerRadiusChangedRevoker{};
};
