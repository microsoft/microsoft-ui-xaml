// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "NumberBox.g.h"
#include "NumberBox.properties.h"
#include "Windows.Globalization.NumberFormatting.h"

#define DEFAULTVALUE 0.0


class NumberBox :
    public ReferenceTracker<NumberBox, winrt::implementation::NumberBoxT>,
    public NumberBoxProperties
{

public:
    NumberBox();
    ~NumberBox() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);




private:

    void OnTextBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args);
    void UpdateTextToValue();
    void SetErrorState(bool state);
    void ProcessInput(double val);
    void SetSpinButtonVisualState();
    winrt::DecimalFormatter Formatter;
    winrt::TextBox m_TextBox{ nullptr };
    winrt::Button m_SpinDown{ nullptr };
    winrt::Button m_SpinUp{ nullptr };


};
