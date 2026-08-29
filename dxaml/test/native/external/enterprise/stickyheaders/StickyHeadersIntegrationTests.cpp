// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StickyHeadersIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <SafeEventRegistration.h>

#include "StickyHeadersHelper.h"
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace StickyHeaders {

    bool StickyHeadersIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool StickyHeadersIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void StickyHeadersIntegrationTests::StickyGroupHeadersEnabled()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupGroupedListView(L"True" /* areStickyGroupHeadersEnabledStringValue */);

        RunOnUIThread([&]()
        {
            listView->ScrollIntoView(listView->Items->GetAt(10));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);

            VERIFY_ARE_EQUAL(itemsPanel->FirstVisibleIndex, 4);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StickyHeadersIntegrationTests::StickyGroupHeadersListHeaderResized()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        auto listView = SetupGroupedListView(L"True" /* areStickyGroupHeadersEnabledStringValue */);
        xaml_controls::Button^ headerButton;

        RunOnUIThread([&]()
        {
            headerButton = ref new xaml_controls::Button();
            headerButton->Content = L"Header";
            headerButton->Height = 100;
            listView->Header = headerButton;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            headerButton->Height = 200;
        });

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void StickyHeadersIntegrationTests::StickyGroupHeadersDisabled()
    {
        TestCleanupWrapper cleanup;

        auto listView = SetupGroupedListView(L"False" /* areStickyGroupHeadersEnabledStringValue */);

        RunOnUIThread([&]()
        {
            listView->ScrollIntoView(listView->Items->GetAt(10));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);

            VERIFY_ARE_EQUAL(itemsPanel->FirstVisibleIndex, 3);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void StickyHeadersIntegrationTests::OverpanBounceNoStickyHeader()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        auto listView = SetupGroupedListViewWithoutAnimations(L"False" /* areStickyGroupHeadersEnabledStringValue */);
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listView, L"ScrollViewer"));
        });

        TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, 0, PointerFinger::Finger1);
        TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, 200, PointerFinger::Finger1);

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
    }

    void StickyHeadersIntegrationTests::ValidateOverpanBounceWithStickyHeader()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        auto listView = SetupGroupedListViewWithoutAnimations(L"True" /* areStickyGroupHeadersEnabledStringValue */);
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listView, L"ScrollViewer"));
        });

        TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, 0, PointerFinger::Finger1);
        TestServices::InputHelper->DynamicPressCenter(scrollViewer, 0, 200, PointerFinger::Finger1);

        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
    }

    void StickyHeadersIntegrationTests::ValidateHeaderStretchInItemsStackPanel()
    {
        TestCleanupWrapper cleanup;
        auto listView = SetupGroupedListViewWithInlineHeaders(L"<ItemsStackPanel Orientation='Vertical' GroupHeaderPlacement='Top'/>");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemContainer1 = listView->ContainerFromIndex(0);
            auto groupHeader = safe_cast<xaml_controls::ListViewHeaderItem^>(listView->GroupHeaderContainerFromItemContainer(itemContainer1));
            auto width = groupHeader->ActualWidth;
            VERIFY_ARE_EQUAL(500, width);
        });

        listView = SetupGroupedListViewWithInlineHeaders(L"<ItemsStackPanel Orientation='Horizontal' GroupHeaderPlacement='Left'/>");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemContainer1 = listView->ContainerFromIndex(0);
            auto groupHeader = safe_cast<xaml_controls::ListViewHeaderItem^>(listView->GroupHeaderContainerFromItemContainer(itemContainer1));
            auto height = groupHeader->ActualHeight;
            VERIFY_ARE_EQUAL(496, static_cast<int>(height));
        });
    }

    void StickyHeadersIntegrationTests::ValidateHeaderStretchInItemsWrapGrid()
    {
        TestCleanupWrapper cleanup;
        auto gridView = SetupGroupedGridViewWithInlineHeaders(L"<ItemsWrapGrid Orientation='Vertical' GroupHeaderPlacement='Left'/>");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemContainer1 = gridView->ContainerFromIndex(0);
            auto groupHeader = safe_cast<xaml_controls::GridViewHeaderItem^>(gridView->GroupHeaderContainerFromItemContainer(itemContainer1));
            auto height = groupHeader->ActualHeight;
            // 10 pixels offset from top.
            VERIFY_ARE_EQUAL(486, static_cast<int>(height));
        });

        gridView = SetupGroupedGridViewWithInlineHeaders(L"<ItemsWrapGrid Orientation='Horizontal' GroupHeaderPlacement='Top'/>");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemContainer1 = gridView->ContainerFromIndex(0);
            auto groupHeader = safe_cast<xaml_controls::GridViewHeaderItem^>(gridView->GroupHeaderContainerFromItemContainer(itemContainer1));
            auto width = groupHeader->ActualWidth;
            VERIFY_ARE_EQUAL(500, width);
        });
    }

    //
    // Private Methods
    //
    xaml_controls::ListView^ StickyHeadersIntegrationTests::SetupGroupedListView(Platform::String^ areStickyGroupHeadersEnabledStringValue)
    {
        xaml_controls::Grid^ rootPanel = nullptr;
        xaml_controls::ListView^ listView = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"        <CollectionViewSource x:Name='cvs'/>"
                L"        <Style TargetType='ListViewItem' x:Key='MyListViewItemStyle'>"
                L"            <Setter Property='Margin' Value='0,0,0,0' />"
                L"        </Style>"
                L"    </Grid.Resources>"
                L"    <ListView x:Name='listView' Height='400' Width='400' ItemsSource='{Binding Source={StaticResource cvs}}' ItemContainerStyle='{StaticResource MyListViewItemStyle}'>"
                L"       <ListView.Resources>"
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>"
                L"       </ListView.Resources>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='0' AreStickyGroupHeadersEnabled='" + areStickyGroupHeadersEnabledStringValue + L"'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <TextBlock Text='{Binding}'/>"
                L"            </DataTemplate>"
                L"        </ListView.ItemTemplate>"
                L"        <ListView.GroupStyle>"
                L"          <GroupStyle>"
                L"            <GroupStyle.Panel>"
                L"                <ItemsPanelTemplate>"
                L"                    <VariableSizedWrapGrid Orientation='Horizontal'/>"
                L"                </ItemsPanelTemplate>"
                L"            </GroupStyle.Panel>"
                L"            <GroupStyle.HeaderTemplate>"
                L"                <DataTemplate>"
                L"                    <Grid Background='Blue'>"
                L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='20'/>"
                L"                    </Grid>"
                L"                </DataTemplate>"
                L"            </GroupStyle.HeaderTemplate>"
                L"            <GroupStyle.ContainerStyle>"
                L"                <Style TargetType='GroupItem'>"
                L"                    <Setter Property='BorderBrush' Value='White'/>"
                L"                    <Setter Property='BorderThickness' Value='3'/>"
                L"                </Style>"
                L"            </GroupStyle.ContainerStyle>"
                L"        </GroupStyle>"
                L"    </ListView.GroupStyle>"
                L"  </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = GetGroupedData();
            VERIFY_IS_NOT_NULL(itemsSource);

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

    xaml_controls::ListView^ StickyHeadersIntegrationTests::SetupGroupedListViewWithoutAnimations(Platform::String^ areStickyGroupHeadersEnabledStringValue)
    {
        xaml_controls::ListView^ listView = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"      <CollectionViewSource x:Name='cvs'/>"
                L"      <Style x:Key='MyCustomListViewItemStyle' TargetType='ListViewItem'>"
                L"         <Setter Property='Template'> "
                L"          <Setter.Value> "
                L"              <ControlTemplate TargetType='ListViewItem'> "
                L"                  <ContentPresenter x:Name='ListViewItemContentPresenter' "
                L"                      ContentTemplate='{TemplateBinding ContentTemplate}' "
                L"                      Content='{TemplateBinding Content}' "
                L"                      FontFamily='{Binding ElementName=SearchTextBox, Path=FontFamily}' "
                L"                      FontSize='{Binding ElementName=SearchTextBox, Path=FontSize}' "
                L"                      FontWeight='{Binding ElementName=SearchTextBox, Path=FontWeight}' "
                L"                      Foreground='{TemplateBinding Foreground}' "
                L"                      HorizontalAlignment='{TemplateBinding HorizontalContentAlignment}' "
                L"                      VerticalAlignment='{TemplateBinding VerticalContentAlignment}' /> "
                L"              </ControlTemplate> "
                L"          </Setter.Value> "
                L"         </Setter> "
                L"       </Style> "
                L"    </Grid.Resources>"
                L"    <ListView x:Name='listView' Height='600' Width='400' ItemsSource='{Binding Source={StaticResource cvs}}' ItemContainerStyle='{StaticResource MyCustomListViewItemStyle}'>"
                L"        <ListView.Transitions>"
                L"          <TransitionCollection />"
                L"        </ListView.Transitions>"
                L"        <ListView.ItemContainerTransitions>"
                L"          <TransitionCollection />"
                L"        </ListView.ItemContainerTransitions>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='0' AreStickyGroupHeadersEnabled='" + areStickyGroupHeadersEnabledStringValue + L"'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <TextBlock Text='{Binding}'/>"
                L"            </DataTemplate>"
                L"        </ListView.ItemTemplate>"
                L"        <ListView.GroupStyle>"
                L"          <GroupStyle>"
                L"            <GroupStyle.Panel>"
                L"                <ItemsPanelTemplate>"
                L"                    <VariableSizedWrapGrid Orientation='Horizontal'/>"
                L"                </ItemsPanelTemplate>"
                L"            </GroupStyle.Panel>"
                L"            <GroupStyle.HeaderTemplate>"
                L"                <DataTemplate>"
                L"                    <Grid Background='Blue'>"
                L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='20'/>"
                L"                    </Grid>"
                L"                </DataTemplate>"
                L"            </GroupStyle.HeaderTemplate>"
                L"            <GroupStyle.ContainerStyle>"
                L"                <Style TargetType='GroupItem'>"
                L"                    <Setter Property='BorderBrush' Value='White'/>"
                L"                    <Setter Property='BorderThickness' Value='3'/>"
                L"                </Style>"
                L"            </GroupStyle.ContainerStyle>"
                L"        </GroupStyle>"
                L"    </ListView.GroupStyle>"
                L"  </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = GetGroupedData();
            VERIFY_IS_NOT_NULL(itemsSource);

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

    xaml_controls::ListView^ StickyHeadersIntegrationTests::SetupGroupedListViewWithInlineHeaders(Platform::String^ panelStringValue)
    {
        xaml_controls::ListView^ listView = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"      <CollectionViewSource x:Name='cvs'/>"
                L"    </Grid.Resources>"
                L"    <ListView x:Name='listView' Height='500' Width='500' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                                   + panelStringValue +
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <TextBlock Text='{Binding}'/>"
                L"            </DataTemplate>"
                L"        </ListView.ItemTemplate>"
                L"        <ListView.GroupStyle>"
                L"          <GroupStyle>"
                L"           <GroupStyle.HeaderContainerStyle>"
                L"                  <Style TargetType='ListViewHeaderItem'>"
                L"                      <Setter Property='HorizontalAlignment' Value='Stretch' />"
                L"                      <Setter Property='VerticalAlignment' Value = 'Stretch' />"
                L"                      <Setter Property='Background' Value='Red' />"
                L"                  </Style>"
                L"            </GroupStyle.HeaderContainerStyle>"
                L"            <GroupStyle.HeaderTemplate>"
                L"                <DataTemplate>"
                L"                    <Grid Background='Blue' >"
                L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='20'/>"
                L"                    </Grid>"
                L"                </DataTemplate>"
                L"            </GroupStyle.HeaderTemplate>"
                L"            <GroupStyle.ContainerStyle>"
                L"                <Style TargetType='GroupItem'>"
                L"                    <Setter Property='BorderBrush' Value='White'/>"
                L"                    <Setter Property='BorderThickness' Value='3'/>"
                L"                </Style>"
                L"            </GroupStyle.ContainerStyle>"
                L"        </GroupStyle>"
                L"    </ListView.GroupStyle>"
                L"  </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = GetGroupedData();
            VERIFY_IS_NOT_NULL(itemsSource);

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return listView;
    }

    xaml_controls::GridView^ StickyHeadersIntegrationTests::SetupGroupedGridViewWithInlineHeaders(Platform::String^ panelStringValue)
    {
        xaml_controls::GridView^ gridView = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"      <CollectionViewSource x:Name='cvs'/>"
                L"    </Grid.Resources>"
                L"    <GridView x:Name='gridView' Height='500' Width='500' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                L"        <GridView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                                  + panelStringValue +
                L"            </ItemsPanelTemplate>"
                L"        </GridView.ItemsPanel>"
                L"        <GridView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <TextBlock Text='{Binding}'/>"
                L"            </DataTemplate>"
                L"        </GridView.ItemTemplate>"
                L"        <GridView.GroupStyle>"
                L"          <GroupStyle>"
                L"           <GroupStyle.HeaderContainerStyle>"
                L"                  <Style TargetType='GridViewHeaderItem'>"
                L"                      <Setter Property='HorizontalAlignment' Value='Stretch' />"
                L"                      <Setter Property='VerticalAlignment' Value = 'Stretch' />"
                L"                      <Setter Property='Background' Value='Red' />"
                L"                  </Style>"
                L"            </GroupStyle.HeaderContainerStyle>"
                L"            <GroupStyle.HeaderTemplate>"
                L"                <DataTemplate>"
                L"                    <Grid Background='Blue' >"
                L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='20'/>"
                L"                    </Grid>"
                L"                </DataTemplate>"
                L"            </GroupStyle.HeaderTemplate>"
                L"            <GroupStyle.ContainerStyle>"
                L"                <Style TargetType='GroupItem'>"
                L"                    <Setter Property='BorderBrush' Value='White'/>"
                L"                    <Setter Property='BorderThickness' Value='3'/>"
                L"                </Style>"
                L"            </GroupStyle.ContainerStyle>"
                L"        </GroupStyle>"
                L"    </GridView.GroupStyle>"
                L"  </GridView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = GetGroupedData();
            VERIFY_IS_NOT_NULL(itemsSource);

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return gridView;
    }

    Platform::Collections::Vector<Platform::Object^>^ StickyHeadersIntegrationTests::GetGroupedData()
    {
        auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

        for (unsigned int i = 0; i < 3; ++i)
        {
            Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ group = nullptr;

            switch (i)
            {
            case 0:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Tennis");
                VERIFY_IS_NOT_NULL(group);

                group->Append(L"Roger Federer");
                group->Append(L"Rafael Nadal");
                group->Append(L"Novak Djokovic");
                group->Append(L"Andy Murray");
                group->Append(L"Grigor Dimitrov");
                break;

            case 1:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Soccer");
                VERIFY_IS_NOT_NULL(group);

                group->Append(L"Cristiano Ronaldo");
                group->Append(L"Lionel Messi");
                group->Append(L"Neymar");
                group->Append(L"Andres Iniesta");
                group->Append(L"Gareth Bale");
                group->Append(L"Xavi");
                group->Append(L"James Rodriguez");
                group->Append(L"Ronaldinho");
                group->Append(L"Arjen Robben");
                break;

            case 2:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Basketball");
                VERIFY_IS_NOT_NULL(group);

                group->Append(L"Allen Iverson");
                group->Append(L"Dwayne Wade");
                group->Append(L"LeBron James");
                group->Append(L"Kevin Durant");
                group->Append(L"Kobe Bryant");
                break;
            }

            groupedData->Append(group);
        }

        return groupedData;
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::StickyHeaders
