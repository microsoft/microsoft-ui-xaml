// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CommandingIntegrationTests.h"

#include <XamlTailored.h>
#include <ControlHelper.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <CommandHelper.h>
#include <EnablePseudoLoc.h>
#include "FocusTestHelper.h"
#include <CommonInputHelper.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Commanding {

    bool CommandingIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool CommandingIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CommandingIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void CommandingIntegrationTests::ValidateUICommandExecute()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto uiCommand = ref new xaml_input::XamlUICommand();
            auto executeRequestedRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, ExecuteRequested);
            bool commandExecuted = false;
            
            executeRequestedRegistration.Attach(uiCommand, [&commandExecuted]() { commandExecuted = true; } );
            uiCommand->Execute(nullptr);
            
            VERIFY_IS_TRUE(commandExecuted);
        });
    }

    void CommandingIntegrationTests::ValidateUICommandCanExecuteDefaultsToTrue()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto uiCommand = ref new xaml_input::XamlUICommand();
            VERIFY_IS_TRUE(uiCommand->CanExecute(nullptr));
        });
    }

    void CommandingIntegrationTests::ValidateUICommandCanExecute()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto uiCommand = ref new xaml_input::XamlUICommand();
            auto canExecuteRequestedRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, CanExecuteRequested);
            
            canExecuteRequestedRegistration.Attach(uiCommand, ref new wf::TypedEventHandler<xaml_input::XamlUICommand^, xaml_input::CanExecuteRequestedEventArgs^>(
                [](xaml_input::XamlUICommand^ sender, xaml_input::CanExecuteRequestedEventArgs^ args)
                {
                    args->CanExecute = false;
                }));
                
            VERIFY_IS_FALSE(uiCommand->CanExecute(nullptr));
        });
    }

    void CommandingIntegrationTests::UICommandCanUpdateCanExecute()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto uiCommand = ref new xaml_input::XamlUICommand();
            auto canExecuteChangedRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, CanExecuteChanged);
            bool canExecuteChangedRaised = false;
            
            canExecuteChangedRegistration.Attach(uiCommand, [&canExecuteChangedRaised]() { canExecuteChangedRaised = true; } );
            uiCommand->NotifyCanExecuteChanged();
            
            VERIFY_IS_TRUE(canExecuteChangedRaised);
        });
    }

    void CommandingIntegrationTests::ValidateChildCommandIsDelegatedTo()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto uiCommand = ref new xaml_input::XamlUICommand();
            
            auto parameter = ref new Platform::String(L"Test string");
            bool executeCalled = false;
            auto childCommand = ref new MenuCommand(ref new ExecuteDelegate([&executeCalled](auto param) { executeCalled = true; }), false /* canExecute */, parameter);
            
            uiCommand->Command = childCommand;
            VERIFY_IS_FALSE(uiCommand->CanExecute(parameter));
            childCommand->CanExecuteFlag = true;
            VERIFY_IS_TRUE(uiCommand->CanExecute(parameter));
            
            uiCommand->Execute(parameter);
            VERIFY_IS_TRUE(executeCalled);
        });
    }

    void CommandingIntegrationTests::ValidateCommandLibraryProperties()
    {
        TestCleanupWrapper cleanup;
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Cut,
            ref new Platform::String(L"Cut"),
            xaml_controls::Symbol::Cut,
            ::Windows::System::VirtualKey::X,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Remove the selected content and put it on the clipboard"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Copy,
            ref new Platform::String(L"Copy"),
            xaml_controls::Symbol::Copy,
            ::Windows::System::VirtualKey::C,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Copy the selected content to the clipboard"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Paste,
            ref new Platform::String(L"Paste"),
            xaml_controls::Symbol::Paste,
            ::Windows::System::VirtualKey::V,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Insert the contents of the clipboard at the current location"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::SelectAll,
            ref new Platform::String(L"Select All"),
            xaml_controls::Symbol::SelectAll,
            ::Windows::System::VirtualKey::A,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Select all content"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Delete,
            ref new Platform::String(L"Delete"),
            xaml_controls::Symbol::Delete,
            ::Windows::System::VirtualKey::Delete,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Delete the selected content"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Share,
            ref new Platform::String(L"Share"),
            xaml_controls::Symbol::Share,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Share the selected content"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Save,
            ref new Platform::String(L"Save"),
            xaml_controls::Symbol::Save,
            ::Windows::System::VirtualKey::S,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Save your changes"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Open,
            ref new Platform::String(L"Open"),
            xaml_controls::Symbol::OpenFile,
            ::Windows::System::VirtualKey::O,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Open"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Close,
            ref new Platform::String(L"Close"),
            xaml_controls::Symbol::Cancel,
            ::Windows::System::VirtualKey::W,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Close"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Pause,
            ref new Platform::String(L"Pause"),
            xaml_controls::Symbol::Pause,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Pause"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Play,
            ref new Platform::String(L"Play"),
            xaml_controls::Symbol::Play,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Play"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Stop,
            ref new Platform::String(L"Stop"),
            xaml_controls::Symbol::Stop,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Stop"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Forward,
            ref new Platform::String(L"Forward"),
            xaml_controls::Symbol::Forward,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Go to the next item"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Backward,
            ref new Platform::String(L"Backward"),
            xaml_controls::Symbol::Back,
            ::Windows::System::VirtualKey::None,
            ::Windows::System::VirtualKeyModifiers::None,
            ref new Platform::String(L"Go to the previous item"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Undo,
            ref new Platform::String(L"Undo"),
            xaml_controls::Symbol::Undo,
            ::Windows::System::VirtualKey::Z,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Reverse the most recent action"));
        
        ValidateCommandLibraryKind(
            xaml_input::StandardUICommandKind::Redo,
            ref new Platform::String(L"Redo"),
            xaml_controls::Symbol::Redo,
            ::Windows::System::VirtualKey::Y,
            ::Windows::System::VirtualKeyModifiers::Control,
            ref new Platform::String(L"Repeat the most recently undone action"));
    }

    void CommandingIntegrationTests::ValidateCommandLibraryPropertiesNonEnglish()
    {
        TestCleanupWrapper cleanup;
        EnablePseudoLoc pseudoLoc;
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Cut,
            ref new Platform::String(L"Cut"),
            ref new Platform::String(L"Remove the selected content and put it on the clipboard"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Copy,
            ref new Platform::String(L"Copy"),
            ref new Platform::String(L"Copy the selected content to the clipboard"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Paste,
            ref new Platform::String(L"Paste"),
            ref new Platform::String(L"Insert the contents of the clipboard at the current location"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::SelectAll,
            ref new Platform::String(L"Select All"),
            ref new Platform::String(L"Select all content"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Delete,
            ref new Platform::String(L"Delete"),
            ref new Platform::String(L"Delete the selected content"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Share,
            ref new Platform::String(L"Share"),
            ref new Platform::String(L"Share the selected content"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Save,
            ref new Platform::String(L"Save"),
            ref new Platform::String(L"Save your changes"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Open,
            ref new Platform::String(L"Open"),
            ref new Platform::String(L"Open"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Close,
            ref new Platform::String(L"Close"),
            ref new Platform::String(L"Close"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Pause,
            ref new Platform::String(L"Pause"),
            ref new Platform::String(L"Pause"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Play,
            ref new Platform::String(L"Play"),
            ref new Platform::String(L"Play"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Stop,
            ref new Platform::String(L"Stop"),
            ref new Platform::String(L"Stop"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Forward,
            ref new Platform::String(L"Forward"),
            ref new Platform::String(L"Go to the next item"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Backward,
            ref new Platform::String(L"Backward"),
            ref new Platform::String(L"Go to the previous item"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Undo,
            ref new Platform::String(L"Undo"),
            ref new Platform::String(L"Reverse the most recent action"));
        
        ValidateCommandLibraryKindPseudoLoc(
            xaml_input::StandardUICommandKind::Redo,
            ref new Platform::String(L"Redo"),
            ref new Platform::String(L"Repeat the most recently undone action"));
    }
    
    void CommandingIntegrationTests::ValidateCommandLibraryCanBeUsedInXaml()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto command = safe_cast<xaml_input::StandardUICommand^>(xaml_markup::XamlReader::Load(
                LR"(<StandardUICommand
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Kind="Cut"/>)"));
                        
            ValidateCommandLibraryKind(
                command,
                ref new Platform::String(L"Cut"),
                xaml_controls::Symbol::Cut,
                ::Windows::System::VirtualKey::X,
                ::Windows::System::VirtualKeyModifiers::Control,
                ref new Platform::String(L"Remove the selected content and put it on the clipboard"));
        });
    }
    
    void CommandingIntegrationTests::ValidateChangingStandardUICommandKindChangesProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto command = ref new xaml_input::StandardUICommand();
                        
            ValidateCommandLibraryKind(
                command,
                nullptr,
                nullptr,
                ::Windows::System::VirtualKey::None,
                ::Windows::System::VirtualKeyModifiers::None,
                nullptr);
                
            command->Kind = xaml_input::StandardUICommandKind::Cut;
                        
            ValidateCommandLibraryKind(
                command,
                ref new Platform::String(L"Cut"),
                xaml_controls::Symbol::Cut,
                ::Windows::System::VirtualKey::X,
                ::Windows::System::VirtualKeyModifiers::Control,
                ref new Platform::String(L"Remove the selected content and put it on the clipboard"));
                
            command->Kind = xaml_input::StandardUICommandKind::Copy;
        
            ValidateCommandLibraryKind(
                command,
                ref new Platform::String(L"Copy"),
                xaml_controls::Symbol::Copy,
                ::Windows::System::VirtualKey::C,
                ::Windows::System::VirtualKeyModifiers::Control,
                ref new Platform::String(L"Copy the selected content to the clipboard"));
        });
    }
    
    void CommandingIntegrationTests::ValidateChangingStandardUICommandKindDoesNotOverrideDirectlySetProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto customLabel = ref new Platform::String(L"Custom label");
            auto customDescription = ref new Platform::String(L"Custom description");
            
            auto command = ref new xaml_input::StandardUICommand(xaml_input::StandardUICommandKind::Cut);
            command->Label = customLabel;
            command->Description = customDescription;
            
            auto newAccelerator = ref new xaml_input::KeyboardAccelerator();
            newAccelerator->Key = ::Windows::System::VirtualKey::A;
            newAccelerator->Modifiers = ::Windows::System::VirtualKeyModifiers::Menu;
            
            command->KeyboardAccelerators->SetAt(0, newAccelerator);
                        
            ValidateCommandLibraryKind(
                command,
                customLabel,
                xaml_controls::Symbol::Cut,
                ::Windows::System::VirtualKey::A,
                ::Windows::System::VirtualKeyModifiers::Menu,
                customDescription);
                
            command->Kind = xaml_input::StandardUICommandKind::Copy;
                        
            ValidateCommandLibraryKind(
                command,
                customLabel,
                xaml_controls::Symbol::Copy,
                ::Windows::System::VirtualKey::A,
                ::Windows::System::VirtualKeyModifiers::Menu,
                customDescription);
                
            auto customSymbol = xaml_controls::Symbol::Favorite;
            auto symbolIconSource = ref new xaml_controls::SymbolIconSource();
            symbolIconSource->Symbol = customSymbol;
            command->IconSource = symbolIconSource;
            command->Kind = xaml_input::StandardUICommandKind::SelectAll;
                        
            ValidateCommandLibraryKind(
                command,
                customLabel,
                customSymbol,
                ::Windows::System::VirtualKey::A,
                ::Windows::System::VirtualKeyModifiers::Menu,
                customDescription);
        });
    }
    
    void CommandingIntegrationTests::ValidateKeyboardAcceleratorsFromCommands()
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto standardCommandButtonClickEvent = std::make_shared<Event>();
        auto standardCommandButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);
        auto customCommandButtonClickEvent = std::make_shared<Event>();
        auto customCommandButtonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        xaml_controls::Button^ standardCommandButton;
        xaml_controls::Button^ customCommandButton;

        RunOnUIThread([&]
        {
            xaml_controls::StackPanel^ stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <StackPanel.Resources>
                            <StandardUICommand x:Name="OpenCommand" Kind="Open" />
                            <XamlUICommand x:Name="CustomCommand" Label="Custom command">
                                <XamlUICommand.KeyboardAccelerators>
                                    <KeyboardAccelerator Key="C" Modifiers="Control" />
                                </XamlUICommand.KeyboardAccelerators>
                            </XamlUICommand>
                        </StackPanel.Resources>
                        <Button x:Name="StandardCommandButton" Command="{StaticResource OpenCommand}" />
                        <Button x:Name="CustomCommandButton"  Command="{StaticResource CustomCommand}" />
                    </StackPanel>)"));

            standardCommandButton = safe_cast<xaml_controls::Button^>(stackPanel->FindName(L"StandardCommandButton"));
            customCommandButton = safe_cast<xaml_controls::Button^>(stackPanel->FindName(L"CustomCommandButton"));

            loadedRegistration.Attach(stackPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"Root panel loaded.");
                loadedEvent->Set();
            });

            standardCommandButtonClickRegistration.Attach(standardCommandButton, [standardCommandButtonClickEvent]()
            {
                LOG_OUTPUT(L"Standard command button invoked.");
                standardCommandButtonClickEvent->Set();
            });

            customCommandButtonClickRegistration.Attach(customCommandButton, [customCommandButtonClickEvent]()
            {
                LOG_OUTPUT(L"Custom command button invoked.");
                customCommandButtonClickEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        LOG_OUTPUT(L"Waiting for root panel to load.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(standardCommandButton, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing Ctrl+O to invoke the standard command button.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_o#$u$_o#$u$_ctrlscan");
        
        standardCommandButtonClickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Pressing Ctrl+C to invoke the custom command button.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrlscan#$d$_c#$u$_c#$u$_ctrlscan");
        
        customCommandButtonClickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void CommandingIntegrationTests::ValidateSelectionFlyoutDismissalOnPointerMove()
    {
        ValidateSelectionFlyoutDismissalOnPointerMove(false /* shouldExpandBeforePointerMove */);
    }
    
    void CommandingIntegrationTests::ValidateSelectionFlyoutDoesNotDismissAfterExpand()
    {
        ValidateSelectionFlyoutDismissalOnPointerMove(true /* shouldExpandBeforePointerMove */);
    }
    
    void CommandingIntegrationTests::ValidateSelectionFlyoutDismissalOnPointerMove(bool shouldExpandBeforePointerMove)
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        xaml_controls::RichEditBox^ richEditBox = nullptr;
        xaml_controls::Button^ button = nullptr;

        RunOnUIThread([&]
        {
            xaml_controls::StackPanel^ stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <RichEditBox x:Name="richEditBox" Margin="50"/>
                        <Button x:Name="button" Content="button" Margin="50"/>
                    </StackPanel>)"));

            VERIFY_IS_NOT_NULL(stackPanel);

            richEditBox = safe_cast<xaml_controls::RichEditBox^>(stackPanel->FindName(L"richEditBox"));
            button = safe_cast<xaml_controls::Button^>(stackPanel->FindName(L"button"));

            loadedRegistration.Attach(stackPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel Loaded event raised.");
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        LOG_OUTPUT(L"Waiting for StackPanel to load.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Focusing RichEditBox control.");
        ControlHelper::EnsureFocused(richEditBox);

        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Hooking up Opened and Closed events.");
            openedRegistration.Attach(richEditBox->SelectionFlyout, [openedEvent]()
            {
                LOG_OUTPUT(L"FlyoutBase Opened event raised.");
                openedEvent->Set();
            });

            closedRegistration.Attach(richEditBox->SelectionFlyout, [closedEvent]()
            {
                LOG_OUTPUT(L"FlyoutBase Closed event raised.");
                closedEvent->Set();
            });
        });

        LOG_OUTPUT(L"Entering text into the first RichEditBox.");
        TestServices::KeyboardHelper->PressKeySequence(L"test");
        TestServices::WindowHelper->WaitForIdle();
        
        wf::Point doubleClickPoint{};
        
        RunOnUIThread([&]
        {
            auto transformToRoot = richEditBox->TransformToVisual(nullptr);
            doubleClickPoint = transformToRoot->TransformPoint(wf::Point(20, 20));
        });

        LOG_OUTPUT(L"Moving mouse over the first RichEditBox.");
        TestServices::InputHelper->MoveMouse(doubleClickPoint);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Double-clicking the first RichEditBox to select text and bring up the SelectionFlyout.");
        TestServices::InputHelper->LeftMouseClick(doubleClickPoint);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(doubleClickPoint);
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Waiting for SelectionFlyout to open.");
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            VERIFY_IS_TRUE(richEditBox->SelectionFlyout->IsOpen);
        });
        
        if (shouldExpandBeforePointerMove)
        {
            xaml_controls::Button^ moreButton = nullptr;
            
            auto commandBarOpenedEvent = std::make_shared<Event>();
            auto commandBarOpenedRegistration = CreateSafeEventRegistration(xaml_controls::CommandBar, Opened);
            
            RunOnUIThread([&]
            {
                auto popups = xaml_media::VisualTreeHelper::GetOpenPopupsForXamlRoot(button->XamlRoot);
                auto commandBar = TreeHelper::GetVisualChildByType<xaml_controls::CommandBar>(safe_cast<xaml::FrameworkElement^>(popups->GetAt(popups->Size - 1)->Child));
                moreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(commandBar, L"MoreButton"));
                
                commandBarOpenedRegistration.Attach(commandBar, [commandBarOpenedEvent]()
                {
                    LOG_OUTPUT(L"CommandBar Opened event raised.");
                    commandBarOpenedEvent->Set();
                });
            });

            TestServices::InputHelper->LeftMouseClick(moreButton);
            
            commandBarOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        LOG_OUTPUT(L"Moving mouse over the Button to hide the SelectionFlyout.");
        TestServices::InputHelper->MoveMouse(button);
        TestServices::WindowHelper->WaitForIdle();

        if (shouldExpandBeforePointerMove)
        {
            LOG_OUTPUT(L"SelectionFlyout should not have closed.");
            VERIFY_IS_FALSE(closedEvent->HasFired());
            
            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(richEditBox->SelectionFlyout->IsOpen);
                
                LOG_OUTPUT(L"Now manually close the SelectionFlyout so we don't have a leftover open popup.");
                richEditBox->SelectionFlyout->Hide();
            });
        }

        LOG_OUTPUT(L"Waiting for SelectionFlyout to close.");
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]
        {
            VERIFY_IS_FALSE(richEditBox->SelectionFlyout->IsOpen);
        });
    }

    void CommandingIntegrationTests::ValidateKeyboardingToOpenSubMenuGivesFocus()
    {
        TestCleanupWrapper cleanup;

        auto gridLoadedEvent = std::make_shared<Event>();
        auto gridLoadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);
        auto outerFlyoutOpenedEvent = std::make_shared<Event>();
        auto outerFlyoutOpenedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto outerFlyoutClosedEvent = std::make_shared<Event>();
        auto outerFlyoutClosedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);
        auto menuFlyoutItem1GotFocusEvent = std::make_shared<Event>();
        auto menuFlyoutItem1GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);
        auto menuFlyoutItem2GotFocusEvent = std::make_shared<Event>();
        auto menuFlyoutItem2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::MenuFlyoutItem, GotFocus);

        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::AppBarButton^ appBarButtonWithFlyout = nullptr;
        xaml_controls::MenuFlyoutSubItem^ menuFlyoutSubItem = nullptr;

        RunOnUIThread([&]
        {
            rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Background="Transparent">
                        <Grid.ContextFlyout>
                            <CommandBarFlyout x:Name="CommandBarFlyout" Placement="BottomEdgeAlignedLeft">
                                <CommandBarFlyout.SecondaryCommands>
                                    <AppBarButton Label="Item 1" />
                                    <AppBarButton Label="Item 2" x:Name="AppBarButtonWithFlyout">
                                        <AppBarButton.Flyout>
                                            <MenuFlyout>
                                                <MenuFlyoutItem Text="Sub item 1" x:Name="MenuFlyoutItem1" />
                                                <MenuFlyoutSubItem Text="Sub item 2" x:Name="MenuFlyoutSubItem">
                                                    <MenuFlyoutItem Text="Sub sub item 1" x:Name="MenuFlyoutItem2" />
                                                </MenuFlyoutSubItem>
                                            </MenuFlyout>
                                        </AppBarButton.Flyout>
                                    </AppBarButton>
                                </CommandBarFlyout.SecondaryCommands>
                            </CommandBarFlyout>
                        </Grid.ContextFlyout>
                    </Grid>)"));

            auto commandBarFlyout = safe_cast<xaml_controls::CommandBarFlyout^>(rootGrid->FindName(L"CommandBarFlyout"));
            appBarButtonWithFlyout = safe_cast<xaml_controls::AppBarButton^>(rootGrid->FindName(L"AppBarButtonWithFlyout"));
            auto menuFlyoutItem1 = safe_cast<xaml_controls::MenuFlyoutItem^>(rootGrid->FindName(L"MenuFlyoutItem1"));
            menuFlyoutSubItem = safe_cast<xaml_controls::MenuFlyoutSubItem^>(rootGrid->FindName(L"MenuFlyoutSubItem"));
            auto menuFlyoutItem2 = safe_cast<xaml_controls::MenuFlyoutItem^>(rootGrid->FindName(L"MenuFlyoutItem2"));

            gridLoadedRegistration.Attach(rootGrid, [gridLoadedEvent]()
            {
                LOG_OUTPUT(L"Grid loaded.");
                gridLoadedEvent->Set();
            });

            outerFlyoutOpenedRegistration.Attach(commandBarFlyout, [outerFlyoutOpenedEvent]()
            {
                LOG_OUTPUT(L"CommandBarFlyout opened.");
                outerFlyoutOpenedEvent->Set();
            });

            outerFlyoutClosedRegistration.Attach(commandBarFlyout, [outerFlyoutClosedEvent]()
            {
                LOG_OUTPUT(L"CommandBarFlyout closed.");
                outerFlyoutClosedEvent->Set();
            });

            menuFlyoutItem1GotFocusRegistration.Attach(menuFlyoutItem1, [menuFlyoutItem1GotFocusEvent]()
            {
                LOG_OUTPUT(L"MenuFlyoutItem1 got focus.");
                menuFlyoutItem1GotFocusEvent->Set();
            });

            menuFlyoutItem2GotFocusRegistration.Attach(menuFlyoutItem2, [menuFlyoutItem2GotFocusEvent]()
            {
                LOG_OUTPUT(L"MenuFlyoutItem2 got focus.");
                menuFlyoutItem2GotFocusEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootGrid;
        });

        LOG_OUTPUT(L"Waiting for the grid to load.");
        gridLoadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Right-click to open the context flyout.");
        TestServices::InputHelper->MoveMouse(rootGrid);
        TestServices::InputHelper->MouseButtonDown(rootGrid, 50, 50, MouseButton::Right);
        TestServices::InputHelper->MouseButtonUp(rootGrid, 50, 50, MouseButton::Right);

        outerFlyoutOpenedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Mouse over the AppBarButton with a flyout to open the flyout. The flyout should get focus.");
        TestServices::InputHelper->MoveMouse(appBarButtonWithFlyout);

        menuFlyoutItem1GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Mouse over the MenuFlyoutSubItem to open the sub-menu. The sub-menu should get focus");
        TestServices::InputHelper->MoveMouse(menuFlyoutSubItem);

        menuFlyoutItem2GotFocusEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Cancel three times to close the flyouts.");
        CommonInputHelper::Cancel(InputDevice::Keyboard);
        CommonInputHelper::Cancel(InputDevice::Keyboard);
        CommonInputHelper::Cancel(InputDevice::Keyboard);

        outerFlyoutClosedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    
    void CommandingIntegrationTests::ValidateNoLayoutCycleAt125PercentScale()
    {
        TestCleanupWrapper cleanup;
        
        TestServices::WindowHelper->SetWindowSizeOverrideWithScale(::Windows::Foundation::Size(400, 400), 1.25f);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        auto openedEvent = std::make_shared<Event>();
        auto openedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Opened);
        auto closedEvent = std::make_shared<Event>();
        auto closedRegistration = CreateSafeEventRegistration(xaml_primitives::FlyoutBase, Closed);

        xaml_controls::Button^ button = nullptr;
        xaml_controls::CommandBarFlyout^ flyout = nullptr;
        xaml_controls::AppBarSeparator^ appBarSeparator = nullptr;

        RunOnUIThread([&]
        {
            xaml_controls::StackPanel^ stackPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel
                        xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation"
                        xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Button x:Name="Button" Content="button" Margin="50">
                            <Button.Flyout>
                                <CommandBarFlyout x:Name="CommandBarFlyout" Placement="Right">
                                    <CommandBarFlyout.SecondaryCommands>
                                        <AppBarButton Label="Undo" Icon="Undo" />
                                        <AppBarSeparator IsTabStop="False" />
                                        <AppBarButton Label="Redo" Icon="Redo" />
                                        <AppBarButton Label="Select all" />
                                        <AppBarToggleButton Label="Favorite" Icon="Favorite" />
                                        <AppBarButton Label="AppBarButton with long label" Visibility="Collapsed" />
                                        <AppBarSeparator x:Name="AppBarSeparator" IsTabStop="False" />
                                        <AppBarButton Label="Text" />
                                        <AppBarButton Label="Text" />
                                        <AppBarSeparator IsTabStop="False" />
                                        <AppBarButton Label="Text" />
                                        <AppBarButton Label="Text" />
                                    </CommandBarFlyout.SecondaryCommands>
                                </CommandBarFlyout>
                            </Button.Flyout>
                        </Button>
                    </StackPanel>)"));

            button = safe_cast<xaml_controls::Button^>(stackPanel->FindName(L"Button"));
            flyout = safe_cast<xaml_controls::CommandBarFlyout^>(stackPanel->FindName(L"CommandBarFlyout"));
            appBarSeparator = safe_cast<xaml_controls::AppBarSeparator^>(stackPanel->FindName(L"AppBarSeparator"));

            loadedRegistration.Attach(stackPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel Loaded event raised.");
                loadedEvent->Set();
            });
            
            openedRegistration.Attach(flyout, [openedEvent]()
            {
                LOG_OUTPUT(L"CommandBarFlyout Opened event raised.");
                openedEvent->Set();
            });

            closedRegistration.Attach(flyout, [closedEvent]()
            {
                LOG_OUTPUT(L"CommandBarFlyout Closed event raised.");
                closedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Opening the flyout.");
        
        RunOnUIThread([&]
        {
            flyout->ShowAt(button);
        });
        
        openedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Try scrolling down. We should not hit a layout cycle.");
        
        // We'll use the AppBarSeparator just as something in the middle of the CommandBarFlyout's ScrollViewer.
        TestServices::InputHelper->ScrollMouseWheel(appBarSeparator, -1);

        LOG_OUTPUT(L"Closing the flyout. The app would have crashed by now if the layout cycle repro'd.");

        RunOnUIThread([&]
        {
            flyout->Hide();
        });
        
        closedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }
    
    void CommandingIntegrationTests::ValidateCommandLibraryKind(
        xaml_input::StandardUICommandKind kind,
        Platform::String^ expectedLabel,
        xaml_controls::Symbol expectedSymbol,
        ::Windows::System::VirtualKey expectedAcceleratorKey,
        ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
        Platform::String^ expectedDescription)
    {
        RunOnUIThread([&]
        {
            ValidateCommandLibraryKind(
                ref new xaml_input::StandardUICommand(kind),
                expectedLabel,
                expectedSymbol,
                expectedAcceleratorKey,
                expectedAcceleratorModifiers,
                expectedDescription);
        });
    }
    
    void CommandingIntegrationTests::ValidateCommandLibraryKind(
        xaml_input::StandardUICommand^ command,
        Platform::String^ expectedLabel,
        xaml_controls::Symbol expectedSymbol,
        ::Windows::System::VirtualKey expectedAcceleratorKey,
        ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
        Platform::String^ expectedDescription)
    {
        RunOnUIThread([&]
        {
            auto expectedSymbolIconSource = ref new xaml_controls::SymbolIconSource();
            expectedSymbolIconSource->Symbol = expectedSymbol;
            
            ValidateCommandLibraryKind(
                command,
                expectedLabel,
                expectedSymbolIconSource,
                expectedAcceleratorKey,
                expectedAcceleratorModifiers,
                expectedDescription);
        });
    }
    
    void CommandingIntegrationTests::ValidateCommandLibraryKind(
        xaml_input::StandardUICommand^ command,
        Platform::String^ expectedLabel,
        xaml_controls::SymbolIconSource^ expectedSymbolIconSource,
        ::Windows::System::VirtualKey expectedAcceleratorKey,
        ::Windows::System::VirtualKeyModifiers expectedAcceleratorModifiers,
        Platform::String^ expectedDescription)
    {
        RunOnUIThread([&]
        {
            LOG_OUTPUT(L"Expected label: \"%s\"", expectedLabel->Data());
            LOG_OUTPUT(L"Actual label: \"%s\"", command->Label->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedLabel, command->Label) == 0);
            
            auto symbolIconSource = safe_cast<xaml_controls::SymbolIconSource^>(command->IconSource);

            if (expectedSymbolIconSource != nullptr)
            {
                VERIFY_IS_NOT_NULL(symbolIconSource);
                VERIFY_ARE_EQUAL(expectedSymbolIconSource->Symbol, symbolIconSource->Symbol);
            }
            else
            {
                VERIFY_IS_NULL(symbolIconSource);
            }

            if (expectedAcceleratorKey != ::Windows::System::VirtualKey::None)
            {
                VERIFY_ARE_EQUAL(1u, command->KeyboardAccelerators->Size);
                xaml_input::KeyboardAccelerator^ actualKeyboardAccelerator = command->KeyboardAccelerators->GetAt(0);

                LOG_OUTPUT(L"Expected keyboard accelerator key: %d", expectedAcceleratorKey);
                LOG_OUTPUT(L"Actual keyboard accelerator key: %d", actualKeyboardAccelerator->Key);
                VERIFY_ARE_EQUAL(expectedAcceleratorKey, actualKeyboardAccelerator->Key);
                LOG_OUTPUT(L"Expected keyboard accelerator key modifiers: %d", expectedAcceleratorModifiers);
                LOG_OUTPUT(L"Actual keyboard accelerator key modifiers: %d", actualKeyboardAccelerator->Modifiers);
                VERIFY_ARE_EQUAL(expectedAcceleratorModifiers, actualKeyboardAccelerator->Modifiers);
            }
            else
            {
                VERIFY_ARE_EQUAL(0u, command->KeyboardAccelerators->Size);
            }

            LOG_OUTPUT(L"Expected description: \"%s\"", expectedDescription->Data());
            LOG_OUTPUT(L"Actual description: \"%s\"", command->Description->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(expectedDescription, command->Description) == 0);
        });
    }

    void CommandingIntegrationTests::ValidateCommandLibraryKindPseudoLoc(
        xaml_input::StandardUICommandKind kind,
        Platform::String^ englishLabel,
        Platform::String^ englishDescription)
    {
        RunOnUIThread([&]
        {
            auto command = ref new xaml_input::StandardUICommand(kind);
            
            LOG_OUTPUT(L"English label: \"%s\"", englishLabel->Data());
            LOG_OUTPUT(L"Pseudo-loc label: \"%s\"", command->Label->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(englishLabel, command->Label) != 0);

            LOG_OUTPUT(L"English description: \"%s\"", englishDescription->Data());
            LOG_OUTPUT(L"Pseudo-loc description: \"%s\"", command->Description->Data());
            VERIFY_IS_TRUE(Platform::String::CompareOrdinal(englishDescription, command->Description) != 0);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Commanding
