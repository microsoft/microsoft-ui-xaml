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
    SetHeader();

    // Initializations - Interactions
    m_SpinDown.Click({ this, &NumberBox::OnSpinDownClick });
    m_SpinUp.Click({ this, &NumberBox::OnSpinUpClick });
    m_TextBox.KeyUp({ this, &NumberBox::OnNumberBoxKeyUp });
    this->PointerWheelChanged({ this, &NumberBox::OnScroll });

    // Initializations - Tools, etc.
    Formatter = winrt::DecimalFormatter();
    IRounder = winrt::IncrementNumberRounder();
    SRounder = winrt::SignificantDigitsNumberRounder();
    UpdateFormatter();
    UpdateRounder();
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
    else if (property == s_IntegerDigitsProperty || property == s_FractionDigitsProperty || property == s_SignificantDigitsProperty || property == s_IsDecimalPointAlwaysDisplayedProperty || property == s_IsZeroSignedProperty)
    {
        UpdateFormatter();
    }
    else if (property == s_RoundingAlgorithmProperty || property == s_NumberRounderProperty || property == s_SignificantDigitPrecisionProperty || property == s_IncrementPrecisionProperty)
    {
        UpdateRounder();
    }
    else if (property == s_HeaderProperty)
    {
        SetHeader();
    }

}

// Trigger any validation, rounding, and processing done onLostFocus
void NumberBox::OnTextBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    ValidateInput();
}

// Performs all validation steps on input given in textbox. Runs on LoseFocus and stepping.
void NumberBox::ValidateInput()
{
    string InputAsString = winrt::to_string(m_TextBox.Text());
    // Handles Empty TextBox Case, current behavior is to set Value to default (0)
    if (InputAsString == "")
    {
        this->Value(0);
        HasError = false;
        return;
    }

    winrt::IReference<double> parsedNum = Formatter.ParseDouble(m_TextBox.Text());
    // TODO: Formatter.Format(value)

    if (parsedNum && IsInBounds(parsedNum.Value()) )
    {
        SetErrorState(false);
        Value(parsedNum.Value());
        UpdateTextToValue();
    }
    else
    {
        SetErrorState(true);
    }
}

// SpinClicks call to decrement or increment, 
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


// Increments or decrements value by StepFrequency, wrapping if necessary
void NumberBox::StepValue(bool sign)
{
    // Validating input before and after
    ValidateInput();
    double newVal = Value();

    if ( sign )
    {
        newVal = newVal + StepFrequency();
    }
    else
    {
        newVal =  newVal - StepFrequency();
    }

    // MinMaxMode Wrapping
    if ( MinMaxMode() == winrt::NumberBoxMinMaxMode::WrapEnabled && !IsInBounds(newVal) )
    {
        while ( newVal > MaxValue() )
        {
            newVal = MinValue() + (newVal - MaxValue()) - 1;
        }
        while ( newVal < MinValue() )
        {
            newVal = MaxValue() - abs(newVal - MinValue()) + 1;
        }
        Value(newVal);
        UpdateTextToValue();
        return;
    }

    // Update Text and Revalidate new value
    Value(newVal);
    UpdateTextToValue();
    ValidateInput();

}




// Runs formatter and updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
    if (!HasError)
    {
        winrt::hstring formattedValue(Formatter.Format(Value()));
        Value( (Formatter.ParseDouble(formattedValue)).Value() );
        m_TextBox.Text(formattedValue);
    }

}

// Handlder for swapping visual states of textbox
// TODO: Implement final visual states in spec
void NumberBox::SetErrorState(bool state)
{
    if (state)
    {
        HasError = true;
        winrt::VisualStateManager::GoToState(*this, L"Invalid", false);
    }
    else
    {
        HasError = false;
        winrt::VisualStateManager::GoToState(*this, L"Valid", false);
    }

}

// Enables or Disables Spin Buttons
// TODO: Styling for buttons
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

// checks if val is in Min/Max bounds based on user's MinMax mode setting
bool NumberBox::IsInBounds(double val)
{
    double min = MinValue();
    double max = MaxValue();
    switch (  MinMaxMode() )
    {
        case winrt::NumberBoxMinMaxMode::None:
            return true;
        case winrt::NumberBoxMinMaxMode::WrapEnabled:
        case winrt::NumberBoxMinMaxMode::MinAndMaxEnabled:
            if (val < min || val > max)
            {
                return false;
            }
            break;
        case winrt::NumberBoxMinMaxMode::MinEnabled:
            if (val < min)
            {
                return false;
            }
            break;
        case winrt::NumberBoxMinMaxMode::MaxEnabled:
            if (val > max)
            {
                return false;
            }
            break;
    }
    return true;
}

void NumberBox::UpdateFormatter()
{
    Formatter.IntegerDigits(IntegerDigits());
    Formatter.FractionDigits(FractionDigits());
    Formatter.SignificantDigits(SignificantDigits());
    Formatter.IsDecimalPointAlwaysDisplayed(IsDecimalPointAlwaysDisplayed());
    Formatter.IsZeroSigned(IsZeroSigned());
  
}

void NumberBox::UpdateRounder()
{
    // Setting a number rounder's RoundingAlgorithm to None can cause a crash because it's not a true value - safer to set Rounder to a null pointer instead
    if ( NumberRounder() == winrt::NumberBoxNumberRounder::None || RoundingAlgorithm() == winrt::RoundingAlgorithm::None)
    {
        Formatter.NumberRounder(nullptr);
        return;
    }

    if (NumberRounder() == winrt::NumberBoxNumberRounder::IncrementNumberRounder) {
        IRounder.Increment(IncrementPrecision());
        IRounder.RoundingAlgorithm(RoundingAlgorithm());
        Formatter.NumberRounder(IRounder);
    }
    else
    {
        SRounder.SignificantDigits( (uint32_t) abs(SignificantDigitPrecision()));
        SRounder.RoundingAlgorithm(RoundingAlgorithm());
        Formatter.NumberRounder(SRounder);
    }

}

void::NumberBox::SetHeader()
{
    /* TODO: Header Code
    winrt::TextBox headerbox;
    headerbox.Text(Header());
    m_TextBox.Header(headerbox);
    */
}



