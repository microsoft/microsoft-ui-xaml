// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "MoCoGroupingIntegrationTests.h"

#include <MocoHelper.h>
#include <SafeEventRegistration.h>
#include <ItemsControlHelper.h>
#include <TestEvent.h>
#include <TreeHelper.h>
#include <TestCleanupWrapper.h>
#include <RuntimeEnabledFeaturesEnum.h>
#include <WUCRenderingScopeGuard.h>
#include "KeyboardInjectionOverride.h"

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MoCo {

    bool MoCoGroupingIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool MoCoGroupingIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool MoCoGroupingIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void MoCoGroupingIntegrationTests::CanGenerateGroupHeaderItemListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpGroupedEnvironment(MocoHelper::ListControlType::ListView, 2, 2);

        PerformGenerateGroupHeaderTest(list, MocoHelper::ListControlType::ListView, 2);
    }

    void MoCoGroupingIntegrationTests::CanGenerateGroupHeaderItemGridView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpGroupedEnvironment(MocoHelper::ListControlType::GridView, 2, 2);

        PerformGenerateGroupHeaderTest(list, MocoHelper::ListControlType::GridView, 2);
    }

    void MoCoGroupingIntegrationTests::PerformGenerateGroupHeaderTest(
        xaml_controls::ListViewBase^ list,
        MocoHelper::ListControlType listControlType,
        int expectedHeaders)
    {
        RunOnUIThread([&] () {
            auto groupHeaders = ref new Platform::Collections::Vector<xaml_controls::ListViewBaseHeaderItem^>();
            TreeHelper::GetVisualChildrenByType<xaml_controls::ListViewBaseHeaderItem>(list, groupHeaders);

            VERIFY_ARE_EQUAL(expectedHeaders, static_cast<int>(groupHeaders->Size));

            for (unsigned int i = 0; i < groupHeaders->Size; i++)
            {
                if (listControlType == MocoHelper::ListControlType::ListView)
                {
                    auto listViewHeaderItem = safe_cast<xaml_controls::ListViewHeaderItem^>(groupHeaders->GetAt(i));
                    VERIFY_IS_NOT_NULL(listViewHeaderItem);
                }
                else
                {
                    auto gridViewHeaderItem = safe_cast<xaml_controls::GridViewHeaderItem^>(groupHeaders->GetAt(i));
                    VERIFY_IS_NOT_NULL(gridViewHeaderItem);
                }
            }
        });
    }

    void MoCoGroupingIntegrationTests::CanCreateBasicGroupedListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpGroupedEnvironment(MocoHelper::ListControlType::ListView, 5, 5);

        PerformBasicGroupingTests(list);
    }

    void MoCoGroupingIntegrationTests::CanCreateBasicGroupedGridView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpGroupedEnvironment(MocoHelper::ListControlType::GridView, 5, 5);

        PerformBasicGroupingTests(list);
    }

    void MoCoGroupingIntegrationTests::PerformBasicGroupingTests(xaml_controls::ListViewBase^ list)
    {
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&] () {
           scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(list, L"ScrollViewer"));

            viewChangedRegistration.Attach(scrollViewer,
            ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
            [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));

            auto collectionView = safe_cast<xaml_data::ICollectionView^>(list->ItemsSource);
            VERIFY_IS_NOT_NULL(collectionView);
            VERIFY_IS_NOT_NULL(collectionView->CollectionGroups);
            VERIFY_ARE_EQUAL(5, static_cast<int>(collectionView->CollectionGroups->Size));
            VERIFY_IS_TRUE(list->IsGrouping);
            VERIFY_ARE_EQUAL(25, static_cast<int>(list->Items->Size));

            list->ScrollIntoView(list->Items->GetAt(20));
        });

        viewChangedEvent->WaitForDefault();

        RunOnUIThread([&] () {
            auto cont = list->ContainerFromItem(list->Items->GetAt(20));
            VERIFY_IS_NOT_NULL(cont);
        });
    }

    void MoCoGroupingIntegrationTests::FirstVisibleItemIsRetainedOnResetListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpGroupedResettableEnvironment(MocoHelper::ListControlType::ListView, 10, 1);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        MocoResettableCollection^ collection = nullptr;

        int indexToRemoveAt = 3;
        double previousVerticalOffset = 0;
        double expectedVerticalOffsetChange = 0;

        RunOnUIThread([&] ()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(list, L"ScrollViewer"));
            collection =
                safe_cast<MocoResettableCollection^>(
                    safe_cast<xaml_data::CollectionViewSource^>(
                        safe_cast<FrameworkElement^>(test_infra::TestServices::WindowHelper->WindowContent)->FindName(L"cvs")
                        )->Source
                    );

            LOG_OUTPUT(L"Scrolling to item %d.", indexToRemoveAt);
            list->ScrollIntoView(collection->GetAt(indexToRemoveAt), xaml_controls::ScrollIntoViewAlignment::Leading);
            xaml_controls::ItemsStackPanel^ isp = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            isp->AreStickyGroupHeadersEnabled = false;
            isp->CacheLength = 4;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            auto containerAtIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(indexToRemoveAt));
            auto containerAboveIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(indexToRemoveAt - 1));

            auto atTransformToRoot = containerAtIndex->TransformToVisual(nullptr);
            auto aboveTransformToRoot = containerAboveIndex->TransformToVisual(nullptr);

            previousVerticalOffset = scrollViewer->VerticalOffset;
            expectedVerticalOffsetChange = atTransformToRoot->TransformPoint(wf::Point(0, 0)).Y - aboveTransformToRoot->TransformPoint(wf::Point(0, 0)).Y;

            LOG_OUTPUT(L"Vertical offset is %.2f. Expecting that to change by %.2f px after removing item %d.", previousVerticalOffset, -expectedVerticalOffsetChange, indexToRemoveAt);

            collection->ResetRemove(indexToRemoveAt);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            VERIFY_ARE_EQUAL((int)(previousVerticalOffset - expectedVerticalOffsetChange), (int)scrollViewer->VerticalOffset);

            LOG_OUTPUT(L"Scrolling to item %d.", indexToRemoveAt);
            list->ScrollIntoView(collection->GetAt(indexToRemoveAt), xaml_controls::ScrollIntoViewAlignment::Leading);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            auto panel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            auto containerAtIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(indexToRemoveAt));
            auto containerAtFlippedIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(collection->Size - 1 - indexToRemoveAt));

            auto atTransformToRoot = containerAtIndex->TransformToVisual(panel);
            auto atFlippedTransformToRoot = containerAtFlippedIndex->TransformToVisual(panel);

            previousVerticalOffset = scrollViewer->VerticalOffset;
            expectedVerticalOffsetChange = atTransformToRoot->TransformPoint(wf::Point(0, 0)).Y - atFlippedTransformToRoot->TransformPoint(wf::Point(0, 0)).Y;

            LOG_OUTPUT(L"Vertical offset is %.2f. Expecting that to change by %.2f px after flipping the collection.", previousVerticalOffset, -expectedVerticalOffsetChange);

            collection->ResetFlip();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            VERIFY_ARE_EQUAL((int)(previousVerticalOffset - expectedVerticalOffsetChange), (int)scrollViewer->VerticalOffset);
        });
    }

    void MoCoGroupingIntegrationTests::RemoveOnlyItemInOnlyNonEmptyGroup()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;
            xaml_controls::ListViewBase^ list = nullptr;

            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='True'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            // create a data source that has one group with one item in it
            data = ref new Platform::Collections::Vector<Object^>();
            auto group = ref new Platform::Collections::Vector<Object^>();
            group->Append("Item0");
            data->Append(group);

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // remove the only item in the only group. There should just be one empty group
            // in the data after this.
            safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(0))->RemoveAt(0);
        });

        // bug caused listview to crash here.
        test_infra::TestServices::WindowHelper->WaitForIdle();

        // verify that the group is now empty
        VERIFY_ARE_EQUAL(safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(0))->Size, 0u);
    }

    void MoCoGroupingIntegrationTests::VerifyEmptyGroupsDoNotContributeToExtent()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        double extent = 0;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView Height='400' x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='True'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            // create a data source that has one group with one item in it
            data = ref new Platform::Collections::Vector<Object^>();
            auto group = ref new Platform::Collections::Vector<Object^>();
            group->Append("Item0");
            data->Append(group);

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // get the extent and add some empty groups
            xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            extent = itemsPanel->ActualHeight;

            // make sure extent is valid
            VERIFY_IS_TRUE(extent > 10);

            // add 10 empty groups
            for (int i = 0; i < 10; i++)
            {
                auto group = ref new Platform::Collections::Vector<int>();
                data->Append(group);
            }
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // verify that the extent is the same even after adding empty groups
            xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);

            VERIFY_ARE_EQUAL(extent, itemsPanel->ActualHeight);
        });
    }

    void MoCoGroupingIntegrationTests::CanGetGroupHeaderContainerFromItemContainer()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;

            // grouped ListView with HidesIfEmpty=false
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='False'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            // create a data source that has two groups with two items each
            data = ref new Platform::Collections::Vector<Object^>();
            auto group0 = ref new Platform::Collections::Vector<int>();
            group0->Append(0);
            group0->Append(1);
            data->Append(group0);

            auto group1 = ref new Platform::Collections::Vector<int>();
            group1->Append(2);
            group1->Append(3);
            data->Append(group1);

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get header for first item
            auto container = list->ContainerFromIndex(0);
            auto header1 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));
            VERIFY_IS_NOT_NULL(header1);

            // verify header1 is from the first group
            auto itemData = safe_cast<Platform::Collections::Vector<int>^>(header1->Content)->GetAt(0);
            VERIFY_ARE_EQUAL(0, itemData);

            // get header for second item
            container = list->ContainerFromIndex(1);
            auto header2 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));
            VERIFY_IS_NOT_NULL(header2);

            // verify header2 is from the first group
            itemData = safe_cast<Platform::Collections::Vector<int>^>(header2->Content)->GetAt(0);
            VERIFY_ARE_EQUAL(0, itemData);

            // get header for 3rd item
            container = list->ContainerFromIndex(2);
            auto header3 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));
            VERIFY_IS_NOT_NULL(header3);

            // verify header3 is from the second group
            itemData = safe_cast<Platform::Collections::Vector<int>^>(header3->Content)->GetAt(0);
            VERIFY_ARE_EQUAL(2, itemData);

            // get header for 4th item
            container = list->ContainerFromIndex(3);
            auto header4 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));
            VERIFY_IS_NOT_NULL(header4);

            // verify header4 is from the second group
            itemData = safe_cast<Platform::Collections::Vector<int>^>(header4->Content)->GetAt(0);
            VERIFY_ARE_EQUAL(2, itemData);

            // pass in a 'bad' container - passing the list itself. this is allowed since the input is a Dependency object
            auto header5 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(list));
            VERIFY_IS_NULL(header5);

            // make source not-grouped
            cvs = ref new xaml_data::CollectionViewSource();
            auto ungroupedData = ref new Platform::Collections::Vector<int>();
            ungroupedData->Append(1);
            ungroupedData->Append(2);
            ungroupedData->Append(3);
            cvs->Source = ungroupedData;
            cvs->IsSourceGrouped = false;
            list->ItemsSource = cvs->View;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get header for first item - it should be null since we are not grouped anymore
            auto container = list->ContainerFromIndex(0);
            auto header1 = safe_cast<xaml_controls::ListViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));
            VERIFY_IS_NULL(header1);
        });
    }

    void MoCoGroupingIntegrationTests::MaintainViewportOnHeaderChanged()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        int currentVisibleIndex = 0;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;


            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' Height='200' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='True'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            data = ref new Platform::Collections::Vector<Object^>();

            // 10 groups
            for (int i = 0; i < 10; i++)
            {
                auto group = ref new Platform::Collections::Vector<int>();
                // 10 items each
                for (int j = 0; j < 10; j++)
                {
                    group->Append(i);
                }

                data->Append(group);
            }

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            itemsPanel->ItemsUpdatingScrollMode = xaml_controls::ItemsUpdatingScrollMode::KeepItemsInView;
            // scroll to the middle
            list->ScrollIntoView(data->GetAt(5));
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            currentVisibleIndex = itemsPanel->FirstVisibleIndex;
            VERIFY_ARE_NOT_EQUAL(0, currentVisibleIndex);

            // add item to top
            auto group1 = safe_cast<Platform::Collections::Vector<int>^>(data->GetAt(0));
            group1->Append(100);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(currentVisibleIndex+1, itemsPanel->FirstVisibleIndex);

            // remove item from top
            auto group1 = safe_cast<Platform::Collections::Vector<int>^>(data->GetAt(0));
            group1->RemoveAt(0);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(currentVisibleIndex, itemsPanel->FirstVisibleIndex);

            // add group on top
            auto group = ref new Platform::Collections::Vector<int>();
            group->Append(1);
            group->Append(2);

            data->InsertAt(0, group);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(currentVisibleIndex+2, itemsPanel->FirstVisibleIndex);

            // remove group on top
            data->RemoveAt(0);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(currentVisibleIndex, itemsPanel->FirstVisibleIndex);
        });
    }

    void MoCoGroupingIntegrationTests::VerifyChoosingGroupHeaderContainerEvent()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_controls::ListViewHeaderItem^ providedHeader1 = nullptr;
        xaml_controls::ListViewHeaderItem^ providedHeader2 = nullptr;

        auto choosingGroupHeaderContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingGroupHeaderContainer);

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            providedHeader1 = ref new xaml_controls::ListViewHeaderItem();
            providedHeader2 = ref new xaml_controls::ListViewHeaderItem();

            // grouped ListView
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.ItemTemplate>" \
                L"          <DataTemplate>" \
                L"              <TextBlock Text = '{Binding}' />" \
                L"          </DataTemplate>" \
                L"       </ListView.ItemTemplate>" \
                L"       <ListView.GroupStyle>" \
                L"          <GroupStyle>" \
                L"              <GroupStyle.HeaderTemplate>" \
                L"                  <DataTemplate>" \
                L"                      <TextBlock Text='{Binding GroupName}' Foreground='White' FontSize='20' />" \
                L"                  </DataTemplate>" \
                L"              </GroupStyle.HeaderTemplate>" \
                L"          </GroupStyle>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
            choosingGroupHeaderContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingGroupHeaderContainerEventArgs^>(
                [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingGroupHeaderContainerEventArgs^ args)
            {
                // provide new containers based on index //
                unsigned int index = 0;
                bool foundGroup = data->IndexOf(args->Group, &index);
                if (foundGroup)
                {
                    // check the args index, to make sure it matches in the data collection
                    VERIFY_ARE_EQUAL(static_cast<signed int>(index), args->GroupIndex);

                    if (index == 0)
                    {
                        args->GroupHeaderContainer = providedHeader1;
                    }
                    else if (index == 1)
                    {
                        args->GroupHeaderContainer = providedHeader2;
                    }
                    else if (index == 2)
                    {
                        // dont give a header. a new one
                        // should be created by the framework.
                    }
                    else
                    {
                        // we only have two groups
                        VERIFY_FAIL();
                    }
                }
                else
                {
                    // the group data we got in the args was incorrect
                    VERIFY_FAIL();
                }
            }));

            data = ref new Platform::Collections::Vector<Object^>();
            // 3 groups
            for (int i = 0; i < 3; i++)
            {
                auto group = ref new Platform::Collections::Vector<int>();
                // 1 item each
                group->Append(i);

                data->Append(group);
            }

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get header containers and verify that they are correct
            auto container1 = list->ContainerFromIndex(0);
            auto header1 = list->GroupHeaderContainerFromItemContainer(container1);
            VERIFY_ARE_EQUAL(providedHeader1, header1);

            auto container2 = list->ContainerFromIndex(1);
            auto header2 = list->GroupHeaderContainerFromItemContainer(container2);
            VERIFY_ARE_EQUAL(providedHeader2, header2);

            auto container3 = list->ContainerFromIndex(2);
            auto header3 = list->GroupHeaderContainerFromItemContainer(container3);
            VERIFY_IS_NOT_NULL(header3);
        });
    }

    void MoCoGroupingIntegrationTests::VerifyMaxGroupHeadersInGarbageSection()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ listView = nullptr;
        xaml_controls::Panel^ itemsPanel = nullptr;
        Platform::Collections::Vector<xaml_controls::ListViewHeaderItem^>^ providedHeaders = nullptr;
        const int maxGarbageHeadersCount = 16;

        auto choosingGroupHeaderContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingGroupHeaderContainer);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            // Markup for grouped ListView.
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='listView'>" \
                L"       <ListView.ItemTemplate>" \
                L"          <DataTemplate>" \
                L"              <TextBlock Text='{Binding}'/>" \
                L"          </DataTemplate>" \
                L"       </ListView.ItemTemplate>" \
                L"       <ListView.GroupStyle>" \
                L"          <GroupStyle>" \
                L"              <GroupStyle.HeaderTemplate>" \
                L"                  <DataTemplate>" \
                L"                      <TextBlock Text='{Binding GroupName}' Foreground='White' FontSize='12'/>" \
                L"                  </DataTemplate>" \
                L"              </GroupStyle.HeaderTemplate>" \
                L"          </GroupStyle>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            LOG_OUTPUT(L"Loading root panel");
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            listView = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"listView"));

            LOG_OUTPUT(L"Attaching ListView.ChoosingGroupHeaderContainer event");
            choosingGroupHeaderContainerRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingGroupHeaderContainerEventArgs^>(
                    [&](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingGroupHeaderContainerEventArgs^ args)
            {
                unsigned int index = 0;

                if (data->IndexOf(args->Group, &index))
                {
                    LOG_OUTPUT(L"ListView.ChoosingGroupHeaderContainer event provides pre-created ListViewHeaderItem for index %d.", index);
                    // Check the args' GroupIndex to make sure it matches the index in the data collection.
                    VERIFY_ARE_EQUAL(static_cast<signed int>(index), args->GroupIndex);

                    args->GroupHeaderContainer = providedHeaders->GetAt(index);
                }
                else
                {
                    LOG_OUTPUT(L"ChoosingGroupHeaderContainerEventArgs.Group could not be found in data collection.");
                    VERIFY_FAIL();
                }
            }));

            providedHeaders = ref new Platform::Collections::Vector<xaml_controls::ListViewHeaderItem^>();
            // Add 2 * maxGarbageHeadersCount group headers.
            for (int i = 0; i < 2 * maxGarbageHeadersCount; i++)
            {
                auto groupHeader = ref new xaml_controls::ListViewHeaderItem();

                providedHeaders->Append(groupHeader);
            }

            data = ref new Platform::Collections::Vector<Object^>();
            // Add 2 * maxGarbageHeadersCount groups.
            for (int i = 0; i < 2 * maxGarbageHeadersCount; i++)
            {
                auto group = ref new Platform::Collections::Vector<int>();
                // Each group has 1 item.
                group->Append(i);
                data->Append(group);
            }

            // Set up the CollectionViewSource and hook it up to the ListView.
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            listView->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Accessing ListView.ItemsPanel.");
            itemsPanel = safe_cast<xaml_controls::Panel^>(listView->ItemsPanelRoot);
            VERIFY_IS_NOT_NULL(itemsPanel);

            auto itemsStackPanel = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);
            LOG_OUTPUT(L"ListView.ItemsPanel.CacheLength: %f.", itemsStackPanel->CacheLength);
            VERIFY_ARE_EQUAL(4.0, itemsStackPanel->CacheLength);

            // Expecting one header child and one item child per group.
            LOG_OUTPUT(L"Original Panel.Children.Count: %d.", itemsPanel->Children->Size);
            VERIFY_ARE_EQUAL(4 * maxGarbageHeadersCount, (int)itemsPanel->Children->Size);
        });

        // Remove first maxGarbageHeadersCount groups one at the time.
        for (int i = 1; i <= maxGarbageHeadersCount; i++)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Removing first group for iteration %d.", i);
                data->RemoveAt(0);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Expecting group header remains in visual tree, in the garbage section.
                LOG_OUTPUT(L"New Panel.Children.Count: %d.", itemsPanel->Children->Size);
                VERIFY_IS_GREATER_THAN((int)itemsPanel->Children->Size, 4 * maxGarbageHeadersCount - 2 * i);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
        }

        // Remove last maxGarbageHeadersCount groups one at the time.
        for (int i = 0; i < maxGarbageHeadersCount; i++)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Removing first group for iteration %d.", i + maxGarbageHeadersCount);
                data->RemoveAt(0);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // Expecting group header is discarded.
                LOG_OUTPUT(L"New Panel.Children.Count: %d.", itemsPanel->Children->Size);
                VERIFY_IS_LESS_THAN((int)itemsPanel->Children->Size, 4 * maxGarbageHeadersCount - i);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoGroupingIntegrationTests::VerifyGroupHeaderPlacement()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <GridView x:Name='list' Height='200' Background='Gray'>" \
                L"       <GridView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='False'>" \
                L"               <GroupStyle.HeaderTemplate>" \
                L"                   <DataTemplate>" \
                L"                       <Grid Background='Red' Width='200'/>" \
                L"                   </DataTemplate>" \
                L"               </GroupStyle.HeaderTemplate>" \
                L"           </GroupStyle>" \
                L"       </GridView.GroupStyle>" \
                L"       <GridView.ItemsPanel>" \
                L"           <ItemsPanelTemplate>" \
                L"               <ItemsWrapGrid GroupHeaderPlacement='Left' />" \
                L"           </ItemsPanelTemplate>"\
                L"       </GridView.ItemsPanel>" \
                L"   </GridView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            // one group with one two items
            data = ref new Platform::Collections::Vector<Object^>();
            auto group = ref new Platform::Collections::Vector<int>();
            group->Append(0);
            group->Append(1);
            data->Append(group);

            // setup the collection view source and hook it up to the gridview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto container = list->ContainerFromIndex(0);
            auto header1 = safe_cast<xaml_controls::GridViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(container));

            auto gt = header1->TransformToVisual(list);
            wf::Point p;
            p.X = 0;
            p.Y = 0;

            auto offset = gt->TransformPoint(p);
            VERIFY_ARE_EQUAL(0, offset.X);
            VERIFY_ARE_EQUAL(0, offset.Y);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoGroupingIntegrationTests::RemoveLastNonEmptyGroup()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='False'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            data = ref new Platform::Collections::Vector<Object^>();
            auto group = ref new Platform::Collections::Vector<Object^>();
            group->Append("Item0");
            group->Append("Item1");
            data->Append(group);
            auto group1 = ref new Platform::Collections::Vector<Object^>();
            data->Append(group1);

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // remove the only item in the only group. There should just be one empty group
            // in the data after this.
            data->RemoveAt(0);
        });

        // bug caused listview to crash here.
        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(-1, itemsPanel->FirstVisibleIndex);
            VERIFY_ARE_EQUAL(-1, itemsPanel->LastVisibleIndex);
        });
    }

    Platform::Collections::Vector<Object^>^ CreateGroupedData(int numGroups, int numItemsPerGroup)
    {
        auto data = ref new Platform::Collections::Vector<Object^>();
        for (int g = 0; g < numGroups; g++)
        {
            auto group = ref new Platform::Collections::Vector<Object^>();
            for (int i = 0; i < numItemsPerGroup; i++)
            {
                group->Append(i + g*1000);
            }
            data->Append(group);
        }

        return data;
    }

    void MoCoGroupingIntegrationTests::MakeMultipleGroupsEmptyIncludingLastOne()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;

        RunOnUIThread([&]()
        {
            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='True'/>" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            // three groups - two items each
            data = CreateGroupedData(3, 2);

            // setup the collection view source and hook it up to the listview
            xaml_data::CollectionViewSource^ cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        // Remove all items in one go from the front
        {
            RunOnUIThread([&]()
            {
                // remove all items items from front.
                for (unsigned int i = 0; i < data->Size; i++)
                {
                    Platform::Collections::Vector<Object^>^ group = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(i));
                    unsigned int size = group->Size;
                    for (unsigned int j = 0; j < size; j++)
                    {
                        group->RemoveAt(0);
                    }
                }
            });

            // bug caused listview to crash here.
            test_infra::TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                VERIFY_ARE_EQUAL(-1, itemsPanel->FirstVisibleIndex);
                VERIFY_ARE_EQUAL(-1, itemsPanel->LastVisibleIndex);
            });
        }

        // Remove all items in one go from the end
        {
            RunOnUIThread([&]()
            {
                // three groups - two items each
                data = CreateGroupedData(3, 2);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // remove all items items from end.
                for (unsigned int i = 0; i < data->Size; i++)
                {
                    Platform::Collections::Vector<Object^>^ group = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(data->Size - 1 - i));
                    unsigned int size = group->Size;
                    for (unsigned int j = 0; j < size; j++)
                    {
                        group->RemoveAt(size - 1 - j);
                    }
                }
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
            RunOnUIThread([&]()
            {
                xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                VERIFY_ARE_EQUAL(-1, itemsPanel->FirstVisibleIndex);
                VERIFY_ARE_EQUAL(-1, itemsPanel->LastVisibleIndex);
            });
        }

        // Remove all items but with layout happening in between
        {
            RunOnUIThread([&]()
            {
                // two groups - two items each
                data = CreateGroupedData(3, 2);
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // remove from group 0 //
                Platform::Collections::Vector<Object^>^ group = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(0));
                unsigned int size = group->Size;
                for (unsigned int i = 0; i < size; i++)
                {
                    group->RemoveAt(0);
                }
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // remove from group 2 //
                Platform::Collections::Vector<Object^>^ group = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(2));
                unsigned int size = group->Size;
                for (unsigned int i = 0; i < size; i++)
                {
                    group->RemoveAt(0);
                }
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // remove from group 1 //
                Platform::Collections::Vector<Object^>^ group = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(1));
                unsigned int size = group->Size;
                for (unsigned int i = 0; i < size; i++)
                {
                    group->RemoveAt(0);
                }
            });

            RunOnUIThread([&]()
            {
                xaml_controls::ItemsStackPanel^ itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                VERIFY_ARE_EQUAL(-1, itemsPanel->FirstVisibleIndex);
                VERIFY_ARE_EQUAL(-1, itemsPanel->LastVisibleIndex);
            });
        }

    }

    void MoCoGroupingIntegrationTests::CanRecycleTrackedGroupHeader()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml(ListView)));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

            MocoHelper::PopulateList(list, 10, true /*isGrouped*/, 100, cvs);
            list->Height = 100;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]() {
            // We scroll to groupI.
            // The viewport is very small and when realizing backward, the panel will have to recycle
            // groupI. The latter might be tracked and we will crash if we recycle a tracked element.
            // We should avoid this situation by cancelling the tracking behavior.
            auto groupI = safe_cast<::Windows::Foundation::Collections::IVector<Object^>^>(cvs->Source)->GetAt(8);
            list->ScrollIntoView(groupI, xaml_controls::ScrollIntoViewAlignment::Leading);

            // This call should crash if the bug repro.
            list->UpdateLayout();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoGroupingIntegrationTests::StickyHeadersWorkAfterItemsChange()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 400.0f));

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            // grouped ListView with HidesIfEmpty=true
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <Grid.Resources >" \
                L"        <Style TargetType='ListViewItem' x:Key='MyListViewItemStyle'>" \
                L"          <Setter Property='Margin' Value='0,0,0,0' />" \
                L"        </Style>" \
                L"   </Grid.Resources >" \
                L"   <ListView x:Name='list' Height='400' ItemContainerStyle='{StaticResource MyListViewItemStyle}'>" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle HidesIfEmpty='False'/>" \
                L"       </ListView.GroupStyle>" \
                L"        <ListView.ItemTemplate>" \
                L"         <DataTemplate>" \
                L"            <Grid Height='40'>" \
                L"               <TextBlock Text='{Binding}' />" \
                L"            </Grid>" \
                L"         </DataTemplate>" \
                L"        </ListView.ItemTemplate>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            data = ref new Platform::Collections::Vector<Object^>();
            // 10 items in first group
            auto group = ref new Platform::Collections::Vector<Object^>();
            for (int i = 0; i < 10; i++)
            {
                group->Append(i);
            }
            data->Append(group);
            // 2 item in second group
            auto group1 = ref new Platform::Collections::Vector<Object^>();
            group1->Append(201);
            group1->Append(202);
            data->Append(group1);
            // 2 item in third group
            auto group2 = ref new Platform::Collections::Vector<Object^>();
            group2->Append(301);
            group2->Append(302);
            data->Append(group2);

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            unsigned int size = list->Items->Size;
            list->ScrollIntoView(list->Items->GetAt(size - 1));
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"1");

        RunOnUIThread([&]()
        {
            //insert 5 items into second group. This will push out group 3 out of the viewport
            auto secondGroup = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(1));
            for (int i = 0; i < 5; i++)
            {
                secondGroup->Append(i);
            }
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            //remove  5 items from second group. This should bring  group 3 back into view
            auto secondGroup = safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(1));
            for (int i = 0; i < 5; i++)
            {
                secondGroup->RemoveAt(0);
            }
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"2");

        // Perform another scroll action, when testing we saw this "fix" the broken transforms
        RunOnUIThread([&]()
        {
            list->ScrollIntoView(list->Items->GetAt(4));
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, L"3");
    }

    void MoCoGroupingIntegrationTests::KeyboardNavigationWithStickyHeaders()
    {
        TestCleanupWrapper cleanup;

        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_controls::ItemsStackPanel^ itemsPanel = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            Platform::String^ xamlText =
                L"<Grid "\
                L"     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "\
                L"     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> "\
                L"  <Grid.Resources> "\
                L"     <CollectionViewSource x:Name='cvs' IsSourceGrouped='True'/> "\
                L"  </Grid.Resources> "\
                L"  <ListView x:Name='list' Height='500' Width='500' "\
                L"           ItemsSource='{Binding Source={StaticResource cvs}}'> "\
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"     <ListView.ItemTemplate> "\
                L"         <DataTemplate> "\
                L"             <TextBlock Text='{Binding}' Foreground='White'/> "\
                L"         </DataTemplate> "\
                L"     </ListView.ItemTemplate> "\
                L"     <ListView.ItemsPanel> "\
                L"         <ItemsPanelTemplate> "\
                L"             <ItemsStackPanel Orientation='Vertical' CacheLength='0' /> "\
                L"         </ItemsPanelTemplate> "\
                L"     </ListView.ItemsPanel> "\
                L"     <ListView.GroupStyle> "\
                L"         <GroupStyle> "\
                L"             <GroupStyle.HeaderTemplate> "\
                L"                 <DataTemplate> "\
                L"                     <Border Background='Red' Height='200' > "\
                L"                         <TextBlock Text='{Binding GroupName}' Foreground='White' FontSize='20'/> "\
                L"                     </Border> "\
                L"                 </DataTemplate> "\
                L"             </GroupStyle.HeaderTemplate> "\
                L"         </GroupStyle> "\
                L"     </ListView.GroupStyle> "\
                L" </ListView> "\
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            data = ref new Platform::Collections::Vector<Object^>();
            // 3 groups, 10 items each
            for (int g = 0; g < 3; g++)
            {
                auto group = ref new Platform::Collections::Vector<Object^>();
                for (int i = 0; i < 10; i++)
                {
                    group->Append(i);
                }
                data->Append(group);
            }

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            list->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(4, itemsPanel->FirstVisibleIndex);
            VERIFY_ARE_EQUAL(9, itemsPanel->LastVisibleIndex);
        });

        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        test_infra::TestServices::WindowHelper->WaitForIdle();
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(0, itemsPanel->FirstVisibleIndex);
            VERIFY_ARE_EQUAL(6, itemsPanel->LastVisibleIndex);
        });
    }

void MoCoGroupingIntegrationTests::AddToEmptyGroupAtBegining()
{
    TestCleanupWrapper cleanup;
    Platform::Collections::Vector<Object^>^ data = nullptr;
    xaml_controls::ListViewBase^ list = nullptr;

    RunOnUIThread([&]() {
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;

        // grouped GridView with HidesIfEmpty=true
        // note that this is a bug fix in the ItemsWrapGrid layout strategy.
        Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
            L"   <GridView x:Name='list' >" \
            L"       <GridView.GroupStyle>" \
            L"           <GroupStyle HidesIfEmpty='True'/>" \
            L"       </GridView.GroupStyle>" \
            L"   </GridView>" \
            L"</Grid>";

        rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
        list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

        // create a data source that has 3 groups with last group containing one item.
        data = ref new Platform::Collections::Vector<Object^>();
        for (int i = 0; i < 2; i++)
        {
            auto group = ref new Platform::Collections::Vector<Object^>();
            data->Append(group);
        }

        // 3rd group with one item in it
        auto group = ref new Platform::Collections::Vector<Object^>();
        group->Append("Group1 Item0");
        data->Append(group);


        // setup the collection view source and hook it up to the listview
        cvs = ref new xaml_data::CollectionViewSource();
        cvs->IsSourceGrouped = true;
        cvs->Source = data;
        list->ItemsSource = cvs->View;

        test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
    });

    test_infra::TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        //add item to group 0
        safe_cast<Platform::Collections::Vector<Object^>^>(data->GetAt(0))->Append("Group0 Item0");
    });

    // bug caused listview to crash here.
    test_infra::TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        // verify first visible index is the first item
        xaml_controls::ItemsWrapGrid^ iwg = safe_cast<xaml_controls::ItemsWrapGrid^>(list->ItemsPanelRoot);
        VERIFY_ARE_EQUAL(0, iwg->FirstVisibleIndex);
    });
}

    void MoCoGroupingIntegrationTests::VerifyNoReorderWithGrouping()
    {
        TestCleanupWrapper cleanup;

        Platform::Collections::Vector<Object^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_data::CollectionViewSource^ cvs = nullptr;

            Platform::String^ xamlText =
                L"<Grid "\
                L"     xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' "\
                L"     xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> "\
                L"  <Grid.Resources> "\
                L"     <CollectionViewSource x:Name='cvs' IsSourceGrouped='True'/> "\
                L"  </Grid.Resources> "\
                L"  <ListView x:Name='list' Height='500' Width='500' "\
                L"            CanReorderItems='True' AllowDrop='True'"\
                L"            ItemsSource='{Binding Source={StaticResource cvs}}'> "\
                L"     <ListView.Resources>" \
                L"         <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"     </ListView.Resources>" \
                L"     <ListView.ItemTemplate> "\
                L"         <DataTemplate> "\
                L"             <TextBlock Text='{Binding}' Foreground='White'/> "\
                L"         </DataTemplate> "\
                L"     </ListView.ItemTemplate> "\
                L"     <ListView.ItemsPanel> "\
                L"         <ItemsPanelTemplate> "\
                L"             <ItemsStackPanel Orientation='Vertical' CacheLength='0' /> "\
                L"         </ItemsPanelTemplate> "\
                L"     </ListView.ItemsPanel> "\
                L"     <ListView.GroupStyle> "\
                L"         <GroupStyle> "\
                L"             <GroupStyle.HeaderTemplate> "\
                L"                 <DataTemplate> "\
                L"                     <Border Background='Red' Height='200' > "\
                L"                         <TextBlock Text='{Binding}' Foreground='White' FontSize='20'/> "\
                L"                     </Border> "\
                L"                 </DataTemplate> "\
                L"             </GroupStyle.HeaderTemplate> "\
                L"         </GroupStyle> "\
                L"     </ListView.GroupStyle> "\
                L" </ListView> "\
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));

            data = ref new Platform::Collections::Vector<Object^>();
            // 3 groups, 10 items each
            for (int g = 0; g < 3; g++)
            {
                auto group = ref new Platform::Collections::Vector<Object^>();
                for (int i = 0; i < 10; i++)
                {
                    group->Append(i);
                }
                data->Append(group);
            }

            // setup the collection view source and hook it up to the listview
            cvs = ref new xaml_data::CollectionViewSource();
            cvs->IsSourceGrouped = true;
            cvs->Source = data;
            list->ItemsSource = cvs->View;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });
        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            list->SelectedIndex = 0;
            auto container = safe_cast<xaml_controls::ListViewItem^>(list->ContainerFromIndex(0));
            container->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        // hold down alt+shift
        {
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_shift");
        }

        // press down twice
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        // press up once
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        // release alt+shift
        {
            KeyboardInjectionIgnoreEventWaitOverride kbdInjectionOverride(KeyboardWaitKind::None);
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#$u$_alt");
        }
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // make sure the selected index is still the same (no reordering happened)
            VERIFY_ARE_EQUAL(list->SelectedIndex, 0);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    //  Repro Steps:
    //  This is what needs to happen to repro the issue.
    //  1. List view with grouping, hooked up to grouped data with some number of groups (at least one group is required)
    //  2. Collapse the ListView
    //  3. Set the source for the CollectionViewSource with same data collection
    //  4. Remove group or groups from the data
    //  5. Make the ListView visible.
    //
    //  These actions must happen in this order for this crash.
    //
    //  Root Cause:
    //  The issue is that when a new source is set on the CollectionViewSource, the collection view
    //  source throws away its CollectionViewGroups to which the ModernCollectionBasePanel is listening
    //  for collection changes to. The hookup to these events for the new collection happens in
    //  ModernCollectionBasePanel::RegisterItemsHostImpl which happens during Measure. Unfortunately we won???t
    //  get a measure anytime soon because we are collapsed. All collection changes until the next time ListView
    //  becomes visible are un-noticed by ModernCollectionBasePanel. When it does become visible, its cache is
    //  incorrect ??? which swiftly leads to a crash.
    void MoCoGroupingIntegrationTests::VerifyCanAssignCVSSourceWhenCollapsed()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<::Windows::Foundation::Collections::IVector<int>^>^ data = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]() {
            Platform::String^ xamlText =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <Grid.Resources>" \
                L"      <CollectionViewSource x:Name='cvs' IsSourceGrouped='true' />" \
                L"   </Grid.Resources>" \
                L"   <ListView Height='400' x:Name='list' ItemsSource='{Binding Source={StaticResource cvs}}' >" \
                L"       <ListView.Resources>" \
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>" \
                L"       </ListView.Resources>" \
                L"       <ListView.GroupStyle>" \
                L"           <GroupStyle />" \
                L"       </ListView.GroupStyle>" \
                L"   </ListView>" \
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

            // Create a data source that has one group with one item in it
            data = ref new Platform::Collections::Vector<::Windows::Foundation::Collections::IVector<int>^>();
            cvs->Source = data;

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            data->Clear();

            LOG_OUTPUT(L"Adding a group");
            auto group = ref new Platform::Collections::Vector<int>();
            data->Append(group);
            list->UpdateLayout();

            LOG_OUTPUT(L"Collapsing ListView");
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;
            list->UpdateLayout();

            LOG_OUTPUT(L"Assigning cvs.Source");
            cvs->Source = data;
            list->UpdateLayout();

            LOG_OUTPUT(L"Removing group");
            data->RemoveAt(0);
            list->UpdateLayout();

            LOG_OUTPUT(L"Making ListView Visible");
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Visible;
            list->UpdateLayout(); // We used to crash here

            VERIFY_ARE_EQUAL(0u, data->Size);
        });

    }

}}}}}}
