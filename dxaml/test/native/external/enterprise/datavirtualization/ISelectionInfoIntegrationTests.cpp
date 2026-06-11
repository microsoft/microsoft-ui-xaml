// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ISelectionInfoIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace DataVirtualization {

    bool ISelectionInfoIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ISelectionInfoIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ISelectionInfoIntegrationTests::CanInvokeDataSourceSelectRangeAfterSelectedIndexChanged()
    {
        TestCleanupWrapper cleanup;

        SelectionInfoStatics::Reset();

        auto listView = SetupEnvironment();

        RunOnUIThread([&]()
        {
            listView->SelectedIndex = 2;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
    }

    void ISelectionInfoIntegrationTests::CanInvokeDataSourceSelectRange()
    {
        TestCleanupWrapper cleanup;

        SelectionInfoStatics::Reset();

        auto listView = SetupEnvironment();
        auto listViewSelectionChangedEvent = std::make_shared<Event>();
        auto listViewSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        RunOnUIThread([&]()
        {
            listViewSelectionChangedRegistration.Attach(listView, ref new xaml_controls::SelectionChangedEventHandler(
                [listViewSelectionChangedEvent]
            (Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ args)
            {
                // check the added ranges
                VERIFY_IS_NULL(args->AddedItems);

                // check the removed ranges
                VERIFY_IS_NULL(args->RemovedItems);

                listViewSelectionChangedEvent->Set();
            }));

            listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(2, 5));
        });
        TestServices::WindowHelper->WaitForIdle();

        listViewSelectionChangedEvent->WaitForDefault();

        VERIFY_IS_TRUE(listViewSelectionChangedEvent->HasFired());

        VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
    }

    void ISelectionInfoIntegrationTests::CanInvokeDataSourceDeselectRange()
    {
        TestCleanupWrapper cleanup;

        SelectionInfoStatics::Reset();

        auto listView = SetupEnvironment();

        RunOnUIThread([&]()
        {
            listView->DeselectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(2, 5));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::DeselectRangeInvoked);
    }

    void ISelectionInfoIntegrationTests::CanInvokeDataSourceIsSelected()
    {
        TestCleanupWrapper cleanup;

        SelectionInfoStatics::Reset();

        auto listView = SetupEnvironment();

        RunOnUIThread([&]()
        {
            listView->ScrollIntoView(m_DataSource->GetAt(20));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::IsSelectedInvoked);
    }

    void ISelectionInfoIntegrationTests::CanInvokeDataSourceGetSelectedRanges()
    {
        TestCleanupWrapper cleanup;

        SelectionInfoStatics::Reset();

        auto listView = SetupEnvironment();

        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(listView->SelectedItems);

            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::GetSelectedRangesInvoked);
    }

    void ISelectionInfoIntegrationTests::ValidateSelectorItemIsSelectedValue()
    {
        TestCleanupWrapper cleanup;

        Microsoft::UI::Xaml::Controls::ListViewItem^ listViewItem = nullptr;

        RunOnUIThread([&]()
        {
            listViewItem = ref new Microsoft::UI::Xaml::Controls::ListViewItem();

            VERIFY_IS_FALSE(listViewItem->IsSelected);
            VERIFY_IS_FALSE(static_cast<bool>(listViewItem->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));

            listViewItem->IsSelected = true;
            VERIFY_IS_TRUE(listViewItem->IsSelected);
            VERIFY_IS_TRUE(static_cast<bool>(listViewItem->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));

            listViewItem->IsSelected = false;
            VERIFY_IS_FALSE(listViewItem->IsSelected);
            VERIFY_IS_FALSE(static_cast<bool>(listViewItem->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));
        });
    }

    void ISelectionInfoIntegrationTests::ValidateSelectorItemIsSelectedBinding()
    {
        TestCleanupWrapper cleanup;

        Microsoft::UI::Xaml::Controls::ListViewItem^ listViewItem1 = nullptr;
        Microsoft::UI::Xaml::Controls::ListViewItem^ listViewItem2 = nullptr;
        Microsoft::UI::Xaml::Data::Binding^ binding = nullptr;

        RunOnUIThread([&]()
        {
            listViewItem1 = ref new Microsoft::UI::Xaml::Controls::ListViewItem();
            listViewItem2 = ref new Microsoft::UI::Xaml::Controls::ListViewItem();
            binding = ref new Microsoft::UI::Xaml::Data::Binding();

            binding->Mode = Microsoft::UI::Xaml::Data::BindingMode::TwoWay;
            binding->Source = listViewItem2;
            binding->Path = ref new PropertyPath(L"IsSelected");

            listViewItem1->SetBinding(listViewItem1->IsSelectedProperty, binding);

            listViewItem2->IsSelected = false;
            VERIFY_IS_FALSE(listViewItem1->IsSelected);
            VERIFY_IS_FALSE(static_cast<bool>(listViewItem1->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));

            listViewItem2->IsSelected = true;
            VERIFY_IS_TRUE(listViewItem1->IsSelected);
            VERIFY_IS_TRUE(static_cast<bool>(listViewItem1->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));

            listViewItem1->IsSelected = false;
            VERIFY_IS_FALSE(listViewItem2->IsSelected);
            VERIFY_IS_FALSE(static_cast<bool>(listViewItem1->GetValue(Microsoft::UI::Xaml::Controls::Primitives::SelectorItem::IsSelectedProperty)));
        });
    }

    void ISelectionInfoIntegrationTests::ValidateCtrlAScenarios()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();

        // setting the focus on the listview
        RunOnUIThread([&]()
        {
            listView->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            listView->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        SelectionInfoStatics::Reset();

        // ctrl + A -> should SelectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned int>(1));

            VERIFY_ARE_EQUAL(selectedRanges->GetAt(0)->FirstIndex, 0);
            VERIFY_ARE_EQUAL(selectedRanges->GetAt(0)->Length, static_cast<unsigned int>(m_NumberOfItems));
        });
        TestServices::WindowHelper->WaitForIdle();

        // ctrl + A -> should DeselectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned int>(0));
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::DeselectRangeInvoked);
    }

    void ISelectionInfoIntegrationTests::ValidateCtrlAScenariosWithoutInterfaceImplementation()
    {
        TestCleanupWrapper cleanup;

        unsigned int numberOfItems = 20;
        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<int>^ items = nullptr;

        // initiliazing
        RunOnUIThread([&]()
        {
            listView = ref new xaml_controls::ListView();

            items = ref new Platform::Collections::Vector<int>(numberOfItems);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            listView->Height = 400;
            listView->Width = 200;
            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Multiple;

            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        // setting the focus on the listview
        RunOnUIThread([&]()
        {
            listView->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
            listView->SelectedIndex = 0;
        });
        TestServices::WindowHelper->WaitForIdle();

        // ctrl + A -> should SelectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned int>(1));

            VERIFY_ARE_EQUAL(selectedRanges->GetAt(0)->FirstIndex, 0);
            VERIFY_ARE_EQUAL(selectedRanges->GetAt(0)->Length, static_cast<unsigned int>(numberOfItems));
        });
        TestServices::WindowHelper->WaitForIdle();

        // ctrl + A -> should DeselectAll
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_a#$u$_a#$u$_ctrl");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            ::Windows::Foundation::Collections::IVectorView<Microsoft::UI::Xaml::Data::ItemIndexRange^>^ selectedRanges = listView->SelectedRanges;
            VERIFY_IS_NOT_NULL(selectedRanges);

            VERIFY_ARE_EQUAL(selectedRanges->Size, static_cast<unsigned int>(0));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void ISelectionInfoIntegrationTests::ValidateTapItem()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;
            listView->SelectedIndex = -1;

            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        SelectionInfoStatics::Reset();

        // Select item
        TestServices::InputHelper->LeftMouseClick(itemAsFE);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
        VERIFY_IS_FALSE(SelectionInfoStatics::DeselectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::GetSelectedRangesInvoked);

        SelectionInfoStatics::Reset();

        // Deselect item
        TestServices::InputHelper->LeftMouseClick(itemAsFE);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(SelectionInfoStatics::SelectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::DeselectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::GetSelectedRangesInvoked);
    }

    void ISelectionInfoIntegrationTests::ValidateClearingSelection()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;
            listView->SelectedIndex = 2;
        });
        TestServices::WindowHelper->WaitForIdle();

        SelectionInfoStatics::Reset();

        RunOnUIThread([&]()
        {
            listView->SelectedIndex = -1;
        });
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(SelectionInfoStatics::SelectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::DeselectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::GetSelectedRangesInvoked);
    }

    void ISelectionInfoIntegrationTests::ValidateCanChangeSelectionMode()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;

            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Select item
        TestServices::InputHelper->Tap(itemAsFE);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // Unselect everything be setting selection mode to None
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::None;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // set the selection mode to Multiple again
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;
        });
        TestServices::WindowHelper->WaitForIdle();

        SelectionInfoStatics::Reset();

        // Select item
        TestServices::InputHelper->Tap(itemAsFE);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
        VERIFY_IS_FALSE(SelectionInfoStatics::DeselectRangeInvoked);
        VERIFY_IS_TRUE(SelectionInfoStatics::GetSelectedRangesInvoked);
    }

    void ISelectionInfoIntegrationTests::ValidateSelectionAutomation()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupEnvironment();
        xaml_automation_peers::ListViewAutomationPeer^ lvPeer;
        xaml_automation_peers::SelectorItemAutomationPeer^ selectorItemAP;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;
            lvPeer = static_cast<xaml_automation_peers::ListViewAutomationPeer^>(xaml_automation_peers::ListViewAutomationPeer::CreatePeerForElement(listView));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            selectorItemAP = static_cast<xaml_automation_peers::SelectorItemAutomationPeer^>(lvPeer->GetChildren()->GetAt(1));
            selectorItemAP->Select();

            VERIFY_IS_TRUE(selectorItemAP->IsSelected);
            VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
            SelectionInfoStatics::Reset();

            selectorItemAP = static_cast<xaml_automation_peers::SelectorItemAutomationPeer^>(lvPeer->GetChildren()->GetAt(2));
            selectorItemAP->AddToSelection();

            VERIFY_IS_TRUE(selectorItemAP->IsSelected);
            VERIFY_IS_TRUE(SelectionInfoStatics::SelectRangeInvoked);
            SelectionInfoStatics::Reset();

            selectorItemAP = static_cast<xaml_automation_peers::SelectorItemAutomationPeer^>(lvPeer->GetChildren()->GetAt(1));
            selectorItemAP->RemoveFromSelection();

            VERIFY_IS_FALSE(selectorItemAP->IsSelected);
            VERIFY_IS_TRUE(SelectionInfoStatics::DeselectRangeInvoked);
            SelectionInfoStatics::Reset();
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    //
    // Private Functions
    //
    xaml_controls::ListView^ ISelectionInfoIntegrationTests::SetupEnvironment()
    {
        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            listView = dynamic_cast<xaml_controls::ListView^> (xaml_markup::XamlReader::Load(
                L"<ListView Height='400' Width='200' SelectionMode='Extended' xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' />"));
            VERIFY_IS_NOT_NULL(listView);

            listView->ItemsSource = m_DataSource;
            TestServices::WindowHelper->WindowContent = listView;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::DataVirtualization
