// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemsStackPanelIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ItemsControlHelper.h>
#include <PanelsHelper.h>
#include <FocusTestHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ItemsStackPanel {

    bool ItemsStackPanelIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ItemsStackPanelIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ItemsStackPanelIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ItemsStackPanelIntegrationTests::CanInstantiate()
    {
        TestCleanupWrapper cleanup;
        Generic::DependencyObjectTests<xaml_controls::ItemsStackPanel>::CanInstantiate();
    }

    void ItemsStackPanelIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;
        Generic::FrameworkElementTests<xaml_controls::ItemsControl>::CanEnterAndLeaveLiveTree(PanelsHelper::CreateItemsControlWithPanel<ItemsControl, xaml_controls::ItemsStackPanel>());
    }

    void ItemsStackPanelIntegrationTests::CanStackItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::ItemsStackPanel>(s_itemCount, L"Orientation='Horizontal'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * i, 100.0f));
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

    void ItemsStackPanelIntegrationTests::CanStackItemsVertically()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::ItemsStackPanel>(s_itemCount, L"Orientation='Vertical'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f, 100.0f * i));
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

    void ItemsStackPanelIntegrationTests::CacheIsUpdatedDuringScrolling()
    {
        TestCleanupWrapper cleanup;
        auto listView = PanelsHelper::AddItemsControlWithPanel<ListView, xaml_controls::ItemsStackPanel>(s_itemCountForVirtualization, L"Orientation='Vertical'");
        auto itemsWrapGrid = ItemsControlHelper::GetPanelFromItemsControl<xaml_controls::ItemsStackPanel>(listView);
        xaml_controls::Control^ firstContainer = nullptr;

        RunOnUIThread([&]()
        {
            firstContainer = safe_cast<xaml_controls::Control^>(listView->ContainerFromIndex(0));
        });

        FocusTestHelper::EnsureFocus(firstContainer, FocusState::Pointer);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstVisibleIndex, 0);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastVisibleIndex, 2);

            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstCacheIndex, 0);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(itemsWrapGrid->LastCacheIndex, 2);
        });

        ItemsControlHelper::ScrollToIndex<ListView, UIElement>(listView, s_itemCountForVirtualization - 1);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstVisibleIndex, 6);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastVisibleIndex, 8);

            VERIFY_IS_LESS_THAN_OR_EQUAL(itemsWrapGrid->FirstCacheIndex, 6);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastCacheIndex, 8);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ItemsStackPanel
