// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"

using namespace std;

NumberBox::NumberBox()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NumberBox);
    SetDefaultStyleKey(this);
}


void NumberBox::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected = *this;
    m_TextBox = GetTemplateChildT<winrt::TextBox>(L"InputBox", controlProtected);

    // Set Text to reflect preset Value
    if (s_ValueProperty != DEFAULTVALUE && m_TextBox) {
        UpdateTextToValue();
    }


    // Register LostFocus Event
    if (m_TextBox) {
        m_TextBox.LostFocus({ this, &NumberBox::OnTextBoxLostFocus });
    }

}

void  NumberBox::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    
    // TODO: Implement
}

void NumberBox::OnTextBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
}

void NumberBox::OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{

    m_TextBox.Text(L"HelloWorld");
}

void NumberBox::UpdateTextToValue()
{
    m_TextBox.Text( winrt::to_hstring( NumberBoxProperties::Value() ));
}



