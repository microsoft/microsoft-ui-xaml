// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Reorders of selected items (ListViewBase::ReorderItemsTo), including SelectionChanged handlers that change the
// items, the selection mode or ItemsSource while the items are moved or re-selected.

#include "pch.h"
#include "RangeSelectionIntegrationTests.h"

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace RangeSelection {

    namespace
    {
        using ItemVector = Platform::Collections::Vector<Platform::Object^>;
        using ListViewHandler = std::function<void(xaml_controls::ListView^, ItemVector^)>;

        enum class HandlerTrigger
        {
            None,
            // The first SelectionChanged that removes an item. Raised while the items are moved, because removing a
            // selected item unselects it.
            WhileMoving,
            // The first SelectionChanged that adds an item. Raised while the moved items are re-selected.
            WhileReselecting,
        };

        std::wstring ItemName(int index)
        {
            return L"Item " + std::to_wstring(index);
        }

        std::vector<std::wstring> ItemNames(std::initializer_list<int> indexes)
        {
            std::vector<std::wstring> names;
            for (const int index : indexes)
            {
                names.push_back(ItemName(index));
            }
            return names;
        }

        std::vector<std::wstring> ItemNames(int first, int count, const wchar_t* prefix = L"Item ")
        {
            std::vector<std::wstring> names;
            for (int i = first; i < first + count; ++i)
            {
                names.push_back(prefix + std::to_wstring(i));
            }
            return names;
        }

        // Every item is boxed separately, so equal names are equal values but different objects.
        ItemVector^ CreateItems(const std::vector<std::wstring>& names)
        {
            auto items = ref new ItemVector();
            for (const auto& name : names)
            {
                items->Append(ref new Platform::String(name.c_str()));
            }
            return items;
        }

        int IndexOfItem(::Windows::Foundation::Collections::IVector<Platform::Object^>^ items, Platform::String^ name)
        {
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                if (Platform::String::CompareOrdinal(safe_cast<Platform::String^>(items->GetAt(i)), name) == 0)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        void RemoveItem(ItemVector^ items, const std::wstring& name)
        {
            const int index = IndexOfItem(items, ref new Platform::String(name.c_str()));
            VERIFY_IS_GREATER_THAN_OR_EQUAL(index, 0);
            items->RemoveAt(static_cast<unsigned int>(index));
        }

        // Verifies the ListView's current ItemsSource, and that SelectedItems, SelectedRanges and the containers all
        // agree on exactly the expected selected rows.
        void VerifyItemsAndSelection(
            xaml_controls::ListView^ listView,
            const std::vector<std::wstring>& expectedItems,
            const std::vector<int>& expectedSelectedIndexes)
        {
            // ItemsSource, or the ListView's own Items when no ItemsSource is set.
            ::Windows::Foundation::Collections::IVector<Platform::Object^>^ items = listView->ItemsSource
                ? safe_cast<::Windows::Foundation::Collections::IVector<Platform::Object^>^>(listView->ItemsSource)
                : listView->Items;
            std::wstring actualItems;
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                actualItems += std::wstring(safe_cast<Platform::String^>(items->GetAt(i))->Data()) + L"; ";
            }
            LOG_OUTPUT(L"Items: %s", actualItems.c_str());

            VERIFY_ARE_EQUAL(static_cast<unsigned int>(expectedItems.size()), items->Size);
            for (unsigned int i = 0; i < items->Size; ++i)
            {
                VERIFY_ARE_EQUAL(0, Platform::String::CompareOrdinal(safe_cast<Platform::String^>(items->GetAt(i)), ref new Platform::String(expectedItems[i].c_str())));
            }

            const auto isExpectedSelected = [&](int index)
            {
                return std::find(expectedSelectedIndexes.begin(), expectedSelectedIndexes.end(), index) != expectedSelectedIndexes.end();
            };

            VERIFY_ARE_EQUAL(static_cast<unsigned int>(expectedSelectedIndexes.size()), listView->SelectedItems->Size);
            for (const int index : expectedSelectedIndexes)
            {
                VERIFY_ARE_NOT_EQUAL(-1, IndexOfItem(listView->SelectedItems, safe_cast<Platform::String^>(items->GetAt(index))));
            }

            unsigned int selectedRangesLength = 0;
            std::wstring actualRanges;
            for (unsigned int i = 0; i < listView->SelectedRanges->Size; ++i)
            {
                auto range = listView->SelectedRanges->GetAt(i);
                actualRanges += std::to_wstring(range->FirstIndex) + L"-" + std::to_wstring(range->LastIndex) + L"; ";
            }
            std::wstring actualSelectedItems;
            for (unsigned int i = 0; i < listView->SelectedItems->Size; ++i)
            {
                actualSelectedItems += std::wstring(safe_cast<Platform::String^>(listView->SelectedItems->GetAt(i))->Data()) + L"; ";
            }
            LOG_OUTPUT(L"SelectedRanges: %s SelectedItems: %s", actualRanges.c_str(), actualSelectedItems.c_str());

            for (unsigned int i = 0; i < listView->SelectedRanges->Size; ++i)
            {
                auto range = listView->SelectedRanges->GetAt(i);
                selectedRangesLength += range->Length;
                for (int index = range->FirstIndex; index <= range->LastIndex; ++index)
                {
                    VERIFY_IS_TRUE(isExpectedSelected(index));
                }
            }
            VERIFY_ARE_EQUAL(static_cast<unsigned int>(expectedSelectedIndexes.size()), selectedRangesLength);

            for (unsigned int i = 0; i < items->Size; ++i)
            {
                auto container = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
                if (container)
                {
                    VERIFY_ARE_EQUAL(isExpectedSelected(static_cast<int>(i)), container->IsSelected);
                }
            }
        }

        struct KeyboardReorderScenario
        {
            std::vector<std::wstring> items = ItemNames(0, 10);
            // Puts the items in ListView.Items instead of an ItemsSource. Its IndexOf compares values rather than
            // references, like most collections (for example .NET's), which matters for duplicate values.
            bool useItemsCollection = false;
            xaml_controls::ListViewSelectionMode selectionMode = xaml_controls::ListViewSelectionMode::Extended;
            std::vector<int> selectedIndexes = { 1, 2, 3 };
            int focusedIndex = 3;
            bool moveDown = true;

            HandlerTrigger trigger = HandlerTrigger::None;
            ListViewHandler handler;

            // The expected state afterwards. expectedItems is compared with the ListView's ItemsSource at that point.
            std::vector<std::wstring> expectedItems;
            std::vector<int> expectedSelectedIndexes;
            int expectedFocusedIndex = -1; // -1: focus isn't checked.
            ListViewHandler additionalChecks; // Receives the original items.
        };

        // Selects the scenario's items, focuses one of them and moves the focused item (with the selected items, if it's
        // selected) with Alt+Shift+Down or Alt+Shift+Up.
        void RunKeyboardReorderScenario(const KeyboardReorderScenario& scenario)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::ListView^ listView = nullptr;
            ItemVector^ items = nullptr;
            auto isArmed = std::make_shared<bool>(false);
            ::Windows::Foundation::EventRegistrationToken selectionChangedToken = {};

            RunOnUIThread([&]()
            {
                listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                    L"<ListView Height='600' Width='200' CanDragItems='True' CanReorderItems='True' AllowDrop='True' "
                    L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));
                listView->SelectionMode = scenario.selectionMode;

                if (scenario.useItemsCollection)
                {
                    for (const auto& name : scenario.items)
                    {
                        listView->Items->Append(ref new Platform::String(name.c_str()));
                    }
                }
                else
                {
                    items = CreateItems(scenario.items);
                    listView->ItemsSource = items;
                }
                TestServices::WindowHelper->WindowContent = listView;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                for (const int index : scenario.selectedIndexes)
                {
                    if (scenario.selectionMode == xaml_controls::ListViewSelectionMode::Single)
                    {
                        listView->SelectedIndex = index;
                    }
                    else
                    {
                        listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(index, 1));
                    }
                }
                VERIFY_ARE_EQUAL(static_cast<unsigned int>(scenario.selectedIndexes.size()), listView->SelectedItems->Size);

                auto focusedContainer = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(scenario.focusedIndex));
                VERIFY_IS_NOT_NULL(focusedContainer);
                focusedContainer->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            if (scenario.trigger != HandlerTrigger::None)
            {
                RunOnUIThread([&]()
                {
                    auto localListView = listView;
                    auto localItems = items;
                    const auto trigger = scenario.trigger;
                    const auto handler = scenario.handler;
                    selectionChangedToken = listView->SelectionChanged += ref new xaml_controls::SelectionChangedEventHandler(
                        [isArmed, localListView, localItems, trigger, handler](Platform::Object^, xaml_controls::SelectionChangedEventArgs^ args)
                        {
                            const bool isTrigger = (trigger == HandlerTrigger::WhileMoving) ? args->RemovedItems->Size > 0 : args->AddedItems->Size > 0;
                            if (*isArmed && isTrigger)
                            {
                                *isArmed = false;
                                LOG_OUTPUT(L"SelectionChanged handler runs.");
                                handler(localListView, localItems);
                            }
                        });
                    *isArmed = true;
                });
            }

            LOG_OUTPUT(L"Moving the focused item with Shift/Alt/%s.", scenario.moveDown ? L"Down" : L"Up");
            TestServices::KeyboardHelper->PressKeySequence(scenario.moveDown
                ? L"$d$_shift#$d$_alt#$d$_down#$u$_down#$u$_alt#$u$_shift"
                : L"$d$_shift#$d$_alt#$d$_up#$u$_up#$u$_alt#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                if (scenario.trigger != HandlerTrigger::None)
                {
                    listView->SelectionChanged -= selectionChangedToken;
                    VERIFY_IS_FALSE(*isArmed);
                }

                VerifyItemsAndSelection(listView, scenario.expectedItems, scenario.expectedSelectedIndexes);

                if (scenario.expectedFocusedIndex >= 0)
                {
                    VERIFY_IS_TRUE(listView->ContainerFromIndex(scenario.expectedFocusedIndex)->Equals(xaml_input::FocusManager::GetFocusedElement(listView->XamlRoot)));
                }

                if (scenario.additionalChecks)
                {
                    scenario.additionalChecks(listView, items);
                }
            });

            // this applies only to Desktop runs
            if (TestServices::Utilities->IsDesktop)
            {
                // Workaround for a known issue
                TestServices::WindowHelper->RestoreForegroundWindow();
            }
        }

        // Selects items 1-3 of twenty and drags item 2 down by 200 pixels. checks receives the original items.
        void RunDragReorderScenario(HandlerTrigger trigger, const ListViewHandler& handler, const ListViewHandler& checks)
        {
            TestCleanupWrapper cleanup;

            xaml_controls::ListView^ listView = nullptr;
            ItemVector^ items = nullptr;
            xaml::FrameworkElement^ draggedContainer = nullptr;
            auto isArmed = std::make_shared<bool>(false);
            ::Windows::Foundation::EventRegistrationToken selectionChangedToken = {};

            RunOnUIThread([&]()
            {
                listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                    L"<ListView Height='500' Width='200' SelectionMode='Extended' CanDragItems='True' CanReorderItems='True' AllowDrop='True' "
                    L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));

                items = CreateItems(ItemNames(0, 20));
                listView->ItemsSource = items;
                TestServices::WindowHelper->WindowContent = listView;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(1, 3));
                VERIFY_ARE_EQUAL(3u, listView->SelectedItems->Size);

                draggedContainer = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(2));
                VERIFY_IS_NOT_NULL(draggedContainer);

                auto localListView = listView;
                auto localItems = items;
                selectionChangedToken = listView->SelectionChanged += ref new xaml_controls::SelectionChangedEventHandler(
                    [isArmed, localListView, localItems, trigger, handler](Platform::Object^, xaml_controls::SelectionChangedEventArgs^ args)
                    {
                        const bool isTrigger = (trigger == HandlerTrigger::WhileMoving) ? args->RemovedItems->Size > 0 : args->AddedItems->Size > 0;
                        if (*isArmed && isTrigger)
                        {
                            *isArmed = false;
                            LOG_OUTPUT(L"SelectionChanged handler runs.");
                            handler(localListView, localItems);
                        }
                    });
                *isArmed = true;
            });

            LOG_OUTPUT(L"Dragging the three selected items further down the list...");
            TestServices::InputHelper->DragFromCenter(draggedContainer, 0, 200, 0.1);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                listView->SelectionChanged -= selectionChangedToken;
                VERIFY_IS_FALSE(*isArmed);
                checks(listView, items);
            });

            // this applies only to Desktop runs
            if (TestServices::Utilities->IsDesktop)
            {
                // Workaround for a known issue
                TestServices::WindowHelper->RestoreForegroundWindow();
            }
        }

        // The default keyboard scenario (items 1-3 selected, item 3 focused, Alt+Shift+Down) moves the selected items past
        // item 4. With handler, the given handler runs at trigger.
        KeyboardReorderScenario DefaultScenario(HandlerTrigger trigger = HandlerTrigger::None, ListViewHandler handler = nullptr)
        {
            KeyboardReorderScenario scenario;
            scenario.trigger = trigger;
            scenario.handler = handler;
            scenario.expectedItems = ItemNames({ 0, 4, 1, 2, 3, 5, 6, 7, 8, 9 });
            scenario.expectedSelectedIndexes = { 2, 3, 4 };
            scenario.expectedFocusedIndex = 4;
            return scenario;
        }
    }

    //
    // SelectedRanges after collection changes (a reorder of an unselected item only inserts and removes items)
    //

    void RangeSelectionIntegrationTests::ValidateSelectedRangesAfterItemsChange()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        ItemVector^ items = nullptr;

        RunOnUIThread([&]()
        {
            listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                L"<ListView Height='600' Width='200' SelectionMode='Extended' "
                L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));
            items = CreateItems(ItemNames(0, 10));
            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(2, 3));
            VerifyItemsAndSelection(listView, ItemNames(0, 10), { 2, 3, 4 });

            LOG_OUTPUT(L"Inserting an item before the selected items.");
            items->InsertAt(0, ref new Platform::String(L"New"));
            std::vector<std::wstring> expectedItems = ItemNames(0, 10);
            expectedItems.insert(expectedItems.begin(), L"New");
            VerifyItemsAndSelection(listView, expectedItems, { 3, 4, 5 });

            LOG_OUTPUT(L"Removing an item before the selected items.");
            items->RemoveAt(0);
            VerifyItemsAndSelection(listView, ItemNames(0, 10), { 2, 3, 4 });

            LOG_OUTPUT(L"Inserting an item between the selected items.");
            items->InsertAt(3, ref new Platform::String(L"New"));
            expectedItems = ItemNames(0, 10);
            expectedItems.insert(expectedItems.begin() + 3, L"New");
            VerifyItemsAndSelection(listView, expectedItems, { 2, 4, 5 });
        });
    }
    //
    // Reorders without handlers
    //

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderDownKeepsSelection()
    {
        RunKeyboardReorderScenario(DefaultScenario());
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderUpKeepsSelection()
    {
        KeyboardReorderScenario scenario;
        scenario.selectedIndexes = { 2, 3, 4 };
        scenario.focusedIndex = 2;
        scenario.moveDown = false;
        scenario.expectedItems = ItemNames({ 0, 2, 3, 4, 1, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 1, 2, 3 };
        scenario.expectedFocusedIndex = 1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderOfNonContiguousSelection()
    {
        KeyboardReorderScenario scenario;
        scenario.selectedIndexes = { 1, 3 };
        scenario.focusedIndex = 3;
        scenario.expectedItems = ItemNames({ 0, 2, 4, 1, 3, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 3, 4 };
        scenario.expectedFocusedIndex = 4;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderInMultipleSelectionMode()
    {
        auto scenario = DefaultScenario();
        scenario.selectionMode = xaml_controls::ListViewSelectionMode::Multiple;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderInSingleSelectionMode()
    {
        KeyboardReorderScenario scenario;
        scenario.selectionMode = xaml_controls::ListViewSelectionMode::Single;
        scenario.selectedIndexes = { 2 };
        scenario.focusedIndex = 2;
        scenario.expectedItems = ItemNames({ 0, 1, 3, 2, 4, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 3 };
        scenario.expectedFocusedIndex = 3;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderOfUnselectedFocusedItem()
    {
        // Only the focused item moves, and the selected items keep their selection.
        KeyboardReorderScenario scenario;
        scenario.focusedIndex = 0;
        scenario.expectedItems = ItemNames({ 1, 0, 2, 3, 4, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 0, 2, 3 };
        scenario.expectedFocusedIndex = 1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderWithDuplicateItems()
    {
        auto scenario = DefaultScenario();
        scenario.items = { L"Item 0", L"Item 1", L"Dup", L"Item 3", L"Item 4", L"Item 5", L"Dup", L"Item 7", L"Item 8", L"Item 9" };
        scenario.expectedItems = { L"Item 0", L"Item 4", L"Item 1", L"Dup", L"Item 3", L"Item 5", L"Dup", L"Item 7", L"Item 8", L"Item 9" };
        RunKeyboardReorderScenario(scenario);
    }

    //
    // Handlers that run while the moved items are re-selected
    //

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerRemovesItemBefore()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^, ItemVector^ items)
        {
            RemoveItem(items, ItemName(0));
        });
        scenario.expectedItems = ItemNames({ 4, 1, 2, 3, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 1, 2, 3 };
        scenario.expectedFocusedIndex = 3;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerRemovesItemAfter()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^, ItemVector^ items)
        {
            RemoveItem(items, ItemName(5));
        });
        scenario.expectedItems = ItemNames({ 0, 4, 1, 2, 3, 6, 7, 8, 9 });
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerRemovesMovedItem()
    {
        // Item 3 is removed before it's re-selected; the other moved items stay selected.
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^, ItemVector^ items)
        {
            RemoveItem(items, ItemName(3));
        });
        scenario.expectedItems = ItemNames({ 0, 4, 1, 2, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 2, 3 };
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerInsertsItem()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^, ItemVector^ items)
        {
            items->InsertAt(0, ref new Platform::String(L"New"));
        });
        scenario.expectedItems = { L"New", L"Item 0", L"Item 4", L"Item 1", L"Item 2", L"Item 3", L"Item 5", L"Item 6", L"Item 7", L"Item 8", L"Item 9" };
        scenario.expectedSelectedIndexes = { 3, 4, 5 };
        scenario.expectedFocusedIndex = 5;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerInsertsItemWithDuplicates()
    {
        // After the insertion, index 3 holds Item 1, and an equal "Dup" sits at index 0. The moved "Dup" (now at index 4)
        // must be the one that's selected. The items are in ListView.Items, whose IndexOf finds the first equal value.
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->Items->InsertAt(1, ref new Platform::String(L"New"));
        });
        scenario.useItemsCollection = true;
        scenario.items = { L"Dup", L"Item 1", L"Dup", L"Item 3", L"Item 4", L"Item 5", L"Item 6", L"Item 7", L"Item 8", L"Item 9" };
        scenario.expectedItems = { L"Dup", L"New", L"Item 4", L"Item 1", L"Dup", L"Item 3", L"Item 5", L"Item 6", L"Item 7", L"Item 8", L"Item 9" };
        scenario.expectedSelectedIndexes = { 3, 4, 5 };
        scenario.expectedFocusedIndex = 5;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerReplacesItemsSource()
    {
        // The new source is left as it is: nothing from the old source is moved or selected in it.
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->ItemsSource = CreateItems(ItemNames(0, 10, L"New "));
        });
        scenario.expectedItems = ItemNames(0, 10, L"New ");
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        scenario.additionalChecks = [](xaml_controls::ListView^, ItemVector^ originalItems)
        {
            // The moves were completed in the original source before the handler ran.
            VERIFY_ARE_EQUAL(10u, originalItems->Size);
            VERIFY_ARE_EQUAL(1, IndexOfItem(originalItems, L"Item 4"));
        };
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerClearsItems()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^, ItemVector^ items)
        {
            items->Clear();
        });
        scenario.expectedItems = {};
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerSwitchesToSingleSelectionMode()
    {
        // The handler's selection is kept; no other moved item is selected after it.
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Single;
            listView->SelectedIndex = 0;
        });
        scenario.expectedSelectedIndexes = { 0 };
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerSwitchesToNoneSelectionMode()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::None;
        });
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }


    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerSelectsAnotherItem()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(9, 1));
        });
        scenario.expectedSelectedIndexes = { 2, 3, 4, 9 };
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerClearsSelection()
    {
        // The handler's change to the item it was notified about (Item 1) is kept; the moved items that weren't
        // re-selected yet are still re-selected.
        auto scenario = DefaultScenario(HandlerTrigger::WhileReselecting, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectedItems->Clear();
        });
        scenario.expectedSelectedIndexes = { 3, 4 };
        RunKeyboardReorderScenario(scenario);
    }
    //
    // Handlers that run while the items are moved. They can't change the items: ItemsControl rejects changes to the
    // items while it handles a change to them. They can change the selection mode and the selection.
    //

    void RangeSelectionIntegrationTests::ValidateReorderWhenMoveHandlerSwitchesToSingleSelectionMode()
    {
        // In Single mode, the first moved item that was selected is selected again.
        auto scenario = DefaultScenario(HandlerTrigger::WhileMoving, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Single;
            listView->SelectedIndex = 0;
        });
        scenario.expectedSelectedIndexes = { 2 };
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenMoveHandlerSwitchesToNoneSelectionMode()
    {
        auto scenario = DefaultScenario(HandlerTrigger::WhileMoving, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::None;
        });
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenMoveHandlerClearsSelection()
    {
        // The moved items get the selection they had before the reorder, whatever happened to it while they moved.
        auto scenario = DefaultScenario(HandlerTrigger::WhileMoving, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectedItems->Clear();
        });
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenMoveHandlerSwitchesToMultipleSelectionMode()
    {
        // Changing the selection mode unselects SelectedItem; the moved items still get their selection back.
        auto scenario = DefaultScenario(HandlerTrigger::WhileMoving, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Multiple;
        });
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenMoveHandlerSelectsAnotherItem()
    {
        // Item 0 is selected; it isn't moved, so its index is the same during and after the moves.
        auto scenario = DefaultScenario(HandlerTrigger::WhileMoving, [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(0, 1));
        });
        scenario.expectedSelectedIndexes = { 0, 2, 3, 4 };
        RunKeyboardReorderScenario(scenario);
    }

    //
    // Drag and drop
    //

    void RangeSelectionIntegrationTests::ValidateDragReorderWhenReselectHandlerRemovesItem()
    {
        RunDragReorderScenario(
            HandlerTrigger::WhileReselecting,
            [](xaml_controls::ListView^, ItemVector^ items)
            {
                RemoveItem(items, ItemName(0));
            },
            [](xaml_controls::ListView^ listView, ItemVector^ items)
            {
                VERIFY_ARE_EQUAL(19u, items->Size);
                VERIFY_ARE_EQUAL(-1, IndexOfItem(items, L"Item 0"));

                const int firstIndex = IndexOfItem(items, L"Item 1");
                LOG_OUTPUT(L"The dragged items now start at index %d.", firstIndex);
                VERIFY_IS_GREATER_THAN(firstIndex, 0);
                VERIFY_ARE_EQUAL(firstIndex + 1, IndexOfItem(items, L"Item 2"));
                VERIFY_ARE_EQUAL(firstIndex + 2, IndexOfItem(items, L"Item 3"));

                std::vector<std::wstring> expectedItems;
                for (unsigned int i = 0; i < items->Size; ++i)
                {
                    expectedItems.push_back(safe_cast<Platform::String^>(items->GetAt(i))->Data());
                }
                VerifyItemsAndSelection(listView, expectedItems, { firstIndex, firstIndex + 1, firstIndex + 2 });
            });
    }

    void RangeSelectionIntegrationTests::ValidateDragReorderWhenReselectHandlerReplacesItemsSource()
    {
        RunDragReorderScenario(
            HandlerTrigger::WhileReselecting,
            [](xaml_controls::ListView^ listView, ItemVector^)
            {
                listView->ItemsSource = CreateItems(ItemNames(0, 20, L"New "));
            },
            [](xaml_controls::ListView^ listView, ItemVector^ originalItems)
            {
                VerifyItemsAndSelection(listView, ItemNames(0, 20, L"New "), {});
                VERIFY_ARE_EQUAL(20u, originalItems->Size);
            });
    }

} } } } } }
