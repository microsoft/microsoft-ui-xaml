﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NumberBox.h"
#include "NumberBoxAutomationPeer.h"
#include "NumberBoxParser.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "winnls.h"

static constexpr wstring_view c_numberBoxHeaderName{ L"HeaderContentPresenter"sv };
static constexpr wstring_view c_numberBoxDownButtonName{ L"DownSpinButton"sv };
static constexpr wstring_view c_numberBoxUpButtonName{ L"UpSpinButton"sv };
static constexpr wstring_view c_numberBoxTextBoxName{ L"InputBox"sv };
static constexpr wstring_view c_numberBoxPopupName{ L"UpDownPopup"sv };
static constexpr wstring_view c_numberBoxPopupDownButtonName{ L"PopupDownSpinButton"sv };
static constexpr wstring_view c_numberBoxPopupUpButtonName{ L"PopupUpSpinButton"sv };
static constexpr wstring_view c_numberBoxPopupContentRootName{ L"PopupContentRoot"sv };

static constexpr double c_popupShadowDepth = 16.0;
static constexpr wstring_view c_numberBoxPopupShadowDepthName{ L"NumberBoxPopupShadowDepth"sv };

// Shockingly, there is no standard function for trimming strings.
const std::wstring c_whitespace = L" \n\r\t\f\v";
std::wstring trim(const std::wstring& s)
{
    const size_t start = s.find_first_not_of(c_whitespace);
    const size_t end = s.find_last_not_of(c_whitespace);
    return (start == std::wstring::npos || end == std::wstring::npos) ? L"" : s.substr(start, end - start + 1);
}

NumberBox::NumberBox()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_NumberBox);

    Loaded({ this, &NumberBox::OnLoaded });

    NumberFormatter(GetRegionalSettingsAwareDecimalFormatter());

    PointerWheelChanged({ this, &NumberBox::OnNumberBoxScroll });

    GotFocus({ this, &NumberBox::OnNumberBoxGotFocus });
    LostFocus({ this, &NumberBox::OnNumberBoxLostFocus });

    SetDefaultStyleKey(this);
    SetDefaultInputScope();

    // We are not revoking this since the event and the listener reside on the same object and as such have the same lifecycle.
    // That means that as soon as the NumberBox gets removed so will the event and the listener.
    this->RegisterPropertyChangedCallback(winrt::AutomationProperties::NameProperty(), { this , &NumberBox::OnAutomationPropertiesNamePropertyChanged });
}

void NumberBox::SetDefaultInputScope()
{
    // Sets the default value of the InputScope property.
    // Note that InputScope is a class that cannot be set to a default value within the IDL.
    const auto inputScopeName = winrt::InputScopeName(winrt::InputScopeNameValue::Number);
    const auto inputScope = winrt::InputScope();
    inputScope.Names().Append(inputScopeName);

    static_cast<NumberBox*>(this)->SetValue(s_InputScopeProperty, ValueHelper<winrt::InputScope>::BoxValueIfNecessary(inputScope));

    return;
}

// This was largely copied from Calculator's GetRegionalSettingsAwareDecimalFormatter()
winrt::DecimalFormatter NumberBox::GetRegionalSettingsAwareDecimalFormatter()
{
    winrt::DecimalFormatter formatter = nullptr;

    WCHAR currentLocale[LOCALE_NAME_MAX_LENGTH] = {};
    if (GetUserDefaultLocaleName(currentLocale, LOCALE_NAME_MAX_LENGTH) != 0)
    {
        // GetUserDefaultLocaleName may return an invalid bcp47 language tag with trailing non-BCP47 friendly characters,
        // which if present would start with an underscore, for example sort order
        // (see https://msdn.microsoft.com/en-us/library/windows/desktop/dd373814(v=vs.85).aspx).
        // Therefore, if there is an underscore in the locale name, trim all characters from the underscore onwards.
        WCHAR* underscore = wcschr(currentLocale, L'_');
        if (underscore != nullptr)
        {
            *underscore = L'\0';
        }

        if (winrt::Language::IsWellFormed(currentLocale))
        {
            std::vector<winrt::hstring> languageList;
            languageList.push_back(winrt::hstring(currentLocale));
            formatter = winrt::DecimalFormatter(languageList, winrt::GlobalizationPreferences::HomeGeographicRegion());
        }
    }

    if (!formatter)
    {
        formatter = winrt::DecimalFormatter();
    }

    formatter.IntegerDigits(1);
    formatter.FractionDigits(0);

    return formatter;
}

void NumberBox::Value(double value)
{
    // When using two way bindings to Value using x:Bind, we could end up with a stack overflow because
    // nan != nan. However in this case, we are using nan as a value to represent value not set (cleared)
    // and that can happen quite often. We can avoid the stack overflow by breaking the cycle here. This is possible
    // for x:Bind since the generated code goes through this property setter. This is not the case for Binding
    // unfortunately. x:Bind is recommended over Binding anyway due to its perf and debuggability benefits.
    if (!std::isnan(value) || !std::isnan(Value()))
    {
        static_cast<NumberBox*>(this)->SetValue(s_ValueProperty, ValueHelper<double>::BoxValueIfNecessary(value));
    }
}

double NumberBox::Value()
{
    return ValueHelper<double>::CastOrUnbox(static_cast<NumberBox*>(this)->GetValue(s_ValueProperty));
}

winrt::AutomationPeer NumberBox::OnCreateAutomationPeer()
{
    return winrt::make<NumberBoxAutomationPeer>(*this);
}

void NumberBox::OnApplyTemplate()
{
    const winrt::IControlProtected controlProtected = *this;

    const auto spinDownName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxDownSpinButtonName);
    const auto spinUpName = ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxUpSpinButtonName);

    if (const auto spinDown = GetTemplateChildT<winrt::RepeatButton>(c_numberBoxDownButtonName, controlProtected))
    {
        m_downButtonClickRevoker = spinDown.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinDownClick });

        // Do localization for the down button
        if (winrt::AutomationProperties::GetName(spinDown).empty())
        {
            winrt::AutomationProperties::SetName(spinDown, spinDownName);
        }
    }

    if (const auto spinUp = GetTemplateChildT<winrt::RepeatButton>(c_numberBoxUpButtonName, controlProtected))
    {
        m_upButtonClickRevoker = spinUp.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinUpClick });

        // Do localization for the up button
        if (winrt::AutomationProperties::GetName(spinUp).empty())
        {
            winrt::AutomationProperties::SetName(spinUp, spinUpName);
        }
    }

    UpdateHeaderPresenterState();

    m_textBox.set([this, controlProtected]() {
        const auto textBox = GetTemplateChildT<winrt::TextBox>(c_numberBoxTextBoxName, controlProtected);
        if (textBox)
        {
            if (SharedHelpers::IsRS3OrHigher())
            {
                // Listen to PreviewKeyDown because textbox eats the down arrow key in some circumstances.
                m_textBoxPreviewKeyDownRevoker = textBox.PreviewKeyDown(winrt::auto_revoke, { this, &NumberBox::OnNumberBoxKeyDown });
            }
            else
            {
                // This is better than nothing.
                m_textBoxKeyDownRevoker = textBox.KeyDown(winrt::auto_revoke, { this, &NumberBox::OnNumberBoxKeyDown });
            }

            m_textBoxKeyUpRevoker = textBox.KeyUp(winrt::auto_revoke, { this, &NumberBox::OnNumberBoxKeyUp });
        }
        return textBox;
    }());

    m_popup.set(GetTemplateChildT<winrt::Popup>(c_numberBoxPopupName, controlProtected));

    if (SharedHelpers::IsThemeShadowAvailable())
    {
        if (const auto popupRoot = GetTemplateChildT<winrt::UIElement>(c_numberBoxPopupContentRootName, controlProtected))
        {
            if (!popupRoot.Shadow())
            {
                popupRoot.Shadow(winrt::ThemeShadow{});
                const auto translation = popupRoot.Translation();

                const double shadowDepth = unbox_value<double>(SharedHelpers::FindInApplicationResources(c_numberBoxPopupShadowDepthName, box_value(c_popupShadowDepth)));

                popupRoot.Translation({ translation.x, translation.y, (float)shadowDepth });
            }
        }
    }

    if (const auto popupSpinDown = GetTemplateChildT<winrt::RepeatButton>(c_numberBoxPopupDownButtonName, controlProtected))
    {
        m_popupDownButtonClickRevoker = popupSpinDown.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinDownClick });
    }

    if (const auto popupSpinUp = GetTemplateChildT<winrt::RepeatButton>(c_numberBoxPopupUpButtonName, controlProtected))
    {
        m_popupUpButtonClickRevoker = popupSpinUp.Click(winrt::auto_revoke, { this, &NumberBox::OnSpinUpClick });
    }

    m_isEnabledChangedRevoker = IsEnabledChanged(winrt::auto_revoke, { this,  &NumberBox::OnIsEnabledChanged });

    // .NET rounds to 12 significant digits when displaying doubles, so we will do the same.
    m_displayRounder.SignificantDigits(12);

    UpdateSpinButtonPlacement();
    UpdateSpinButtonEnabled();

    UpdateVisualStateForIsEnabledChange();

    ReevaluateForwardedUIAName();

    if (ReadLocalValue(s_ValueProperty) == winrt::DependencyProperty::UnsetValue()
        && ReadLocalValue(s_TextProperty) != winrt::DependencyProperty::UnsetValue())
    {
        // If Text has been set, but Value hasn't, update Value based on Text.
        UpdateValueToText();
    }
    else
    {
        UpdateTextToValue();
    }
}

void NumberBox::OnLoaded(winrt::IInspectable const&, winrt::RoutedEventArgs const&)
{
    // This is done OnLoaded so TextBox VisualStates can be updated properly.
    UpdateSpinButtonPlacement();
}

void NumberBox::OnValuePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    // This handler may change Value; don't send extra events in that case.
    if (!m_valueUpdating)
    {
        const auto oldValue = unbox_value<double>(args.OldValue());

        auto scopeGuard = gsl::finally([this]()
        {
            m_valueUpdating = false;
        });
        m_valueUpdating = true;

        CoerceValue();

        const auto newValue = Value();
        if (newValue != oldValue && !(std::isnan(newValue) && std::isnan(oldValue)))
        {
            // Fire ValueChanged event
            const auto valueChangedArgs = winrt::make_self<NumberBoxValueChangedEventArgs>(oldValue, newValue);
            m_valueChangedEventSource(*this, *valueChangedArgs);

            // Fire value property change for UIA
            if (const auto peer = winrt::FrameworkElementAutomationPeer::FromElement(*this).as<winrt::NumberBoxAutomationPeer>())
            {
                winrt::get_self<NumberBoxAutomationPeer>(peer)->RaiseValueChangedEvent(oldValue, newValue);
            }
        }

        UpdateTextToValue();
        UpdateSpinButtonEnabled();
    }
}

void NumberBox::OnMinimumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    CoerceMaximum();
    CoerceValue();

    UpdateSpinButtonEnabled();
    ReevaluateForwardedUIAName();
}

void NumberBox::OnMaximumPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    CoerceMinimum();
    CoerceValue();

    UpdateSpinButtonEnabled();
    ReevaluateForwardedUIAName();
}

void NumberBox::OnSmallChangePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSpinButtonEnabled();
}

void NumberBox::OnIsWrapEnabledPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateSpinButtonEnabled();
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
    UpdateSpinButtonPlacement();
}

void NumberBox::OnTextPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (!m_textUpdating)
    {
        UpdateValueToText();
    }
}

void NumberBox::UpdateValueToText()
{
    if (auto && textBox = m_textBox.get())
    {
        textBox.Text(Text());
        ValidateInput();
    }
}

void NumberBox::OnHeaderPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateHeaderPresenterState();
}

void NumberBox::OnHeaderTemplatePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    UpdateHeaderPresenterState();
}

void NumberBox::OnValidationModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    ValidateInput();
    UpdateSpinButtonEnabled();
}

void NumberBox::OnIsEnabledChanged(const winrt::IInspectable&, const winrt::DependencyPropertyChangedEventArgs&)
{
    UpdateVisualStateForIsEnabledChange();
}

void NumberBox::OnAutomationPropertiesNamePropertyChanged(const winrt::DependencyObject&, const winrt::DependencyProperty&)
{
    ReevaluateForwardedUIAName();
}

void NumberBox::ReevaluateForwardedUIAName()
{
    if (const auto textBox = m_textBox.get())
    {
        const auto name = winrt::AutomationProperties::GetName(*this);
        const auto minimum = Minimum() == -std::numeric_limits<double>::max() ?
            winrt::hstring{} :
            winrt::hstring{ L" " + ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxMinimumValueStatus) + winrt::to_hstring(Minimum()) };
        const auto maximum = Maximum() == std::numeric_limits<double>::max() ?
            winrt::hstring{} :
            winrt::hstring{ L" " + ResourceAccessor::GetLocalizedStringResource(SR_NumberBoxMaximumValueStatus) + winrt::to_hstring(Maximum()) };

        if (!name.empty())
        {
            // AutomationProperties.Name is a non empty string, we will use that value.
            winrt::AutomationProperties::SetName(textBox, name + minimum + maximum );
        }
        else
        {
            if (const auto headerAsString = Header().try_as<winrt::IReference<winrt::hstring>>())
            {
                // Header is a string, we can use that as our UIA name.
                winrt::AutomationProperties::SetName(textBox, headerAsString.Value() + minimum + maximum );
            }
        }
    }
}

void NumberBox::UpdateVisualStateForIsEnabledChange()
{
    winrt::VisualStateManager::GoToState(*this, IsEnabled() ? L"Normal" : L"Disabled", false);
}

void NumberBox::OnNumberBoxGotFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    // When the control receives focus, select the text
    if (auto && textBox = m_textBox.get())
    {
        textBox.SelectAll();
    }

    if (SpinButtonPlacementMode() == winrt::NumberBoxSpinButtonPlacementMode::Compact)
    {
        if (auto && popup = m_popup.get())
        {
            popup.IsOpen(true);
        }
    }
}

void NumberBox::OnNumberBoxLostFocus(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    ValidateInput();

    if (auto && popup = m_popup.get())
    {
        popup.IsOpen(false);
    }
}

void NumberBox::CoerceMinimum()
{
    const auto max = Maximum();
    if (Minimum() > max)
    {
        Minimum(max);
    }
}

void NumberBox::CoerceMaximum()
{
    const auto min = Minimum();
    if (Maximum() < min)
    {
        Maximum(min);
    }
}

void NumberBox::CoerceValue()
{
    // Validate that the value is in bounds
    const auto value = Value();
    if (!std::isnan(value) && !IsInBounds(value) && ValidationMode() == winrt::NumberBoxValidationMode::InvalidInputOverwritten)
    {
        // Coerce value to be within range
        const auto max = Maximum();
        if (value > max)
        {
            Value(max);
        }
        else
        {
            Value(Minimum());
        }
    }
}

void NumberBox::ValidateInput()
{
    // Validate the content of the inner textbox
    if (auto&& textBox = m_textBox.get())
    {
        const auto text = trim(textBox.Text().data());

        // Handles empty TextBox case, set text to current value
        if (text.empty())
        {
            Value(std::numeric_limits<double>::quiet_NaN());
        }
        else
        {
            // Setting NumberFormatter to something that isn't an INumberParser will throw an exception, so this should be safe
            const auto numberParser = NumberFormatter().as<winrt::INumberParser>();

            const winrt::IReference<double> value = AcceptsExpression()
                ? NumberBoxParser::Compute(text, numberParser)
                : numberParser.ParseDouble(text);

            if (!value)
            {
                if (ValidationMode() == winrt::NumberBoxValidationMode::InvalidInputOverwritten)
                {
                    // Override text to current value
                    UpdateTextToValue();
                }
            }
            else
            {
                if (value.Value() == Value())
                {
                    // Even if the value hasn't changed, we still want to update the text (e.g. Value is 3, user types 1 + 2, we want to replace the text with 3)
                    UpdateTextToValue();
                }
                else
                {
                    Value(value.Value());
                }
            }
        }
    }
}

void NumberBox::OnSpinDownClick(winrt::IInspectable const&  sender, winrt::RoutedEventArgs const& args)
{
    StepValue(-SmallChange());
}

void NumberBox::OnSpinUpClick(winrt::IInspectable const& sender, winrt::RoutedEventArgs const& args)
{
    StepValue(SmallChange());
}

void NumberBox::OnNumberBoxKeyDown(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    // Handle these on key down so that we get repeat behavior.
    switch (args.OriginalKey())
    {
        case winrt::VirtualKey::Up:
            StepValue(SmallChange());
            args.Handled(true); 
            break;

        case winrt::VirtualKey::Down:
            StepValue(-SmallChange());
            args.Handled(true);
            break;

        case winrt::VirtualKey::PageUp:
            StepValue(LargeChange());
            args.Handled(true);
            break;

        case winrt::VirtualKey::PageDown:
            StepValue(-LargeChange());
            args.Handled(true);
            break;
    }
}

void NumberBox::OnNumberBoxKeyUp(winrt::IInspectable const& sender, winrt::KeyRoutedEventArgs const& args)
{
    switch (args.OriginalKey())
    {
    case winrt::VirtualKey::Enter:
    case winrt::VirtualKey::GamepadA:
        ValidateInput();
        args.Handled(true);
        break;

    case winrt::VirtualKey::Escape:
    case winrt::VirtualKey::GamepadB:
        UpdateTextToValue();
        args.Handled(true);
        break;
    }
}

void NumberBox::OnNumberBoxScroll(winrt::IInspectable const& sender, winrt::PointerRoutedEventArgs const& args)
{
    if (auto && textBox = m_textBox.get())
    {
        if (textBox.FocusState() != winrt::FocusState::Unfocused)
        {
            const auto delta = args.GetCurrentPoint(*this).Properties().MouseWheelDelta();
            if (delta > 0)
            {
                StepValue(SmallChange());
            }
            else if (delta < 0)
            {
                StepValue(-SmallChange());
            }
            // Only set as handled when we actually changed our state.
            args.Handled(true);
        }
    }
}

void NumberBox::StepValue(double change)
{
    // Before adjusting the value, validate the contents of the textbox so we don't override it.
    ValidateInput();

    auto newVal = Value();
    if (!std::isnan(newVal))
    {
        newVal += change;

        if (IsWrapEnabled())
        {
            const auto max = Maximum();
            const auto min = Minimum();

            if (newVal > max)
            {
                newVal = min;
            }
            else if (newVal < min)
            {
                newVal = max;
            }
        }

        Value(newVal);

        // We don't want the caret to move to the front of the text for example when using the up/down arrows
        // to change the numberbox value.
        MoveCaretToTextEnd();
    }
}

// Updates TextBox.Text with the formatted Value
void NumberBox::UpdateTextToValue()
{
    if (auto && textBox = m_textBox.get())
    {
        winrt::hstring newText = L"";

        const auto value = Value();
        if (!std::isnan(value))
        {
            // Rounding the value here will prevent displaying digits caused by floating point imprecision.
            const auto roundedValue = m_displayRounder.RoundDouble(value);
            newText = NumberFormatter().FormatDouble(roundedValue);
        }

        textBox.Text(newText);

        auto scopeGuard = gsl::finally([this]()
        {
            m_textUpdating = false;
        });
        m_textUpdating = true;
        Text(newText.data());
    }
}

void NumberBox::UpdateSpinButtonPlacement()
{
    const auto spinButtonMode = SpinButtonPlacementMode();
    auto state = L"SpinButtonsCollapsed";

    if (spinButtonMode == winrt::NumberBoxSpinButtonPlacementMode::Inline)
    {
        state = L"SpinButtonsVisible";
    }
    else if (spinButtonMode == winrt::NumberBoxSpinButtonPlacementMode::Compact)
    {
        state = L"SpinButtonsPopup";
    }

    winrt::VisualStateManager::GoToState(*this, state, false);
    if (const auto textbox = m_textBox.get())
    {
        winrt::VisualStateManager::GoToState(textbox, state, false);
    }
}

void NumberBox::UpdateSpinButtonEnabled()
{
    const auto value = Value();
    bool isUpButtonEnabled = false;
    bool isDownButtonEnabled = false;

    if (!std::isnan(value))
    {
        if (IsWrapEnabled() || ValidationMode() != winrt::NumberBoxValidationMode::InvalidInputOverwritten)
        {
            // If wrapping is enabled, or invalid values are allowed, then the buttons should be enabled
            isUpButtonEnabled = true;
            isDownButtonEnabled = true;
        }
        else
        {
            if (value < Maximum())
            {
                isUpButtonEnabled = true;
            }
            if (value > Minimum())
            {
                isDownButtonEnabled = true;
            }
        }
    }

    winrt::VisualStateManager::GoToState(*this, isUpButtonEnabled ? L"UpSpinButtonEnabled" : L"UpSpinButtonDisabled", false);
    winrt::VisualStateManager::GoToState(*this, isDownButtonEnabled ? L"DownSpinButtonEnabled" : L"DownSpinButtonDisabled", false);
}

bool NumberBox::IsInBounds(double value)
{
    return (value >= Minimum() && value <= Maximum());
}

void NumberBox::UpdateHeaderPresenterState()
{
    bool shouldShowHeader = false;

    // Load header presenter as late as possible

    // To enable lightweight styling, collapse header presenter if there is no header specified
    if (const auto header = Header())
    {
        // Check if header is string or not
        if (const auto headerAsString = header.try_as<winrt::IReference<winrt::hstring>>())
        {
            if (!headerAsString.Value().empty())
            {
                // Header is not empty string
                shouldShowHeader = true;
            }
        }
        else
        {
            // Header is not a string, so let's show header presenter
            shouldShowHeader = true;
            // When our header isn't a string, we use the NumberBox's UIA name for the textbox's UIA name.
            if (const auto textBox = m_textBox.get())
            {
                winrt::AutomationProperties::SetName(textBox, winrt::AutomationProperties::GetName(*this));
            }
        }
    }
    if(const auto headerTemplate = HeaderTemplate())
    {
        shouldShowHeader = true;
    }

    if(shouldShowHeader && m_headerPresenter == nullptr)
    {
        if (const auto headerPresenter = GetTemplateChildT<winrt::ContentPresenter>(c_numberBoxHeaderName, (winrt::IControlProtected)*this))
        {
            // Set presenter to enable lightweight styling of the headers margin
            m_headerPresenter.set(headerPresenter);
        }
    }

    if (auto&& headerPresenter = m_headerPresenter.get())
    {
        headerPresenter.Visibility(shouldShowHeader ? winrt::Visibility::Visible : winrt::Visibility::Collapsed);
    }

    ReevaluateForwardedUIAName();
}

void NumberBox::MoveCaretToTextEnd()
{
    if (auto && textBox = m_textBox.get())
    {
        // This places the caret at the end of the text.
        textBox.Select(static_cast<int32_t>(textBox.Text().size()), 0);
    }
}
