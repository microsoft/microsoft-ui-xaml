// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ListBoxIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ControlHelper.h>
#include <ItemsControlHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ListBox {

    bool ListBoxIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ListBoxIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ListBoxIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ListBoxIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ListBox>::CanInstantiate();
    }

    void ListBoxIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::ListBox>::CanEnterAndLeaveLiveTree();
    }

    void ListBoxIntegrationTests::DefaultContainerIsListBoxItem()
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        for (int i = 0; i < s_itemCountWithoutVirtualization; i++)
        {
            RunOnUIThread([&] ()
            {
                auto containerAtIndex = listBox->ContainerFromIndex(i);

                LOG_OUTPUT(L"Container %d is a %s.", i, GetClassName(containerAtIndex->GetType())->Data());

                auto listBoxItem = dynamic_cast<xaml_controls::ListBoxItem^>(containerAtIndex);
                VERIFY_IS_NOT_NULL(listBoxItem);
            });
        }
    }

    void ListBoxIntegrationTests::VirtualizesWithEnoughItems()
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);
        auto containersVector = ItemsControlHelper::GetContainersFromVisualTree(listBox);

        LOG_OUTPUT(L"After adding %d items, %d containers exist.", s_itemCountWithoutVirtualization, containersVector->Size);
        VERIFY_ARE_EQUAL((unsigned int)s_itemCountWithoutVirtualization, containersVector->Size);

        listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithVirtualization);
        containersVector = ItemsControlHelper::GetContainersFromVisualTree(listBox);

        LOG_OUTPUT(L"After adding %d items, %d containers exist.", s_itemCountWithVirtualization, containersVector->Size);
        VERIFY_IS_LESS_THAN(containersVector->Size, (unsigned int)s_itemCountWithVirtualization);
    }

    void ListBoxIntegrationTests::CanScrollItemIntoView()
    {
        TestCleanupWrapper cleanup;

        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);
        xaml_controls::ListBoxItem^ lastContainer = nullptr;
        wf::Point originalPosition;

        RunOnUIThread([&] ()
        {
            lastContainer = dynamic_cast<xaml_controls::ListBoxItem^>(listBox->ContainerFromIndex(s_itemCountWithoutVirtualization - 1));

            VERIFY_IS_NOT_NULL(lastContainer);

            auto transformToRoot = lastContainer->TransformToVisual(nullptr);
            originalPosition = transformToRoot->TransformPoint(wf::Point(0, 0));

            LOG_OUTPUT(L"Before scroll, last item is at position (%.2f, %.2f)",
                originalPosition.X, originalPosition.Y);
        });

        ItemsControlHelper::ScrollToIndex<xaml_controls::ListBox, Platform::String>(listBox, s_itemCountWithoutVirtualization - 1);

        RunOnUIThread([&] ()
        {
            auto transformToRoot = lastContainer->TransformToVisual(nullptr);
            wf::Point newPosition = transformToRoot->TransformPoint(wf::Point(0, 0));

            LOG_OUTPUT(L"After scroll, last item is at position (%.2f, %.2f), for a delta of (%.2f, %.2f)",
                newPosition.X, newPosition.Y,
                newPosition.X - originalPosition.X, newPosition.Y - originalPosition.Y);

            VERIFY_ARE_EQUAL(originalPosition.X, newPosition.X);
            VERIFY_IS_GREATER_THAN(originalPosition.Y, newPosition.Y);
        });
    }

    void ListBoxIntegrationTests::ValidateUIElementTree()
    {
        ControlHelper::ValidateUIElementTree(
            wf::Size(400, 800),
            1.f,
            []()
            {
                xaml_controls::StackPanel^ rootPanel = nullptr;
                xaml_controls::StackPanel^ verticalRootPanel1 = nullptr;
                xaml_controls::StackPanel^ verticalRootPanel2 = nullptr;

                xaml_controls::ListBox^ disabledListBox = nullptr;

                xaml_controls::ListBox^ listBox = nullptr;
                xaml_controls::ListBoxItem^ restItem = nullptr;
                xaml_controls::ListBoxItem^ hoverItem = nullptr;
                xaml_controls::ListBoxItem^ pressItem = nullptr;
                xaml_controls::ListBoxItem^ selectedItem = nullptr;
                xaml_controls::ListBoxItem^ selectedUnfocusedItem = nullptr;
                xaml_controls::ListBoxItem^ hoverSelectedItem = nullptr;
                xaml_controls::ListBoxItem^ pressSelectedItem = nullptr;

                RunOnUIThread([&]()
                {
                    rootPanel = ref new xaml_controls::StackPanel();
                    rootPanel->Orientation = xaml_controls::Orientation::Vertical;
                    rootPanel->HorizontalAlignment = xaml::HorizontalAlignment::Left;

                    disabledListBox = ref new xaml_controls::ListBox();
                    disabledListBox->IsEnabled = false;

                    xaml_controls::TextBlock^ disabledTextBlock1 = ref new xaml_controls::TextBlock();
                    disabledTextBlock1->Text = "Disabled ListBoxItem1";
                    xaml_controls::ListBoxItem^ disabledItem1 = ref new xaml_controls::ListBoxItem();
                    disabledItem1->Content = disabledTextBlock1;
                    disabledListBox->Items->Append(disabledItem1);

                    xaml_controls::TextBlock^ disabledTextBlock2 = ref new xaml_controls::TextBlock();
                    disabledTextBlock2->Text = "Disabled ListBoxItem2";
                    xaml_controls::ListBoxItem^ disabledItem2 = ref new xaml_controls::ListBoxItem();
                    disabledItem2->Content = disabledTextBlock2;
                    disabledListBox->Items->Append(disabledItem2);

                    listBox = ref new xaml_controls::ListBox();

                    xaml_controls::TextBlock^ restTextBlock = ref new xaml_controls::TextBlock();
                    restTextBlock->Text = "Unselected ListBoxItem";
                    restItem = ref new xaml_controls::ListBoxItem();
                    restItem->Content = restTextBlock;
                    listBox->Items->Append(restItem);

                    xaml_controls::TextBlock^ hoverTextBlock = ref new xaml_controls::TextBlock();
                    hoverTextBlock->Text = "PointerOver ListBoxItem";
                    hoverItem = ref new xaml_controls::ListBoxItem();
                    hoverItem->Content = hoverTextBlock;
                    listBox->Items->Append(hoverItem);

                    xaml_controls::TextBlock^ pressTextBlock = ref new xaml_controls::TextBlock();
                    pressTextBlock->Text = "Pressed ListBoxItem";
                    pressItem = ref new xaml_controls::ListBoxItem();
                    pressItem->Content = pressTextBlock;
                    listBox->Items->Append(pressItem);

                    xaml_controls::TextBlock^ selectedTextBlock = ref new xaml_controls::TextBlock();
                    selectedTextBlock->Text = "Selected ListBoxItem";
                    selectedItem = ref new xaml_controls::ListBoxItem();
                    selectedItem->Content = selectedTextBlock;
                    listBox->Items->Append(selectedItem);

                    xaml_controls::TextBlock^ selectedUnfocusedTextBlock = ref new xaml_controls::TextBlock();
                    selectedUnfocusedTextBlock->Text = "Selected Unfocused ListBoxItem";
                    selectedUnfocusedItem = ref new xaml_controls::ListBoxItem();
                    selectedUnfocusedItem->Content = selectedUnfocusedTextBlock;
                    listBox->Items->Append(selectedUnfocusedItem);

                    xaml_controls::TextBlock^ hoverSelectedTextBlock = ref new xaml_controls::TextBlock();
                    hoverSelectedTextBlock->Text = "Selected PointerOver ListBoxItem";
                    hoverSelectedItem = ref new xaml_controls::ListBoxItem();
                    hoverSelectedItem->Content = hoverSelectedTextBlock;
                    listBox->Items->Append(hoverSelectedItem);

                    xaml_controls::TextBlock^ pressSelectedTextBlock = ref new xaml_controls::TextBlock();
                    pressSelectedTextBlock->Text = "Selected Pressed ListBoxItem";
                    pressSelectedItem = ref new xaml_controls::ListBoxItem();
                    pressSelectedItem->Content = pressSelectedTextBlock;
                    listBox->Items->Append(pressSelectedItem);

                    rootPanel->Children->Append(listBox);
                    rootPanel->Children->Append(disabledListBox);

                    TestServices::WindowHelper->WindowContent = rootPanel;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    VisualStateManager::GoToState(hoverItem, "PointerOver", true);
                    VisualStateManager::GoToState(hoverItem, "Unselected", true);

                    VisualStateManager::GoToState(pressItem, "Pressed", true);
                    VisualStateManager::GoToState(pressItem, "Unselected", true);

                    VisualStateManager::GoToState(selectedItem, "Normal", true);
                    VisualStateManager::GoToState(selectedItem, "Selected", true);

                    VisualStateManager::GoToState(selectedUnfocusedItem, "Normal", true);
                    VisualStateManager::GoToState(selectedUnfocusedItem, "SelectedUnfocused", true);

                    VisualStateManager::GoToState(hoverSelectedItem, "PointerOver", true);
                    VisualStateManager::GoToState(hoverSelectedItem, "SelectedPointerOver", true);

                    VisualStateManager::GoToState(pressSelectedItem, "Pressed", true);
                    VisualStateManager::GoToState(pressSelectedItem, "SelectedPressed", true);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootPanel;
            });
    }

    void ListBoxIntegrationTests::ScrollingPreservesVirtualization()
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithVirtualization);
        auto virtualizingStackPanel = ItemsControlHelper::GetPanelFromItemsControl<xaml_controls::VirtualizingStackPanel>(listBox);
        auto cleanUpVirtualizedItemRegistration = CreateSafeEventRegistration(xaml_controls::VirtualizingStackPanel, CleanUpVirtualizedItemEvent);
        auto cleanUpVirtualizedItemEventCount = std::make_shared<int>();

        // We want to always be able to scroll, so we'll start a 10 items down and proceed 10 items at a time
        // to ensure that the item we're trying to scroll into view is always out of view.
        const int ScrollIncrement = 10;

        RunOnUIThread([&] ()
        {
            // Prevent the list from receiving focus at the start of the test, which would cause the first item to remain realized.
            listBox->IsEnabled = false;

            cleanUpVirtualizedItemRegistration.Attach(virtualizingStackPanel,
                ref new xaml_controls::CleanUpVirtualizedItemEventHandler(
                [cleanUpVirtualizedItemEventCount](Platform::Object^ sender, xaml_controls::CleanUpVirtualizedItemEventArgs^ args)
            {
                *cleanUpVirtualizedItemEventCount += 1;
            }));
        });

        for (int i = ScrollIncrement; i < s_itemCountWithVirtualization; i += ScrollIncrement)
        {
            *cleanUpVirtualizedItemEventCount = 0;

            ItemsControlHelper::ScrollToIndex<xaml_controls::ListBox, Platform::String>(listBox, i);

            auto containersVector = ItemsControlHelper::GetContainersFromVisualTree(listBox);

            LOG_OUTPUT(L"%d containers exist now. %d containers were cleaned up by virtualization.", containersVector->Size, *cleanUpVirtualizedItemEventCount);
            VERIFY_IS_LESS_THAN(containersVector->Size, (unsigned int)s_itemCountWithVirtualization);

            // The first time we scroll, we won't have as many items unrealized as the number of items we scrolled
            // because starting with the top X items already in view means we won't have scrolled as far as other times.
            // After the first time, however, we're scrolling by exactly the height of the number of items we scrolled,
            // so precisely that many items should have been unrealized.
            if (i > ScrollIncrement)
            {
                // If a whole number of ListBoxItems does not fit exactly into the ListBox, we number of items that we clean up might not
                // exactly match the scroll increment.
                VERIFY_IS_TRUE(*cleanUpVirtualizedItemEventCount == ScrollIncrement || *cleanUpVirtualizedItemEventCount == ScrollIncrement + 1);
            }
            else
            {
                VERIFY_IS_GREATER_THAN(*cleanUpVirtualizedItemEventCount, 0);
            }
        }
    }

    void ListBoxIntegrationTests::CanSelectSingleItemsOnDesktop()
    {
        VerifyCanSelectSingleItems(false /* doTap */);
    }

    void ListBoxIntegrationTests::CanSelectMultipleItemsOnDesktop()
    {
        VerifyCanSelectMultipleItems(false /* doTap */);
    }

    void ListBoxIntegrationTests::CanExtendSelection()
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        RunOnUIThread([&] ()
        {
            listBox->SelectionMode = xaml_controls::SelectionMode::Extended;
        });

        LOG_OUTPUT(L"Selecting with no keys held down.");
        SelectItemAtIndex(listBox, 1, false /* doTap */);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 1 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 1");
            auto selectedString = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_IS_NOT_NULL(selectedString);
            VERIFY_ARE_EQUAL(expectedString, selectedString);
        });

        SelectItemAtIndex(listBox, 3, false /* doTap */);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 3 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 3");
            auto selectedString = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_IS_NOT_NULL(selectedString);
            VERIFY_ARE_EQUAL(expectedString, selectedString);
        });

        SelectItemAtIndex(listBox, 1, false /* doTap */);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 1 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 1");
            auto selectedString = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_IS_NOT_NULL(selectedString);
            VERIFY_ARE_EQUAL(expectedString, selectedString);
        });

        LOG_OUTPUT(L"Selecting while holding control down.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl");
        SelectItemAtIndex(listBox, 3, false /* doTap */);
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl");

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that items 1 and 3 are selected...");
            VERIFY_ARE_EQUAL(2u, listBox->SelectedItems->Size);

            auto expectedString1 = ref new Platform::String(L"Item 1");
            auto selectedString1 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            auto expectedString2 = ref new Platform::String(L"Item 3");
            auto selectedString2 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(1));

            VERIFY_IS_NOT_NULL(selectedString1);
            VERIFY_ARE_EQUAL(expectedString1, selectedString1);

            VERIFY_IS_NOT_NULL(selectedString2);
            VERIFY_ARE_EQUAL(expectedString2, selectedString2);
        });

        SelectItemAtIndex(listBox, 1, false /* doTap */);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 1 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 1");
            auto selectedString = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_IS_NOT_NULL(selectedString);
            VERIFY_ARE_EQUAL(expectedString, selectedString);
        });

        LOG_OUTPUT(L"Selecting while holding shift down.");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift");
        SelectItemAtIndex(listBox, 3, false /* doTap */);
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift");

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that items 1, 2, and 3 are selected...");
            VERIFY_ARE_EQUAL(3u, listBox->SelectedItems->Size);

            auto expectedString1 = ref new Platform::String(L"Item 1");
            auto selectedString1 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            auto expectedString2 = ref new Platform::String(L"Item 2");
            auto selectedString2 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(1));

            auto expectedString3 = ref new Platform::String(L"Item 3");
            auto selectedString3 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(2));

            VERIFY_IS_NOT_NULL(selectedString1);
            VERIFY_ARE_EQUAL(expectedString1, selectedString1);

            VERIFY_IS_NOT_NULL(selectedString2);
            VERIFY_ARE_EQUAL(expectedString2, selectedString2);

            VERIFY_IS_NOT_NULL(selectedString3);
            VERIFY_ARE_EQUAL(expectedString3, selectedString3);
        });
    }

    void ListBoxIntegrationTests::CanSelectSingleItemsOnPhone()
    {
        VerifyCanSelectSingleItems(true /* doTap */);
    }

    void ListBoxIntegrationTests::CanSelectMultipleItemsOnPhone()
    {
        VerifyCanSelectMultipleItems(true /* doTap */);
    }

    void ListBoxIntegrationTests::VerifyListBoxItemFocus()
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);
        int indexOfItem = 1;

        SelectItemAtIndex(listBox, indexOfItem, true /* doTap */);

        RunOnUIThread([&] ()
        {
            auto listBoxItem = safe_cast<xaml_controls::ListBoxItem^>(listBox->ContainerFromIndex(indexOfItem));

            VERIFY_ARE_EQUAL(xaml::FocusState::Pointer, listBoxItem->FocusState);
        });
    }

    void ListBoxIntegrationTests::VerifyCanSelectSingleItems(bool doTap)
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        SelectItemAtIndex(listBox, 1, doTap);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 1 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 1");
            auto selectedStringFromItem = dynamic_cast<Platform::String^>(listBox->SelectedItem);
            auto selectedStringFromList = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_ARE_EQUAL(1, listBox->SelectedIndex);

            VERIFY_IS_NOT_NULL(selectedStringFromItem);
            VERIFY_IS_NOT_NULL(selectedStringFromList);
            VERIFY_ARE_EQUAL(expectedString, selectedStringFromItem);
            VERIFY_ARE_EQUAL(expectedString, selectedStringFromList);
        });

        SelectItemAtIndex(listBox, 2, doTap);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 2 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 2");
            auto selectedStringFromItem = dynamic_cast<Platform::String^>(listBox->SelectedItem);
            auto selectedStringFromList = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_ARE_EQUAL(2, listBox->SelectedIndex);

            VERIFY_IS_NOT_NULL(selectedStringFromItem);
            VERIFY_IS_NOT_NULL(selectedStringFromList);
            VERIFY_ARE_EQUAL(expectedString, selectedStringFromItem);
            VERIFY_ARE_EQUAL(expectedString, selectedStringFromList);
        });
    }

    void ListBoxIntegrationTests::VerifyCanSelectMultipleItems(bool doTap)
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        RunOnUIThread([&] ()
        {
            listBox->SelectionMode = xaml_controls::SelectionMode::Multiple;
        });

        SelectItemAtIndex(listBox, 1, doTap);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that item 1 is selected...");
            VERIFY_ARE_EQUAL(1u, listBox->SelectedItems->Size);

            auto expectedString = ref new Platform::String(L"Item 1");
            auto selectedString = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            VERIFY_IS_NOT_NULL(selectedString);
            VERIFY_ARE_EQUAL(expectedString, selectedString);
        });

        SelectItemAtIndex(listBox, 2, doTap);

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Verifying that items 1 and 2 are selected...");
            VERIFY_ARE_EQUAL(2u, listBox->SelectedItems->Size);

            auto expectedString1 = ref new Platform::String(L"Item 1");
            auto selectedString1 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(0));

            auto expectedString2 = ref new Platform::String(L"Item 2");
            auto selectedString2 = dynamic_cast<Platform::String^>(listBox->SelectedItems->GetAt(1));

            VERIFY_IS_NOT_NULL(selectedString1);
            VERIFY_ARE_EQUAL(expectedString1, selectedString1);

            VERIFY_IS_NOT_NULL(selectedString2);
            VERIFY_ARE_EQUAL(expectedString2, selectedString2);
        });

        listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);
    }

    void ListBoxIntegrationTests::SelectItemAtIndex(xaml_controls::ListBox^ listBox, int index, bool doTap)
    {
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListBox, SelectionChanged);

        xaml_controls::ListBoxItem^ listBoxItemToSelect = nullptr;

        RunOnUIThread([&] ()
        {
            selectionChangedRegistration.Attach(listBox, ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));
        });

        RunOnUIThread([&] ()
        {
            listBoxItemToSelect = dynamic_cast<xaml_controls::ListBoxItem^>(listBox->ContainerFromIndex(index));
            VERIFY_IS_NOT_NULL(listBoxItemToSelect);
        });


        if (doTap)
        {
            LOG_OUTPUT(L"Tapping on item at index %d.", index);
            TestServices::InputHelper->Tap(listBoxItemToSelect);
        }
        else
        {
            LOG_OUTPUT(L"Clicking on item at index %d.", index);
            TestServices::InputHelper->LeftMouseClick(listBoxItemToSelect);
        }

        LOG_OUTPUT(L"Waiting for selection to change...");
        selectionChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"Done.");
    }

    void ListBoxIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        double expectedListBoxDefaultWidth = 400;
        double expectedListBoxDefaultHeight = 120;
        double expectedListBoxWidthSpecifiedWidth = 200;
        double expectedListBoxWidthSpecifiedHeight = 100;

        xaml_controls::ListBox^ listBoxDefault = nullptr;
        xaml_controls::ListBox^ listBoxWithSpecifiedSize = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                       <ListBox x:Name="listBoxDefault" >
                         <ListBoxItem>Item1</ListBoxItem>
                         <ListBoxItem>Item2</ListBoxItem>
                         <ListBoxItem>Item3</ListBoxItem>
                       </ListBox>
                       <ListBox x:Name="listBoxWithSpecifiedSize" Width="200" Height="100">
                         <ListBoxItem>Item1</ListBoxItem>
                         <ListBoxItem>Item2</ListBoxItem>
                         <ListBoxItem>
                           <Rectangle Width="400" Height="20" Fill="Red" />
                         </ListBoxItem>
                       </ListBox>
                    </StackPanel>)"));

            TestServices::WindowHelper->WindowContent = rootPanel;

            listBoxDefault = safe_cast<xaml_controls::ListBox^>(rootPanel->FindName(L"listBoxDefault"));
            listBoxWithSpecifiedSize = safe_cast<xaml_controls::ListBox^>(rootPanel->FindName(L"listBoxWithSpecifiedSize"));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedListBoxDefaultWidth, listBoxDefault->ActualWidth);
            VERIFY_ARE_EQUAL(expectedListBoxDefaultHeight, listBoxDefault->ActualHeight);

            VERIFY_ARE_EQUAL(expectedListBoxWidthSpecifiedWidth, listBoxWithSpecifiedSize->ActualWidth);
            VERIFY_ARE_EQUAL(expectedListBoxWidthSpecifiedHeight, listBoxWithSpecifiedSize->ActualHeight);
        });
    }

    void ListBoxIntegrationTests::ValidateCtrlAScenariosWithVirtualization()
    {
        ValidateCtrlAScenarios(s_itemCountWithVirtualization);
    }

    void ListBoxIntegrationTests::ValidateCtrlAScenariosWithoutVirtualization()
    {
        ValidateCtrlAScenarios(s_itemCountWithoutVirtualization);
    }

    void ListBoxIntegrationTests::ValidateCtrlAScenarios(const int itemCount)
    {
        TestCleanupWrapper cleanup;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, itemCount);

        // setting the focus on the listbox
        RunOnUIThread([&]()
        {
            listBox->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            listBox->SelectedIndex = 0;
            listBox->SelectionMode = xaml_controls::SelectionMode::Multiple;
        });
        TestServices::WindowHelper->WaitForIdle();

        // ctrl + A -> should SelectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(listBox->SelectedItems->Size, static_cast<unsigned int>(itemCount));
        });
        TestServices::WindowHelper->WaitForIdle();

        // ctrl + A -> should DeselectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(listBox->SelectedItems->Size, static_cast<unsigned int>(0));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ListBoxIntegrationTests::ValidateListBoxItemVisibilityChanged()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListBoxItem^ listBoxItem = nullptr;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        xaml::VisualStateGroup^ commonStatesVisualStateGroup;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            listBoxItem = safe_cast<xaml_controls::ListBoxItem^>(listBox->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listBoxItem);

            // go to state that is different than Normal state
            // since Visibility change forces the normal state when item is not actually selected
            VisualStateManager::GoToState(listBoxItem, "Selected", false);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Register for VisualStateGroup.CurrentStateChanged
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listBoxItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);

            VERIFY_IS_TRUE(groups->Size > 0);

            commonStatesVisualStateGroup = groups->GetAt(0);

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                stateChangedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // changing the visibility will change the visual state
            listBoxItem->Visibility = xaml::Visibility::Collapsed;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(stateChangedEvent->HasFired());
    }

    void ListBoxIntegrationTests::ValidateListBoxItemRightTapped()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListBoxItem^ listBoxItem = nullptr;
        auto listBox = ItemsControlHelper::AddItemsControl<xaml_controls::ListBox>(false /* addItemsDirectly */, s_itemCountWithoutVirtualization);

        auto rightTappedEvent = std::make_shared<Event>();
        auto rightTappedRegistration = CreateSafeEventRegistration(xaml_controls::ListBoxItem, RightTapped);

        // get the listboxitem and attach the righttapped handler
        RunOnUIThread([&]()
        {
            listBoxItem = safe_cast<xaml_controls::ListBoxItem^>(listBox->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listBoxItem);

            rightTappedRegistration.Attach(listBoxItem, ref new xaml::Input::RightTappedEventHandler(
                [rightTappedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                rightTappedEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Hold(listBoxItem);
        rightTappedEvent->WaitForDefault();
    }

    // Validates ability to navigate to last item and remain there with the keyboard.
    void ListBoxIntegrationTests::CanNavigateToAndRemainOnLastItem()
    {
        TestCleanupWrapper cleanup;

        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListBox, Loaded);
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListBox, SelectionChanged);
        auto loadedEvent = std::make_shared<Event>();
        auto selectionChangedEvent = std::make_shared<Event>();
        xaml_controls::ListBox^ listBox = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Setting up UI with ListBox.");
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <ListBox x:Name="listBox" Width="100" Height="200" BorderThickness="0">
                            <ListBoxItem Height="40">Item1</ListBoxItem>
                            <ListBoxItem Height="40">Item2</ListBoxItem>
                            <ListBoxItem Height="40">Item3</ListBoxItem>
                            <ListBoxItem Height="40">Item4</ListBoxItem>
                            <ListBoxItem Height="40">Item5</ListBoxItem>
                            <ListBoxItem Height="40">Item6</ListBoxItem>
                        </ListBox>
                    </StackPanel>)"));

            listBox = safe_cast<xaml_controls::ListBox^>(rootPanel->FindName(L"listBox"));
            VERIFY_IS_NOT_NULL(listBox);

            loadedRegistration.Attach(listBox, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            selectionChangedRegistration.Attach(listBox, ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                selectionChangedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Waiting for ListBox to be loaded.");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        selectionChangedEvent->Reset();

        LOG_OUTPUT(L"Selecting 5th item.");
        SelectItemAtIndex(listBox, 4, false /*doTap*/);
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ListBox.SelectedIndex: %d.", listBox->SelectedIndex);
            VERIFY_ARE_EQUAL(4, listBox->SelectedIndex);

            scrollViewer = dynamic_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listBox, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewer);

            LOG_OUTPUT(L"ScrollViewer.VerticalOffset: %lf.", scrollViewer->VerticalOffset);
            VERIFY_ARE_EQUAL(2, scrollViewer->VerticalOffset);
        });

        selectionChangedEvent->Reset();

        LOG_OUTPUT(L"Navigating to 6th and last item with down arrow key.");
        TestServices::KeyboardHelper->Down();
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ListBox.SelectedIndex: %d.", listBox->SelectedIndex);
            VERIFY_ARE_EQUAL(5, listBox->SelectedIndex);

            LOG_OUTPUT(L"ScrollViewer.VerticalOffset: %lf.", scrollViewer->VerticalOffset);
            VERIFY_ARE_EQUAL(3, scrollViewer->VerticalOffset);
        });

        LOG_OUTPUT(L"Pressing down arrow key a couple times again.");
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"ListBox.SelectedIndex: %d.", listBox->SelectedIndex);
            VERIFY_ARE_EQUAL(5, listBox->SelectedIndex);

            // Expecting the vertical offset to still be 3 after reaching the end of the list.
            LOG_OUTPUT(L"ScrollViewer.VerticalOffset: %lf.", scrollViewer->VerticalOffset);
            VERIFY_ARE_EQUAL(3, scrollViewer->VerticalOffset);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ListBox
