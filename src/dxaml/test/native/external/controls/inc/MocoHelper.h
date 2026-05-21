// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestDataModel.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace Platform;

#define ListViewBaseXaml(ControlName) \
    L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
    L"    <Grid.Resources>" \
    L"      <CollectionViewSource x:Name='cvs'/>" \
    L"    </Grid.Resources>" \
    L"    <" L#ControlName L" x:Name='list' Height='500' Width='500' ItemsSource='{Binding Source={StaticResource cvs}}'>" \
    L"        <" L#ControlName L".ItemTemplate>" \
    L"            <DataTemplate>" \
    L"                <Grid Background='Orange' Height='150' Width='150'>" \
    L"                    <StackPanel Orientation='Vertical'>" \
    L"                        <TextBlock Text='{Binding}' />" \
    L"                    </StackPanel>" \
    L"                </Grid>" \
    L"            </DataTemplate>" \
    L"        </" L#ControlName L".ItemTemplate>" \
    L"        <" L#ControlName L".GroupStyle>" \
    L"          <GroupStyle>" \
    L"            <GroupStyle.Panel>" \
    L"                <ItemsPanelTemplate>" \
    L"                    <VariableSizedWrapGrid Orientation='Horizontal'/>" \
    L"                </ItemsPanelTemplate>" \
    L"            </GroupStyle.Panel>" \
    L"            <GroupStyle.HeaderTemplate>" \
    L"                <DataTemplate>" \
    L"                    <Grid Background='Blue'>" \
    L"                        <TextBlock Text='{Binding}' Foreground='White' FontSize='17' />" \
    L"                    </Grid>" \
    L"                </DataTemplate>" \
    L"            </GroupStyle.HeaderTemplate>" \
    L"            <GroupStyle.ContainerStyle>" \
    L"                <Style TargetType='GroupItem'>" \
    L"                    <Setter Property='BorderBrush' Value='White' />" \
    L"                    <Setter Property='BorderThickness' Value='3' />" \
    L"                </Style>" \
    L"            </GroupStyle.ContainerStyle>" \
    L"        </GroupStyle>" \
    L"    </" L#ControlName L".GroupStyle>" \
    L"  </" L#ControlName L">" \
    L"</Grid>"

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    class MocoHelper
    {
    public:
        enum class ListControlType
        {
            ListView,
            GridView
        };

        enum class DataCollectionType
        {
            Flat,
            Grouped
        };

        static xaml_controls::ListViewBase^ SetUpBasicEnvironment(ListControlType listType, int itemsCount, int width = 500, int height = 500)
        {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_controls::ListViewBase^ list = nullptr;
            RunOnUIThread([&] () {
                    rootPanel = ref new xaml_controls::Grid();
                    if (listType == ListControlType::ListView)
                    {
                        list = ref new xaml_controls::ListView();
                    }
                    else
                    {
                        list = ref new xaml_controls::GridView();
                    }
                    rootPanel->Children->Append(list);
                    test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
                    list->Height = height;
                    list->Width = width;
                    PopulateList(list, itemsCount);
            });
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return list;
        }

        static xaml_controls::ListViewBase^  SetUpGroupedEnvironment(
            MocoHelper::ListControlType listType,
            int groupCount,
            int itemsCount)
        {
            xaml_controls::ListViewBase^ list = nullptr;

            RunOnUIThread([&] () {
                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_data::CollectionViewSource^ cvs = nullptr;

                if (listType == MocoHelper::ListControlType::ListView)
                {
                    rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml(ListView)));
                }
                else
                {
                    rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml(GridView)));
                }
                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

                MocoHelper::PopulateList(list, groupCount, true /*isGrouped*/, itemsCount, cvs);

                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
            });
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return list;
        }

        static xaml_controls::ListViewBase^ SetUpResettableEnvironment(ListControlType listType, int itemsCount, int width = 500, int height = 500)
        {
            xaml_controls::Grid^ rootPanel = nullptr;
            xaml_controls::ListViewBase^ list = nullptr;
            RunOnUIThread([&] () {
                    rootPanel = ref new xaml_controls::Grid();
                    if (listType == ListControlType::ListView)
                    {
                        list = ref new xaml_controls::ListView();
                    }
                    else
                    {
                        list = ref new xaml_controls::GridView();
                    }
                    rootPanel->Children->Append(list);
                    test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
                    list->Height = height;
                    list->Width = width;
                    PopulateList(list, itemsCount, false /* isGrouped */, 0, nullptr, true /* isResettable */);
            });
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return list;
        }

        static xaml_controls::ListViewBase^  SetUpGroupedResettableEnvironment(
            MocoHelper::ListControlType listType,
            int groupCount,
            int itemsCount)
        {
            xaml_controls::ListViewBase^ list = nullptr;

            RunOnUIThread([&] () {
                xaml_controls::Grid^ rootPanel = nullptr;
                xaml_data::CollectionViewSource^ cvs = nullptr;

                if (listType == MocoHelper::ListControlType::ListView)
                {
                    rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml(ListView)));
                }
                else
                {
                    rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(ListViewBaseXaml(GridView)));
                }
                list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
                cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

                MocoHelper::PopulateList(list, groupCount, true /*isGrouped*/, itemsCount, cvs, true /* isResettable */);

                test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
            });
            test_infra::TestServices::WindowHelper->WaitForIdle();

            return list;
        }

        static void SelectItemAtIndex(xaml_controls::ListViewBase^ list, int index, bool doTap)
        {
            auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, SelectionChanged);
            std::shared_ptr<Event> selectionChangedEvent = std::make_shared<Event>();
            selectionChangedRegistration.Attach(list,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ e) {
                selectionChangedEvent->Set();
            }));

            xaml::FrameworkElement^ itemToSelect = nullptr;

            RunOnUIThread([&] ()
            {
                itemToSelect = dynamic_cast<xaml::FrameworkElement^>(list->ContainerFromIndex(index));
                VERIFY_IS_NOT_NULL(itemToSelect);
            });

            if (doTap)
            {
                test_infra::TestServices::InputHelper->Tap(itemToSelect);
            }
            else
            {
                test_infra::TestServices::InputHelper->LeftMouseClick(itemToSelect);
            }
            selectionChangedEvent->WaitForDefault();
        }

        static void PopulateList(xaml_controls::ListViewBase^ list, int count = 25, bool isGrouped = false, int itemsCount = 4, xaml_data::CollectionViewSource^ cvs = nullptr, bool isResettable = false)
        {
            if (isGrouped)
            {
                if (isResettable)
                {
                    auto dataCollection = MocoBasicDataSource::GetGroupedResettableDataCollection(count, itemsCount, false);
                    cvs->Source = dataCollection;
                    cvs->IsSourceGrouped = true;
                }
                else
                {
                    auto dataCollection = MocoBasicDataSource::GetGroupedDataCollection(count, itemsCount, false);
                    cvs->Source = dataCollection;
                    cvs->IsSourceGrouped = true;
                }
            }
            else
            {
                if (isResettable)
                {
                    auto dataCollection = MocoBasicDataSource::GetResettableDataCollection(count);
                    list->ItemsSource = dataCollection;
                }
                else
                {
                    auto dataCollection = MocoBasicDataSource::GetFlatDataCollection(count);
                    list->ItemsSource = dataCollection;
                }
            }
        }
    };
}}}}}