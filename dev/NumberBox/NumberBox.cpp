// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "NumberBoxAutomationPeer.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Utils.h"

NumberBox::NumberBox()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NumberBox);

    // Default values for the number formatter
    auto formatter = winrt::DecimalFormatter();
    formatter.IntegerDigits(1);
    formatter.FractionDigits(0);
    NumberFormatter(formatter);

    SetDefaultStyleKey(this);
}

winrt::AutomationPeer NumberBox::OnCreateAutomationPeer()
{
    return winrt::make<NumberBoxAutomationPeer>(*this);
}

void NumberBox::OnApplyTemplate()
{
    // Initializations - Visual Components
    winrt::IControlProtected controlProtected = *this;

    m_errorTextBlock.set(GetTemplateChildT<winrt::TextBlock>(L"ErrorTextMessage", controlProtected));

    // Initializations - Visual States
    SetSpinButtonVisualState();
    
    // Initializations - Interactions
    if (auto warningIcon = GetTemplateChildT<winrt::FontIcon>(L"ValidationIcon", controlProtected))
    {
        winrt::ToolTipService::SetToolTip(warningIcon, m_errorToolTip);
    }

    if (auto spinDown = GetTemplateChildT<winrt::RepeatButton>(L"DownSpinButton", controlProtected))
    {
        spinDown.Click({ this, &NumberBox::OnSpinDownClick });

        // Do localization for the down button
        if (winrt::AutomationProperties::GetName(spinDown).empty())
        {
            auto spinDownName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxDownSpinButtonName);
            winrt::AutomationProperties::SetName(spinDown, spinDownName);
        }
    }

    if (auto spinUp = GetTemplateChildT<winrt::RepeatButton>(L"UpSpinButton", controlProtected))
    {
        spinUp.Click({ this, &NumberBox::OnSpinUpClick });

        // Do localization for the up button
        if (winrt::AutomationProperties::GetName(spinUp).empty())
        {
            auto spinUpName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxUpSpinButtonName);
            winrt::AutomationProperties::SetName(spinUp, spinUpName);
        }
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

    // Initializing precision formatter. This formatter works neutrally to protect against floating point imprecision resulting from stepping/calc
    m_stepPrecisionFormatter.FractionDigits(0);
    m_stepPrecisionFormatter.IntegerDigits(1);
    m_stepPrecisionFormatter.NumberRounder(nullptr);
    m_stepPrecisionRounder.RoundingAlgorithm(winrt::RoundingAlgorithm::RoundHalfAwayFromZero);

    // Set Text to reflect preset Value
    UpdateTextToValue();
}

void NumberBox::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateValue();

    auto oldValue = unbox_value<double>(args.OldValue());
    if (Value() != oldValue)
    {
        // Fire ValueChanged event
        auto valueChangedArgs = winrt::make_self<NumberBoxValueChangedEventArgs>(oldValue, Value());
        m_valueChangedEventSource(*this, *valueChangedArgs);

        // Fire value property change for UIA
        if (auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).as<winrt::NumberBoxAutomationPeer>())
        {
            winrt::get_self<NumberBoxAutomationPeer>(peer)->RaiseValueChangedEvent(oldValue, Value());
        }
    }

    UpdateTextToValue();
}

void NumberBox::OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateValue();
}

void NumberBox::OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateValue();
}

void NumberBox::OnNumberFormatterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // Update text with new formatting
    UpdateTextToValue();
}

void NumberBox::ValidateNumberFormatter(winrt::INumberFormatter2 value)
{
    // NumberFormatter also needs to be an INumberParser
    if (!value.try_as<winrt::INumberParser>())
    {
        throw winrt::hresult_error(E_INVALIDARG);
    }
}

void NumberBox::OnSpinButtonPlacementModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    SetSpinButtonVisualState();
}

void NumberBox::OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto textBox = m_textBox.get())
    {
        textBox.Text(Text());
    }
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

void NumberBox::ValidateValue()
{
    // Parsable value that is not in bounds
    auto value = Value();
    if (!IsInBounds(value))
    {
        if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
        {
            // Coerse value to be within range
            if (value > Maximum())
            {
                Value(Maximum());
            }
            else if (value < Minimum())
            {
                Value(Minimum());
            }
        }
        else
        {
            SetErrorState(ValidationState::InvalidRange);
        }
    }
    else
    {
        SetErrorState(ValidationState::Valid);
    }
}

// Performs all validation steps on input given in textbox. Runs on LoseFocus and stepping.
void NumberBox::ValidateInput()
{
    SetErrorState(ValidationState::Valid);
    if (auto textBox = m_textBox.get())
    {
        auto text = textBox.Text();
        
        // Handles Empty TextBox Case, current behavior is to set Value to default (0)
        if (text.empty())
        {
            Value(0);
            m_hasError = false;
            return;
        }

        if (AcceptsCalculation() && IsFormulaic(text))
        {
            NormalizeShorthandOperations();
            EvaluateInputCalculation();

            // Divide by 0 error state
            if (fpclassify(Value()) == FP_NAN)
            {
                SetErrorState(ValidationState::InvalidDivide);
                return;
            }
        }

        // Setting NumberFormatter to something that isn't an INumberParser will throw an exception, so this should be safe
        auto numberParser = NumberFormatter().as<winrt::INumberParser>();
        auto parsedNum = numberParser.ParseDouble(text);

        if (!parsedNum)
        {
            if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::InvalidInputOverwritten)
            {
                // Override value to last valid value
                UpdateTextToValue();
            }
            else
            {
                SetErrorState(ValidationState::InvalidInput);
            }
        }
        else
        {
            Value(parsedNum.Value());
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
        newVal += Step();
    }
    else
    {
        newVal -= Step();
    }

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

    // Safeguard for floating point imprecision errors
    int StepFreqSigDigits = ComputePrecisionRounderSigDigits(newVal);
    m_stepPrecisionRounder.SignificantDigits(StepFreqSigDigits);
    newVal = m_stepPrecisionRounder.RoundDouble(newVal);

    // Update Text and Revalidate new value
    Value(newVal);
}


// Check if text resembles formulaic input to determine if parser should be executed
bool NumberBox::IsFormulaic(const winrt::hstring& in)
{
    std::wstring input(in);
    std::wregex formula(L"^([0-9()\\s]*[+-/*^%]+[0-9()\\s]*)+$");
    std::wregex negval(L"^-([0-9])+(.([0-9])+)?$");
    return (std::regex_match(input, formula) && !std::regex_match(input, negval));
}

// Computes the number of significant digits that precision rounder should use. This helps to prevent floating point imprecision errors. 
int NumberBox::ComputePrecisionRounderSigDigits(double newVal)
{
    auto const oldVal = Value();

    // Run formatter on both values to discard trailing and leading 0's.
    std::wstring formattedVal(m_stepPrecisionFormatter.Format(oldVal));
    std::wstring formattedStep(m_stepPrecisionFormatter.Format(Step()));
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
void NumberBox::EvaluateInputCalculation()
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
        auto formattedValue = NumberFormatter().FormatDouble(Value());
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
    winrt::hstring errorMessage;
    switch (state)
    {
        case ValidationState::InvalidInput:
        {
            if (AcceptsCalculation())
            {
                errorMessage = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxErrorNumbersAndSymbols);
            }
            else
            {
                errorMessage = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxErrorNumbersOnly);
            }
            break;
        }

        case ValidationState::InvalidRange:
        {
            auto formatter = NumberFormatter();
            auto formattedMin = formatter.FormatDouble(Minimum());
            auto formattedMax = formatter.FormatDouble(Maximum());
            errorMessage = StringUtil::FormatString(
                ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxErrorRange),
                formattedMin.data(),
                formattedMax.data());
            break;
        }

        case ValidationState::InvalidDivide:
            errorMessage = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxErrorDivByZero);
            break;

        case ValidationState::Invalid:
            errorMessage = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxErrorInvalidInput);
            break;
    }

    if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::IconMessage)
    {
        winrt::VisualStateManager::GoToState(*this, L"InvalidIcon", false);
        m_errorToolTip.Content(box_value(errorMessage));
    }
    else if (BasicValidationMode() == winrt::NumberBoxBasicValidationMode::TextBlockMessage)
    {
        winrt::VisualStateManager::GoToState(*this, L"InvalidText", false);
        if (auto errorTextBlock = m_errorTextBlock.get())
        {
            errorTextBlock.Text(errorMessage);
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

bool NumberBox::IsInBounds(double value)
{
    return (value >= Minimum() && value <= Maximum());
}

