// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "HubIntegrationTests.h"

#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <Collection.h>
#include <TreeHelper.h>
#include <FileLoader.h>
#include <ControlHelper.h>
#include <TestCleanupWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace Hub {

    bool HubIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool HubIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool HubIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void HubIntegrationTests::CanInstantiate()
    {
        TestCleanupWrapper cleanup;

        Generic::DependencyObjectTests<xaml_controls::Hub>::CanInstantiate();
    }

    void HubIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        TestCleanupWrapper cleanup;

        Generic::FrameworkElementTests<xaml_controls::Hub>::CanEnterAndLeaveLiveTree();
    }

    void HubIntegrationTests::VerifyCanGetSetProperties()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = SetupHubTest();

        xaml_controls::HubSection^ hubSection;
        Platform::String^ hubHeader = L"Hub Header";
        Platform::String^ sectionHeader = L"Section Header";
        xaml::DataTemplate^ hubHeaderTemplate;
        xaml::DataTemplate^ sectionHeaderTemplate;
        xaml::DataTemplate^ sectionContentTemplate;

        // For the following properties we verify that we can Set them to a new value
        // and that a Get returns the value that we had set.
        // We also verify that we can Set/Get null on those that support null.
        //   Hub::Header
        //   Hub::HeaderTemplate
        //   HubSection::Header
        //   HubSection::HeaderTemplate
        //   HubSection::ContentTemplate
        //   HubSection::IsHeaderInteractive

        RunOnUIThread([&]()
        {
            hubSection = hub->Sections->GetAt(0);

            hubHeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                      <TextBlock Foreground="Red" Text="{Binding}" FontSize="15"/>
                  </DataTemplate>)"));

            sectionHeaderTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                      <TextBlock Foreground="Green" Text="{Binding}" FontSize="15"/>
                  </DataTemplate>)"));

            sectionContentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                LR"(<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                      <Rectangle Height="100" Width="100" Fill="Blue" />
                  </DataTemplate>)"));

            hub->Header = hubHeader;
            hub->HeaderTemplate = hubHeaderTemplate;
            hubSection->Header = sectionHeader;
            hubSection->HeaderTemplate = sectionHeaderTemplate;
            hubSection->ContentTemplate = sectionContentTemplate;
            hubSection->IsHeaderInteractive = true;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(hubHeader == safe_cast<Platform::String^>(hub->Header));
            VERIFY_ARE_EQUAL(hubHeaderTemplate, hub->HeaderTemplate);
            VERIFY_IS_TRUE(sectionHeader == safe_cast<Platform::String^>(hubSection->Header));
            VERIFY_ARE_EQUAL(sectionHeaderTemplate, hubSection->HeaderTemplate);
            VERIFY_ARE_EQUAL(sectionContentTemplate, hubSection->ContentTemplate);
            VERIFY_IS_TRUE(hubSection->IsHeaderInteractive);

            hub->Header = nullptr;
            hub->HeaderTemplate = nullptr;
            hubSection->Header = nullptr;
            hubSection->HeaderTemplate = nullptr;
            hubSection->ContentTemplate = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_NULL(hub->Header);
            VERIFY_IS_NULL(hub->HeaderTemplate);
            VERIFY_IS_NULL(hubSection->Header);
            VERIFY_IS_NULL(hubSection->HeaderTemplate);
            VERIFY_IS_NULL(hubSection->ContentTemplate);
        });
    }

    void HubIntegrationTests::CanAddRemoveSections()
    {
        TestCleanupWrapper cleanup;

        // In this test we add and remove HubSections from Hub::Sections.
        // In each case, we verify:
        //   a. Hub::Sections collection is as expected.
        //   b. Hub's ItemsPanel (the panel used to lay out the items) Children collection is as expected.
        //   c. Hub::SectionHeaders collection is as expected.
        //   d. Hub::SectionsInViewChanged event fires as appropriate.
        //   e. Hub::SectionsInView collection is as expected.
        // The actions that we cover in this test are:
        //   a. Remove items.
        //   b. Append items.
        //   c. Clear collection.
        //   d. Insert items.
        //   e. Replace items (i.e. Vector<T>.SetAt)

        xaml_controls::Hub^ hub = SetupHubTest();
        RunOnUIThread([&]()
        {
            // We set up the Hub so that at most 3 items are in view at once
            // (other HubSections we create will also have Width=100).
            hub->Width = 250;
            hub->Sections->GetAt(0)->Width = 100;
            hub->Sections->GetAt(1)->Width = 100;
            hub->Sections->GetAt(2)->Width = 100;
        });
        TestServices::WindowHelper->WaitForIdle();

        xaml_controls::ItemsStackPanel^ itemsPanel;
        unsigned int expectedItemsInView = 3;
        std::vector<xaml_controls::HubSection^> expectedItems;

        // This function is called a number of times during this test to verify that Hub.Sections, Hub.SectionHeaders and ItemsPanel.Children
        // contain the expected items (verified against expectedItems collection).
        std::function<void()> validateCollections = [&]()
        {
            VERIFY_ARE_EQUAL(expectedItems.size(), hub->Sections->Size);
            for (auto i = 0u; i < expectedItems.size(); i++)
            {
                VERIFY_ARE_EQUAL(expectedItems[i], hub->Sections->GetAt(i));
            }

            VERIFY_ARE_EQUAL(expectedItems.size(), itemsPanel->Children->Size);
            for (auto i = 0u; i < expectedItems.size(); i++)
            {
                VERIFY_ARE_EQUAL(expectedItems[i], itemsPanel->Children->GetAt(i));
            }

            VERIFY_ARE_EQUAL(expectedItems.size(), hub->SectionHeaders->Size);
            for (auto i = 0u; i < expectedItems.size(); i++)
            {
                auto headerExpected = safe_cast<Platform::String^>(expectedItems[i]->Header);
                auto headerActual = safe_cast<Platform::String^>(hub->SectionHeaders->GetAt(i));
                VERIFY_ARE_EQUAL(headerExpected, headerActual);
            }
        };

        // We need to handle this collection separately, due to the fact that it is not always correct.
        // TODO: Hub.SectionsInView does not update in response to Hub.Sections.Clear()
        std::function<void()> validateSectionsInView = [&]()
        {
            VERIFY_ARE_EQUAL(expectedItemsInView, hub->SectionsInView->Size);
            for (auto i = 0u; i < expectedItemsInView; i++)
            {
                VERIFY_ARE_EQUAL(expectedItems[i], hub->SectionsInView->GetAt(i));
            }
        };

        Event sectionsInViewChangedEvent;
        auto sectionsInViewRegistration = CreateSafeEventRegistration(xaml_controls::Hub, SectionsInViewChanged);
        sectionsInViewRegistration.Attach(hub, [&]() { sectionsInViewChangedEvent.Set(); });

        RunOnUIThread([&]()
        {
            itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(TreeHelper::GetVisualChildByName(hub, L"Panel"));

            VERIFY_ARE_EQUAL(3u, hub->Sections->Size);
            expectedItems.push_back(hub->Sections->GetAt(0));
            expectedItems.push_back(hub->Sections->GetAt(1));
            expectedItems.push_back(hub->Sections->GetAt(2));

            LOG_OUTPUT(L"Validate collections after initial load");
            validateCollections();

            // Test removing elements
            LOG_OUTPUT(L"Removing elements from Hub::Sections");
            hub->Sections->RemoveAtEnd();
            hub->Sections->RemoveAt(1);

            expectedItems.pop_back();
            expectedItems.pop_back();
            expectedItemsInView = 1;
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate collections after removing elements from Hub::Sections");
            validateCollections();
            validateSectionsInView();

            // Test Appending elements:
            expectedItems.push_back(CreateHubSection(L"Item 2"));
            expectedItems.push_back(CreateHubSection(L"Item 3"));
            expectedItems.push_back(CreateHubSection(L"Item 4"));
            expectedItems.push_back(CreateHubSection(L"Item 5"));

            LOG_OUTPUT(L"Appending elements to Hub::Sections");
            hub->Sections->Append(expectedItems[1]);
            hub->Sections->Append(expectedItems[2]);
            hub->Sections->Append(expectedItems[3]);
            hub->Sections->Append(expectedItems[4]);

            expectedItemsInView = 3;
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate collections after appending elements to Hub::Sections");
            validateCollections();
            validateSectionsInView();

            // Test Clearing elements:
            LOG_OUTPUT(L"Clearing Hub::Sections");
            hub->Sections->Clear();

            expectedItems.clear();
            expectedItemsInView = 0;
        });
        // sectionsInViewChangedEvent.WaitForDefault(); // TODO: SectionsInView issue
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate collections after clearing Hub::Sections");
            validateCollections();
            //validateSectionsInView(); // TODO: SectionsInView issue

            auto item1 = CreateHubSection(L"Item 1");
            auto item2 = CreateHubSection(L"Item 2");
            auto item3 = CreateHubSection(L"Item 3");

            // Test Inserting elements:
            LOG_OUTPUT(L"Inserting element into Hub::Sections");
            hub->Sections->Append(item1);
            hub->Sections->Append(item3);
            hub->Sections->InsertAt(1, item2);

            expectedItems.push_back(item1);
            expectedItems.push_back(item2);
            expectedItems.push_back(item3);
            expectedItemsInView = 3;
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate collections after inserting into Hub::Sections");
            validateCollections();
            validateSectionsInView();

            //Test SetAt
            auto item = CreateHubSection(L"New Item");

            LOG_OUTPUT(L"Setting element in Hub::Sections");
            hub->Sections->SetAt(1, item);
            expectedItems[1] = item;

        });
        //sectionsInViewChangedEvent.WaitForDefault(); // TODO: SectionsInView issue
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Validate collections after setting element in Hub::Sections");
            validateCollections();
            //validateSectionsInView(); // TODO: SectionsInView issue
        });
    }

    void HubIntegrationTests::CanSetHubSectionVisibility()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = SetupHubTest();
        xaml_controls::HubSection^ item1;
        xaml_controls::HubSection^ item2;
        xaml_controls::HubSection^ item3;
        xaml_controls::ScrollViewer^ scrollViewer;
        RunOnUIThread([&]()
        {
            item1 = hub->Sections->GetAt(0);
            item2 = hub->Sections->GetAt(1);
            item3 = hub->Sections->GetAt(2);
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(hub, L"ScrollViewer"));
        });
        TestServices::WindowHelper->WaitForIdle();

        Event sectionsInViewChangedEvent;
        auto sectionsInViewRegistration = CreateSafeEventRegistration(xaml_controls::Hub, SectionsInViewChanged);
        sectionsInViewRegistration.Attach(hub, [&](){ sectionsInViewChangedEvent.Set(); });
        RunOnUIThread([&]()
        {
            item2->Visibility = xaml::Visibility::Collapsed;
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        //A single Flick should get us to the end. We should not snap to an invisible section.
        TestServices::InputHelper->Flick(item1, FlickDirection::West);
        TestServices::WindowHelper->WaitForIdle();
        ValidateHubOnLastSection(scrollViewer);

        //A single Flick should get us to the start.
        TestServices::InputHelper->Flick(item3, FlickDirection::East);
        TestServices::WindowHelper->WaitForIdle();
        ValidateHubOnFirstSection(scrollViewer);

        RunOnUIThread([&]()
        {
            item2->Visibility = xaml::Visibility::Visible;
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::HubZeroItems()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]() {
            auto gridRoot = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                L"      <Hub x:Name='hubContainer' Header='.' > "
                L"      </Hub> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(gridRoot);

            TestServices::WindowHelper->WindowContent = gridRoot;
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::HubOneSection()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = nullptr;
        xaml_controls::HubSection^ hubSection = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        RunOnUIThread([&]()
        {
            auto gridRoot = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                        Width="400" Height="600" HorizontalAlignment="Left" VerticalAlignment="Top" >
                      <Hub x:Name="hub" Header="Header" >
                          <HubSection x:Name="item1" Header="Item Header" >
                              <DataTemplate>
                                  <Grid>
                                      <Rectangle Width="300" Height="50" Fill="Blue" VerticalAlignment="Top" />
                                  </Grid>
                              </DataTemplate>
                          </HubSection>
                      </Hub>
                </Grid>)"));

            hub = safe_cast<xaml_controls::Hub^>(gridRoot->FindName(L"hub"));
            hubSection = safe_cast<xaml_controls::HubSection^>(gridRoot->FindName(L"item1"));

            TestServices::WindowHelper->WindowContent = gridRoot;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(hub, L"ScrollViewer"));
        });

        bool scrollViewerViewChangedFired = false;
        auto scrollViewerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        scrollViewerViewChangedRegistration.Attach(scrollViewer, [&]()
        {
            scrollViewerViewChangedFired = true;
        });

        TestServices::InputHelper->Flick(hubSection, FlickDirection::West);
        TestServices::WindowHelper->WaitForIdle();

        // Since our single HubSection fits in the Hub, we expect that flicking on the Hub should NOT
        // result in the ScrollViewer scrolling.
        VERIFY_IS_FALSE(scrollViewerViewChangedFired);
    }

    void HubIntegrationTests::HubDefaultValues()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = nullptr;
        xaml_controls::HubSection^ section = nullptr;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"HubDefaultValues: Creating instance of hub control.");

            hub = ref new Microsoft::UI::Xaml::Controls::Hub();
            section = ref new xaml_controls::HubSection();

            VERIFY_IS_NOT_NULL(hub);
            VERIFY_IS_NOT_NULL(section);

            hub->Sections->Append(section);

            TestServices::WindowHelper->WindowContent = hub;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"HubDefaultValues: test - verifying default property values.");
            VERIFY_IS_NULL(hub->Header);
            VERIFY_IS_NULL(hub->HeaderTemplate);
            VERIFY_IS_NOT_NULL(hub->Sections);
            VERIFY_ARE_EQUAL(hub->DefaultSectionIndex, -1);
            VERIFY_ARE_EQUAL(hub->SectionHeaders->Size, 1u);
            VERIFY_IS_NULL(section->Header);
            VERIFY_IS_NULL(section->HeaderTemplate);
            VERIFY_IS_NULL(section->ContentTemplate);
            VERIFY_IS_FALSE(section->IsHeaderInteractive);
        });
    }

    void HubIntegrationTests::HubDefaultLayout()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]() {
            Microsoft::UI::Xaml::Controls::Hub^ hubControl = nullptr;

            auto gridRoot = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                L"      <Hub x:Name='hubContainer' Header='.' > "
                L"          <HubSection x:Name='item1' Header='.' > "
                L"              <DataTemplate> "
                L"                  <Grid> "
                L"                      <Rectangle Height='50' Fill='Blue' VerticalAlignment='Top' /> "
                L"                  </Grid> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item2' Header='.' > "
                L"              <DataTemplate> "
                L"                  <Border BorderBrush='Purple' BorderThickness='5' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"</Grid>"));

            VERIFY_IS_NOT_NULL(gridRoot);

            TestServices::WindowHelper->WindowContent = gridRoot;

            hubControl = dynamic_cast<Microsoft::UI::Xaml::Controls::Hub^>(gridRoot->FindName(L"hubContainer"));
            VERIFY_IS_NOT_NULL(hubControl);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::ValidateTextProperties()
    {
        TestCleanupWrapper cleanup;

        Microsoft::UI::Xaml::Controls::Hub^ hubControl = nullptr;
        xaml_controls::HubSection^ hubSection = nullptr;

        RunOnUIThread([&]()
        {
            auto gridRoot = dynamic_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                L"      <Hub x:Name='hubContainer' Header='HubHeader.' > "
                L"          <HubSection x:Name='hubSection1' Header='Section Item1 a b c d e f g h i j k l m n o p q' > "
                L"              <DataTemplate> "
                L"                  <Grid> "
                L"                      <Rectangle Height='50' Fill='Blue' VerticalAlignment='Top' /> "
                L"                  </Grid> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"</Grid>"));

            TestServices::WindowHelper->WindowContent = gridRoot;

            hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(gridRoot->FindName(L"hubContainer"));
            VERIFY_IS_NOT_NULL(hubControl);
            hubSection = safe_cast<Microsoft::UI::Xaml::Controls::HubSection^>(gridRoot->FindName(L"hubSection1"));
            VERIFY_IS_NOT_NULL(hubSection);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headerHost = safe_cast<xaml_controls::Border^>(TreeHelper::GetVisualChildByName(hubControl, L"HeaderHost"));
            VERIFY_IS_NOT_NULL(headerHost);
            auto headerCP = safe_cast<xaml_controls::ContentPresenter^>(xaml_media::VisualTreeHelper::GetChild(headerHost, 0));
            VERIFY_IS_NOT_NULL(headerCP);
            auto textBlockHeader = safe_cast<xaml_controls::TextBlock^>(xaml_media::VisualTreeHelper::GetChild(headerCP, 0));
            VERIFY_IS_NOT_NULL(textBlockHeader);

            VERIFY_ARE_EQUAL(headerCP->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);
            VERIFY_ARE_EQUAL(textBlockHeader->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);

            auto headerButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection, L"HeaderButton"));
            VERIFY_IS_NOT_NULL(headerButton);
            auto sectionHeaderCP = safe_cast<xaml_controls::ContentPresenter^>(TreeHelper::GetVisualChildByName(headerButton, L"ContentPresenter"));
            VERIFY_IS_NOT_NULL(sectionHeaderCP);
            auto textBlockSectionHeader = safe_cast<xaml_controls::TextBlock^>(xaml_media::VisualTreeHelper::GetChild(sectionHeaderCP, 0));
            VERIFY_IS_NOT_NULL(textBlockSectionHeader);

            VERIFY_ARE_EQUAL(sectionHeaderCP->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);
            VERIFY_ARE_EQUAL(textBlockSectionHeader->OpticalMarginAlignment, Microsoft::UI::Xaml::OpticalMarginAlignment::TrimSideBearings);
        });
    }

    void HubIntegrationTests::HubSectionsInViewChanged()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(480, 600));

        xaml_controls::Hub^ hubControl = SetupHubTest();

        xaml_controls::HubSection^ item1 = nullptr;
        xaml_controls::HubSection^ item2 = nullptr;
        xaml_controls::HubSection^ item3 = nullptr;
        Event sectionsInViewChangedEvent;
        auto sectionsInViewRegistration = CreateSafeEventRegistration(xaml_controls::Hub, SectionsInViewChanged);

        RunOnUIThread([&]()
        {
            item1 = safe_cast<xaml_controls::HubSection^>(hubControl->FindName(L"item1"));
            item2 = safe_cast<xaml_controls::HubSection^>(hubControl->FindName(L"item2"));
            item3 = safe_cast<xaml_controls::HubSection^>(hubControl->FindName(L"item3"));

            // We start with 2 items in view. We will later Pan to bring all three items into view:
            hubControl->Width = 400;
            item1->Width = 250;
            item2->Width = 250;
            item3->Width = 250;
        });
        TestServices::WindowHelper->WaitForIdle();

        unsigned int expectedAddedSections = 1;
        unsigned int expectedRemovedSections = 0;
        xaml_controls::HubSection^ expectedAddedItem = item3;
        xaml_controls::HubSection^ expectedRemovedItem = nullptr;

        sectionsInViewRegistration.Attach(hubControl, ref new xaml_controls::SectionsInViewChangedEventHandler(
            [&](Platform::Object^ sender, xaml_controls::SectionsInViewChangedEventArgs^ args)
        {
            LOG_OUTPUT(L"SectionInViewChanged: args->AddedSections->Size %d, args->RemovedSections->Size %d", args->AddedSections->Size, args->RemovedSections->Size);

            VERIFY_IS_NOT_NULL(sender);
            VERIFY_IS_NOT_NULL(args);
            VERIFY_ARE_EQUAL(hubControl, safe_cast<xaml_controls::Hub^>(sender));
            VERIFY_ARE_EQUAL(expectedAddedSections, args->AddedSections->Size);
            VERIFY_ARE_EQUAL(expectedRemovedSections, args->RemovedSections->Size);

            if (expectedAddedItem != nullptr)
            {
                VERIFY_ARE_EQUAL(expectedAddedItem, args->AddedSections->GetAt(0));
            }
            if (expectedRemovedItem != nullptr)
            {
                VERIFY_ARE_EQUAL(expectedRemovedItem, args->RemovedSections->GetAt(0));
            }

            sectionsInViewChangedEvent.Set();
        }));


        // Trigger an Add by panning on the Hub to bring a new section (item3) into view.
        // Note it is important that we Pan here and not Flick as we need to make sure that we don't also trigger a Remove
        // by scrolling past the first item (item1).
        TestServices::InputHelper->PanFromCenter(item1, -120 /*relX*/, 0 /*relY*/, 0.1 /*velocityFactor*/);
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        // Trigger a Remove by removing an item from Hub->Sections:
        expectedAddedSections = 0;
        expectedRemovedSections = 1;
        expectedAddedItem = nullptr;
        expectedRemovedItem = item2;

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Removing an item from the Hub.");
            hubControl->Sections->RemoveAt(1);
        });
        sectionsInViewChangedEvent.WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::DoSectionChange(
        xaml_controls::Hub^ hubControl,
        xaml_controls::ScrollViewer^ scrollViewer,
        Platform::String^ sectionName,
        FlickDirection direction)
    {
        xaml_controls::HubSection^ hubSection = nullptr;

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto sectionsInViewChangedEvent = std::make_shared<Event>();
        auto sectionsInViewRegistration = CreateSafeEventRegistration(xaml_controls::Hub, SectionsInViewChanged);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"DoSectionChange: section name is %s.", sectionName->Data());

            hubSection = safe_cast<xaml_controls::HubSection^>(hubControl->FindName(sectionName));
            VERIFY_IS_NOT_NULL(hubSection);

            viewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    LOG_OUTPUT(L"HubSectionsChange: ViewChanged event fired!.");
                    viewChangedEvent->Set();
                }
            }));

            sectionsInViewRegistration.Attach(
                hubControl,
                ref new xaml_controls::SectionsInViewChangedEventHandler(
                [sectionsInViewChangedEvent](Platform::Object^, xaml_controls::SectionsInViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"HubSectionsChange: SectionInViewChanged event fired!.");
                sectionsInViewChangedEvent->Set();
            }));
        });

        LOG_OUTPUT(L"DoSectionChange: flick on the section.");
        TestServices::InputHelper->Flick(hubSection, direction);
        TestServices::WindowHelper->WaitForIdle();

        sectionsInViewChangedEvent->WaitForDefault();
        viewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"DoSectionChange scrollViewer offset=%f  scrollableWidth=%f", scrollViewer->HorizontalOffset, scrollViewer->ScrollableWidth);
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_controls::Hub^ HubIntegrationTests::SetupHubTest()
    {
        Microsoft::UI::Xaml::Controls::Hub^ hubControl = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"SetupHubTest()");
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top' > "
                L"      <Hub x:Name='hubContainer' Background='Red' Header='. - + : L : + ' ManipulationMode='All'> "
                L"          <HubSection x:Name='item1' Header='item1_0123456789' Width='300'> "
                L"              <DataTemplate> "
                L"                  <Grid> "
                L"                      <Rectangle Height='50' Fill='Red' VerticalAlignment='Top' /> "
                L"                  </Grid> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item2' Header='item2_abcdefghijklmn' Background='Blue' Width='300'> "
                L"              <DataTemplate> "
                L"                  <Border BorderBrush='Yellow' BorderThickness='5' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item3' Header='item3_opqrstuvwxyz' Background='Green' Width='300'> "
                L"              <DataTemplate> "
                L"                  <Border BorderBrush='White' BorderThickness='5' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"</Grid>"));

            hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(rootPanel->FindName(L"hubContainer"));
            VERIFY_IS_NOT_NULL(hubControl);

            loadedRegistration.Attach(
                rootPanel,
                ref new RoutedEventHandler([loadedEvent](Platform::Object^ sender, RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        return hubControl;
    }

    void HubIntegrationTests::ValidateHubOnFirstSection(xaml_controls::ScrollViewer^ scrollViewer)
    {
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
    }

    void HubIntegrationTests::ValidateHubOnLastSection(xaml_controls::ScrollViewer^ scrollViewer)
    {
        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL((int)scrollViewer->HorizontalOffset, (int)scrollViewer->ScrollableWidth);
        });
    }

    xaml_controls::Hub^ HubIntegrationTests::SetupHubSeeMoreTest()
    {
        Microsoft::UI::Xaml::Controls::Hub^ hubControl = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' > "
                L"      <Hub x:Name='hubControl' Background='Beige' Header='Hub SeeMore Test' > "
                L"          <HubSection x:Name='item1' Header='item1_0123456789' IsHeaderInteractive='true'> "
                L"              <DataTemplate> "
                L"                  <Border Width='400' Height='400' BorderBrush='Green' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item2' Header='item2_abcdefghijklmn' Background='Green'> "
                L"              <DataTemplate> "
                L"                  <Border Width='1000' Height='400' BorderBrush='Blue' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item3' Header='item3_opqrstuvwxyz' Background='Blue'> "
                L"              <DataTemplate> "
                L"                  <Border Width='400' Height='400' BorderBrush='Red' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"</Grid>"));

            hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(rootPanel->FindName(L"hubControl"));
            VERIFY_IS_NOT_NULL(hubControl);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return hubControl;
    }

    xaml_controls::SemanticZoom^ HubIntegrationTests::SetupHubSemanticZoomedTest()
    {
        xaml_controls::SemanticZoom^ semanticZoom = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                L"<Grid "
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"  x:Name='LayoutRoot' > "
                L"  <SemanticZoom x:Name='sezo'> "
                L"    <SemanticZoom.ZoomedInView> "
                L"      <Hub x:Name='hubControl' Background='Beige' Header='Hub Semantic Zoom Test' > "
                L"          <HubSection x:Name='item1' Header='item1_0123456789' IsHeaderInteractive='true'> "
                L"              <DataTemplate> "
                L"                  <Border Width='400' Height='400' BorderBrush='Green' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item2' Header='item2_abcdefghijklmn' Background='Green'> "
                L"              <DataTemplate> "
                L"                  <Border Width='1000' Height='400' BorderBrush='Blue' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"          <HubSection x:Name='item3' Header='item3_opqrstuvwxyz' Background='Blue'> "
                L"              <DataTemplate> "
                L"                  <Border Width='400' Height='400' BorderBrush='Red' BorderThickness='10' /> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"    </SemanticZoom.ZoomedInView> "
                L"    <SemanticZoom.ZoomedOutView> "
                L"      <GridView  x:Name='zoomed_out_view' ItemsSource='{Binding ElementName = hubControl, Path = SectionHeaders}' Background='{StaticResource ApplicationPageBackgroundThemeBrush}'> "
                L"        <GridView.ItemsPanel> "
                L"          <ItemsPanelTemplate> "
                L"            <WrapGrid HorizontalChildrenAlignment='Center' VerticalChildrenAlignment='Center' MaximumRowsOrColumns='4' Orientation='Vertical' /> "
                L"          </ItemsPanelTemplate> "
                L"        </GridView.ItemsPanel> "
                L"        <GridView.ItemContainerStyle> "
                L"          <Style TargetType='GridViewItem'> "
                L"            <Setter Property='Height' Value='200' /> "
                L"            <Setter Property='Width' Value='225' /> "
                L"          </Style> "
                L"        </GridView.ItemContainerStyle> "
                L"        <GridView.ItemTemplate> "
                L"          <DataTemplate> "
                L"            <Border> "
                L"              <TextBlock Text='{Binding }' FontSize='30' FontWeight='Bold' HorizontalAlignment='Center' VerticalAlignment='Center' /> "
                L"            </Border> "
                L"          </DataTemplate> "
                L"        </GridView.ItemTemplate> "
                L"      </GridView> "
                L"    </SemanticZoom.ZoomedOutView> "
                L"  </SemanticZoom> "
                L"</Grid>"));

            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(rootPanel->FindName(L"sezo"));
            VERIFY_IS_NOT_NULL(semanticZoom);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        return semanticZoom;
    }

    xaml_controls::Hub^ HubIntegrationTests::SetupHubWithHeaderButtonsTest()
    {
        xaml_controls::Hub^ hubControl = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(  <Grid
                     xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml"
                     x:Name="LayoutRoot" Background="#000000" Width="480" Height="768" HorizontalAlignment="Left" VerticalAlignment="Top" >
                         <Hub x:Name="hubContainer" Background="Red" Header="Hub Header" ManipulationMode="All">
                             <HubSection x:Name="item1" Header="item1" Width="300">
                                 <HubSection.HeaderTemplate>
                                     <DataTemplate>
                                         <Button Content = "Header 1" />
                                     </DataTemplate>
                                 </HubSection.HeaderTemplate>
                                 <DataTemplate>
                                     <Grid>
                                         <Rectangle Height="50" Fill="Red" VerticalAlignment="Top" />
                                     </Grid>
                                 </DataTemplate>
                             </HubSection>
                             <HubSection x:Name="item2" Header="item2" Background="Blue" Width="300">
                                 <HubSection.HeaderTemplate>
                                     <DataTemplate>
                                         <Button Content = "Header 2" />
                                     </DataTemplate>
                                 </HubSection.HeaderTemplate>
                                 <DataTemplate>
                                     <Border BorderBrush="Yellow" BorderThickness="5" />
                                 </DataTemplate>
                             </HubSection>
                             <HubSection x:Name="item3" Header="item3" Background="Green" Width="300">
                                 <HubSection.HeaderTemplate>
                                     <DataTemplate>
                                         <Button Content = "Header 3" />
                                     </DataTemplate>
                                 </HubSection.HeaderTemplate>
                                 <DataTemplate>
                                     <Border BorderBrush="White" BorderThickness="5" />
                                 </DataTemplate>
                             </HubSection>
                         </Hub>
                     </Grid>)"));

            hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(rootPanel->FindName(L"hubContainer"));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        return hubControl;
    }

    void HubIntegrationTests::HubSectionHeaderClick()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::HubSection^ hubSection = nullptr;
        xaml_controls::Button^ headerButton = nullptr;
        xaml_controls::Button^ seeMoreButton = nullptr;

        xaml_controls::Hub^ hubControl = SetupHubSeeMoreTest();

        auto sectionHeaderClickClickEvent = std::make_shared<Event>();
        auto sectionHeaderClickRegistration = CreateSafeEventRegistration(xaml_controls::Hub, SectionHeaderClick);
        auto sectionHeaderClickCount = std::make_shared<int>();

        RunOnUIThread([&]()
        {
            hubSection = safe_cast<xaml_controls::HubSection^>(hubControl->FindName(L"item1"));
            VERIFY_IS_NOT_NULL(hubSection);

            sectionHeaderClickRegistration.Attach(
                hubControl,
                ref new xaml_controls::HubSectionHeaderClickEventHandler(
                [&](Platform::Object^ sender, xaml_controls::HubSectionHeaderClickEventArgs^ args)
            {
                LOG_OUTPUT(L"HubSectionHeaderClick: SectionHeaderClick event fired!.");

                VERIFY_IS_NOT_NULL(sender);
                VERIFY_IS_NOT_NULL(args);
                VERIFY_ARE_EQUAL(hubControl, safe_cast<xaml_controls::Hub^>(sender));
                VERIFY_ARE_EQUAL(hubSection, args->Section);

                *sectionHeaderClickCount += 1;
                sectionHeaderClickClickEvent->Set();
            }));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            headerButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection, L"HeaderButton"));
            seeMoreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection, L"SeeMoreButton"));

            VERIFY_IS_TRUE(hubSection->IsHeaderInteractive);
            VERIFY_IS_TRUE(seeMoreButton->Visibility == Visibility::Visible);
        });

        TestServices::InputHelper->Tap(headerButton);
        TestServices::InputHelper->Tap(seeMoreButton);

        TestServices::WindowHelper->WaitForIdle();
        sectionHeaderClickClickEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(*sectionHeaderClickCount == 1);
        });
    }

    void HubIntegrationTests::ValidateSemanticZoomedOutView()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::HubSection^ hubSection = nullptr;
        xaml_controls::Button^ headerButton = nullptr;

        xaml_controls::SemanticZoom^ semanticZoom = SetupHubSemanticZoomedTest();

        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            hubSection = safe_cast<xaml_controls::HubSection^>(semanticZoom->FindName(L"item1"));
            VERIFY_IS_NOT_NULL(hubSection);

            viewChangeCompletedRegistration.Attach(
                semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeCompleted raised.");
                VERIFY_IS_TRUE(args->IsSourceZoomedInView);
                viewChangeCompletedEvent->Set();
            }));

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            headerButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection, L"HeaderButton"));
            VERIFY_IS_TRUE(hubSection->IsHeaderInteractive);
        });

        TestServices::InputHelper->Tap(headerButton);
        TestServices::WindowHelper->WaitForIdle();

        viewChangeCompletedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

    }

    void HubIntegrationTests::ValidateUIETInSemanticZoom()
    {
        ValidateTreeParams params(
            wf::Size(400, 800),
            1.f,
            []()
            {
                xaml_controls::Grid^ rootGrid = nullptr;
                xaml_controls::Hub^ hub = nullptr;

                RunOnUIThread([&]()
                {
                    rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                        LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <SemanticZoom IsTabStop="False">
                            <SemanticZoom.ZoomedInView>
                                <Hub x:Name="hub" Header="Hub ValidateUIETInSemanticZoom Test" >
                                    <HubSection Header="Item 1" IsHeaderInteractive="True" >
                                        <DataTemplate>
                                            <Border Width="50" Height="200" Background="Red" />
                                        </DataTemplate>
                                    </HubSection>
                                    <HubSection Header="Item 2" >
                                        <DataTemplate>
                                            <Border Width="50" Height="200" Background="Orange" />
                                        </DataTemplate>
                                    </HubSection>
                                    <HubSection Header="Item 3">
                                        <DataTemplate>
                                            <Border Width="50" Height="200" Background="Yellow" />
                                        </DataTemplate>
                                    </HubSection>
                                    <HubSection Header="Item 4" IsEnabled="False" >
                                        <DataTemplate>
                                            <Border Width="50" Height="200" Background="Green" />
                                        </DataTemplate>
                                    </HubSection>
                                </Hub>
                            </SemanticZoom.ZoomedInView>
                        </SemanticZoom>
                    </Grid>)"));

                    hub = safe_cast<xaml_controls::Hub^>(rootGrid->FindName(L"hub"));
                    TestServices::WindowHelper->WindowContent = rootGrid;
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    auto item1 = hub->Sections->GetAt(0);
                    item1->Focus(xaml::FocusState::Pointer);

                    auto item2 = hub->Sections->GetAt(1);
                    auto headerButton2 = safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(item2, L"HeaderButton"));
                    VisualStateManager::GoToState(headerButton2, "PointerOver", false);

                    auto item3 = hub->Sections->GetAt(2);
                    auto headerButton3 = safe_cast<xaml_controls::Control^>(TreeHelper::GetVisualChildByName(item3, L"HeaderButton"));
                    VisualStateManager::GoToState(headerButton3, "Pressed", false);
                });
                TestServices::WindowHelper->WaitForIdle();

                return rootGrid;
            });

        // The hub tests puts some elements in its tree that are not high-contrast elements,
        // so disable the color validation testing.
        params.SkipHCColorValidationOnMasterMismatch = true;

        ControlHelper::ValidateUIElementTree(params);
    }

    void HubIntegrationTests::ValidateUIElementTree()
    {
        ValidateTreeParams params(
            wf::Size(400, 800),
            1.f,
            []()
            {
                return SetupValidateUIElementTreeTest(xaml_controls::Orientation::Horizontal);
            }
        );

        // The hub tests puts some elements in its tree that are not high-contrast elements,
        // so disable the color validation testing.
        params.SkipHCColorValidationOnMasterMismatch = true;

        ControlHelper::ValidateUIElementTree(params);

        // Extra pass to validate the vertical case.  We just care about structure here and
        // are not looking to validate the different themes.
        {
            TestCleanupWrapper cleanup;

            TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 800));

            auto rootGrid = SetupValidateUIElementTreeTest(xaml_controls::Orientation::Vertical);
            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyUIElementTree(L"Vertical");
        }
    }

    xaml_controls::Panel^ HubIntegrationTests::SetupValidateUIElementTreeTest(xaml_controls::Orientation orientation)
    {
        xaml_controls::Grid^ rootGrid = nullptr;
        xaml_controls::Hub^ hub = nullptr;

        RunOnUIThread([&]()
        {
            rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Hub x:Name="hub" Header="Hub Header" >
                            <HubSection IsHeaderInteractive="True" >
                                <HubSection.HeaderTemplate>
                                    <DataTemplate>
                                        <Grid>
                                            <Grid.ColumnDefinitions>
                                                <ColumnDefinition Width="50" />
                                                <ColumnDefinition Width="*" />
                                            </Grid.ColumnDefinitions>
                                            <Rectangle Width="50" Height="50" Fill="Blue" />
                                            <TextBlock Grid.Column="1" Text="HubSection 1" FontSize="28" HorizontalAlignment="Stretch" VerticalAlignment="Center" TextAlignment="Center" />
                                        </Grid>
                                    </DataTemplate>
                                </HubSection.HeaderTemplate>
                                <DataTemplate>
                                    <Border Width="50" Height="200" Background="Green" />
                                </DataTemplate>
                            </HubSection>
                            <HubSection Header="Section 2" IsHeaderInteractive="True" >
                                <DataTemplate>
                                    <Border Width="500" Height="100" Background="Blue" />
                                </DataTemplate>
                            </HubSection>
                            <HubSection Header="Section 3" >
                                <DataTemplate>
                                    <Border Width="400" Height="400" Background="Red" />
                                </DataTemplate>
                            </HubSection>
                        </Hub>
                    </Grid>)"));

            hub = safe_cast<xaml_controls::Hub^>(rootGrid->FindName(L"hub"));
            hub->Orientation = orientation;

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto item1 = hub->Sections->GetAt(0);
            item1->Focus(xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();

        return rootGrid;
    }

    void HubIntegrationTests::HubSectionsEnteredOnlyOnce()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Hub^ hubControl = nullptr;
        xaml_controls::HubSection^ hubSection = nullptr;
        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::HubSection, Loaded);
        int loadedCount = 0;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlString =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"      <Hub x:Name='hubContainer' Background='Red' Header='. - + : L : + ' ManipulationMode='All'> "
                L"          <HubSection Header='HubSection1' Width='300'> "
                L"              <DataTemplate> "
                L"                  <Grid> "
                L"                      <Rectangle Height='50' Fill='Red' VerticalAlignment='Top' /> "
                L"                  </Grid> "
                L"              </DataTemplate> "
                L"          </HubSection> "
                L"      </Hub> "
                L"</Grid>";

            auto panel = safe_cast<xaml_controls::Panel^>(xaml_markup::XamlReader::Load(xamlString));

            hubControl = safe_cast<Microsoft::UI::Xaml::Controls::Hub^>(panel->FindName(L"hubContainer"));
            VERIFY_IS_NOT_NULL(hubControl);

            hubSection = safe_cast<Microsoft::UI::Xaml::Controls::HubSection^>(hubControl->Sections->GetAt(0));
            VERIFY_IS_NOT_NULL(hubSection);

            loadedRegistration.Attach(hubSection, [&]()
            {
                loadedCount++;
                loadedEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = panel;
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        VERIFY_ARE_EQUAL(loadedCount, 1);
    }

    void HubIntegrationTests::ValidateSectionHeadersBinding()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = nullptr;
        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                      <StackPanel>
                        <Hub x:Name="hub" Header="Hub Header" >
                            <HubSection Header="Section 1" />
                            <HubSection Header="Section 2" />
                            <HubSection Header="Section 3" />
                        </Hub>
                        <GridView x:Name="gridView" ItemsSource="{Binding ElementName=hub, Path=SectionHeaders}" Height="100" />
                      </StackPanel>
                    </Grid>)"));

            hub = safe_cast<xaml_controls::Hub^>(rootGrid->FindName(L"hub"));
            gridView = safe_cast<xaml_controls::GridView^>(rootGrid->FindName(L"gridView"));

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(hub->SectionHeaders->Size, 3u);
            VERIFY_ARE_EQUAL(gridView->Items->Size, 3u);

            for (int i = 0; i < (int)hub->SectionHeaders->Size; i++)
            {
                auto sectionHeader = hub->SectionHeaders->GetAt(i);
                auto gridViewItem = gridView->Items->GetAt(i);

                VERIFY_IS_TRUE(sectionHeader == gridViewItem);
            }
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::ValidateSeeMoreButton()
    {
        TestCleanupWrapper cleanup;

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        xaml_controls::HubSection^ hubSection1 = nullptr;

        RunOnUIThread([&]()
        {
            auto rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Hub x:Name="hub" Header="Hub">
                            <HubSection x:Name="hubSection1" IsHeaderInteractive="True">
                                <HubSection.HeaderTemplate>
                                    <DataTemplate>
                                        <Grid>
                                            <Grid.ColumnDefinitions>
                                                <ColumnDefinition Width="50" />
                                                <ColumnDefinition Width="*" />
                                            </Grid.ColumnDefinitions>
                                            <Rectangle Width="50" Height="50" Fill="Blue" />
                                            <TextBlock Grid.Column="1" Text="HubSection 1" FontSize="28" HorizontalAlignment="Stretch" VerticalAlignment="Center" TextAlignment="Center" />
                                        </Grid>
                                    </DataTemplate>
                                </HubSection.HeaderTemplate>
                                <DataTemplate>
                                    <Border Width="100" Height="200" Background="Red" />
                                </DataTemplate>
                            </HubSection>
                            <HubSection Header="Header2" IsHeaderInteractive="True">
                                <DataTemplate>
                                    <Border Width="100" Height="200" Background="Blue" />
                                </DataTemplate>
                            </HubSection>
                        </Hub>
                    </Grid>)"));

            hubSection1 = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSection1"));

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto headerButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection1, L"HeaderButton"));
            VERIFY_ARE_EQUAL(hubSection1->HeaderTemplate, headerButton->ContentTemplate);

            auto seeMoreButton = safe_cast<xaml_controls::Button^>(TreeHelper::GetVisualChildByName(hubSection1, L"SeeMoreButton"));
            VERIFY_IS_NULL(seeMoreButton->ContentTemplate);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void HubIntegrationTests::ValidateFootprint()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 600));

        double expectedHubWidth = 400;
        double expectedHubHeight = 600;
        double expectedHubWithSpecifiedSizeWidth = 200;
        double expectedHubWithSpecifiedSizeHeight = 300;
        double expectedHubSectionEmptyWidth = 24;
        double expectedHubSectionWithHeaderWidth = 154;
        double expectedHubSectionWithHeaderInteractiveWidth = 328;
        double expectedHubSectionWithHeaderTemplateWidth = 359;
        double expectedHubSectionWithWideContentWidth = 524;
        double expectedHubSectiontHeight = 600;
        double expectedHubSectionWithSpecifiedSizeWidth = 500;
        double expectedHubSectionWithSpecifiedSizeHeight = 1000;

        xaml_controls::Hub^ hub = nullptr;
        xaml_controls::Hub^ hubWithSpecifiedSize = nullptr;
        xaml_controls::HubSection^ hubSectionEmpty = nullptr;
        xaml_controls::HubSection^ hubSectionWithHeader = nullptr;
        xaml_controls::HubSection^ hubSectionWithHeaderInteractive = nullptr;
        xaml_controls::HubSection^ hubSectionWithHeaderTemplate = nullptr;
        xaml_controls::HubSection^ hubSectionWithWideContent = nullptr;
        xaml_controls::HubSection^ hubSectionWithSpecifiedSize = nullptr;

        RunOnUIThread([&]()
        {
            auto rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(
                LR"(<Grid xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                        <Hub x:Name="hub" Header="Hub">
                            <HubSection x:Name="hubSectionEmpty">
                            </HubSection>
                            <HubSection x:Name="hubSectionWithHeader" Header="SectionHeader">
                            </HubSection>
                            <HubSection x:Name="hubSectionWithHeaderInteractive" Header="SectionHeaderInteractive" IsHeaderInteractive="True">
                            </HubSection>
                            <HubSection x:Name="hubSectionWithHeaderTemplate" IsHeaderInteractive="True">
                                <HubSection.HeaderTemplate>
                                    <DataTemplate>
                                        <Grid>
                                            <Grid.ColumnDefinitions>
                                                <ColumnDefinition Width="50" />
                                                <ColumnDefinition Width="*" />
                                            </Grid.ColumnDefinitions>
                                            <Rectangle Width="50" Height="50" Fill="Blue" />
                                            <TextBlock Grid.Column="1" Text="HeaderTemplate " FontSize="28" HorizontalAlignment="Stretch" VerticalAlignment="Center" TextAlignment="Center" />
                                        </Grid>
                                    </DataTemplate>
                                </HubSection.HeaderTemplate>
                                <DataTemplate>
                                    <Border Width="100" Height="200" Background="Red" />
                                </DataTemplate>
                            </HubSection>
                            <HubSection x:Name="hubSectionWithWideContent" Header="SectionHeaderWideContent" IsHeaderInteractive="True">
                                <DataTemplate>
                                    <Border Width="500" Height="300" Background="Blue" />
                                </DataTemplate>
                            </HubSection>
                            <HubSection x:Name="hubSectionWithSpecifiedSize" Width="500" Height="1000" Header="SectionHeaderWideSpecifiedSize" IsHeaderInteractive="True">
                                <DataTemplate>
                                    <Border Width="200" Height="300" Background="Red" />
                                </DataTemplate>
                            </HubSection>
                        </Hub>
                        <Hub x:Name="hubWithSpecifiedSize" Width="200" Height="300">
                        </Hub>
                    </Grid>)"));

            hub = safe_cast<xaml_controls::Hub^>(rootGrid->FindName(L"hub"));

            hubSectionEmpty = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionEmpty"));
            hubSectionWithHeader = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionWithHeader"));
            hubSectionWithHeaderInteractive = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionWithHeaderInteractive"));
            hubSectionWithHeaderTemplate = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionWithHeaderTemplate"));
            hubSectionWithWideContent = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionWithWideContent"));
            hubSectionWithSpecifiedSize = safe_cast<xaml_controls::HubSection^>(rootGrid->FindName(L"hubSectionWithSpecifiedSize"));

            hubWithSpecifiedSize = safe_cast<xaml_controls::Hub^>(rootGrid->FindName(L"hubWithSpecifiedSize"));

            TestServices::WindowHelper->WindowContent = rootGrid;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(expectedHubWidth, hub->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubHeight, hub->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubWithSpecifiedSizeWidth, hubWithSpecifiedSize->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubWithSpecifiedSizeHeight, hubWithSpecifiedSize->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionEmptyWidth, hubSectionEmpty->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectiontHeight, hubSectionEmpty->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionWithHeaderWidth, hubSectionWithHeader->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectiontHeight, hubSectionWithHeader->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionWithHeaderInteractiveWidth, hubSectionWithHeaderInteractive->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectiontHeight, hubSectionWithHeaderInteractive->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionWithHeaderTemplateWidth, hubSectionWithHeaderTemplate->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectiontHeight, hubSectionWithHeaderTemplate->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionWithWideContentWidth, hubSectionWithWideContent->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectiontHeight, hubSectionWithWideContent->ActualHeight);

            VERIFY_ARE_EQUAL(expectedHubSectionWithSpecifiedSizeWidth, hubSectionWithSpecifiedSize->ActualWidth);
            VERIFY_ARE_EQUAL(expectedHubSectionWithSpecifiedSizeHeight, hubSectionWithSpecifiedSize->ActualHeight);
        });
    }

    void HubIntegrationTests::NavigateThroughHubSectionsUsingKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Hub^ hub = SetupHubTest();
        xaml_controls::HubSection^ sect0;
        xaml_controls::HubSection^ sect1;
        xaml_controls::HubSection^ sect2;
        xaml_controls::ScrollViewer^ scrollViewer;

        RunOnUIThread([&]()
        {
            sect0 = hub->Sections->GetAt(0);
            sect1 = hub->Sections->GetAt(1);
            sect2 = hub->Sections->GetAt(2);
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(hub, L"ScrollViewer"));

            hub->Width = 300;

            sect0->Width = 600;
            sect1->Width = 600;
            sect2->Width = 600;

            //Hub doesn't handle arrow keys unless sections are focusable.
            //The three lines below can be removed when this is fixed.
            sect0->IsTabStop = true;
            sect1->IsTabStop = true;
            sect2->IsTabStop = true;

            sect0->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(sect0));
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the right arrow key, expect the scroll viewer to scroll 45 to the right.
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 45);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the left arrow key, expect the scroll viewer to scroll 45 to the left, back to 0.
        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing PageDown, expect the scroll viewer to jump the width of the hub(300) right.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 300);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing PageDown, expect the scroll viewer to jump the width of the hub(300) left, back to 0.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When calling Scroll to Section, expect the ScrollViewer to scroll until the section is focused.
        //In this case 1200 to the right, because the two sections before it were each 600 wide.
        RunOnUIThread([&]()
        {
            hub->ScrollToSection(sect2);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 1200);
        });
    }

    void HubIntegrationTests::NavigateThroughHubSectionsWithHeaderButtonsUsingKeyboard()
    {
        TestCleanupWrapper cleanup;

        LOG_OUTPUT(L"NavigateThroughHubSectionsWithHeaderButtonsUsingKeyboard: Start.");

        xaml_controls::HubSection^ sect0 = nullptr;
        xaml_controls::HubSection^ sect1 = nullptr;
        xaml_controls::HubSection^ sect2 = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        xaml_controls::Hub^ hub = SetupHubWithHeaderButtonsTest();

        auto hubSectionGotFocusEvent = std::make_shared<Event>();
        auto gotFocusRegistration = CreateSafeEventRegistration(xaml_controls::HubSection, GotFocus);

        RunOnUIThread([&]()
        {
            sect0 = hub->Sections->GetAt(0);
            sect1 = hub->Sections->GetAt(1);
            sect2 = hub->Sections->GetAt(2);
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(hub, L"ScrollViewer"));

            hub->Width = 300;

            sect0->Width = 600;
            sect1->Width = 600;
            sect2->Width = 600;

            gotFocusRegistration.Attach(sect0, [&]()
            {
                LOG_OUTPUT(L"HubSection: Got Focus Event Fired.");
                hubSectionGotFocusEvent->Set();
            });

            sect0->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();
        hubSectionGotFocusEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the right arrow key, expect the scroll viewer to jump to the next header button.
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 413);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the left arrow key, expect the scroll viewer to jump to the previous header button.
        TestServices::KeyboardHelper->Left();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the PageDown key, expect the scroll viewer to jump to the next header button.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 413);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When pressing the PageUp key, expect the scroll viewer to jump to the previous header button.
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 0);
        });
        TestServices::WindowHelper->WaitForIdle();

        //When calling Scroll to Section, expect the ScrollViewer to scroll until the section is focused.
        //In this case 1200 to the right, because the two sections before it were each 600 wide.
        RunOnUIThread([&]()
        {
            hub->ScrollToSection(sect2);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"HorizontalOffset = %f", scrollViewer->HorizontalOffset);
            VERIFY_ARE_EQUAL(scrollViewer->HorizontalOffset, 1200);
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"NavigateThroughHubSectionsWithHeaderButtonsUsingKeyboard: Completed.");
    }

    xaml_controls::HubSection^ HubIntegrationTests::CreateHubSection(Platform::String^ name, Platform::Object^ content, Platform::Object^ header, double width)
    {
        auto contentTemplate = safe_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
            LR"(<DataTemplate xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml">
                    <Grid Background="LightBlue">
                        <ContentPresenter Content="{Binding}" />
                    </Grid>
                </DataTemplate>)"));

        auto item = ref new xaml_controls::HubSection();
        item->ContentTemplate = contentTemplate;
        item->Width = width;
        item->Name = name;
        item->Header = header != nullptr ? header : name;
        item->DataContext = content != nullptr ? content : name;

        return item;
    }

} } } } } }
