// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RangeSelectionIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace RangeSelection {

    namespace
    {
        int IndexOfItem(Platform::Collections::Vector<Platform::Object^>^ items, Platform::String^ name)
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

        // Loads an Extended ListView with ten items, selects items 1-3, focuses item 3 and presses Alt+Shift+Down,
        // which moves the three items past item 4. onFirstReselect runs once, from the first SelectionChanged
        // that re-selects one of the moved items.
        void ReorderSelectedItemsWithKeyboard(
            std::function<void(xaml_controls::ListView^, Platform::Collections::Vector<Platform::Object^>^)> onFirstReselect,
            xaml_controls::ListView^& listView,
            Platform::Collections::Vector<Platform::Object^>^& items)
        {
            auto isArmed = std::make_shared<bool>(false);
            ::Windows::Foundation::EventRegistrationToken selectionChangedToken = {};

            RunOnUIThread([&]()
            {
                listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                    L"<ListView Height='500' Width='200' SelectionMode='Extended' CanDragItems='True' CanReorderItems='True' AllowDrop='True' "
                    L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));

                items = ref new Platform::Collections::Vector<Platform::Object^>();
                for (int i = 0; i < 10; ++i)
                {
                    items->Append(ref new Platform::String((L"Item " + std::to_wstring(i)).c_str()));
                }

                listView->ItemsSource = items;
                TestServices::WindowHelper->WindowContent = listView;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(1, 3));
                VERIFY_ARE_EQUAL(3u, listView->SelectedItems->Size);

                auto localListView = listView;
                auto localItems = items;
                selectionChangedToken = listView->SelectionChanged += ref new xaml_controls::SelectionChangedEventHandler(
                    [isArmed, localListView, localItems, onFirstReselect](Platform::Object^, xaml_controls::SelectionChangedEventArgs^ args)
                    {
                        if (*isArmed && args->AddedItems->Size > 0)
                        {
                            *isArmed = false;
                            onFirstReselect(localListView, localItems);
                        }
                    });
                *isArmed = true;

                auto focusedContainer = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(3));
                VERIFY_IS_NOT_NULL(focusedContainer);
                focusedContainer->Focus(xaml::FocusState::Keyboard);
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Moving the three selected items down with Shift/Alt/Down.");
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#$d$_alt#$d$_down#$u$_down#$u$_alt#$u$_shift");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                listView->SelectionChanged -= selectionChangedToken;
                VERIFY_IS_FALSE(*isArmed);
            });
        }
    }
    bool RangeSelectionIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool RangeSelectionIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void RangeSelectionIntegrationTests::ValidateSelectedRangesAfterSelectedIndexChanged()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment(30);

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            listView->SelectedIndex = 5;

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned>(1));

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, listView->SelectedIndex);
            VERIFY_ARE_EQUAL(returnRange->Length, static_cast<unsigned>(1));
        });
    }

    void RangeSelectionIntegrationTests::ValidateSelectedRangesAfterSelectedItemReorder()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment(30);
        xaml::DependencyObject^ listViewItemAsDO = nullptr;

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            listView->CanReorderItems = true;
            listView->AllowDrop = true;

            listView->SelectedIndex = 5;

            listViewItemAsDO = listView->ContainerFromItem(listView->SelectedItem);
            VERIFY_IS_NOT_NULL(listViewItemAsDO);

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned>(1));

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, listView->SelectedIndex);
            VERIFY_ARE_EQUAL(returnRange->Length, static_cast<unsigned>(1));
        });

        TestServices::InputHelper->DragFromCenter(safe_cast<xaml::FrameworkElement^>(listViewItemAsDO), 0, 100, 0.1);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            VERIFY_ARE_NOT_EQUAL(listView->SelectedIndex, -1);

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned>(1));

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, listView->SelectedIndex);
            VERIFY_ARE_EQUAL(returnRange->Length, static_cast<unsigned>(1));
        });

        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }
    }

    void RangeSelectionIntegrationTests::ValidateSelectionAfterMultipleSelectedItemsReorder()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ items = nullptr;
        xaml::FrameworkElement^ draggedContainer = nullptr;
        Platform::String^ selectedNames[] = { L"Item 1", L"Item 2", L"Item 3" };

        RunOnUIThread([&]()
        {
            listView = safe_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                L"<ListView Height='500' Width='200' SelectionMode='Extended' CanDragItems='True' CanReorderItems='True' AllowDrop='True' "
                L"xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));

            items = ref new Platform::Collections::Vector<Platform::Object^>();
            for (int i = 0; i < 20; ++i)
            {
                items->Append(ref new Platform::String((L"Item " + std::to_wstring(i)).c_str()));
            }

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
        });

        LOG_OUTPUT(L"Dragging the three selected items further down the list...");
        TestServices::InputHelper->DragFromCenter(draggedContainer, 0, 200, 0.1);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto indexOf = [&](Platform::String^ name)
            {
                for (unsigned int i = 0; i < items->Size; ++i)
                {
                    if (Platform::String::CompareOrdinal(safe_cast<Platform::String^>(items->GetAt(i)), name) == 0)
                    {
                        return static_cast<int>(i);
                    }
                }
                return -1;
            };

            const int firstIndex = indexOf(selectedNames[0]);
            LOG_OUTPUT(L"The dragged items now start at index %d.", firstIndex);
            VERIFY_IS_GREATER_THAN(firstIndex, 1);

            // The dragged items keep their relative order and all of them stay selected.
            VERIFY_ARE_EQUAL(3u, listView->SelectedItems->Size);
            for (int i = 0; i < 3; ++i)
            {
                VERIFY_ARE_EQUAL(firstIndex + i, indexOf(selectedNames[i]));

                bool isInSelectedItems = false;
                for (unsigned int j = 0; j < listView->SelectedItems->Size; ++j)
                {
                    isInSelectedItems |= Platform::String::CompareOrdinal(safe_cast<Platform::String^>(listView->SelectedItems->GetAt(j)), selectedNames[i]) == 0;
                }
                VERIFY_IS_TRUE(isInSelectedItems);

                auto container = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(firstIndex + i));
                VERIFY_IS_NOT_NULL(container);
                VERIFY_IS_TRUE(container->IsSelected);
            }

            VERIFY_ARE_EQUAL(1u, listView->SelectedRanges->Size);
            VERIFY_ARE_EQUAL(firstIndex, listView->SelectedRanges->GetAt(0)->FirstIndex);
            VERIFY_ARE_EQUAL(3u, listView->SelectedRanges->GetAt(0)->Length);
        });

        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }
    }

    void RangeSelectionIntegrationTests::ValidateReorderReselectionAfterItemsChangeInSelectionChanged()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ items = nullptr;

        ReorderSelectedItemsWithKeyboard(
            [](xaml_controls::ListView^, Platform::Collections::Vector<Platform::Object^>^ items)
            {
                LOG_OUTPUT(L"SelectionChanged handler: removing item 0.");
                items->RemoveAt(0);
            },
            listView,
            items);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(-1, IndexOfItem(items, L"Item 0"));
            VERIFY_ARE_EQUAL(0, IndexOfItem(items, L"Item 4"));

            // The moved items keep their order and their selection.
            VERIFY_ARE_EQUAL(3u, listView->SelectedItems->Size);
            for (int i = 1; i <= 3; ++i)
            {
                auto name = ref new Platform::String((L"Item " + std::to_wstring(i)).c_str());
                VERIFY_ARE_EQUAL(i, IndexOfItem(items, name));

                auto container = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(container);
                VERIFY_IS_TRUE(container->IsSelected);
            }

            // Keyboard focus stays on the item that had it before the reorder.
            VERIFY_IS_TRUE(listView->ContainerFromIndex(3)->Equals(xaml_input::FocusManager::GetFocusedElement(listView->XamlRoot)));
        });

        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }
    }

    void RangeSelectionIntegrationTests::ValidateSelectAll()
    {
        TestCleanupWrapper cleanup;

        unsigned int size = 100;
        auto listView = SetupEnvironment(size);

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVector<Platform::Object^>^ selectedItems = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            listView->SelectAll();

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned>(1));

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, 0);
            VERIFY_ARE_EQUAL(returnRange->Length, size);

            selectedItems = listView->SelectedItems;
            VERIFY_IS_NOT_NULL(selectedItems);

            VERIFY_ARE_EQUAL(selectedItems->Size, size);
        });
    }

    void RangeSelectionIntegrationTests::ValidateSelectRange()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment(30);

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVector<Platform::Object^>^ selectedItems = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(8, 2));

            selectedItems = listView->SelectedItems;
            VERIFY_IS_NOT_NULL(selectedItems);

            VERIFY_ARE_EQUAL(selectedItems->Size, static_cast<unsigned>(2));

            VERIFY_ARE_EQUAL(listView->SelectedIndex, 8);

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned>(1));

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, 8);
            VERIFY_ARE_EQUAL(returnRange->Length, static_cast<unsigned>(2));
        });
    }

    void RangeSelectionIntegrationTests::ValidateDeselectRange()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment(30);

        RunOnUIThread([&]()
        {
            Microsoft::UI::Xaml::Data::ItemIndexRange^ returnRange = nullptr;
            ::Windows::Foundation::Collections::IVector<Platform::Object^>^ selectedItems = nullptr;
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = nullptr;

            // Selecting an initial range
            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(5, 20));

            listView->DeselectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(11, 3));

            selectedItems = listView->SelectedItems;
            VERIFY_IS_NOT_NULL(selectedItems);

            VERIFY_ARE_EQUAL(selectedItems->Size, (unsigned int)17);

            VERIFY_ARE_EQUAL(listView->SelectedIndex, 5);

            selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, (unsigned int)2);

            returnRange = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, 5);
            VERIFY_ARE_EQUAL(returnRange->Length, (unsigned int)6);

            returnRange = listView->SelectedRanges->GetAt(1);
            VERIFY_ARE_EQUAL(returnRange->FirstIndex, 14);
            VERIFY_ARE_EQUAL(returnRange->Length, (unsigned int)11);
        });
    }

    void RangeSelectionIntegrationTests::CanRangeSelectUnrealizedItemsWithShift()
    {
        TestCleanupWrapper cleanup;

        const int itemsCount = 1000;
        auto listView = SetupEnvironment(itemsCount);
        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        // Note: the first container is always pinned by the ItemsStackPanel.
        xaml::FrameworkElement^ secondContainer = nullptr;
        xaml::FrameworkElement^ thirdContainer = nullptr;
        xaml::FrameworkElement^ lastContainer = nullptr;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Multiple;
            secondContainer = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(1));
            thirdContainer = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(2));

            selectionChangedRegistration.Attach(listView, ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e)
            {
                selectionChangedEvent->Set();
            }));
        });

        LOG_OUTPUT(L"Selecting second container...");
        TestServices::InputHelper->Tap(secondContainer);
        selectionChangedEvent->WaitForDefault();

        LOG_OUTPUT(L"Selecting third container...");
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift");
        TestServices::InputHelper->Tap(thirdContainer);
        selectionChangedEvent->WaitForDefault();
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift");
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Scrolling to the end...");
        RunOnUIThread([&]()
        {
            listView->ScrollIntoView(listView->Items->GetAt(itemsCount - 1));
            listView->UpdateLayout();
            lastContainer = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(itemsCount - 1));
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift");
        TestServices::InputHelper->Tap(lastContainer);
        selectionChangedEvent->WaitForDefault();
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift");

        // So far so good, we didn't crash like we used to

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validating selection...");
            VERIFY_ARE_EQUAL(1u, listView->SelectedRanges->Size);
            auto range = listView->SelectedRanges->GetAt(0);
            VERIFY_ARE_EQUAL(1, range->FirstIndex);
            VERIFY_ARE_EQUAL(itemsCount - 1, static_cast<int>(range->Length));
            VERIFY_ARE_EQUAL(itemsCount - 1, static_cast<int>(listView->SelectedItems->Size));
        });
    }

    //
    // Private Methods
    //
    xaml_controls::ListView^ RangeSelectionIntegrationTests::SetupEnvironment(unsigned int size)
    {
        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<int>^ items = nullptr;

        RunOnUIThread([&]()
        {
            listView = dynamic_cast<xaml_controls::ListView^> (xaml_markup::XamlReader::Load(
                L"<ListView Height='500' Width='200' SelectionMode='Extended' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));
            VERIFY_IS_NOT_NULL(listView);

            items = ref new Platform::Collections::Vector<int>(size);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::RangeSelection