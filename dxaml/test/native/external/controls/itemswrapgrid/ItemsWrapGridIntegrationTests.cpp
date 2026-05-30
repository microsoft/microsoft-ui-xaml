// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemsWrapGridIntegrationTests.h"

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>

#include <ItemsControlHelper.h>
#include <PanelsHelper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ItemsWrapGrid {

    bool ItemsWrapGridIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ItemsWrapGridIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ItemsWrapGridIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void ItemsWrapGridIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::ItemsWrapGrid>::CanInstantiate();
    }

    void ItemsWrapGridIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;
        Generic::FrameworkElementTests<xaml_controls::ItemsControl>::CanEnterAndLeaveLiveTree(PanelsHelper::CreateItemsControlWithPanel<ItemsControl, xaml_controls::ItemsWrapGrid>());
    }

    void ItemsWrapGridIntegrationTests::CanWrapItemsHorizontally()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::ItemsWrapGrid>(s_itemCount, L"Orientation='Horizontal'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * (i % 3), 100.0f * (i / 3)));
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

    void ItemsWrapGridIntegrationTests::CanWrapItemsVertically()
    {
        TestCleanupWrapper cleanup;
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::ItemsWrapGrid>(s_itemCount, L"Orientation='Vertical'");
        auto expectedPositions = ref new Platform::Collections::Vector<wf::Point>();

        for (int i = 0; i < s_itemCount; i++)
        {
            expectedPositions->Append(wf::Point(100.0f * (i / 3), 100.0f * (i % 3)));
        }

        PanelsHelper::VerifyItemPositions(itemsControl, expectedPositions);
    }

    void ItemsWrapGridIntegrationTests::CacheIsUpdatedDuringScrolling()
    {
        TestCleanupWrapper cleanup;
        auto listView = PanelsHelper::AddItemsControlWithPanel<ListView, xaml_controls::ItemsWrapGrid>(s_itemCountForVirtualization, L"Orientation='Horizontal'");
        auto itemsWrapGrid = ItemsControlHelper::GetPanelFromItemsControl<xaml_controls::ItemsWrapGrid>(listView);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstVisibleIndex, 0);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastVisibleIndex, 8);

            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstCacheIndex, 0);
            VERIFY_IS_GREATER_THAN_OR_EQUAL(itemsWrapGrid->LastCacheIndex, 8);
        });

        ItemsControlHelper::ScrollToIndex<ListView, UIElement>(listView, s_itemCountForVirtualization - 1);

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(itemsWrapGrid->FirstVisibleIndex, 18);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastVisibleIndex, 26);

            VERIFY_IS_LESS_THAN_OR_EQUAL(itemsWrapGrid->FirstCacheIndex, 18);
            VERIFY_ARE_EQUAL(itemsWrapGrid->LastCacheIndex, 26);
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ItemsWrapGrid
