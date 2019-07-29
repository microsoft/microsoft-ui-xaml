// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"


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
    SetPlaceHolderText();

    // Initializations - Interactions
    m_SpinDown.Click({ this, &NumberBox::OnSpinDownClick });
    m_SpinUp.Click({ this, &NumberBox::OnSpinUpClick });
    m_TextBox.KeyUp({ this, &NumberBox::OnNumberBoxKeyUp });
    PointerWheelChanged({ this, &NumberBox::OnScroll });

    // Initializations - Tools, etc.
    m_formatter = winrt::DecimalFormatter();
    m_iRounder = winrt::IncrementNumberRounder();
    m_sRounder = winrt::SignificantDigitsNumberRounder();
    UpdateFormatter();
    UpdateRounder();
    // Set Text to reflect preset Value
    if (s_ValueProperty != 0.0 && m_TextBox)
    {
        UpdateTextToValue();
    }

    // Register LostFocus Event
    if (m_TextBox)
    {
        m_TextBox.LostFocus({ this, &NumberBox::OnTextBoxLostFocus });
    }
}


void NumberBox::OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SetHeader();
}

void NumberBox::OnSpinButtonPlacementModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SetSpinButtonVisualState();
}

void NumberBox::OnPlaceholderTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SetPlaceHolderText();
}

void NumberBox::OnFractionDigitsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{

 m_formatter.FractionDigits(FractionDigits());
}

void NumberBox::OnIntegerDigitsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_formatter.IntegerDigits(IntegerDigits());
}

void NumberBox::OnSignificantDigitsPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_formatter.SignificantDigits(SignificantDigits());
}

void NumberBox::OnIsDecimalPointAlwaysDisplayedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_formatter.IsDecimalPointAlwaysDisplayed(IsDecimalPointAlwaysDisplayed());
}

void NumberBox::OnIsZeroSignedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_formatter.IsZeroSigned(IsZeroSigned());
}

void NumberBox::OnRoundingAlgorithmPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRounder();
}

void NumberBox::OnNumberRounderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRounder();
}

void NumberBox::OnIncrementPrecisionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRounder();
}

void NumberBox::OnSignificantDigitPrecisionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateRounder();
}

void NumberBox::OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    m_TextBox.Text(Text());
}


void NumberBox::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (m_TextBox)
    {
       // UpdateTextToValue(); Causing bugs currently, disabled. 
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
    // Handles Empty TextBox Case, current behavior is to set Value to default (0)
    if (m_TextBox.Text() == L"")
    {
        Value(0);
        m_hasError = false;
        return;
    }

    if (AcceptsCalculation() && IsFormulaic(m_TextBox.Text()))
    {
            EvaluateInput();   
    }

    auto parsedNum = m_formatter.ParseDouble(m_TextBox.Text());

    if (parsedNum && IsInBounds(parsedNum.Value()) )
    {
        SetErrorState(false);
        Value(parsedNum.Value());
        UpdateTextToValue();
    }
    else
    {
        if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
        {
            // Revert to previous value
            SetErrorState(false);
            UpdateTextToValue();
            return;
        }
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
    if (HyperScrollEnabled()) {
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
}

// Increments or decrements value by StepFrequency, wrapping if necessary
void NumberBox::StepValue(bool sign)
{
    // Validating input before and after
    ValidateInput();
    double newVal = Value();

    if (sign)
    {
        newVal += StepFrequency();
    }
    else
    {
        newVal -= StepFrequency();
    }

    // MinMaxMode Wrapping
    if (MinMaxMode() == winrt::NumberBoxMinMaxMode::WrapEnabled && !IsInBounds(newVal))
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

    // Input Overwriting - Coerce to min or max
    if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten && !IsInBounds(newVal) )
    {
        if (newVal > MaxValue() && (MinMaxMode() == winrt::NumberBoxMinMaxMode::MaxEnabled || MinMaxMode() == winrt::NumberBoxMinMaxMode::MinAndMaxEnabled) )
        {
            newVal = MaxValue();
        }
        else if (newVal < MinValue() && (MinMaxMode() == winrt::NumberBoxMinMaxMode::MinEnabled || MinMaxMode() == winrt::NumberBoxMinMaxMode::MinAndMaxEnabled))
        {
            newVal = MinValue();
        }
    }

    // Update Text and Revalidate new value
    Value(newVal);
    UpdateTextToValue();
    ValidateInput();
}

// Check if text resembles formulaic input to determine if parser should be executed
bool NumberBox::IsFormulaic(winrt::hstring in)
{
    std::regex r("^([0-9()\\s]*[+-/*^]+[0-9()\\s]*)+$");
    return (std::regex_match(winrt::to_string(in), r));
}

void NumberBox::EvaluateInput()
{
    double val;

    try
    {
        val = NumberBoxParser::Compute(m_TextBox.Text());
    }
    catch (std::exception e)
    {
        // User probably entered a malformed expression. Cancel evaluation and invalidate. 
        return;
    }
   Value(val);
   UpdateTextToValue(); 
}

// Runs formatter and updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
        winrt::hstring formattedValue(m_formatter.Format(Value()));
        Value( (m_formatter.ParseDouble(formattedValue)).Value() );
        m_TextBox.Text(formattedValue);
}

// Handlder for swapping visual states of textbox
// TODO: Implement final visual states in spec
void NumberBox::SetErrorState(bool state)
{
    if (state && BasicValidationMode() != winrt::NumberBoxBasicValidationMode::Disabled)
    {
        m_hasError = true;
        winrt::VisualStateManager::GoToState(*this, L"Invalid", false);
    }
    else
    {
        m_hasError = false;
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
    m_formatter.IntegerDigits(IntegerDigits());
    m_formatter.FractionDigits(FractionDigits());
    m_formatter.SignificantDigits(SignificantDigits());
    m_formatter.IsDecimalPointAlwaysDisplayed(IsDecimalPointAlwaysDisplayed());
    m_formatter.IsZeroSigned(IsZeroSigned());
}

void NumberBox::UpdateRounder()
{
    // Setting a number rounder's RoundingAlgorithm to None can cause a crash because it's not a true value - safer to set Rounder to a null pointer instead
    if ( NumberRounder() == winrt::NumberBoxNumberRounder::None || RoundingAlgorithm() == winrt::RoundingAlgorithm::None)
    {
        m_formatter.NumberRounder(nullptr);
        return;
    }

    else if (NumberRounder() == winrt::NumberBoxNumberRounder::IncrementNumberRounder) {
        m_iRounder.Increment(IncrementPrecision());
        m_iRounder.RoundingAlgorithm(RoundingAlgorithm());
        m_formatter.NumberRounder(m_iRounder);
    }
    else
    {
        m_sRounder.SignificantDigits( (uint32_t) abs(SignificantDigitPrecision()));
        m_sRounder.RoundingAlgorithm(RoundingAlgorithm());
        m_formatter.NumberRounder(m_sRounder);
    }

}

void NumberBox::SetHeader()
{
    /* TODO: Header Code currently disabled
    winrt::TextBox headerbox;
    headerbox.Text(Header());
    m_TextBox.Header(headerbox);
    */
}

// Sets TextBox placeholder text
void NumberBox::SetPlaceHolderText()
{
    m_TextBox.PlaceholderText(PlaceholderText());
}



