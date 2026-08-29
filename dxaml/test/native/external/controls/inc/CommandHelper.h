// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    delegate void ExecuteDelegate(Platform::Object^ parameter);
    delegate bool CanExecuteDelegate(Platform::Object^ parameter);

    ref class MenuCommand sealed : public Microsoft::UI::Xaml::Input::ICommand
    {
        ExecuteDelegate^ _executeDelegate;

    public:
        MenuCommand(ExecuteDelegate^ executeDelegate, bool canExecuteFlag, Platform::String^ expectedParam)
        {
            _executeDelegate = executeDelegate;
            CanExecuteFlag = canExecuteFlag;
            m_expectedParam = expectedParam;
        }

        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;

        virtual void Execute(Platform::Object^ parameter)
        {
            LOG_OUTPUT(L"MenuCommand: Invoke Execute()!");
            VERIFY_IS_TRUE(m_expectedParam == parameter->ToString());

            _executeDelegate(parameter);
        }

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            return CanExecuteFlag;
        }

        property bool CanExecuteFlag;

    private:
        Platform::String^ m_expectedParam;
    };
    
    namespace CommandHelper
    {
        template <class CommandControlType>
        inline void ValidateSettingUICommandSetsProperties(
          xaml::DependencyProperty^ commandProperty,
          xaml::DependencyProperty^ labelProperty,
          xaml::DependencyProperty^ iconProperty)
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]
            {
                auto control = ref new CommandControlType();
                
                LOG_OUTPUT(L"Assigning the command to the control. We expect the control to pick up the command's properties.");
                ValidateSettingUICommandSetsProperties_SetCommand(commandProperty, control);
                ValidateSettingUICommandSetsProperties_VerifySetProperties(labelProperty, iconProperty, control);
                
                LOG_OUTPUT(L"Clearing the control's command. We expect the control to clear the properties that were set.");
                ValidateSettingUICommandSetsProperties_ClearCommand(commandProperty, control);
                ValidateSettingUICommandSetsProperties_VerifyClearedProperties(labelProperty, iconProperty, control);
            });
        }
        
        inline void ValidateSettingUICommandSetsProperties_SetCommand(
          xaml::DependencyProperty^ commandProperty,
          xaml_controls::Control^ control)
        {
            Platform::String^ labelText = ref new Platform::String(L"Label");
            xaml_controls::Symbol expectedSymbol = xaml_controls::Symbol::Favorite;
            ::Windows::System::VirtualKey expectedKey = ::Windows::System::VirtualKey::F;
            ::Windows::System::VirtualKeyModifiers expectedKeyModifiers = ::Windows::System::VirtualKeyModifiers::Control;
            xaml_input::KeyboardAccelerator^ keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = expectedKey;
            keyboardAccelerator->Modifiers = expectedKeyModifiers;
            Platform::String^ accessKeyText = ref new Platform::String(L"F");
            Platform::String^ descriptionText = ref new Platform::String(L"Description");
            
            auto symbolIconSource = ref new xaml_controls::SymbolIconSource();
            symbolIconSource->Symbol = expectedSymbol;
            
            auto uiCommand = ref new xaml_input::XamlUICommand();
            uiCommand->Label = labelText;
            uiCommand->IconSource = symbolIconSource;
            uiCommand->KeyboardAccelerators->Append(keyboardAccelerator);
            uiCommand->AccessKey = accessKeyText;
            uiCommand->Description = descriptionText;
            
            control->SetValue(commandProperty, uiCommand);
        }
        
        inline void ValidateSettingUICommandSetsProperties_VerifySetProperties(
          xaml::DependencyProperty^ labelProperty,
          xaml::DependencyProperty^ iconProperty,
          xaml_controls::Control^ control)
        {
            Platform::String^ labelText = ref new Platform::String(L"Label");
            xaml_controls::Symbol expectedSymbol = xaml_controls::Symbol::Favorite;
            ::Windows::System::VirtualKey expectedKey = ::Windows::System::VirtualKey::F;
            ::Windows::System::VirtualKeyModifiers expectedKeyModifiers = ::Windows::System::VirtualKeyModifiers::Control;
            xaml_input::KeyboardAccelerator^ keyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
            keyboardAccelerator->Key = expectedKey;
            keyboardAccelerator->Modifiers = expectedKeyModifiers;
            Platform::String^ accessKeyText = ref new Platform::String(L"F");
            Platform::String^ descriptionText = ref new Platform::String(L"Description");
            
            if (labelProperty != nullptr)
            {
                auto controlLabelText = safe_cast<Platform::String^>(control->GetValue(labelProperty));

                LOG_OUTPUT(L"Expected content text: \"%s\"", labelText->Data());
                LOG_OUTPUT(L"Actual content text: \"%s\"", controlLabelText->Data());
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(labelText, controlLabelText) == 0);
            }

            if (iconProperty != nullptr)
            {
                auto iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(control->GetValue(iconProperty));
                VERIFY_IS_NOT_NULL(iconSourceElement);

                auto symbolIconSource = safe_cast<xaml_controls::SymbolIconSource^>(iconSourceElement->IconSource);
                VERIFY_IS_NOT_NULL(symbolIconSource);
                VERIFY_ARE_EQUAL(expectedSymbol, symbolIconSource->Symbol);
            }

            VERIFY_ARE_EQUAL(1u, control->KeyboardAccelerators->Size);
            xaml_input::KeyboardAccelerator^ actualKeyboardAccelerator = control->KeyboardAccelerators->GetAt(0);

            LOG_OUTPUT(L"Expected keyboard accelerator key: %d", expectedKey);
            LOG_OUTPUT(L"Actual keyboard accelerator key: %d", actualKeyboardAccelerator->Key);
            VERIFY_ARE_EQUAL(expectedKey, actualKeyboardAccelerator->Key);
            LOG_OUTPUT(L"Expected keyboard accelerator key modifiers: %d", expectedKeyModifiers);
            LOG_OUTPUT(L"Actual keyboard accelerator key modifiers: %d", actualKeyboardAccelerator->Modifiers);
            VERIFY_ARE_EQUAL(expectedKeyModifiers, actualKeyboardAccelerator->Modifiers);

            LOG_OUTPUT(L"Expected access key: \"%s\"", accessKeyText->Data());
            LOG_OUTPUT(L"Actual access key: \"%s\"", control->AccessKey->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(accessKeyText, control->AccessKey) == 0);

            Platform::String^ helpText = xaml_automation::AutomationProperties::GetHelpText(control);
            LOG_OUTPUT(L"Expected help text: \"%s\"", descriptionText->Data());
            LOG_OUTPUT(L"Actual help text: \"%s\"", helpText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(descriptionText, helpText) == 0);

            Platform::String^ toolTipText = safe_cast<Platform::String^>(xaml_controls::ToolTipService::GetToolTip(control));
            VERIFY_IS_NOT_NULL(toolTipText);

            LOG_OUTPUT(L"Expected tool tip text: \"%s\"", descriptionText->Data());
            LOG_OUTPUT(L"Actual tool tip text: \"%s\"", toolTipText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(descriptionText, toolTipText) == 0);
        }
        
        inline void ValidateSettingUICommandSetsProperties_ClearCommand(
          xaml::DependencyProperty^ commandProperty,
          xaml_controls::Control^ control)
        {
            control->SetValue(commandProperty, nullptr);
        }
        
        inline void ValidateSettingUICommandSetsProperties_VerifyClearedProperties(
          xaml::DependencyProperty^ labelProperty,
          xaml::DependencyProperty^ iconProperty,
          xaml_controls::Control^ control)
        {
            if (labelProperty != nullptr)
            {
                auto controlLabelText = safe_cast<Platform::String^>(control->GetValue(labelProperty));

                LOG_OUTPUT(L"Expected content text: \"\"");
                LOG_OUTPUT(L"Actual content text: \"%s\"", controlLabelText->Data());
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(ref new Platform::String(L""), controlLabelText) == 0);
            }

            if (iconProperty != nullptr)
            {
                auto iconSourceElement = safe_cast<xaml_controls::IconSourceElement^>(control->GetValue(iconProperty));

                VERIFY_IS_NULL(iconSourceElement);
            }

            VERIFY_ARE_EQUAL(0u, control->KeyboardAccelerators->Size);

            LOG_OUTPUT(L"Expected access key: \"\"");
            LOG_OUTPUT(L"Actual access key: \"%s\"", control->AccessKey->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(ref new Platform::String(L""), control->AccessKey) == 0);

            Platform::String^ helpText = xaml_automation::AutomationProperties::GetHelpText(control);
            LOG_OUTPUT(L"Expected help text: \"\"");
            LOG_OUTPUT(L"Actual help text: \"%s\"", helpText->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(ref new Platform::String(L""), helpText) == 0);

            VERIFY_IS_NULL(xaml_controls::ToolTipService::GetToolTip(control));
        }
        
        template <class CommandControlType>
        inline void ValidateSettingUICommandDoesNotOverwriteProperties(
          xaml::DependencyProperty^ commandProperty,
          xaml::DependencyProperty^ labelProperty,
          xaml::DependencyProperty^ iconProperty)
        {
            TestCleanupWrapper cleanup;

            RunOnUIThread([&]
            {
                Platform::String^ controlLabelText = ref new Platform::String(L"Control label");
                Platform::String^ commandLabelText = ref new Platform::String(L"Label");
                xaml_controls::Symbol controlSymbol = xaml_controls::Symbol::OpenFile;
                xaml_controls::Symbol commandSymbol = xaml_controls::Symbol::Favorite;
                ::Windows::System::VirtualKey controlKey = ::Windows::System::VirtualKey::A;
                ::Windows::System::VirtualKeyModifiers controlKeyModifiers = ::Windows::System::VirtualKeyModifiers::Menu;
                xaml_input::KeyboardAccelerator^ controlKeyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
                controlKeyboardAccelerator->Key = controlKey;
                controlKeyboardAccelerator->Modifiers = controlKeyModifiers;
                ::Windows::System::VirtualKey commandKey = ::Windows::System::VirtualKey::F;
                ::Windows::System::VirtualKeyModifiers commandKeyModifiers = ::Windows::System::VirtualKeyModifiers::Control;
                xaml_input::KeyboardAccelerator^ commandKeyboardAccelerator = ref new xaml_input::KeyboardAccelerator();
                commandKeyboardAccelerator->Key = commandKey;
                commandKeyboardAccelerator->Modifiers = commandKeyModifiers;
                Platform::String^ controlAccessKey = ref new Platform::String(L"A");
                Platform::String^ commandAccessKey = ref new Platform::String(L"F");
                Platform::String^ controlHelpText = ref new Platform::String(L"Control help text");
                Platform::String^ controlToolTipText = ref new Platform::String(L"Control tool tip text");
                Platform::String^ commandDescriptionText = ref new Platform::String(L"Command description");
                
                auto controlSymbolIcon = ref new xaml_controls::SymbolIcon();
                controlSymbolIcon->Symbol = controlSymbol;
                auto commandSymbolIconSource = ref new xaml_controls::SymbolIconSource();
                commandSymbolIconSource->Symbol = commandSymbol;
                
                auto control = ref new CommandControlType();

                if (labelProperty != nullptr)
                {
                    control->SetValue(labelProperty, controlLabelText);
                }

                if (iconProperty != nullptr)
                {
                    control->SetValue(iconProperty, controlSymbolIcon);
                }
                
                control->KeyboardAccelerators->Append(controlKeyboardAccelerator);
                control->AccessKey = controlAccessKey;
                xaml_automation::AutomationProperties::SetHelpText(control, controlHelpText);
                xaml_controls::ToolTipService::SetToolTip(control, controlToolTipText);
                
                auto uiCommand = ref new xaml_input::XamlUICommand();
                uiCommand->Label = commandLabelText;
                uiCommand->IconSource = commandSymbolIconSource;
                uiCommand->KeyboardAccelerators->Append(commandKeyboardAccelerator);
                uiCommand->AccessKey = commandAccessKey;
                uiCommand->Description = commandDescriptionText;
                control->Command = uiCommand;

                if (labelProperty != nullptr)
                {
                    auto actualcontrolLabelText = safe_cast<Platform::String^>(control->GetValue(labelProperty));

                    LOG_OUTPUT(L"Expected content text: \"%s\"", controlLabelText->Data());
                    LOG_OUTPUT(L"Actual content text: \"%s\"", actualcontrolLabelText->Data());
                    VERIFY_IS_TRUE(Platform::String::CompareOrdinal(controlLabelText, actualcontrolLabelText) == 0);
                }

                if (iconProperty != nullptr)
                {
                    auto symbolIcon = safe_cast<xaml_controls::SymbolIcon^>(control->GetValue(iconProperty));

                    VERIFY_IS_NOT_NULL(symbolIcon);
                    VERIFY_ARE_EQUAL(controlSymbol, symbolIcon->Symbol);
                }

                VERIFY_ARE_EQUAL(1u, control->KeyboardAccelerators->Size);
                xaml_input::KeyboardAccelerator^ actualKeyboardAccelerator = control->KeyboardAccelerators->GetAt(0);

                LOG_OUTPUT(L"Expected keyboard accelerator key: %d", controlKey);
                LOG_OUTPUT(L"Actual keyboard accelerator key: %d", actualKeyboardAccelerator->Key);
                VERIFY_ARE_EQUAL(controlKey, actualKeyboardAccelerator->Key);
                LOG_OUTPUT(L"Expected keyboard accelerator key modifiers: %d", controlKeyModifiers);
                LOG_OUTPUT(L"Actual keyboard accelerator key modifiers: %d", actualKeyboardAccelerator->Modifiers);
                VERIFY_ARE_EQUAL(controlKeyModifiers, actualKeyboardAccelerator->Modifiers);

                LOG_OUTPUT(L"Expected access key: \"%s\"", controlAccessKey->Data());
                LOG_OUTPUT(L"Actual access key: \"%s\"", control->AccessKey->Data());
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(controlAccessKey, control->AccessKey) == 0);

                Platform::String^ actualHelpText = xaml_automation::AutomationProperties::GetHelpText(control);
                LOG_OUTPUT(L"Expected help text: \"%s\"", controlHelpText->Data());
                LOG_OUTPUT(L"Actual help text: \"%s\"", actualHelpText->Data());
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(controlHelpText, actualHelpText) == 0);

                Platform::String^ actualToolTipText = safe_cast<Platform::String^>(xaml_controls::ToolTipService::GetToolTip(control));
                VERIFY_IS_NOT_NULL(actualToolTipText);

                LOG_OUTPUT(L"Expected tool tip text: \"%s\"", controlToolTipText->Data());
                LOG_OUTPUT(L"Actual tool tip text: \"%s\"", actualToolTipText->Data());
                VERIFY_IS_TRUE(Platform::String::CompareOrdinal(controlToolTipText, actualToolTipText) == 0);
            });
        }
    }

} } } } }
