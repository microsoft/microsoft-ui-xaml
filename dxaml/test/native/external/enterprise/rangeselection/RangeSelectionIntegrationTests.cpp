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