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

    // Nine 100x100 items in a 300x300 non-grouped ItemsControl give three items per line and three lines:
    //   Orientation=Vertical:   item i is in column i / 3, row i % 3 (items stack downward, lines wrap to the right).
    //   Orientation=Horizontal: item i is in column i % 3, row i / 3 (items stack rightward, lines wrap downward).
    // A point in the leading half of item k along the stacking direction lies before k; the trailing half lies after k.
    // Item 4 is in the middle of the middle line, so neither neighbor is on the panel's edge.
    void ItemsWrapGridIntegrationTests::VerifyGetInsertionIndexesVertical()
    {
        TestCleanupWrapper cleanup;
        const auto probes = ProbeInsertionIndexes(Orientation::Vertical);

        VERIFY_ARE_EQUAL(3, probes.leadingHalfOfItem4.first);
        VERIFY_ARE_EQUAL(4, probes.leadingHalfOfItem4.second);
        VERIFY_ARE_EQUAL(4, probes.trailingHalfOfItem4.first);
        VERIFY_ARE_EQUAL(5, probes.trailingHalfOfItem4.second);
        VERIFY_ARE_EQUAL(8, probes.trailingHalfOfLastItem.first);
        VERIFY_ARE_EQUAL(-1, probes.trailingHalfOfLastItem.second);
    }

    void ItemsWrapGridIntegrationTests::VerifyGetInsertionIndexesHorizontal()
    {
        TestCleanupWrapper cleanup;
        const auto probes = ProbeInsertionIndexes(Orientation::Horizontal);

        VERIFY_ARE_EQUAL(3, probes.leadingHalfOfItem4.first);
        VERIFY_ARE_EQUAL(4, probes.leadingHalfOfItem4.second);
        VERIFY_ARE_EQUAL(4, probes.trailingHalfOfItem4.first);
        VERIFY_ARE_EQUAL(5, probes.trailingHalfOfItem4.second);
        VERIFY_ARE_EQUAL(8, probes.trailingHalfOfLastItem.first);
        VERIFY_ARE_EQUAL(-1, probes.trailingHalfOfLastItem.second);
    }

    ItemsWrapGridIntegrationTests::InsertionIndexProbes ItemsWrapGridIntegrationTests::ProbeInsertionIndexes(xaml_controls::Orientation orientation)
    {
        const int itemCount = 9;
        const bool isVertical = (orientation == Orientation::Vertical);
        auto itemsControl = PanelsHelper::AddItemsControlWithPanel<ItemsControl, xaml_controls::ItemsWrapGrid>(
            itemCount,
            isVertical ? L"Orientation='Vertical'" : L"Orientation='Horizontal'");
        auto itemsWrapGrid = ItemsControlHelper::GetPanelFromItemsControl<xaml_controls::ItemsWrapGrid>(itemsControl);
        TestServices::WindowHelper->WaitForIdle();

        // A point inside item 'index', 'stackingOffset' pixels into it along the stacking direction and centered on the other axis.
        auto pointInItem = [isVertical](int index, float stackingOffset)
        {
            const float lineStart = 100.0f * (index / 3);
            const float stackingStart = 100.0f * (index % 3);
            return isVertical ?
                wf::Point(lineStart + 50.0f, stackingStart + stackingOffset) :
                wf::Point(stackingStart + stackingOffset, lineStart + 50.0f);
        };

        InsertionIndexProbes probes = {};

        RunOnUIThread([&]()
        {
            auto insertionPanel = safe_cast<xaml_controls::IInsertionPanel^>(itemsWrapGrid);
            insertionPanel->GetInsertionIndexes(pointInItem(4, 20.0f), &probes.leadingHalfOfItem4.first, &probes.leadingHalfOfItem4.second);
            insertionPanel->GetInsertionIndexes(pointInItem(4, 80.0f), &probes.trailingHalfOfItem4.first, &probes.trailingHalfOfItem4.second);
            insertionPanel->GetInsertionIndexes(pointInItem(itemCount - 1, 80.0f), &probes.trailingHalfOfLastItem.first, &probes.trailingHalfOfLastItem.second);
        });

        LOG_OUTPUT(L"Leading half of item 4: first=%d second=%d.", probes.leadingHalfOfItem4.first, probes.leadingHalfOfItem4.second);
        LOG_OUTPUT(L"Trailing half of item 4: first=%d second=%d.", probes.trailingHalfOfItem4.first, probes.trailingHalfOfItem4.second);
        LOG_OUTPUT(L"Trailing half of the last item: first=%d second=%d.", probes.trailingHalfOfLastItem.first, probes.trailingHalfOfLastItem.second);
        return probes;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ItemsWrapGrid
