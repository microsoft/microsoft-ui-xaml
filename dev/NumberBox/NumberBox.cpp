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
    // Initializations - Visual Components
    winrt::IControlProtected controlProtected = *this;
    m_TextBox = GetTemplateChildT<winrt::TextBox>(L"InputBox", controlProtected);
    m_SpinDown = GetTemplateChildT<winrt::Button>(L"DownSpinButton", controlProtected);
    m_SpinUp = GetTemplateChildT<winrt::Button>(L"UpSpinButton", controlProtected);

    // Initializations - Visual States
    SetSpinButtonVisualState();

    // Initializations - Interactions
    m_SpinDown.Click({ this, &NumberBox::OnSpinDownClick });
    m_SpinUp.Click({ this, &NumberBox::OnSpinUpClick });
    m_TextBox.KeyUp({ this, &NumberBox::OnNumberBoxKeyUp });
    this->PointerWheelChanged({ this, &NumberBox::OnScroll });

    // Initializations - Tools, etc.
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
        Value( parsedNum.Value() );
        UpdateTextToValue();
    }
    else
    {
        winrt::VisualStateManager::GoToState(*this, L"Invalid", false);
    }

}


void NumberBox::OnSpinDownClick(winrt::IInspectable const&  sender, winrt::RoutedEventArgs const& args)
{
    StepValue(false);
}
void NumberBox::OnSpinUpClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    StepValue(true);
}
void NumberBox::OnNumberBoxKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    switch (args.Key()) {
        case winrt::VirtualKey::Up:
        case winrt::VirtualKey::GamepadDPadUp:
            StepValue(true);
            break;
        case winrt::VirtualKey::Down:
        case winrt::VirtualKey::GamepadDPadDown:
            StepValue(false);
            break;
    }
}
void NumberBox::OnScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args) {
    if (!HyperScrollEnabled()) {
        return;
    }
    int delta = args.GetCurrentPoint(*this).Properties().MouseWheelDelta();

    if (delta > 0)
    {
        StepValue(true);
    }
    else if (delta < 0)
    {
        StepValue(false);
    }

}



void NumberBox::StepValue(bool sign)
{
    // TODO: ValidateInput before stepping - refactor LostFocus

    if ( sign )
    {
        Value( Value() + StepFrequency() );
    }
    else
    {
        Value(Value() - StepFrequency() );
    }

    // TODO: MinMaxMode Wrapping

    UpdateTextToValue();

}









// Updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
    m_TextBox.Text( winrt::to_hstring( NumberBoxProperties::Value() ));
}

void NumberBox::SetErrorState(bool state)
{

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

bool IsOutOfBounds(double val)
{
    return true;
    // TODO: Bounds using MinMax Modes
}



