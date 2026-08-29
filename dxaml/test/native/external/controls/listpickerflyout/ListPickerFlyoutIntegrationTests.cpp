// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListPickerFlyoutIntegrationTests.h"

#include <generic\DependencyObjectTests.h>

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <SafeEventRegistration.h>
#include <Collection.h>

#include <ControlHelper.h>
#include <FlyoutHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListPickerFlyout {

    bool ListPickerFlyoutIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }
     
    bool ListPickerFlyoutIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ListPickerFlyoutIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ListPickerFlyoutIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ListPickerFlyout>::CanInstantiate();
    }

    void ListPickerFlyoutIntegrationTests::VerifyProperties()
    {
        TestCleanupWrapper cleanup;
        auto listPickerFlyout = SetupListPickerFlyoutTest(false /*isMultipleSelection*/, false /*shouldLaunchFlyout*/);

        LOG_OUTPUT(L"VerifyProperties: verify the SelectedIndex property. -1 1 2 -1");

        // Verify the default value.
        ValidateSelectionState(listPickerFlyout, -1);

        // Verify the SelectedIndex property by setting the selected index to the index 1, 2 and -1(default).
        RunOnUIThread([&]()
        {
            listPickerFlyout->SelectedIndex = 1;
        });
        ValidateSelectionState(listPickerFlyout, 1);

        RunOnUIThread([&]()
        {
            listPickerFlyout->SetValue(xaml_controls::ListPickerFlyout::SelectedIndexProperty, 2);
        });
        ValidateSelectionState(listPickerFlyout, 2);

        RunOnUIThread([&]()
        {
            listPickerFlyout->SelectedIndex = -1;
        });
        ValidateSelectionState(listPickerFlyout, -1);

        // Verify the SelectedItem property by setting the selected item with the index 0 and -1(default).
        LOG_OUTPUT(L"VerifyProperties: verify the SelectedItem property. 0 -1");
        RunOnUIThread([&]()
        {
            auto itemsSource = dynamic_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
            listPickerFlyout->SelectedItem = itemsSource->GetAt(0);
        });
        ValidateSelectionState(listPickerFlyout, 0);

        RunOnUIThread([&]()
        {
            listPickerFlyout->SelectedItem = nullptr;
        });
        ValidateSelectionState(listPickerFlyout, -1);

        // Verify the SelectedValue property by setting the selected value with the index 0, 2 and -1(default).
        LOG_OUTPUT(L"VerifyProperties: verify the SelectedValue property. 0 2 -1");
        RunOnUIThread([&]()
        {
            auto itemsSource = dynamic_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
            listPickerFlyout->SelectedValue = itemsSource->GetAt(0);
        });
        ValidateSelectionState(listPickerFlyout, 0);

        RunOnUIThread([&]()
        {
            auto itemsSource = dynamic_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
            listPickerFlyout->SetValue(xaml_controls::ListPickerFlyout::SelectedValueProperty, itemsSource->GetAt(2));
        });
        ValidateSelectionState(listPickerFlyout, 2);

        RunOnUIThread([&]()
        {
            listPickerFlyout->SelectedValue = nullptr;
        });
        ValidateSelectionState(listPickerFlyout, -1);
    }

    void ListPickerFlyoutIntegrationTests::CanSelectAnItem()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBlock^ textBlockItem = nullptr;
        auto itemsPickedEvent = std::make_shared<Event>();
        auto itemsPickedRegistration = CreateSafeEventRegistration(xaml_controls::ListPickerFlyout, ItemsPicked);

        xaml_controls::ListPickerFlyout^ listPickerFlyout = SetupListPickerFlyoutTest(false /*isMultipleSelection*/, true /*shouldLaunchFlyout*/);

        RunOnUIThread([&]()
        {
            auto itemsVector = safe_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
            textBlockItem = dynamic_cast<xaml_controls::TextBlock^>(itemsVector->GetAt(3));
            VERIFY_IS_NOT_NULL(textBlockItem);

            itemsPickedRegistration.Attach(listPickerFlyout, [itemsPickedEvent]()
            {
                LOG_OUTPUT(L"ItemsPicked event fired!");
                itemsPickedEvent->Set();
            });
        });

        // Tap the textblock item to select it.
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Tap(textBlockItem);

        // We expect the itemsPicked event to fire after selecting the textblock item.
        itemsPickedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void ListPickerFlyoutIntegrationTests::CanSelectMultipleItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::TextBlock^ selectedTextBlockItem = nullptr;
        xaml_controls::ListPickerFlyout^ listPickerFlyout = SetupListPickerFlyoutTest(true /*isMultipleSelection*/, true /*shouldLaunchFlyout*/);

        // We will pre-select one item programatically, and then tap a textblock item.
        const unsigned int expectedNumberOfSelectedItems = 2;

        RunOnUIThread([&]()
        {
            // Pre-select an item
            listPickerFlyout->SelectedIndex = 0;

            // Get a textblock item to select
            auto itemsVector = safe_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
            selectedTextBlockItem = dynamic_cast<xaml_controls::TextBlock^>(itemsVector->GetAt(3));
            VERIFY_IS_NOT_NULL(selectedTextBlockItem);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tap the textblock item to focus it.
        TestServices::InputHelper->Tap(selectedTextBlockItem);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Get the list of selected items.
            auto selectedItems = listPickerFlyout->SelectedItems;
            auto selectedItemsSize = selectedItems->Size;

            VERIFY_ARE_EQUAL(selectedItemsSize, expectedNumberOfSelectedItems);
        });

        // Close the picker flyout.
        FlyoutHelper::HideFlyout(listPickerFlyout);
        TestServices::WindowHelper->WaitForIdle();
    }

    void ListPickerFlyoutIntegrationTests::ValidateSelectionState(
        xaml_controls::ListPickerFlyout^ listPickerFlyout,
        int expectedSelectedIndex)
    {
        RunOnUIThread([&listPickerFlyout, expectedSelectedIndex]()
        {
            auto selectedIndex = listPickerFlyout->SelectedIndex;
            VERIFY_ARE_EQUAL(selectedIndex, expectedSelectedIndex);

            auto selectedItem = dynamic_cast<Platform::String^>(listPickerFlyout->SelectedItem);
            auto selectedValue = dynamic_cast<Platform::String^>(listPickerFlyout->SelectedValue);
            auto selectedItems = listPickerFlyout->SelectedItems;

            if (expectedSelectedIndex == -1)
            {
                LOG_OUTPUT(L"VerifyProperties: selected index is -1.");
                VERIFY_IS_NULL(selectedItem);
                VERIFY_IS_NULL(selectedValue);
                VERIFY_IS_TRUE(0 == selectedItems->Size);
            }
            else
            {
                auto itemsSource = dynamic_cast<Platform::Collections::Vector<Platform::Object^>^>(listPickerFlyout->ItemsSource);
                auto expectedSelectedItem = dynamic_cast<Platform::String^>(itemsSource->GetAt(selectedIndex));

                LOG_OUTPUT(L"VerifyProperties: selected item=%s expected=%s", selectedItem->Data(), expectedSelectedItem->Data());
                VERIFY_ARE_EQUAL(selectedItem, expectedSelectedItem);

                LOG_OUTPUT(L"VerifyProperties: selected items size==%d ", selectedItems->Size);
                VERIFY_IS_TRUE(selectedItems->Size > 0);

                auto firstSelectedItem = dynamic_cast<Platform::String^>(selectedItems->GetAt(0));
                VERIFY_ARE_EQUAL(firstSelectedItem, expectedSelectedItem);
            }
        });
    }

    xaml_controls::ListPickerFlyout^ ListPickerFlyoutIntegrationTests::SetupListPickerFlyoutTest(
        bool isMultipleSelection, bool shouldLaunchFlyout)
    {
        xaml_controls::Button^ button = nullptr;
        xaml_controls::ListPickerFlyout^ listPickerFlyout = nullptr;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' > "
                L"  <Button x:Name='button' Content='Test ListPickerFlyout' Background='Blue' Margin='0,100,0,0'> "
                L"    <Button.Flyout> "
                L"      <ListPickerFlyout />"
                L"    </Button.Flyout> "
                L"  </Button> "
                L"</StackPanel>"));

            button = dynamic_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            listPickerFlyout = dynamic_cast<xaml_controls::ListPickerFlyout^>(button->Flyout);
            VERIFY_IS_NOT_NULL(listPickerFlyout);

            listPickerFlyout->SelectionMode = isMultipleSelection ? xaml_controls::ListPickerFlyoutSelectionMode::Multiple : xaml_controls::ListPickerFlyoutSelectionMode::Single;

            loadedRegistration.Attach(
                rootPanel,
                ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            auto itemsVector = ref new Platform::Collections::Vector<Platform::Object^>();
            for (int i = 0; i < 3; i++)
            {
                auto s = ref new Platform::String(L"Item ");
                s += i;
                itemsVector->Append(s);
            }

            auto textBlock = ref new xaml_controls::TextBlock();
            textBlock->Text = "TextBlock Item";
            itemsVector->Append(textBlock);

            auto buttonAdded = ref new xaml_controls::Button();
            buttonAdded->Content = "Button Item.";
            itemsVector->Append(buttonAdded);

            listPickerFlyout->ItemsSource = itemsVector;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();

        if (shouldLaunchFlyout)
        {
            LOG_OUTPUT(L"SetupListPickerFlyoutTest: launch the list picker flyout by using Tap.");
            ControlHelper::DoClickUsingTap(button);
        }

        TestServices::WindowHelper->WaitForIdle();

        return listPickerFlyout;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListPickerFlyout
