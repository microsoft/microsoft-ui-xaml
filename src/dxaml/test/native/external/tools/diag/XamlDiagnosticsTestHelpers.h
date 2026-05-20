// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <StickyHeadersHelper.h>
#include <RuntimeEnabledFeatureOverride.h>
#include "XamlOM.WinUI.h"

class XamlDiagnosticsTestHelpers
{
public:
    static Platform::String^ gridString;
    static Platform::String^ resourcesString;
    static Platform::String^ resourcesInDictionaryString;
    static Platform::String^ resourcesStaticResourceInBindingString;
    static Platform::String^ simpleListViewAndCommandBarString;
    static Platform::String^ userControlWithCustomButtonStyleString;
    static Platform::String^ buttonWithCustomStyleDuplicatedSettersString;
    static Platform::String^ gridWithInvisibleElementsString;
    static Platform::String^ relativePanelWithCollapsedElementsString;
    static Platform::String^ gridWithDisabledElementsString;
    static Platform::String^ modifiableSetterAndStoryboardString;
    static Platform::String^ visualStatesAndTriggersString;
    static Platform::String^ stackPanelWithButtonString;
    static Platform::String^ stackPanelWithNestedStylesString;

    static xaml_controls::Grid^ SetupGrid()
    {
        xaml_controls::Grid^ grid;
       
        Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
        {
            grid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(gridString));
        });

        return grid;
    }

    static xaml_controls::Grid^ SetupGroupedListView()
    {
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;
        xaml::Controls::Grid^ rootPanel;
        Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
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
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='0' AreStickyGroupHeadersEnabled='True'/>"
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

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));

            itemsSource = GetGroupedData();

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = true;
        });

        return rootPanel;
    }

    static xaml_controls::Grid^ SetupAppAndMergedDictionaries()
    {
        xaml::Controls::Grid^ rootPanel;
        Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
        {
            auto appResources = xaml::Application::Current->Resources;
            
            appResources->Insert(L"testApp", ref new xaml::Media::SolidColorBrush(Microsoft::UI::Colors::Black));
            
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name = 'parent'>\r\n"
                L"  <Grid.Resources>\r\n"
                L"    <ResourceDictionary>\r\n"
                L"      <ResourceDictionary.MergedDictionaries>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='testOverwritten' Color='Red' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <SolidColorBrush x:Key='testOverwritten' Color='Green' />\r\n"
                L"        </ResourceDictionary>\r\n"
                L"        <ResourceDictionary>\r\n"
                L"          <ResourceDictionary.ThemeDictionaries>\r\n"
                L"            <ResourceDictionary x:Key='Light'>\r\n"
                L"              <SolidColorBrush x:Key='testThemed' Color='Yellow' />\r\n"
                L"            </ResourceDictionary>\r\n"
                L"            <ResourceDictionary x:Key='Dark'>\r\n"
                L"              <SolidColorBrush x:Key='testThemed' Color='Blue' />\r\n"
                L"            </ResourceDictionary>\r\n"
                L"          </ResourceDictionary.ThemeDictionaries>\r\n"
                L"        </ResourceDictionary>\r\n"
                L"      </ResourceDictionary.MergedDictionaries>\r\n"
                L"      <Color x:Key='MyColor'>Blue</Color>\r\n"
                L"      <SolidColorBrush x:Key='RedBrush'>Red</SolidColorBrush>\r\n"
                L"      <x:Double x:Key='MyDouble'>250</x:Double>\r\n"
                L"      <Style TargetType='Rectangle'>\r\n"
                L"         <Setter Property='Width' Value='50'/>\r\n"
                L"      </Style>\r\n"
                L"    </ResourceDictionary>\r\n"
                L"  </Grid.Resources>\r\n"
                L"  <Button x:Name='buttonOverwritten' Background='{StaticResource testOverwritten}' Width='200'/>\r\n"
                L"  <Button x:Name='buttonApp' Background='{StaticResource testApp}' />\r\n"
                L"  <Button x:Name='buttonStaticThemed' Background='{StaticResource testThemed}' />\r\n"
                L"  <Button x:Name='buttonThemed' Background='{ThemeResource testThemed}' />\r\n"
                L"  <Button x:Name='buttonNoResource' Background='Pink' HorizontalAlignment='Right'>\r\n"
                L"    <Button.Resources>\r\n"
                L"      <SolidColorBrush x:Key='RedBrush'>Green</SolidColorBrush>\r\n"
                L"    </Button.Resources>\r\n"
                L"  </Button>"
                L"  <Button x:Name='redBrushButton' Background='{StaticResource RedBrush}' />\r\n"
                L"  <Ellipse x:Name='redBrushEllipse' Fill='{StaticResource RedBrush}' />\r\n"
                L"</Grid>\r\n"));
        });
        
        return rootPanel;
    }

    static Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper SetupGridAndWait(std::function<void(void)>&& cleanupRoutine)
    {
        Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper cleanup(std::move(cleanupRoutine));

        auto uielement = SetupGrid();
        Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
        {
            test_infra::TestServices::WindowHelper->WindowContent = uielement;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        return cleanup;
    }

    static Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper SetupGridAndWait()
    {
        Microsoft::UI::Xaml::Tests::Common::TestCleanupWrapper cleanup;
    
        auto uielement = SetupGrid();
        Microsoft::UI::Xaml::Tests::Common::RunOnUIThread([&]()
        {
            test_infra::TestServices::WindowHelper->WindowContent = uielement;
        });
    
        test_infra::TestServices::WindowHelper->WaitForIdle();
    
        return cleanup;
    }

    static InstanceHandle GetInstanceHandleForObject(Platform::Object^ object)
    {
        wrl::ComPtr<IInspectable> spObject = reinterpret_cast<IInspectable*>(object);
        wrl::ComPtr<IInspectable> spObjectAsInsp;

        // Doing this query interface makes the pointer point to the IInspectable version of the object
        // instead of just an ICompositionObject that is sitting in an IInspectable pointer
        spObject.As(&spObjectAsInsp);
        return reinterpret_cast<InstanceHandle>(spObjectAsInsp.Get());
    }

private:
    static Platform::Collections::Vector<Platform::Object^>^ GetGroupedData()
    {
        auto groupedData = ref new Platform::Collections::Vector<Platform::Object^>();

        for (unsigned int i = 0; i < 3; ++i)
        {
            Microsoft::UI::Xaml::Tests::Common::GroupedHeader^ group = nullptr;

            switch (i)
            {
            case 0:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Baseball");
                VERIFY_IS_NOT_NULL(group);
                group->Append(L"Robinson Cano");
                break;

            case 1:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Soccer");
                VERIFY_IS_NOT_NULL(group);
                group->Append(L"Lionel Messi");
                break;

            case 2:
                group = ref new Microsoft::UI::Xaml::Tests::Common::GroupedHeader(L"Basketball");
                VERIFY_IS_NOT_NULL(group);
                group->Append(L"Kevin Durant");
                break;
            }

            groupedData->Append(group);
        }

        return groupedData;
    }
};

namespace Tests { namespace Tools { namespace XamlDiagnostics {
    [Microsoft::UI::Xaml::Data::BindableAttribute]
    public ref class Model sealed
    {
    public:
        Model()
        {
            m_readWrite = "ReadWrite";
        }

        property Platform::String^ ReadOnly
        {
            Platform::String^ get() { return "ReadOnly"; }
        }

        property Platform::String^ ReadWrite
        {
            Platform::String^ get() { return m_readWrite; }
            void set(Platform::String^ value)
            {
                m_readWrite = value;
            }
        };

    private:
        Platform::String^ m_readWrite;
    };

}}}
