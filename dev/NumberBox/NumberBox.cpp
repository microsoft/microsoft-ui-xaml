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

    if (ReadLocalValue(winrt::RangeBase::MaximumProperty()) == winrt::DependencyProperty::UnsetValue())
    {
        Maximum(std::numeric_limits<double>::max());
    }

    if (ReadLocalValue(winrt::RangeBase::MinimumProperty()) == winrt::DependencyProperty::UnsetValue())
    {
        Minimum(-std::numeric_limits<double>::max());
    }

    //### need to implement this! ....or do I? Won't it do that for me?
    //RegisterPropertyChangedCallback(winrt::RangeBase::MinimumProperty(), { this, &NumberBox::OnMinimumPropertyChanged });
    //RegisterPropertyChangedCallback(winrt::RangeBase::MaximumProperty(), { this, &NumberBox::OnMaximumPropertyChanged });
}

void NumberBox::OnApplyTemplate()
{
    // Initializations - Visual Components
    winrt::IControlProtected controlProtected = *this;

    m_errorTextBlock.set(GetTemplateChildT<winrt::TextBlock>(L"ErrorTextMessage", controlProtected));

    // Initializations - Visual States
    SetSpinButtonVisualState();
    SetHeader();
    SetPlaceHolderText();
    
    // Initializations - Interactions
    if (auto warningIcon = GetTemplateChildT<winrt::FontIcon>(L"ValidationIcon", controlProtected))
    {
        winrt::ToolTipService::SetToolTip(warningIcon, m_errorToolTip);
    }

    if (auto spinDown = GetTemplateChildT<winrt::RepeatButton>(L"DownSpinButton", controlProtected))
    {
        spinDown.Click({ this, &NumberBox::OnSpinDownClick });
    }

    if (auto spinUp = GetTemplateChildT<winrt::RepeatButton>(L"UpSpinButton", controlProtected))
    {
        spinUp.Click({ this, &NumberBox::OnSpinUpClick });
    }

    m_textBox.set([this, controlProtected]() {
        auto textBox = GetTemplateChildT<winrt::TextBox>(L"InputBox", controlProtected);
        if (textBox)
        {
            textBox.LostFocus({ this, &NumberBox::OnTextBoxLostFocus });
            textBox.KeyUp({ this, &NumberBox::OnNumberBoxKeyUp });
        }
        return textBox;
    }());

    PointerWheelChanged({ this, &NumberBox::OnScroll });

    // Initializations - Tools, etc.
    UpdateFormatter();
    UpdateRounder();

    // Initializing precision formatter. This formatter works neutrally to protect against floating point imprecision resulting from stepping/calc
    m_stepPrecisionFormatter.FractionDigits(0);
    m_stepPrecisionFormatter.IntegerDigits(1);
    m_stepPrecisionFormatter.NumberRounder(nullptr);
    m_stepPrecisionRounder.RoundingAlgorithm(winrt::RoundingAlgorithm::RoundHalfAwayFromZero);

    // Set Text to reflect preset Value
    UpdateTextToValue();
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
    if (auto textBox = m_textBox.get())
    {
        textBox.Text(Text());
    }
}


void NumberBox::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateTextToValue();
}

void NumberBox::OnBasicValidationModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
        if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::Disabled)
        {
            SetErrorState(ValidationState::Valid);
        }
        else
        {
            // Revalidate input if it's changed
            ValidateInput();
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
    if (auto textBox = m_textBox.get())
    {
        auto text = textBox.Text();
        
        // ### well, 0 might not be in bounds, we'd be better defaulting to min probably?
        // Handles Empty TextBox Case, current behavior is to set Value to default (0)
        if (text == L"")
        {
            Value(0);
            m_hasError = false;
            return;
        }

        if (AcceptsCalculation() && IsFormulaic(text))
        {
            NormalizeShorthandOperations();
            EvaluateInput();

            // Divide by 0 error state
            if (fpclassify(Value()) == FP_NAN)
            {
                SetErrorState(ValidationState::InvalidDivide);
                return;
            }
        }

        auto parsedNum = m_formatter.ParseDouble(text);

        // Rounding separately because rounding affects Value property while formatting does not
        if (parsedNum && NumberRounder() != winrt::NumberBoxNumberRounder::None)
        {
            if (NumberRounder() == winrt::NumberBoxNumberRounder::IncrementNumberRounder)
            {
                parsedNum = m_iRounder.RoundDouble(parsedNum.Value());
            }
            else
            {
                parsedNum = m_sRounder.RoundDouble(parsedNum.Value());
            }
        }

        // Valid, in bounds value
        if (parsedNum && GetBoundState(parsedNum.Value()) == BoundState::InBounds )
        {
            SetErrorState(ValidationState::Valid);
            Value(parsedNum.Value());
            UpdateTextToValue();
        }
        else
        {
            // No parsable value
            if (!parsedNum && BasicValidationMode() != winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
            {
                SetErrorState(ValidationState::InvalidInput);
                return;
            }

            // Value needs to be overwritten
            if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
            {
                // Revert to previous value
                SetErrorState(ValidationState::Valid);
                UpdateTextToValue();
                return;
            }

            // Parsable value that is not in bounds
            BoundState invalidState = GetBoundState(parsedNum.Value());

            switch (invalidState)
            {
                case BoundState::OverMax:
                    SetErrorState(ValidationState::InvalidMax);
                    break;
                case BoundState::UnderMin:
                    SetErrorState(ValidationState::InvalidMin);
                    break;
                default:
                    SetErrorState(ValidationState::Invalid);
            }

            Value(parsedNum.Value());
            UpdateTextToValue();
        }
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
    // ### Gamepad Interactions still untested
    switch (args.OriginalKey())
    {
        case winrt::VirtualKey::Enter:
        case winrt::VirtualKey::GamepadA:
            ValidateInput();
            break;

        case winrt::VirtualKey::Escape:
        case winrt::VirtualKey::GamepadB:
            UpdateTextToValue();
            break;

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

void NumberBox::OnScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (HyperScrollEnabled())
    {
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
void NumberBox::StepValue(bool isPositive)
{
    // Validating input before and after
    ValidateInput();
    double oldVal = Value();
    double newVal = oldVal;

    if (isPositive)
    {
        newVal += SmallChange();
    }
    else
    {
        newVal -= SmallChange();
    }

    //### hmmmm
    /*if (WrapEnabled() && GetBoundState(newVal) != BoundState::InBounds)
    {
        while (newVal > Maximum())
        {
            newVal = Minimum() + (newVal - Maximum()) - 1;
        }
        while (newVal < Minimum())
        {
            newVal = Maximum() - abs(newVal - Minimum()) + 1;
        }
    }*/
    //### how is wrap supposed to work? because I don't think it makes sense as it was before.
    if (WrapEnabled())
    {
        if (newVal > Maximum())
        {
            newVal = Minimum();
        }
        if (newVal < Minimum())
        {
            newVal = Maximum();
        }
    }

    //### don't think I need to do this
    // Input Overwriting - Coerce to min or max
    /*else if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten && GetBoundState(newVal) != BoundState::InBounds)
    {
        if (newVal > Maximum() && (MinMaxMode() == winrt::NumberBoxMinMaxMode::MaxEnabled || MinMaxMode() == winrt::NumberBoxMinMaxMode::MinAndMaxEnabled))
        {
            newVal = Maximum();
        }
        else if (newVal < Minimum() && (MinMaxMode() == winrt::NumberBoxMinMaxMode::MinEnabled || MinMaxMode() == winrt::NumberBoxMinMaxMode::MinAndMaxEnabled))
        {
            newVal = Minimum();
        }
    }*/

    //### maybe not this either?
    // Safeguard for floating point imprecision errors
    int StepFreqSigDigits = ComputePrecisionRounderSigDigits(newVal);
    m_stepPrecisionRounder.SignificantDigits(StepFreqSigDigits);
    newVal = m_stepPrecisionRounder.RoundDouble(newVal);

    // Update Text and Revalidate new value
    Value(newVal);
    UpdateTextToValue();
    ValidateInput();
}


// Check if text resembles formulaic input to determine if parser should be executed
bool NumberBox::IsFormulaic(const winrt::hstring& in)
{
    std::wstring input(in);
    std::wregex formula(L"^([0-9()\\s]*[+-/*^%]+[0-9()\\s]*)+$");
    std::wregex negval(L"^-([0-9])+(.([0-9])+)?$");
    return (std::regex_match(input, formula) && !std::regex_match(input, negval));
}

// ### what does this do???
// Computes the number of significant digits that precision rounder should use. This helps to prevent floating point imprecision errors. 
int NumberBox::ComputePrecisionRounderSigDigits(double newVal)
{
    auto const oldVal = Value();

    // Run formatter on both values to discard trailing and leading 0's.
    std::wstring formattedVal(m_stepPrecisionFormatter.Format(oldVal));
    std::wstring formattedStep(m_stepPrecisionFormatter.Format(SmallChange()));
    std::wstring formattedNew(m_stepPrecisionFormatter.Format(newVal));

    // Get size of only decimal portion of both old numbers. 
    int oldValSig = static_cast<int>(formattedVal.substr(formattedVal.find_first_of('.') + 1).size());
    int StepSig = static_cast<int>(formattedStep.substr(formattedStep.find_first_of('.') + 1).size());

    // Pick bigger of two decimal sigDigits
    int result = std::max(oldValSig, StepSig);

    // append # of integer digits from new value
    result += (int) formattedNew.substr(0, formattedNew.find_first_of('.')).size();
    return result;
}

// Appends current value to start of special shorthand functions in form "(Operation Operand)*"
void NumberBox::NormalizeShorthandOperations()
{
    if (auto textBox = m_textBox.get())
    {
        std::wregex r(L"^\\s*([+-/*^%]+[0-9()\\s]*)+$");
        if (std::regex_match(textBox.Text().data(), r))
        {
            std::wstringstream ss;
            ss << Value() << textBox.Text().data();
            textBox.Text(ss.str());
        }
    }
}

// Run value entered through NumberParser
void NumberBox::EvaluateInput()
{
    if (auto textBox = m_textBox.get())
    {
        auto val = NumberBoxParser::Compute(textBox.Text());

        // No calculation could be done
        if (val == std::nullopt)
        {
            return;
        }
        if (std::isnan(val.value()))
        {
            Value(val.value());
        }

        Value(val.value());
        UpdateTextToValue();
   }
}

// Runs formatter and updates TextBox to it's value property, run on construction if Value != 0
void NumberBox::UpdateTextToValue()
{
    if (auto textBox = m_textBox.get())
    {
        auto formattedValue = m_formatter.Format(Value());
        textBox.Text(formattedValue);
    }
}

// Handler for swapping visual states of textbox
void NumberBox::SetErrorState(ValidationState state)
{
    // No Error Raised
    if (state == ValidationState::Valid || BasicValidationMode() == winrt::NumberBoxBasicValidationMode::Disabled)
    {
        m_hasError = false;
        winrt::VisualStateManager::GoToState(*this, L"Valid", false);
        return;
    }

    // Build Validation message based on error that user made
    std::wstringstream msg;
    switch (state)
    {
        case ValidationState::InvalidInput:
            if (AcceptsCalculation())
            {
                msg << "Only use numbers and ()+-*/^.";
            }
            else
            {
                msg << "Only use numbers.";
            }
            break;

        case ValidationState::InvalidMin:
            msg << "Min is " << Minimum() << ".";
            break;

        case ValidationState::InvalidMax:
            msg << "Max is " << Maximum() << ".";
            break;

        case ValidationState::InvalidDivide:
            msg << "Division by 0 unsupported.";
            break;

        case ValidationState::Invalid:
            msg << "Invalid Input.";
            break;
    }
    m_ValidationMessage = msg.str();

    if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::IconMessage)
    {
        winrt::VisualStateManager::GoToState(*this, L"InvalidIcon", false);
        m_errorToolTip.Content(msg.str);
        //tooltip.Content(box_value(ResourceAccessor::GetLocalizedStringResource(SR_TabViewAddButtonTooltip)));
    }
    else if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::TextBlockMessage)
    {
        winrt::VisualStateManager::GoToState(*this, L"InvalidText", false);
        if (auto errorTextBlock = m_errorTextBlock.get())
        {
            errorTextBlock.Text(msg.str());
        }
    }
    m_hasError = true;
}

// Enables or Disables Spin Buttons
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

// ### not sure I need ANY of this
// checks if val is in Min/Max bounds based on user's MinMax mode setting
NumberBox::BoundState NumberBox::GetBoundState(double val)
{
/*
    switch (  MinMaxMode() )
    {
        case winrt::NumberBoxMinMaxMode::None:
            return BoundState::InBounds;
        case winrt::NumberBoxMinMaxMode::WrapEnabled:
        case winrt::NumberBoxMinMaxMode::MinAndMaxEnabled:
            if (val < Minimum())
            {
                return BoundState::UnderMin;
            }
            else if (val > Maximum())
            {
                return BoundState::OverMax;
            }
            break;
        case winrt::NumberBoxMinMaxMode::MinEnabled:
            if (val < Minimum())
            {
                return BoundState::UnderMin;
            }
            break;
        case winrt::NumberBoxMinMaxMode::MaxEnabled:
            if (val > Maximum())
            {
                return BoundState::OverMax;
            }
            break;
    }*/
    return BoundState::InBounds;
}

// Builds number formatter based on properties set
void NumberBox::UpdateFormatter()
{
    m_formatter.IntegerDigits(IntegerDigits());
    m_formatter.FractionDigits(FractionDigits());
    m_formatter.SignificantDigits(SignificantDigits());
    m_formatter.IsDecimalPointAlwaysDisplayed(IsDecimalPointAlwaysDisplayed());
    m_formatter.IsZeroSigned(IsZeroSigned());
}

// Initializes NumberRounder based on properties set.
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
        m_sRounder.SignificantDigits(static_cast<int>(abs(SignificantDigitPrecision())));
        m_sRounder.RoundingAlgorithm(RoundingAlgorithm());
        m_formatter.NumberRounder(m_sRounder); 
    }
}

void NumberBox::SetHeader()
{
    /*
    winrt::hstring a = Header();
    if (Header() != L"" )
    {
        winrt::TextBlock headerbox;
        headerbox.Text(Header());
        m_TextBox.Header(headerbox); 
    } */
}

// Sets TextBox placeholder text
// ### why
void NumberBox::SetPlaceHolderText()
{
    if (auto textBox = m_textBox.get())
    {
        textBox.PlaceholderText(PlaceholderText());
    }
}



