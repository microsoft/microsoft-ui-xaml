// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "NumberBox.g.h"
#include "NumberBoxValueChangedEventArgs.g.h"
#include "NumberBox.properties.h"
#include "Windows.Globalization.NumberFormatting.h"
#include <regex>

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

    // IUIElement
    virtual winrt::AutomationPeer OnCreateAutomationPeer();

    // IFrameworkElement
    void OnApplyTemplate();

    void OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSpinButtonPlacementModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnPlaceholderTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnBasicValidationModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnHyperScrollEnabledPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

    void OnNumberFormatterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void ValidateNumberFormatter(winrt::INumberFormatter2 value);

private:

    void OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSpinDownClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnSpinUpClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnNumberBoxKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args);
    void OnScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args);

    void ValidateInput();
    void CoerceValue();
    void UpdateTextToValue();
    int ComputePrecisionRounderSigDigits(double newVal);

    void SetSpinButtonVisualState();
    void StepValue(bool isPositive);
    void StepValueUp() { StepValue(true); }
    void StepValueDown() { StepValue(false); }

    bool IsInBounds(double value);

    winrt::DecimalFormatter m_stepPrecisionFormatter{};
    winrt::SignificantDigitsNumberRounder m_stepPrecisionRounder{};

    tracker_ref<winrt::TextBox> m_textBox{ this };

    winrt::RepeatButton::Click_revoker m_upButtonClickRevoker{};
    winrt::RepeatButton::Click_revoker m_downButtonClickRevoker{};
    winrt::TextBox::LostFocus_revoker m_textBoxLostFocusRevoker{};
    winrt::TextBox::KeyUp_revoker m_textBoxKeyUpRevoker{};
};
