// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "CommandingContainerIntegrationTests.h"

#include <collection.h>
#include <XamlTailored.h>
#include <ControlHelper.h>
#include <TreeHelper.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include "FeatureFlags.h"

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Commanding {

#if WI_IS_FEATURE_PRESENT(Feature_CommandingImprovements)
    bool CommandingContainerIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool CommandingContainerIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool CommandingContainerIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    // We need to set up a bunch of state before we can run commanding container tests, including
    // some event handlers that will get detached if we run our test case after this method returns.
    // To get around that, we pass in our test code as a function to be run within this scope
    // instead of calling this method to set up our state and then returning.
    template <typename TFunction>
    void RunCommandingContainerTest(const TFunction& runTestFunction)
    {
        // TODO 15431546: Remove this once the CommandingContainer leak is diagnosed and fixed.
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\Commanding\\CommandingContainerScenarios.xaml";
        auto stackPanel = safe_cast<xaml_controls::StackPanel^>(LoadXamlFileOnUIThread(xamlFile));

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        xaml_controls::AppBarToggleButton^ boldButton = nullptr;
        xaml_controls::AppBarToggleButton^ italicButton = nullptr;
        xaml_controls::RichEditBox^ firstEditBox = nullptr;
        xaml_controls::RichEditBox^ secondEditBox = nullptr;
        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]
        {
            boldButton = safe_cast<xaml_controls::AppBarToggleButton^>(stackPanel->FindName(L"BoldButton"));
            italicButton = safe_cast<xaml_controls::AppBarToggleButton^>(stackPanel->FindName(L"ItalicButton"));
            firstEditBox = safe_cast<xaml_controls::RichEditBox^>(stackPanel->FindName(L"FirstEditBox"));
            secondEditBox = safe_cast<xaml_controls::RichEditBox^>(stackPanel->FindName(L"SecondEditBox"));
            textBox = safe_cast<xaml_controls::TextBox^>(stackPanel->FindName(L"TextBox"));
            listView = safe_cast<xaml_controls::ListView^>(stackPanel->FindName(L"ListView"));

            wfc::IVector<Platform::String^>^ listItems = ref new Platform::Collections::Vector<Platform::String^>();
            listItems->Append(ref new Platform::String(L"Item 1"));
            listItems->Append(ref new Platform::String(L"Item 2"));
            listItems->Append(ref new Platform::String(L"Item 3"));
            listItems->Append(ref new Platform::String(L"Item 4"));
            listView->ItemsSource = listItems;

            loadedRegistration.Attach(stackPanel, [loadedEvent]()
            {
                LOG_OUTPUT(L"StackPanel.Loaded event raised.");
                loadedEvent->Set();
            });
            TestServices::WindowHelper->WindowContent = stackPanel;
        });

        LOG_OUTPUT(L"Waiting for StackPanel.Loaded event.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        auto richEditContextChangedRegistration = CreateSafeEventRegistration(xaml_controls::CommandingContainer, ContextChanged);
        auto toggleBoldRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, ExecuteRequested);
        auto toggleItalicRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, ExecuteRequested);
        auto canPerformRichEditBoldCommandRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, CanExecuteRequested);
        auto canPerformRichEditItalicCommandRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, CanExecuteRequested);
        auto firstTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
        auto firstSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);
        auto secondTextChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, TextChanged);
        auto secondSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::RichEditBox, SelectionChanged);
        auto deleteItemsRegistration = CreateSafeEventRegistration(xaml_input::XamlUICommand, ExecuteRequested);
        auto listViewContextChangedRegistration = CreateSafeEventRegistration(xaml_controls::CommandingContainer, ContextChanged);
        auto listViewSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        RunOnUIThread([&]
        {
            xaml_controls::CommandingContainer^ richEditCommandingContainer = safe_cast<xaml_controls::CommandingContainer^>(stackPanel->FindName(L"RichEditCommandingContainer"));
            xaml_controls::CommandingContainer^ listViewCommandingContainer = safe_cast<xaml_controls::CommandingContainer^>(stackPanel->FindName(L"ListViewCommandingContainer"));
            xaml_input::XamlUICommand^ boldCommand = safe_cast<xaml_input::XamlUICommand^>(stackPanel->FindName(L"BoldCommand"));
            xaml_input::XamlUICommand^ italicCommand = safe_cast<xaml_input::XamlUICommand^>(stackPanel->FindName(L"ItalicCommand"));
            xaml_input::XamlUICommand^ deleteItemsCommand = safe_cast<xaml_input::XamlUICommand^>(stackPanel->FindName(L"DeleteItemsCommand"));

            VERIFY_IS_NOT_NULL(richEditCommandingContainer);
            VERIFY_IS_NOT_NULL(listViewCommandingContainer);
            VERIFY_IS_NOT_NULL(boldCommand);
            VERIFY_IS_NOT_NULL(italicCommand);
            VERIFY_IS_NOT_NULL(deleteItemsCommand);

            richEditContextChangedRegistration.Attach(
                richEditCommandingContainer,
                ref new wf::TypedEventHandler<xaml_controls::CommandingContainer^, xaml_controls::CommandingContextChangedEventArgs^>(
                [=](xaml_controls::CommandingContainer^ sender, xaml_controls::CommandingContextChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"CommandingContainer.ContextChanged event raised for richEditCommandingContainer.");

                    xaml_controls::RichEditBox^ targetRichEditBox = dynamic_cast<xaml_controls::RichEditBox^>(args->CommandTarget);

                    boldCommand->NotifyCanExecuteChanged();
                    italicCommand->NotifyCanExecuteChanged();

                    if (targetRichEditBox != nullptr)
                    {
                        boldButton->IsChecked = targetRichEditBox->Document->Selection->CharacterFormat->Bold == Microsoft::UI::Text::FormatEffect::On;
                        italicButton->IsChecked = targetRichEditBox->Document->Selection->CharacterFormat->Italic == Microsoft::UI::Text::FormatEffect::On;
                    }
                    else
                    {
                        boldButton->IsChecked = false;
                        italicButton->IsChecked = false;
                    }
                }));

            toggleBoldRegistration.Attach(
                boldCommand,
                ref new wf::TypedEventHandler<xaml_input::XamlUICommand^, xaml_input::ExecuteRequestedEventArgs^>(
                [=](xaml_input::XamlUICommand^ sender, xaml_input::ExecuteRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"XamlUICommand.ExecuteRequested event raised for boldCommand.");

                    xaml_controls::RichEditBox^ targetRichEditBox = dynamic_cast<xaml_controls::RichEditBox^>(args->CommandTarget);

                    if (targetRichEditBox != nullptr && targetRichEditBox->Document->Selection != nullptr)
                    {
                        Microsoft::UI::Text::ITextSelection^ selectedText = targetRichEditBox->Document->Selection;
                        Microsoft::UI::Text::ITextCharacterFormat^ charFormatting = selectedText->CharacterFormat;
                        charFormatting->Bold = Microsoft::UI::Text::FormatEffect::Toggle;
                        selectedText->CharacterFormat = charFormatting;

                        xaml_controls::CommandingContainer::NotifyContextChanged(targetRichEditBox);
                    }
                }));

            toggleItalicRegistration.Attach(
                italicCommand,
                ref new wf::TypedEventHandler<xaml_input::XamlUICommand^, xaml_input::ExecuteRequestedEventArgs^>(
                [=](xaml_input::XamlUICommand^ sender, xaml_input::ExecuteRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"XamlUICommand.ExecuteRequested event raised for italicCommand.");

                    xaml_controls::RichEditBox^ targetRichEditBox = dynamic_cast<xaml_controls::RichEditBox^>(args->CommandTarget);

                    if (targetRichEditBox != nullptr && targetRichEditBox->Document->Selection != nullptr)
                    {
                        Microsoft::UI::Text::ITextSelection^ selectedText = targetRichEditBox->Document->Selection;
                        Microsoft::UI::Text::ITextCharacterFormat^ charFormatting = selectedText->CharacterFormat;
                        charFormatting->Italic = Microsoft::UI::Text::FormatEffect::Toggle;
                        selectedText->CharacterFormat = charFormatting;

                        xaml_controls::CommandingContainer::NotifyContextChanged(targetRichEditBox);
                    }
                }));

            auto canPerformRichEditCommandFunction =
                ref new wf::TypedEventHandler<xaml_input::XamlUICommand^, xaml_input::CanExecuteRequestedEventArgs^>(
                [=](xaml_input::XamlUICommand^ sender, xaml_input::CanExecuteRequestedEventArgs^ args)
                {
                    LOG_OUTPUT(L"XamlUICommand.CanExecuteRequested event raised.");

                    args->CanExecute = dynamic_cast<xaml_controls::RichEditBox^>(args->CommandTarget) != nullptr;
                });

            canPerformRichEditBoldCommandRegistration.Attach(boldCommand, canPerformRichEditCommandFunction);
            canPerformRichEditItalicCommandRegistration.Attach(italicCommand, canPerformRichEditCommandFunction);

            auto updateRichEditBoxContentFunction =
                ref new xaml::RoutedEventHandler([=](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
                {
                    LOG_OUTPUT(L"RichEditBox.TextChanged|SelectionChanged event raised.");

                    auto focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
                    auto richEditBox = dynamic_cast<xaml_controls::RichEditBox^>(sender);

                    if (richEditBox != nullptr && sender == focusedElement)
                    {
                        xaml_controls::CommandingContainer::NotifyContextChanged(dynamic_cast<xaml_controls::RichEditBox^>(sender));
                    }
                });

            firstTextChangedRegistration.Attach(firstEditBox, updateRichEditBoxContentFunction);
            firstSelectionChangedRegistration.Attach(firstEditBox, updateRichEditBoxContentFunction);
            secondTextChangedRegistration.Attach(secondEditBox, updateRichEditBoxContentFunction);
            secondSelectionChangedRegistration.Attach(secondEditBox, updateRichEditBoxContentFunction);

            deleteItemsRegistration.Attach(
                deleteItemsCommand,
                ref new wf::TypedEventHandler<xaml_input::XamlUICommand^, xaml_input::ExecuteRequestedEventArgs^>(
                    [=](xaml_input::XamlUICommand^ sender, xaml_input::ExecuteRequestedEventArgs^ args)
            {
                LOG_OUTPUT(L"XamlUICommand.ExecuteRequested event raised for deleteItemsCommand.");

                xaml_controls::ListView^ listView = dynamic_cast<xaml_controls::ListView^>(args->ListCommandTarget);

                if (listView != nullptr)
                {
                    wfc::IVector<Platform::String^>^ itemsSource = safe_cast<wfc::IVector<Platform::String^>^>(listView->ItemsSource);
                    wfc::IVector<Platform::Object^>^ selectedItems = safe_cast<wfc::IVector<Platform::Object^>^>(listView->SelectedItems);

                    if (selectedItems->Size > 0)
                    {
                        for (int i = static_cast<int>(selectedItems->Size - 1); i >= 0; i--)
                        {
                            unsigned int itemIndex = 0;

                            if (itemsSource->IndexOf(safe_cast<Platform::String^>(selectedItems->GetAt(i)), &itemIndex))
                            {
                                itemsSource->RemoveAt(i);
                            }
                        }
                    }
                    else
                    {
                        unsigned int itemIndex = 0;

                        if (itemsSource->IndexOf(safe_cast<Platform::String^>(listView->ItemFromContainer(dynamic_cast<xaml_controls::ListViewItem^>(args->CommandTarget))), &itemIndex))
                        {
                            itemsSource->RemoveAt(itemIndex);
                        }
                    }
                }
            }));

            listViewContextChangedRegistration.Attach(
                listViewCommandingContainer,
                ref new wf::TypedEventHandler<xaml_controls::CommandingContainer^, xaml_controls::CommandingContextChangedEventArgs^>(
                [=](xaml_controls::CommandingContainer^ sender, xaml_controls::CommandingContextChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"CommandingContainer.ContextChanged event raised for listViewCommandingContainer.");

                    deleteItemsCommand->NotifyCanExecuteChanged();
                }));

            listViewSelectionChangedRegistration.Attach(
                listView,
                ref new xaml_controls::SelectionChangedEventHandler([=](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"ListView.SelectionChanged event raised for listView.");

                    xaml_controls::CommandingContainer::NotifyContextChanged(dynamic_cast<xaml::UIElement^>(sender));
                }));
        });

        runTestFunction(boldButton, italicButton, firstEditBox, secondEditBox, textBox, listView);
    }

    void CommandingContainerIntegrationTests::ValidateCommandingContainerProvidesCommandTarget()
    {
        TestCleanupWrapper cleanup;

        RunCommandingContainerTest(
            [](
                xaml_controls::AppBarToggleButton^ boldButton,
                xaml_controls::AppBarToggleButton^ italicButton,
                xaml_controls::RichEditBox^ firstEditBox,
                xaml_controls::RichEditBox^ secondEditBox,
                xaml_controls::TextBox^ textBox,
                xaml_controls::ListView^ listView
            ) -> void
        {
            LOG_OUTPUT(L"Ensuring firstEditBox has focus.");
            ControlHelper::EnsureFocused(firstEditBox);

            LOG_OUTPUT(L"Enter text into the first edit box.");
            TestServices::KeyboardHelper->PressKeySequence(L"test");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Select all the text in the first edit box.");
                firstEditBox->Document->Selection->SetRange(0, firstEditBox->Document->Selection->EndPosition);
            });

            LOG_OUTPUT(L"Tap on the bold button. The selection should become bold.");
            TestServices::InputHelper->Tap(boldButton);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Text::FormatEffect::On, firstEditBox->Document->Selection->CharacterFormat->Bold);
            });

            LOG_OUTPUT(L"Tap on the italic button. The selection should become italic.");
            TestServices::InputHelper->Tap(italicButton);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(Microsoft::UI::Text::FormatEffect::On, firstEditBox->Document->Selection->CharacterFormat->Italic);
            });
        });
    }

    void CommandingContainerIntegrationTests::ValidateCommandingContainerProvidesListCommandTarget()
    {
        TestCleanupWrapper cleanup;

        RunCommandingContainerTest(
            [](
                xaml_controls::AppBarToggleButton^ boldButton,
                xaml_controls::AppBarToggleButton^ italicButton,
                xaml_controls::RichEditBox^ firstEditBox,
                xaml_controls::RichEditBox^ secondEditBox,
                xaml_controls::TextBox^ textBox,
                xaml_controls::ListView^ listView
            ) -> void
        {
            xaml_controls::ListViewItem^ listViewItem1 = nullptr;
            xaml_controls::ListViewItem^ listViewItem2 = nullptr;
            xaml_controls::AppBarButton^ deleteButton = nullptr;

            RunOnUIThread([&]
            {
                listViewItem1 = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(0));
                listViewItem2 = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(1));
                deleteButton = TreeHelper::GetVisualChildByType<xaml_controls::AppBarButton>(safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(3)));
            });

            LOG_OUTPUT(L"First select the first two items.");
            TestServices::InputHelper->Tap(listViewItem1, 0.2f, 0.5f);
            TestServices::InputHelper->Tap(listViewItem2, 0.2f, 0.5f);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Now press the delete key to delete them.");
            TestServices::KeyboardHelper->Delete();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(2u, listView->Items->Size);
            });

            LOG_OUTPUT(L"Now tap on the last delete button to delete that item.");
            TestServices::InputHelper->Tap(deleteButton);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_ARE_EQUAL(1u, listView->Items->Size);
            });
        });
    }

    void CommandingContainerIntegrationTests::ValidateCommandingContainerTracksContext()
    {
        TestCleanupWrapper cleanup;

        RunCommandingContainerTest(
            [](
                xaml_controls::AppBarToggleButton^ boldButton,
                xaml_controls::AppBarToggleButton^ italicButton,
                xaml_controls::RichEditBox^ firstEditBox,
                xaml_controls::RichEditBox^ secondEditBox,
                xaml_controls::TextBox^ textBox,
                xaml_controls::ListView^ listView
            ) -> void
        {
            ControlHelper::EnsureFocused(firstEditBox);

            LOG_OUTPUT(L"Enter text into the first edit box.");
            TestServices::KeyboardHelper->PressKeySequence(L"test");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                LOG_OUTPUT(L"Select the first two characters in the first edit box.");
                firstEditBox->Document->Selection->SetRange(0, 2);
            });

            LOG_OUTPUT(L"Tap on the bold button. The selection should become bold.");
            TestServices::InputHelper->Tap(boldButton);
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Put focus at the end of the first edit box. The bold button should no longer be checked since the caret is no longer in bold text.");
            ControlHelper::EnsureFocused(firstEditBox);
            TestServices::KeyboardHelper->Right();
            TestServices::KeyboardHelper->Right();
            TestServices::KeyboardHelper->Right();
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_IS_FALSE(boldButton->IsChecked->Value);
            });

            LOG_OUTPUT(L"Hit the left arrow key three times. The bold button should now be checked since the caret is in bold text.");
            TestServices::KeyboardHelper->Left();
            TestServices::KeyboardHelper->Left();
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_IS_TRUE(boldButton->IsChecked->Value);
            });

            LOG_OUTPUT(L"Put focus in the regular text box. The bold button should now be unchecked and disabled since a rich edit box no longer has focus.");
            ControlHelper::EnsureFocused(textBox);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]
            {
                VERIFY_IS_FALSE(boldButton->IsChecked->Value);
                VERIFY_IS_TRUE(ControlHelper::IsInVisualState(boldButton, ref new Platform::String(L"CommonStates"), ref new Platform::String(L"Disabled")));
            });
        });
    }
#endif
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Commanding
