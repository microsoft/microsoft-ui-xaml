// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

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

        std::vector<std::wstring> ItemNames(std::initializer_list<int> indexes)
        {
            std::vector<std::wstring> names;
            for (const int index : indexes)
            {
                names.push_back(L"Item " + std::to_wstring(index));
            }
            return names;
        }

        ItemVector^ CreateItems(const std::vector<std::wstring>& names)
        {
            auto items = ref new ItemVector();
            for (const auto& name : names)
            {
                items->Append(ref new Platform::String(name.c_str()));
            }
            return items;
        }

        struct KeyboardReorderScenario
        {
            std::vector<std::wstring> items = ItemNames({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 });
            xaml_controls::ListViewSelectionMode selectionMode = xaml_controls::ListViewSelectionMode::Extended;
            std::vector<int> selectedIndexes = { 1, 2, 3 };
            int focusedIndex = 3;
            bool moveDown = true;
            ListViewHandler onReselect;
            std::vector<std::wstring> expectedItems = ItemNames({ 0, 4, 1, 2, 3, 5, 6, 7, 8, 9 });
            std::vector<int> expectedSelectedIndexes = { 2, 3, 4 };
            int expectedFocusedIndex = 4;
        };

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
                items = CreateItems(scenario.items);
                listView->ItemsSource = items;
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

            if (scenario.onReselect)
            {
                RunOnUIThread([&]()
                {
                    const auto handler = scenario.onReselect;
                    auto localItems = items;
                    auto localListView = listView;
                    selectionChangedToken = listView->SelectionChanged += ref new xaml_controls::SelectionChangedEventHandler(
                        [isArmed, localListView, localItems, handler](Platform::Object^, xaml_controls::SelectionChangedEventArgs^ args)
                        {
                            if (*isArmed && args->AddedItems->Size > 0)
                            {
                                *isArmed = false;
                                handler(localListView, localItems);
                            }
                        });
                    *isArmed = true;
                });
            }

            TestServices::KeyboardHelper->PressKeySequence(scenario.moveDown
                ? L"$d$_shift#$d$_alt#$d$_down#$u$_down#$u$_alt#$u$_shift"
                : L"$d$_shift#$d$_alt#$d$_up#$u$_up#$u$_alt#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                if (scenario.onReselect)
                {
                    listView->SelectionChanged -= selectionChangedToken;
                    VERIFY_IS_FALSE(*isArmed);
                }

                auto currentItems = safe_cast<::Windows::Foundation::Collections::IVector<Platform::Object^>^>(listView->ItemsSource);
                VERIFY_ARE_EQUAL(static_cast<unsigned int>(scenario.expectedItems.size()), currentItems->Size);
                for (unsigned int i = 0; i < currentItems->Size; ++i)
                {
                    VERIFY_ARE_EQUAL(0, Platform::String::CompareOrdinal(
                        ref new Platform::String(scenario.expectedItems[i].c_str()),
                        safe_cast<Platform::String^>(currentItems->GetAt(i))));
                }

                VERIFY_ARE_EQUAL(static_cast<unsigned int>(scenario.expectedSelectedIndexes.size()), listView->SelectedItems->Size);
                for (unsigned int i = 0; i < scenario.expectedSelectedIndexes.size(); ++i)
                {
                    VERIFY_ARE_EQUAL(0, Platform::String::CompareOrdinal(
                        safe_cast<Platform::String^>(currentItems->GetAt(scenario.expectedSelectedIndexes[i])),
                        safe_cast<Platform::String^>(listView->SelectedItems->GetAt(i))));
                }

                unsigned int selectedRangesLength = 0;
                for (unsigned int i = 0; i < listView->SelectedRanges->Size; ++i)
                {
                    auto range = listView->SelectedRanges->GetAt(i);
                    selectedRangesLength += range->Length;
                    for (int index = range->FirstIndex; index <= range->LastIndex; ++index)
                    {
                        VERIFY_IS_TRUE(std::find(scenario.expectedSelectedIndexes.begin(), scenario.expectedSelectedIndexes.end(), index)
                            != scenario.expectedSelectedIndexes.end());
                    }
                }
                VERIFY_ARE_EQUAL(static_cast<unsigned int>(scenario.expectedSelectedIndexes.size()), selectedRangesLength);

                for (unsigned int i = 0; i < currentItems->Size; ++i)
                {
                    auto container = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
                    if (container)
                    {
                        const bool expectedSelected = std::find(scenario.expectedSelectedIndexes.begin(), scenario.expectedSelectedIndexes.end(), i)
                            != scenario.expectedSelectedIndexes.end();
                        VERIFY_ARE_EQUAL(expectedSelected, container->IsSelected);
                    }
                }

                if (scenario.expectedFocusedIndex >= 0)
                {
                    VERIFY_IS_TRUE(listView->ContainerFromIndex(scenario.expectedFocusedIndex)->Equals(
                        xaml_input::FocusManager::GetFocusedElement(listView->XamlRoot)));
                }
            });

            if (TestServices::Utilities->IsDesktop)
            {
                TestServices::WindowHelper->RestoreForegroundWindow();
            }
        }
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
        scenario.expectedItems = ItemNames({ 0, 2, 4, 1, 3, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 3, 4 };
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderInMultipleSelectionMode()
    {
        KeyboardReorderScenario scenario;
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

    void RangeSelectionIntegrationTests::ValidateKeyboardReorderWithDuplicateItems()
    {
        KeyboardReorderScenario scenario;
        scenario.items[2] = L"Dup";
        scenario.items[6] = L"Dup";
        scenario.expectedItems[3] = L"Dup";
        scenario.expectedItems[6] = L"Dup";
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerRemovesItemBefore()
    {
        KeyboardReorderScenario scenario;
        scenario.onReselect = [](xaml_controls::ListView^, ItemVector^ items) { items->RemoveAt(0); };
        scenario.expectedItems = ItemNames({ 4, 1, 2, 3, 5, 6, 7, 8, 9 });
        scenario.expectedSelectedIndexes = { 1, 2, 3 };
        scenario.expectedFocusedIndex = 3;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerReplacesItemsSource()
    {
        KeyboardReorderScenario scenario;
        scenario.expectedItems = { L"New 0", L"New 1", L"New 2" };
        const auto replacement = scenario.expectedItems;
        scenario.onReselect = [replacement](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->ItemsSource = CreateItems(replacement);
        };
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerClearsItems()
    {
        KeyboardReorderScenario scenario;
        scenario.onReselect = [](xaml_controls::ListView^, ItemVector^ items) { items->Clear(); };
        scenario.expectedItems = {};
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerSwitchesToSingleSelectionMode()
    {
        KeyboardReorderScenario scenario;
        scenario.onReselect = [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Single;
            listView->SelectedIndex = 0;
        };
        scenario.expectedSelectedIndexes = { 0 };
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

    void RangeSelectionIntegrationTests::ValidateReorderWhenReselectHandlerSwitchesToNoneSelectionMode()
    {
        KeyboardReorderScenario scenario;
        scenario.onReselect = [](xaml_controls::ListView^ listView, ItemVector^)
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::None;
        };
        scenario.expectedSelectedIndexes = {};
        scenario.expectedFocusedIndex = -1;
        RunKeyboardReorderScenario(scenario);
    }

} } } } } }
