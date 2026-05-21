// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "StickyHeaders.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Input;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

#define ListViewBaseXaml \
    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
    L"    <Grid.Resources>" \
    L"        <CollectionViewSource x:Name='cvs'/>" \
    L"        <Style TargetType='ListViewItem' x:Key='MyListViewItemStyle'>" \
    L"            <Setter Property='Margin' Value='0,0,0,0' />" \
    L"        </Style>" \
    L"    </Grid.Resources>" \
    L"    <ListView x:Name='list' Height='400' Width='400' ShowsScrollingPlaceholders='False' ItemsSource='{Binding Source={StaticResource cvs}}' ItemContainerStyle='{StaticResource MyListViewItemStyle}' ScrollViewer.VerticalScrollBarVisibility='Hidden' ScrollViewer.VerticalScrollMode='Enabled'>" \
    L"        <ListView.ItemsPanel>" \
    L"            <ItemsPanelTemplate>" \
    L"                <ItemsStackPanel CacheLength='0'/>" \
    L"            </ItemsPanelTemplate>" \
    L"        </ListView.ItemsPanel>" \
    L"        <ListView.ItemTemplate>" \
    L"            <DataTemplate>" \
    L"                <TextBlock Text='{Binding}' />" \
    L"            </DataTemplate>" \
    L"        </ListView.ItemTemplate>" \
    L"        <ListView.GroupStyle>" \
    L"          <GroupStyle>" \
    L"            <GroupStyle.HeaderTemplate>" \
    L"                <DataTemplate>" \
    L"                    <Grid Background='Blue'>" \
    L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='17' />" \
    L"                    </Grid>" \
    L"                </DataTemplate>" \
    L"            </GroupStyle.HeaderTemplate>" \
    L"        </GroupStyle>" \
    L"    </ListView.GroupStyle>" \
    L"  </ListView>" \
    L"</Grid>"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool StickyHeaders::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool StickyHeaders::ClassCleanup()
        {
            return true;
        }

        bool StickyHeaders::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ StickyHeaders::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        void StickyHeaders::PopulateList(xaml_controls::ListViewBase^ list, int count, int itemsCount, xaml_data::CollectionViewSource^ cvs)
        {
            auto dataCollection = MocoBasicDataSource::GetGroupedDataCollection(count, itemsCount);
            cvs->Source = dataCollection;
            cvs->IsSourceGrouped = true;
        }

        xaml_controls::ListViewBase^ StickyHeaders::SetupUI()
        {
            xaml_controls::ListViewBase^ list = nullptr;

            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ListViewBase, Loaded);

            RunOnUIThread([&] () {
                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_data::CollectionViewSource^ cvs = nullptr;

                rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml));
                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

                PopulateList(list, 10, 4, cvs);

                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;

                loadedRegistration.Attach(list, ref new xaml::RoutedEventHandler(
                    [spHasLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
                {
                    spHasLoadedEvent->Set();
                }));

            });
            spHasLoadedEvent->WaitForDefault();
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return list;
        }

        void StickyHeaders::BasicsInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // Scenario 1: Set up a basic ListView with sticky header groups, verify the DComp tree
            xaml_controls::ListViewBase^ listViewBase = SetupUI();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void StickyHeaders::ScrollIntoViewInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::ListViewBase^ listViewBase = SetupUI();

            // Scenario 1:  Scroll an off-screen item into view, causing sticky headers to shift
            RunOnUIThread([&] () {
                listViewBase->ScrollIntoView(listViewBase->Items->GetAt(20));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void StickyHeaders::TapAStickyInternal()
        {
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            xaml_controls::ListViewBase^ listViewBase = SetupUI();

            FrameworkElement^ stickyHeader = nullptr;

            // First scroll an off-screen item into view, causing sticky headers to shift
            RunOnUIThread([&] () {
                listViewBase->Margin = xaml::Thickness({0, 30, 0, 0});
                Object^ stickyHeaderItem = listViewBase->Items->GetAt(20);
                listViewBase->ScrollIntoView(stickyHeaderItem);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&] () {
                // Unfortunately ListView has no API to retrieve group headers.
                // The workaround is to dig into its ItemsStackPanel, which has everyone in a flat list.
                // Since we've turned off virtualization, only visible items are in this collection.
                // Headers are always stored first in the ItemsStackPanel collection, so we're interested
                // in the item at position 0.
                stickyHeader = safe_cast<FrameworkElement^>(listViewBase->ItemsPanelRoot->Children->GetAt(0));
            });

            std::shared_ptr<Event> spWasTappedEvent = std::make_shared<Event>();
            auto tappedRegistration = CreateSafeEventRegistration(UIElement, Tapped);

            tappedRegistration.Attach(stickyHeader, ref new xaml_input::TappedEventHandler(
                [spWasTappedEvent](Platform::Object^, TappedRoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Sticky Header Tapped");
                spWasTappedEvent->Set();
            }));

            // Now tap on the sticky header just brought into view
            TestServices::InputHelper->Tap(stickyHeader);
            spWasTappedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        void StickyHeaders::BasicsWUCFull()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            BasicsInternal();
        }

        void StickyHeaders::ScrollIntoViewWUCFull()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            ScrollIntoViewInternal();
        }

        void StickyHeaders::TapAStickyWUCFull()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree);
            TapAStickyInternal();
        }

    } } }
} } } }
