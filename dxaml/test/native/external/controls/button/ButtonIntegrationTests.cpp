// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ButtonIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>
#include <generic\ButtonBaseTests.h>

#include <XamlTailored.h>
#include <CommandHelper.h>
#include <ControlHelper.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Button {


    // Helper class to verify that ICommands can be fired on a tap/click
    // Fires the event Executed when ICOmmand::Execute() runs
    ref class CommandTestHelper : public Microsoft::UI::Xaml::Input::ICommand {
    public:
        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;
        virtual event wf::EventHandler<Object^>^ CanExecuteCalled;
        virtual event wf::EventHandler<Object^>^ Executed;

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            CanExecuteCalled(this, nullptr);
            if (parameter && dynamic_cast<Platform::String^>(parameter) == ActivatingCommandParameter)
            {
                return true;
            }
            return false;
        }

        virtual void Execute(Platform::Object^ parameter)
        {
            Executed(this, nullptr);
        }

        // To test the CommandParameter functionality, I will cache a 'magic' string that when matched will cause CanExecute to return true.
        // This also allows the Execute event to fire.
        property Platform::String ^ ActivatingCommandParameter;
    };

    // Helper class to verify changes to CanExecute without needing to set CommandParameter -
    // setting Button.CommandParameter bypasses CanExecuteChanged since the button updates on the property change instead.
    ref class CommandTestHelperNoParameter sealed : public Microsoft::UI::Xaml::Input::ICommand {
    public:
        virtual event wf::EventHandler<Object^>^ CanExecuteChanged;
        virtual event wf::EventHandler<Object^>^ CanExecuteCalled;
        virtual event wf::EventHandler<Object^>^ Executed;

        CommandTestHelperNoParameter()
        {
            canExecute = false;
        }

        virtual bool CanExecute(Platform::Object^ parameter)
        {
            CanExecuteCalled(this, nullptr);
            return canExecute;
        }

        virtual void Execute(Platform::Object^ parameter)
        {
            Executed(this, nullptr);
        }

        void SetCanExecute(bool canExecute)
        {
            if (this->canExecute != canExecute)
            {
                this->canExecute = canExecute;
                CanExecuteChanged(this, nullptr);
            }
        }

    private:
        bool canExecute;
    };

    bool ButtonIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ButtonIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ButtonIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ButtonIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::Button>::CanInstantiate();
    }

    void ButtonIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::Button>::CanEnterAndLeaveLiveTree();
    }

    void ButtonIntegrationTests::CanClickUsingTap()
    {
        Generic::ButtonBaseTests<xaml_controls::Button>::CanClickUsingTap();
    }

    void ButtonIntegrationTests::CanFireRightTappedEventUsingHold()
    {
        Generic::ButtonBaseTests<xaml_controls::Button>::CanFireRightTappedEventUsingHold();
    }

    void ButtonIntegrationTests::CanRecoverKeyboardInvocationAfterPointerCancellation()
    {
        // Test for https://github.com/microsoft/microsoft-ui-xaml/issues/11148
        Generic::ButtonBaseTests<xaml_primitives::ToggleButton>::CanRecoverKeyboardInvocationAfterPointerCancellation();
    }

    void ButtonIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            [] () {
                xaml_controls::Button^ restButton = nullptr;
                xaml_controls::Button^ pointerOverButton = nullptr;
                xaml_controls::Button^ pressedButton = nullptr;
                xaml_controls::Button^ disabledButton = nullptr;

                xaml_controls::Button^ focusedRestButton = nullptr;
                xaml_controls::Button^ focusedpointerOverButton = nullptr;
                xaml_controls::Button^ focusedPressedButton = nullptr;

                xaml_controls::StackPanel^ rootPanel = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();

                    restButton = ref new xaml_controls::Button();
                    restButton->Content = "Button";
                    rootPanel->Children->Append(restButton);

                    pointerOverButton = ref new xaml_controls::Button();
                    pointerOverButton->Content = "PointerOver Button";
                    rootPanel->Children->Append(pointerOverButton);

                    pressedButton = ref new xaml_controls::Button();
                    pressedButton->Content = "Pressed Button";
                    rootPanel->Children->Append(pressedButton);

                    disabledButton = ref new xaml_controls::Button();
                    disabledButton->Content = "Disabled Button";
                    disabledButton->IsEnabled = false;
                    rootPanel->Children->Append(disabledButton);

                    focusedRestButton = ref new xaml_controls::Button();
                    focusedRestButton->Content = "Focused Rest Button";
                    rootPanel->Children->Append(focusedRestButton);

                    focusedpointerOverButton = ref new xaml_controls::Button();
                    focusedpointerOverButton->Content = "Focused PointerOver Button";
                    rootPanel->Children->Append(focusedpointerOverButton);

                    focusedPressedButton = ref new xaml_controls::Button();
                    focusedPressedButton->Content = "Focused Pressed Button";
                    rootPanel->Children->Append(focusedPressedButton);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });

                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(pointerOverButton, "PointerOver", false);
                    VisualStateManager::GoToState(pressedButton, "Pressed", false);
                    VisualStateManager::GoToState(focusedRestButton, "Focused", false);
                    VisualStateManager::GoToState(focusedpointerOverButton, "Focused", false);
                    VisualStateManager::GoToState(focusedpointerOverButton, "PointerOver", false);
                    VisualStateManager::GoToState(focusedPressedButton, "Focused", false);
                    VisualStateManager::GoToState(focusedPressedButton, "Pressed", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            }
        );
    }

    void ButtonIntegrationTests::VerifyStatesForMouseInteraction()
    {
        TestCleanupWrapper cleanup([]()
        {
            TestServices::InputHelper->MoveMouse(wf::Point(0,0));
            TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
        });

        // Interact with a Button with the mouse and make sure that it goes to the correct VSM states.
        // We also verify the values of the IsPointerOver, IsPressed and FocusState properties.

        xaml_controls::Button^ dummyButton = nullptr;
        xaml_controls::Button^ testButton = nullptr;
        xaml::VisualStateGroup^ buttonVisualStateGroup = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                    <Button x:Name="dummyButton" Content="Button 1" />
                    <Button x:Name="testButton" Content="Test Button" />
                </StackPanel>)"));

            dummyButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"dummyButton"));
            testButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"testButton"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto templateRoot = dynamic_cast<xaml::FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(testButton, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
            VERIFY_IS_TRUE(groups->Size == 1);
            buttonVisualStateGroup = groups->GetAt(0);
        });

        // Move the mouse to a dummy control to be certain that the mouse is not over the test button.
        TestServices::InputHelper->MoveMouse(dummyButton);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(testButton->IsPointerOver);
            VERIFY_IS_FALSE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Normal"), buttonVisualStateGroup->CurrentState->Name);
        });

        TestServices::InputHelper->MoveMouse(testButton);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(testButton->IsPointerOver);
            VERIFY_IS_FALSE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"PointerOver"), buttonVisualStateGroup->CurrentState->Name);
        });

        TestServices::InputHelper->MouseButtonDown(testButton, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(testButton->IsPointerOver);
            VERIFY_IS_TRUE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Pointer, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Pressed"), buttonVisualStateGroup->CurrentState->Name);
        });

        TestServices::InputHelper->MouseButtonUp(testButton, 0, 0, MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(testButton->IsPointerOver);
            VERIFY_IS_FALSE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Pointer, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"PointerOver"), buttonVisualStateGroup->CurrentState->Name);
        });

        TestServices::InputHelper->MoveMouse(dummyButton);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(testButton->IsPointerOver);
            VERIFY_IS_FALSE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Pointer, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Normal"), buttonVisualStateGroup->CurrentState->Name);

            testButton->IsEnabled = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(testButton->IsPointerOver);
            VERIFY_IS_FALSE(testButton->IsPressed);
            VERIFY_ARE_EQUAL(xaml::FocusState::Unfocused, testButton->FocusState);
            VERIFY_ARE_EQUAL(Platform::StringReference(L"Disabled"), buttonVisualStateGroup->CurrentState->Name);
        });
    }

    void ButtonIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;

        double const expectedButtonWidth_WithTextContent = 66;
        double const expectedButtonWidth_WithNoContent = 24;
        double const expectedButtonWidth_WithLargeContent = 100 + expectedButtonWidth_WithNoContent;

        double const expectedButtonHeight_WithTextContent = 32;
        double const expectedButtonHeight_WithNoContent = 13;
        double const expectedButtonHeight_WithLargeContent = 100 + expectedButtonHeight_WithNoContent;

        xaml_controls::Button^ buttonWithTextContent;
        xaml_controls::Button^ buttonWithNoContent;
        xaml_controls::Button^ buttonWithLargeContent;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Button x:Name="buttonWithTextContent" Content="Button"/>
                        <Button x:Name="buttonWithNoContent" />
                        <Button x:Name="buttonWithLargeContent" >
                            <Rectangle Height="100" Width="100" Fill="Red" />
                        </Button>
                    </StackPanel>)"));

            buttonWithTextContent = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonWithTextContent"));
            buttonWithNoContent = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonWithNoContent"));
            buttonWithLargeContent = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonWithLargeContent"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Verify Footprint of Button with text content:
            VERIFY_ARE_EQUAL(expectedButtonWidth_WithTextContent, buttonWithTextContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedButtonHeight_WithTextContent, buttonWithTextContent->ActualHeight);

            // Verify Footprint of Button with no content:
            VERIFY_ARE_EQUAL(expectedButtonWidth_WithNoContent, buttonWithNoContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedButtonHeight_WithNoContent, buttonWithNoContent->ActualHeight);

            // Verify Footprint of Button with large content:
            VERIFY_ARE_EQUAL(expectedButtonWidth_WithLargeContent, buttonWithLargeContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedButtonHeight_WithLargeContent, buttonWithLargeContent->ActualHeight);
        });
    }

    void ButtonIntegrationTests::CanActivateWithClickModeHover()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Button^ button = nullptr;

        auto clickEvent = std::make_shared<Event>();
        auto clickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            button->ClickMode = xaml_controls::ClickMode::Hover;
            button->Content = "ButtonContent";

            clickRegistration.Attach(button, [clickEvent]() {clickEvent->Set();} );

            TestServices::WindowHelper->WindowContent = button;
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::WindowHelper->WaitForIdle();

        // Move the mouse off of the repeat button initially.  Otherwise sometimes the system will not
        // correctly detect the mouse is over the button.
        TestServices::InputHelper->MoveMouse(wf::Point(500, 500));
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(button);
        clickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ButtonIntegrationTests::CanExecuteICommands()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        CommandTestHelper^ command = ref new CommandTestHelper();
        command->ActivatingCommandParameter = m_commandParameter; // When this is set as the CommandParameter for the button, Command.CanExecute returns true

        auto commandExecutedEvent = std::make_shared<Event>();
        auto canExecuteCalledEvent = std::make_shared<Event>();

        auto commandExecutedRegistration = CreateSafeEventRegistration(CommandTestHelper, Executed);
        auto canExecuteCalledRegistration = CreateSafeEventRegistration(CommandTestHelper, CanExecuteCalled);

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            button->Content = "ButtonContent";
            button->Command = command;

            commandExecutedRegistration.Attach(command, [commandExecutedEvent]() {commandExecutedEvent->Set(); });
            canExecuteCalledRegistration.Attach(command, [canExecuteCalledEvent]() {canExecuteCalledEvent->Set(); });

            TestServices::WindowHelper->WindowContent = button;
        });

        // The ICommand helper class will only fire Execute when the CommandProperty passed in is set to the locally defined string object m_commandParameter
        // canExecuteCalledEvent and commandExecutedEvent should not fire in this case
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Tap(button);
        VERIFY_IS_FALSE(canExecuteCalledEvent->HasFired());
        VERIFY_IS_FALSE(commandExecutedEvent->HasFired());

        TestServices::InputHelper->LeftMouseClick(button);
        VERIFY_IS_FALSE(canExecuteCalledEvent->HasFired());
        VERIFY_IS_FALSE(commandExecutedEvent->HasFired());

        // Set the ICommand so CanExecute() returns true.  Should cause button.IsEnabled to return true as well
        RunOnUIThread([&]()
        {
            button->CommandParameter = m_commandParameter;
            VERIFY_IS_TRUE(button->IsEnabled);
        });

        TestServices::InputHelper->Tap(button);
        canExecuteCalledEvent->WaitForDefault();
        commandExecutedEvent->WaitForDefault();

        // Let's remove the CommandParameter and verify that the Command does not execute.  Button.IsEnabled should return false
        RunOnUIThread([&]()
        {
            button->CommandParameter = nullptr;
            VERIFY_IS_FALSE(button->IsEnabled);
        });
        canExecuteCalledEvent->Reset();
        commandExecutedEvent->Reset();

        TestServices::InputHelper->Tap(button);
        VERIFY_IS_FALSE(canExecuteCalledEvent->HasFired());
        VERIFY_IS_FALSE(commandExecutedEvent->HasFired());
    }

    void ButtonIntegrationTests::CanDetectChangesToCanExecuteWithoutBeingInVisualTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Button^ button = nullptr;

        CommandTestHelperNoParameter^ command = ref new CommandTestHelperNoParameter();

        auto loadedEvent = std::make_shared<Event>();
        auto unloadedEvent = std::make_shared<Event>();

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Loaded);
        auto unloadedRegistration = CreateSafeEventRegistration(xaml_controls::Button, Unloaded);

        RunOnUIThread([&]()
        {
            button = ref new xaml_controls::Button();
            button->Content = "ButtonContent";
            button->Command = command;

            loadedRegistration.Attach(button, [loadedEvent]() { loadedEvent->Set(); });
            unloadedRegistration.Attach(button, [unloadedEvent]() { unloadedEvent->Set(); });
            
            TestServices::WindowHelper->WindowContent = button;
        });

        LOG_OUTPUT(L"Wait for the button to be added the visual tree. It should be disabled since CanExecute is false.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Now remove the button from the visual tree.");
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(button->IsEnabled);

            TestServices::WindowHelper->WindowContent = nullptr;
        });

        unloadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Change CanExecute to true. The button should now be enabled as soon as we add it back to the visual tree.");
        RunOnUIThread([&]()
        {
            command->SetCanExecute(true);
            TestServices::WindowHelper->WindowContent = button;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(button->IsEnabled);
        });
    }

    void ButtonIntegrationTests::CanInheritFromButtonWithoutImplementingIWeakReferenceSource()
    {
        TestCleanupWrapper cleanup;

        class MyButton
            : public wrl::RuntimeClass<wrl::InhibitWeakReferencePolicy, IInspectable, wrl::ComposableBase<IInspectable>>
        {
            InspectableClass(L"MyButton", BaseTrust);
        public:
            _Check_return_ HRESULT RuntimeClassInitialize()
            {
                try
                {
                    Platform::Object^ innerFactory;
                    Platform::Object^ nonDelegatingInnerInspectable;
                    THROW_IF_FAILED(wf::GetActivationFactory(
                        wrl::Wrappers::HStringReference(L"Microsoft.UI.Xaml.Controls.Button").Get(),
                        reinterpret_cast<IInspectable**>(&innerFactory)));
                    safe_cast<xaml_controls::IButtonFactory^>(innerFactory)->CreateInstance(
                        reinterpret_cast<Platform::Object^>(static_cast<IInspectable*>(this)),
                        &nonDelegatingInnerInspectable);
                    // No need to SetComposableBasePointers for this test.
                }
                CATCH_RETURN();
                return S_OK;
            }
        };

        RunOnUIThread([&]()
        {
            wrl::ComPtr<MyButton> myButton;
            LOG_OUTPUT(L"Creating MyButton. Expecting no access violation.");
            VERIFY_SUCCEEDED(wrl::MakeAndInitialize<MyButton>(&myButton));
        });
    }

    void ButtonIntegrationTests::ValidateAccentButtonUIElementTree()
    {
        TestCleanupWrapper cleanup;

        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 600),
            1.f,
            // Test setup.
            []()
        {
            xaml_controls::Button^ pointerOverButton = nullptr;
            xaml_controls::Button^ pressedButton = nullptr;

            xaml_controls::StackPanel^ root = nullptr;
            RunOnUIThread([&]()
            {
                root = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                    LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                                <Button x:Name="RestButton" Content="Rest" Style="{StaticResource AccentButtonStyle}"/>
                                <Button x:Name="PointerOverButton" Content="PointerOver" Style="{StaticResource AccentButtonStyle}"/>
                                <Button x:Name="PressedButton" Content="Pressed" Style="{StaticResource AccentButtonStyle}"/>
                                <Button x:Name="DisabledButton" Content="Disabled" IsEnabled="False" Style="{StaticResource AccentButtonStyle}"/>
                            </StackPanel>)"));

                pointerOverButton = safe_cast<xaml_controls::Button^>(root->FindName(L"PointerOverButton"));
                pressedButton = safe_cast<xaml_controls::Button^>(root->FindName(L"PressedButton"));

                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VisualStateManager::GoToState(pointerOverButton, "PointerOver", false);
                VisualStateManager::GoToState(pressedButton, "Pressed", false);
            });
            TestServices::WindowHelper->WaitForIdle();

            return root;
        });
    }

    void ButtonIntegrationTests::ValidateSettingUICommandSetsProperties()
    {
        CommandHelper::ValidateSettingUICommandSetsProperties<xaml_controls::Button>(
            xaml_controls::Button::CommandProperty,
            xaml_controls::Button::ContentProperty,
            nullptr);
    }

    void ButtonIntegrationTests::ValidateSettingUICommandDoesNotOverwriteProperties()
    {
        CommandHelper::ValidateSettingUICommandDoesNotOverwriteProperties<xaml_controls::Button>(
            xaml_controls::Button::CommandProperty,
            xaml_controls::Button::ContentProperty,
            nullptr);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Button
