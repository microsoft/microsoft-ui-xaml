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
    m_SpinDown = GetTemplateChildT<winrt::Button>(L"DownSpinButton", controlProtected);
    m_SpinUp = GetTemplateChildT<winrt::Button>(L"UpSpinButton", controlProtected);
    SetSpinButtonVisualState();
    Formatter = winrt::DecimalFormatter();

    // Set Text to reflect preset Value
    if (s_ValueProperty != DEFAULTVALUE && m_TextBox) {
        UpdateTextToValue();
    }


    // Register LostFocus Event
    if ( m_TextBox ) {
        m_TextBox.LostFocus({ this, &NumberBox::OnTextBoxLostFocus });
    }
}

void  NumberBox::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();
    
    // Update visual state for spin buttons if placement mode changed
    if (property == s_SpinButtonPlacementModeProperty)
    {
        SetSpinButtonVisualState();
    }
    
    
    // TODO: Implement
}

void NumberBox::OnTextBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
}

void NumberBox::OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    string InputAsString = winrt::to_string(m_TextBox.Text());
    // Handles Empty TextBox Case, current behavior is to set Value to default
    if ( InputAsString == "" )
    {
        this -> Value(0);
        return;
    }

    winrt::IReference<double> parsedNum = Formatter.ParseDouble(m_TextBox.Text());

    if (parsedNum)
    {
        SetErrorState(false);
        ProcessInput( parsedNum.Value() );
    }




}

// Updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
    m_TextBox.Text( winrt::to_hstring( NumberBoxProperties::Value() ));
}

void NumberBox::SetErrorState(bool state)
{

}

// if any preprocessing needs to be done to the input before it is used, it can be added here
void NumberBox::ProcessInput(double val)
{
    Value(val);
    UpdateTextToValue();
}

void NumberBox::SetSpinButtonVisualState()
{

    if ( SpinButtonPlacementMode() == winrt::NumberBoxSpinButtonPlacementMode::Inline )
    {
        winrt::VisualStateManager::GoToState(*this, L"SpinButtonsVisible", false);
    }
    else if ( SpinButtonPlacementMode() == winrt::NumberBoxSpinButtonPlacementMode::Hidden )
    {
        winrt::VisualStateManager::GoToState(*this, L"SpinButtonsCollapsed", false);
    }
}



