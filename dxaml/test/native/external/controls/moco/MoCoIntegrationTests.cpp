// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "MoCoIntegrationTests.h"

#include <SafeEventRegistration.h>
#include <ItemsControlHelper.h>
#include <TestEvent.h>
#include <ControlHelper.h>
#include <MocoHelper.h>
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <Collection.h>
#include <StoryboardMonitorWrapper.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

#include <CustomTypeMetadataProvider.h>
#include <CustomListViewItemPresenter.h>
#include <CustomListViewItemPanel.h>
#include "KeyboardInjectionOverride.h"
#include <WUCRenderingScopeGuard.h>
#include "FocusTestHelper.h"
#include <Utils.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace MoCo {

    bool MoCoIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();

        return true;
    }

    bool MoCoIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider());

        // Leak: TemplateContent peer not being unpegged. We do this in TestSetup
        // so it affects every test.
        TestServices::ErrorHandlingHelper->IgnoreLeaksForTest();
        return true;
    }

    bool MoCoIntegrationTests::TestCleanup()
    {
        // this applies only to Desktop runs
        if (TestServices::Utilities->IsDesktop)
        {
            // Workaround for a known issue
            TestServices::WindowHelper->RestoreForegroundWindow();
        }

        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void MoCoIntegrationTests::CanInstantiateListView()
    {
         TestCleanupWrapper cleanup;
         Generic::DependencyObjectTests<xaml_controls::ListView>::CanInstantiate();
    }

    void MoCoIntegrationTests::CanInstantiateGridView()
    {
         TestCleanupWrapper cleanup;
         Generic::DependencyObjectTests<xaml_controls::GridView>::CanInstantiate();
    }

    void MoCoIntegrationTests::CanEnterAndLeaveLiveTreeListView()
    {
         TestCleanupWrapper cleanup;
         Generic::FrameworkElementTests<xaml_controls::ListView>::CanEnterAndLeaveLiveTree();
    }

    void MoCoIntegrationTests::CanEnterAndLeaveLiveTreeGridView()
    {
         TestCleanupWrapper cleanup;

         Generic::FrameworkElementTests<xaml_controls::GridView>::CanEnterAndLeaveLiveTree();
    }

    void MoCoIntegrationTests::CanSelectItemListView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListViewBase^ list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::ListView, 5, 500, 500);

        PerformSelectionTests(list);
    }

    void MoCoIntegrationTests::CanSelectItemGridView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListViewBase^ list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::GridView, 5, 500 ,500);

        PerformSelectionTests(list);
    }

    void MoCoIntegrationTests::PerformSelectionTests(xaml_controls::ListViewBase^ list)
    {
        LOG_OUTPUT(L"Testing programmatic selection.");
        RunOnUIThread([&] () {
            LOG_OUTPUT(L"Verifying the initial selected count.");
            unsigned int selectedItemCount = list->SelectedItems->Size;
            VERIFY_ARE_EQUAL(0, (int)selectedItemCount);

            LOG_OUTPUT(L"Verifying second item is selected.");
            list->SelectedIndex = 1;
            selectedItemCount = list->SelectedItems->Size;
            VERIFY_ARE_EQUAL(1, (int)selectedItemCount);

            list->IsSwipeEnabled = TRUE;
        });
        test_infra::TestServices::WindowHelper->WaitForIdle();

        // Test Selection with touch
        LOG_OUTPUT(L"Testing touch selection.");
        MocoHelper::SelectItemAtIndex(list, 2, true);
        RunOnUIThread([&] () {
            VERIFY_ARE_EQUAL(2, list->SelectedIndex);
        });
        test_infra::TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::CanVirtualizeAndRealizeListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::ListView, 25, 50, 50);

        PerformVirtualizationAndRealizationTests(list);
    }

    void MoCoIntegrationTests::CanVirtualizeAndRealizeGridView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::GridView, 25, 50, 50);

        PerformVirtualizationAndRealizationTests(list);
    }

    void MoCoIntegrationTests::CanUseCICAndReturnAListViewItemWithCustomItemTemplate()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ListViewItem^ providedContainer = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            providedContainer = ref new xaml_controls::ListViewItem();
            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 1, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [collection, providedContainer](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                VERIFY_IS_NOT_NULL(args);
                VERIFY_IS_NULL(args->ItemContainer);
                VERIFY_IS_NOT_NULL(args->Item);

                args->ItemContainer = providedContainer;

                providedContainer->ContentTemplate = dynamic_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                    L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                    L"  <Grid Name=\"testing\" />"
                    L"</DataTemplate>"
                    ));

                args->IsContainerPrepared = true;
            }));

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [list, providedContainer](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                VERIFY_IS_NOT_NULL(args);
                VERIFY_ARE_EQUAL(args->ItemContainer, providedContainer);
                VERIFY_IS_NOT_NULL(args->Item);

                VERIFY_IS_TRUE(args->ItemContainer->ContentTemplateRoot->GetValue(FrameworkElement::NameProperty)->ToString() == "testing");
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();


    }

    void MoCoIntegrationTests::CanUseCICAsADataTemplateSelectorReplacement()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);
        std::vector<xaml_controls::ListViewItem^> notChoosenContainers;
        std::vector<xaml_controls::ListViewItem^> recycledContainers;
        bool choosingMode = false;

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            list->ItemsPanel = dynamic_cast<xaml_controls::ItemsPanelTemplate^>(xaml_markup::XamlReader::Load(
                L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"  <ItemsStackPanel CacheLength='4' />"
                L"</ItemsPanelTemplate>"
                ));

            MocoHelper::PopulateList(list, 1000, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [collection, &notChoosenContainers, &recycledContainers, &choosingMode](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                if (args->ItemContainer == nullptr)
                {
                    // we are starting up
                    args->ItemContainer = ref new xaml_controls::ListViewItem();
                }
                else
                {
                    // choosing mode false means we are going to ignore the suggestion and create a new one
                    // in effect, this is testing that we do not get suggested the same container each time but are getting new suggestions
                    if (choosingMode == false)
                    {
                        // make sure that we haven't already put this guy in
                        VERIFY_IS_TRUE(std::find(notChoosenContainers.begin(), notChoosenContainers.end(), safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer)) == notChoosenContainers.end());

                        // aha, we are recycling..
                        // let us not take this suggestion.. i didn't like it for instance
                        if (notChoosenContainers.size() < 5)
                        {
                            // store 5 containers in there
                            notChoosenContainers.push_back(safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer));
                        }

                        args->ItemContainer = ref new xaml_controls::ListViewItem;

                        if (notChoosenContainers.size() == 5)
                        {
                            // ok, enough of this
                            choosingMode = true;
                        }
                    }
                    else
                    // choosing mode true means we are going to start using our backup
                    // in effect, this is testing that the containers we got were properly recycled and are still valid to be used
                    {
                        // let us start using the choosen containers
                        if (!notChoosenContainers.empty())
                        {
                            auto iter = std::find(recycledContainers.begin(), recycledContainers.end(), safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer));
                            // it had better have been registered
                            VERIFY_IS_TRUE(iter != recycledContainers.end());
                            recycledContainers.erase(iter);

                            args->ItemContainer = notChoosenContainers.back();
                            notChoosenContainers.pop_back();
                        }
                        else
                        {
                            VERIFY_IS_TRUE(recycledContainers.size() == 1);  // still have to raise CCC for it :)

                            // switch back again
                            choosingMode = false;
                        }
                    }
                }

                args->IsContainerPrepared = true;
            }));

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [list, &recycledContainers, &notChoosenContainers](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                if (args->InRecycleQueue)
                {
                    // should not yet be in here
                    VERIFY_IS_TRUE(std::find(recycledContainers.begin(), recycledContainers.end(), safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer)) == recycledContainers.end());
                    recycledContainers.push_back(safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer));

                    // and since we have not run cic yet, should not be in cic either
                    VERIFY_IS_TRUE(std::find(notChoosenContainers.begin(), notChoosenContainers.end(), safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer)) == notChoosenContainers.end());

                }
                else
                {
                    auto iter = std::find(recycledContainers.begin(), recycledContainers.end(), safe_cast<xaml_controls::ListViewItem^>(args->ItemContainer));
                    if (iter != recycledContainers.end())
                    {
                        recycledContainers.erase(iter);
                    }
                    args->ItemContainer->Content = args->ItemIndex;
                    args->Handled = true;
                }
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        for (int i = 30; i < 50; i++)
        {
            // scroll down one at a time
            RunOnUIThread([&]()
            {
                list->ScrollIntoView(collection->GetAt(i));
            });

            test_infra::TestServices::WindowHelper->WaitForIdle();
        }

        VERIFY_IS_TRUE(recycledContainers.size() == notChoosenContainers.size());
    }

    void MoCoIntegrationTests::PerformVirtualizationAndRealizationTests(xaml_controls::ListViewBase^ list)
    {
        LOG_OUTPUT(L"Starting virtualization and realization tests.");
        int scrollIndex = 0;

        RunOnUIThread([&] ()
        {
            LOG_OUTPUT(L"Check if the last item is virtualized");
            auto lastItem = list->ContainerFromIndex(list->Items->Size - 1);
            VERIFY_IS_NULL(lastItem);

            LOG_OUTPUT(L"Check if first item is realized");
            auto firstItem = list->ContainerFromIndex(0);
            VERIFY_IS_NOT_NULL(firstItem);

            scrollIndex = list->Items->Size- 1;
        });

        ItemsControlHelper::ScrollToIndex<xaml_controls::ListViewBase, Object>(list, scrollIndex);

        RunOnUIThread([&] ()
        {
            // Calling update layout so the virtualization path can kick in.
            list->UpdateLayout();

            LOG_OUTPUT(L"Check if the last item is realized");
            auto lastItem = list->ContainerFromIndex(list->Items->Size - 1);
            VERIFY_IS_NOT_NULL(lastItem);

            LOG_OUTPUT(L"Check if first item is virtualized.");
            auto firstItem = list->ContainerFromIndex(1);
            VERIFY_IS_NULL(firstItem);
        });
    }

    void MoCoIntegrationTests::ContainerContentChangingTestListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::ListView, 25, 50, 50);

        PerformContainerContentChangingTests(list);
    }

    void MoCoIntegrationTests::ContainerContentChangingTestGridView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::GridView, 25, 50, 50);

        PerformContainerContentChangingTests(list);
    }

    void MoCoIntegrationTests::PerformContainerContentChangingTests(xaml_controls::ListViewBase^ list)
    {
        LOG_OUTPUT(L"Starting ContainerContentChanging tests.");

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);
        auto cCCEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();
        int scrollIndex = 0;

        RunOnUIThread([&] ()
        {
            scrollIndex = list->Items->Size - 1;

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

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [cCCEvent, list](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                VERIFY_IS_NOT_NULL(args);
                VERIFY_IS_NOT_NULL(args->ItemContainer);

                // In some cases container will not have an associated item, this is not an error scenario so omitting the verification
                // for that cases.
                if (args->Item == nullptr)
                {
                    auto itemFromArgs = safe_cast<MocoBasicFlatDataModel^>(args->Item);
                    VERIFY_IS_NOT_NULL(itemFromArgs);

                    auto itemFromSource = safe_cast<MocoBasicFlatDataModel^>(list->Items->GetAt(args->ItemIndex));
                    VERIFY_IS_NOT_NULL(itemFromSource);

                    VERIFY_IS_TRUE(itemFromArgs->Name == itemFromSource->Name);
                }

                cCCEvent->Set();
            }));

            list->ScrollIntoView(list->Items->GetAt(scrollIndex));
        });

        LOG_OUTPUT(L"Waiting for ViewChanged event.");
        viewChangedEvent->WaitForDefault();
        LOG_OUTPUT(L"Waiting for ContentContainerChanging event.");
        cCCEvent->WaitForDefault();
    }

    void MoCoIntegrationTests::FirstVisibleItemIsRetainedOnResetListView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpResettableEnvironment(MocoHelper::ListControlType::ListView, 50);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        MocoResettableCollection^ collection = nullptr;

        int indexToRemoveAt = 15;
        double previousVerticalOffset = 0;
        double expectedVerticalOffsetChange = 0;
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&] ()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(list, L"ScrollViewer"));
            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            viewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            LOG_OUTPUT(L"Scrolling to item %d.", indexToRemoveAt);
            list->ScrollIntoView(collection->GetAt(indexToRemoveAt), xaml_controls::ScrollIntoViewAlignment::Leading);
        });

        LOG_OUTPUT(L"Waiting for ViewChanged event.");
        viewChangedEvent->WaitForDefault();

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

            viewChangedEvent->Reset();

            LOG_OUTPUT(L"Scrolling to item %d.", indexToRemoveAt);
            list->ScrollIntoView(collection->GetAt(indexToRemoveAt), xaml_controls::ScrollIntoViewAlignment::Leading);
        });

        LOG_OUTPUT(L"Waiting for ViewChanged event.");
        viewChangedEvent->WaitForDefault();

        RunOnUIThread([&] ()
        {
            auto containerAtIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(indexToRemoveAt));
            auto containerAtFlippedIndex = safe_cast<FrameworkElement^>(list->ContainerFromIndex(collection->Size - 1 - indexToRemoveAt));

            auto atTransformToRoot = containerAtIndex->TransformToVisual(nullptr);
            auto atFlippedTransformToRoot = containerAtFlippedIndex->TransformToVisual(nullptr);

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

    void MoCoIntegrationTests::ListViewItemsHaveAReasonableSuggestionPattern()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpResettableEnvironment(MocoHelper::ListControlType::ListView, 2);
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        DependencyObject^ originalContainer1 = nullptr;
        DependencyObject^ originalContainer2 = nullptr;
        MocoResettableCollection^ collection = nullptr;
        int cicCalls = 0;

        RunOnUIThread([&] ()
        {
            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);
            originalContainer1 = list->ContainerFromIndex(0);
            originalContainer2 = list->ContainerFromIndex(1);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [&cicCalls, &originalContainer1, &originalContainer2](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                if (args->ItemIndex == 0)
                {
                    VERIFY_ARE_EQUAL(args->ItemContainer, originalContainer1);
                }
                else if (args->ItemIndex == 1)
                {
                    VERIFY_ARE_EQUAL(args->ItemContainer, originalContainer2);
                }
                else
                {
                    VERIFY_FAIL();
                }

                cicCalls++;
            }));

            collection->Reset();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(cicCalls == 2);
    }

    void MoCoIntegrationTests::ListViewItemsAreTheSameAfterResetWithChoosingItemContainer()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ListViewItem^ providedContainer1 = nullptr;
        xaml_controls::ListViewItem^ providedContainer2 = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        RunOnUIThread([&] ()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            providedContainer1 = ref new xaml_controls::ListViewItem();
            providedContainer2 = ref new xaml_controls::ListViewItem();
            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 2, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [collection, providedContainer1, providedContainer2](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                unsigned int index = 0;
                bool wasFound = collection->IndexOf(args->Item, &index);

                if (wasFound)
                {
                    if (index == 0)
                    {
                        args->ItemContainer = providedContainer1;
                    }
                    else if (index == 1)
                    {
                        args->ItemContainer = providedContainer2;
                    }
                    else
                    {
                        // We only have two items, so if this item is not at position 0 or 1, something is very wrong.
                        VERIFY_FAIL();
                    }
                }
                else
                {
                    // If we got an item not in our collection, then something is very wrong.
                    VERIFY_FAIL();
                }
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            DependencyObject^ originalContainer1 = list->ContainerFromIndex(0);
            DependencyObject^ originalContainer2 = list->ContainerFromIndex(1);

            VERIFY_ARE_EQUAL(originalContainer1, providedContainer1);
            VERIFY_ARE_EQUAL(originalContainer2, providedContainer2);

            collection->Reset();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            DependencyObject^ newContainer1 = list->ContainerFromIndex(0);
            DependencyObject^ newContainer2 = list->ContainerFromIndex(1);

            VERIFY_ARE_EQUAL(newContainer1, providedContainer1);
            VERIFY_ARE_EQUAL(newContainer2, providedContainer2);
        });
    }

    void MoCoIntegrationTests::ValidateGridViewVisualTree()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 666.67f));
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <GridView>"
                L"    <x:String>Item 1</x:String>"
                L"    <x:String>Item 2</x:String>"
                L"    <x:String>Item 3</x:String>"
                L"  </GridView>"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Set focus so that we get consistent results with the verification
            ((xaml_controls::GridView^)rootPanel->Children->GetAt(0))->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTree();
    }

    void MoCoIntegrationTests::ValidateListViewVisualTree()
    {
        TestCleanupWrapper cleanup;
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400, 666.67f));
        xaml_controls::Grid^ rootPanel = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ListView>"
                L"    <x:String>Item 1</x:String>"
                L"    <x:String>Item 2</x:String>"
                L"    <x:String>Item 3</x:String>"
                L"  </ListView>"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            // Set focus so that we get consistent results with the verification
            ((xaml_controls::ListView^)rootPanel->Children->GetAt(0))->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
        });

        TestServices::WindowHelper->WaitForIdle();
        TestServices::Utilities->VerifyUIElementTree();
    }

    void MoCoIntegrationTests::CanAccessAndEditConvergedPropertiesGridView()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            auto gridViewItemPresenter = ref new xaml_primitives::GridViewItemPresenter();
            auto aliceBlue = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::AliceBlue);
            auto thickness = xaml::ThicknessHelper::FromUniformLength(10);

            gridViewItemPresenter->DragBackground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->DragBackground, aliceBlue);

            gridViewItemPresenter->SelectedPointerOverBorderBrush = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectedPointerOverBorderBrush, aliceBlue);

            gridViewItemPresenter->DragOpacity = 0.5;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->DragOpacity, 0.5);

            gridViewItemPresenter->ReorderHintOffset = 10.0;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->ReorderHintOffset, 10.0);

            gridViewItemPresenter->VerticalContentAlignment = xaml::VerticalAlignment::Center;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->VerticalContentAlignment, xaml::VerticalAlignment::Center);

            gridViewItemPresenter->PointerOverBackgroundMargin = thickness;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->PointerOverBackgroundMargin, thickness);

            gridViewItemPresenter->ContentMargin = thickness;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->ContentMargin, thickness);

            gridViewItemPresenter->Padding = thickness;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->Padding, thickness);

            gridViewItemPresenter->SelectedForeground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectedForeground, aliceBlue);

            gridViewItemPresenter->CheckSelectingBrush = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->CheckSelectingBrush, aliceBlue);

            gridViewItemPresenter->SelectedPointerOverBackground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectedPointerOverBackground, aliceBlue);

            gridViewItemPresenter->PlaceholderBackground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->PlaceholderBackground, aliceBlue);

            gridViewItemPresenter->FocusBorderBrush = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->FocusBorderBrush, aliceBlue);

            gridViewItemPresenter->SelectedBackground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectedBackground, aliceBlue);

            gridViewItemPresenter->DisabledOpacity = 1;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->DisabledOpacity, 1);

            gridViewItemPresenter->PointerOverBackground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->PointerOverBackground, aliceBlue);

            gridViewItemPresenter->SelectedBorderThickness = thickness;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectedBorderThickness, thickness);

            gridViewItemPresenter->CheckBrush = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->CheckBrush, aliceBlue);

            gridViewItemPresenter->DragForeground = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->DragForeground, aliceBlue);

            gridViewItemPresenter->SelectionCheckMarkVisualEnabled = false;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->SelectionCheckMarkVisualEnabled, false);

            gridViewItemPresenter->HorizontalContentAlignment = xaml::HorizontalAlignment::Center;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->HorizontalContentAlignment, xaml::HorizontalAlignment::Center);

            gridViewItemPresenter->CheckHintBrush = aliceBlue;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->CheckHintBrush, aliceBlue);

            gridViewItemPresenter->GridViewItemPresenterHorizontalContentAlignment = xaml::HorizontalAlignment::Center;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->GridViewItemPresenterHorizontalContentAlignment, xaml::HorizontalAlignment::Center);

            gridViewItemPresenter->GridViewItemPresenterVerticalContentAlignment = xaml::VerticalAlignment::Center;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->GridViewItemPresenterVerticalContentAlignment, xaml::VerticalAlignment::Center);

            gridViewItemPresenter->GridViewItemPresenterPadding = thickness;
            VERIFY_ARE_EQUAL(gridViewItemPresenter->GridViewItemPresenterPadding, thickness);
        });
    }

    void MoCoIntegrationTests::CanAccessAndEditConvergedPropertiesListView()
    {
        TestCleanupWrapper cleanup;
        RunOnUIThread([&]()
        {
            auto listViewItemPresenter = ref new xaml_primitives::ListViewItemPresenter();
            auto aliceBlue = ref new xaml_media::SolidColorBrush(Microsoft::UI::Colors::AliceBlue);
            auto thickness = xaml::ThicknessHelper::FromUniformLength(10);

            listViewItemPresenter->DragBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->DragBackground, aliceBlue);

            listViewItemPresenter->SelectedPointerOverBorderBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectedPointerOverBorderBrush, aliceBlue);

            listViewItemPresenter->DragOpacity = 0.5;
            VERIFY_ARE_EQUAL(listViewItemPresenter->DragOpacity, 0.5);

            listViewItemPresenter->ReorderHintOffset = 10.0;
            VERIFY_ARE_EQUAL(listViewItemPresenter->ReorderHintOffset, 10.0);

            listViewItemPresenter->VerticalContentAlignment = xaml::VerticalAlignment::Center;
            VERIFY_ARE_EQUAL(listViewItemPresenter->VerticalContentAlignment, xaml::VerticalAlignment::Center);

            listViewItemPresenter->PointerOverBackgroundMargin = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->PointerOverBackgroundMargin, thickness);

            listViewItemPresenter->ContentMargin = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->ContentMargin, thickness);

            listViewItemPresenter->Padding = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->Padding, thickness);

            listViewItemPresenter->SelectedForeground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectedForeground, aliceBlue);

            listViewItemPresenter->CheckSelectingBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->CheckSelectingBrush, aliceBlue);

            listViewItemPresenter->SelectedPointerOverBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectedPointerOverBackground, aliceBlue);

            listViewItemPresenter->PlaceholderBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->PlaceholderBackground, aliceBlue);

            listViewItemPresenter->FocusBorderBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->FocusBorderBrush, aliceBlue);

            listViewItemPresenter->SelectedBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectedBackground, aliceBlue);

            listViewItemPresenter->DisabledOpacity = 1;
            VERIFY_ARE_EQUAL(listViewItemPresenter->DisabledOpacity, 1);

            listViewItemPresenter->PointerOverBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->PointerOverBackground, aliceBlue);

            listViewItemPresenter->SelectedBorderThickness = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectedBorderThickness, thickness);

            listViewItemPresenter->CheckBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->CheckBrush, aliceBlue);

            listViewItemPresenter->DragForeground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->DragForeground, aliceBlue);

            listViewItemPresenter->SelectionCheckMarkVisualEnabled = false;
            VERIFY_ARE_EQUAL(listViewItemPresenter->SelectionCheckMarkVisualEnabled, false);

            listViewItemPresenter->HorizontalContentAlignment = xaml::HorizontalAlignment::Center;
            VERIFY_ARE_EQUAL(listViewItemPresenter->HorizontalContentAlignment, xaml::HorizontalAlignment::Center);

            listViewItemPresenter->CheckHintBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->CheckHintBrush, aliceBlue);

            listViewItemPresenter->ListViewItemPresenterHorizontalContentAlignment = xaml::HorizontalAlignment::Center;
            VERIFY_ARE_EQUAL(listViewItemPresenter->ListViewItemPresenterHorizontalContentAlignment, xaml::HorizontalAlignment::Center);

            listViewItemPresenter->ListViewItemPresenterVerticalContentAlignment = xaml::VerticalAlignment::Center;
            VERIFY_ARE_EQUAL(listViewItemPresenter->ListViewItemPresenterVerticalContentAlignment, xaml::VerticalAlignment::Center);

            listViewItemPresenter->ListViewItemPresenterPadding = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->ListViewItemPresenterPadding, thickness);

            listViewItemPresenter->RevealBackgroundShowsAboveContent = true;
            VERIFY_ARE_EQUAL(listViewItemPresenter->RevealBackgroundShowsAboveContent, true);

            listViewItemPresenter->RevealBackground = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->RevealBackground, aliceBlue);

            listViewItemPresenter->RevealBorderBrush = aliceBlue;
            VERIFY_ARE_EQUAL(listViewItemPresenter->RevealBorderBrush, aliceBlue);

            listViewItemPresenter->RevealBorderThickness = thickness;
            VERIFY_ARE_EQUAL(listViewItemPresenter->RevealBorderThickness, thickness);

        });
    }

    void MoCoIntegrationTests::ManuallyCreatedListViewItemsAreUsedAsContainers()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        Platform::Collections::Vector<xaml_controls::ListViewItem^>^ listViewItemList = nullptr;

        RunOnUIThread([&] ()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();
            list = ref new xaml_controls::ListView();
            listViewItemList = ref new Platform::Collections::Vector<xaml_controls::ListViewItem^>();

            list->Height = 500;
            list->Width = 500;

            for (int i = 0; i < 50; i++)
            {
                Platform::String^ itemText = L"Item ";
                itemText += (i + 1);

                xaml_controls::ListViewItem^ listViewItem = ref new xaml_controls::ListViewItem();

                listViewItem->Content = itemText;
                listViewItemList->Append(listViewItem);
            }

            list->ItemsSource = listViewItemList;
            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            DependencyObject^ container1 = list->ContainerFromIndex(0);
            DependencyObject^ container2 = list->ContainerFromIndex(1);
            DependencyObject^ container3 = list->ContainerFromIndex(2);

            VERIFY_ARE_EQUAL(container1, listViewItemList->GetAt(0));
            VERIFY_ARE_EQUAL(container2, listViewItemList->GetAt(1));
            VERIFY_ARE_EQUAL(container3, listViewItemList->GetAt(2));
        });

        ItemsControlHelper::ScrollToIndex<xaml_controls::ListViewBase, xaml_controls::ListViewItem>(list, 25);

        RunOnUIThread([&] ()
        {
            DependencyObject^ container1 = list->ContainerFromIndex(23);
            DependencyObject^ container2 = list->ContainerFromIndex(24);
            DependencyObject^ container3 = list->ContainerFromIndex(25);

            VERIFY_ARE_EQUAL(container1, listViewItemList->GetAt(23));
            VERIFY_ARE_EQUAL(container2, listViewItemList->GetAt(24));
            VERIFY_ARE_EQUAL(container3, listViewItemList->GetAt(25));
        });
    }

    struct ObjectComparator
    {
        bool operator()(Platform::Object^ const& left, Platform::Object^ const& right) const
        {
            return left->GetHashCode() < right->GetHashCode();
        }
    };

    void MoCoIntegrationTests::CanReuseOldListViewItemsOnReset()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        Platform::Collections::Map<Platform::Object^, xaml_primitives::SelectorItem^, ObjectComparator>^ selectorItemByItemMap = nullptr;
        MocoResettableCollection^ collection = nullptr;
        DependencyObject^ originalContainer1 = nullptr;
        DependencyObject^ originalContainer2 = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);
        auto containerContentChangingRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ContainerContentChanging);
        std::shared_ptr<bool> handlingReset = std::make_shared<bool>();

        RunOnUIThread([&] ()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();
            list = ref new xaml_controls::ListView();
            selectorItemByItemMap = ref new Platform::Collections::Map<Platform::Object^, xaml_primitives::SelectorItem^, ObjectComparator>();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 2, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [selectorItemByItemMap](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                if (selectorItemByItemMap->HasKey(args->Item))
                {
                    args->ItemContainer = selectorItemByItemMap->Lookup(args->Item);
                    args->IsContainerPrepared = true;
                    selectorItemByItemMap->Remove(args->Item);
                }
                else if (args->ItemContainer == nullptr)
                {
                    args->ItemContainer = ref new xaml_controls::ListViewItem();
                }
            }));

            containerContentChangingRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [handlingReset, selectorItemByItemMap](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                if (args->InRecycleQueue && *handlingReset)
                {
                    selectorItemByItemMap->Insert(args->Item, args->ItemContainer);
                }
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            originalContainer1 = list->ContainerFromIndex(0);
            originalContainer2 = list->ContainerFromIndex(1);

            *handlingReset = true;
            collection->Reset();
            // as it stands, this will not recycle the pinned container (special item 0)
            *handlingReset = false;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            DependencyObject^ newContainer2 = list->ContainerFromIndex(1);

            // since container 0 is not recycled, we can't have re-used that one
            VERIFY_ARE_EQUAL(newContainer2, originalContainer2);

            // will have gotten one and removed one
            VERIFY_ARE_EQUAL(selectorItemByItemMap->Size, 0u);
        });
    }

    void MoCoIntegrationTests::CanReuseContainersFromRecycleQueue()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ItemsStackPanel^ isp = nullptr;
        Platform::Collections::Map<Platform::Object^, xaml_primitives::SelectorItem^, ObjectComparator>^ selectorItemByItemMap = nullptr;
        Platform::Collections::Vector<Object^>^ collection = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);
        auto containerContentChangingRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ContainerContentChanging);
        bool useRQContainers = false;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <Grid.Resources >" \
                L"        <Style TargetType='ListViewItem' x:Key='MyListViewItemStyle'>" \
                L"          <Setter Property='Margin' Value='0,0,0,0' />" \
                L"        </Style>" \
                L"   </Grid.Resources >" \
                L"   <ListView x:Name='list' Height='500' Width='500' ItemContainerStyle='{StaticResource MyListViewItemStyle}'>" \
                L"        <ListView.ItemTemplate>" \
                L"          <DataTemplate>" \
                L"            <Grid Height='50' >" \
                L"               <TextBlock Text='{Binding}' />" \
                L"            </Grid>" \
                L"          </DataTemplate>" \
                L"        </ListView.ItemTemplate>" \
                L"   </ListView>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"list"));

            list->ShowsScrollingPlaceholders = false;
            selectorItemByItemMap = ref new Platform::Collections::Map<Platform::Object^, xaml_primitives::SelectorItem^, ObjectComparator>();

            list->ItemsPanel = dynamic_cast<xaml_controls::ItemsPanelTemplate^>(xaml_markup::XamlReader::Load(
                L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"  <ItemsStackPanel CacheLength='4' />"
                L"</ItemsPanelTemplate>"
                ));

            MocoHelper::PopulateList(list, 200);

            collection = safe_cast<Platform::Collections::Vector<Object^>^>(list->ItemsSource);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [&useRQContainers, selectorItemByItemMap](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                if (useRQContainers && selectorItemByItemMap->Size > 0)
                {
                    // testing our behavior
                    args->ItemContainer = selectorItemByItemMap->First()->Current->Value;
                    selectorItemByItemMap->Remove(selectorItemByItemMap->First()->Current->Key);
                }
                else
                {
                    if (selectorItemByItemMap->HasKey(args->Item))
                    {
                        args->ItemContainer = selectorItemByItemMap->Lookup(args->Item);
                        selectorItemByItemMap->Remove(args->Item);
                        args->IsContainerPrepared = true;
                    }
                    else if (args->ItemContainer == nullptr)
                    {
                        args->ItemContainer = ref new xaml_controls::ListViewItem();
                    }
                    else
                    {
                        // args->Item does not match something in our map, but
                        // there is an args->ItemContainer. We might be reusing the
                        // container for a different item. In that case remove it from
                        // our map.
                        bool found = false;
                        auto it = begin(selectorItemByItemMap);
                        while (it != end(selectorItemByItemMap))
                        {
                            found = ((*it)->Value == args->ItemContainer);
                            if (found)
                                break;
                            ++it;
                        }
                        if (found)
                        {
                            selectorItemByItemMap->Remove((*it)->Key);
                        }
                    }
                }
            }));

            containerContentChangingRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [selectorItemByItemMap](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                if (args->InRecycleQueue)
                {
                    selectorItemByItemMap->Insert(args->Item, args->ItemContainer);
                }
            }));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);

            // this seems to be a bug, I was expecting a higher number here. Fix this once that bug is fixed.
            VERIFY_ARE_EQUAL(isp->LastCacheIndex, 50);

            // we have cache from index 0 to 50, meaning 51 children, nothing in recyclequeue
            VERIFY_ARE_EQUAL((int)isp->Children->Size, 51);

            LOG_OUTPUT(L"scrolling to 150");
            list->ScrollIntoView(collection->GetAt(150), Microsoft::UI::Xaml::Controls::ScrollIntoViewAlignment::Leading);

            useRQContainers = true; // starts testing our behavior
            // this will kick off a layoutpass, understanding that it is disconnected, we will get all containers in the RQ
            list->UpdateLayout();

            LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);

            // this was a disconnected jump, so we can expect the recycle queue to be filled up right now
            VERIFY_ARE_EQUAL(isp->FirstCacheIndex, 149);
            VERIFY_ARE_EQUAL(isp->FirstVisibleIndex, 150);
            VERIFY_ARE_EQUAL(isp->LastVisibleIndex, 159);
            VERIFY_ARE_EQUAL(isp->LastCacheIndex, 160);

            // expect some to be left
            VERIFY_ARE_EQUAL((int)selectorItemByItemMap->Size, 38);
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL((int)selectorItemByItemMap->Size, 0);
            LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);

            // so we have given back all the recycled containers. We should not have created any more ourselves
            VERIFY_ARE_EQUAL((int)isp->Children->Size, (isp->LastCacheIndex - isp->FirstCacheIndex) + 1 /* special container index 0 */ + 1 /* indices to count */);
        });

    }

    void MoCoIntegrationTests::CanCallGetGroupBoundsOnRecycledHeader()
    {
        TestCleanupWrapper cleanup;
        Platform::Collections::Vector<Object^>^ data = nullptr;

        RunOnUIThread([&]() {
            xaml_controls::ListViewHeaderItem^ listViewHeaderItem = ref new xaml_controls::ListViewHeaderItem();
            // since there is no parent list view - this is equivalent to the state we will be in for
            // a recycled header (for this test we want the parent listview pointer to be null)
            auto automationPeer = Microsoft::UI::Xaml::Automation::Peers::FrameworkElementAutomationPeer::CreatePeerForElement(listViewHeaderItem);

            // This will ultimately call ListViewBaseHeaderItem::GetGroupBounds which
            // crashes before this change when the parent listview is null
            auto boundingRect = automationPeer->GetBoundingRectangle();

            // Verify that we do not crash but get
            // empty bounds
            VERIFY_ARE_EQUAL(0u, boundingRect.Top);
            VERIFY_ARE_EQUAL(0u, boundingRect.Bottom);
            VERIFY_ARE_EQUAL(0u, boundingRect.Left);
            VERIFY_ARE_EQUAL(0u, boundingRect.Right);
        });
    }

    void MoCoIntegrationTests::VerifyDisablePlaceholdersConvergedBehavior()
    {
        TestCleanupWrapper cleanup;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        xaml_controls::SemanticZoom^ sezo = nullptr;
        Platform::Collections::Vector<int>^ data = nullptr;

        RunOnUIThread([&]() {
             sezo = static_cast<Microsoft::UI::Xaml::Controls::SemanticZoom^>(Microsoft::UI::Xaml::Markup::XamlReader::Load(
                L" <SemanticZoom x:Name='sezo' "
                L"               xmlns:local='using:Tests.External.Controls.CustomTypes'"
                L"               xmlns = 'http://schemas.microsoft.com/winfx/2006/xaml/presentation' "
                L"               xmlns:x = 'http://schemas.microsoft.com/winfx/2006/xaml' Width='500' Height='100' >"
                L"     <SemanticZoom.Resources>"
                L"         <ControlTemplate TargetType='ListViewItem' x:Key='MyListViewChromeTemplate'>"
                L"             <local:CustomListViewItemPresenter>"
                L"                 <ContentPresenter"
                L"                 ContentTemplate='{TemplateBinding ContentTemplate}'"
                L"                 Content='{TemplateBinding Content}'"
                L"             />"
                L"             </local:CustomListViewItemPresenter>"
                L"         </ControlTemplate>"
                L"         <Style TargetType='ListViewItem'>"
                L"             <Setter Property='Template' Value='{StaticResource MyListViewChromeTemplate}' />"
                L"         </Style>"
                L"         <CollectionViewSource x:Name='cvs' />"
                L"         </SemanticZoom.Resources>"
                L"         <SemanticZoom.ZoomedInView>"
                L"             <ListView  x:Name='lv1' ShowsScrollingPlaceholders='True' "
                L"               ItemsSource='{Binding  Source={StaticResource cvs}}' >"
                L"                   <ListView.ItemsPanel>"
                L"                       <ItemsPanelTemplate>"
                L"                            <ItemsStackPanel Orientation='Vertical' CacheLength = '0' />"
                L"                       </ItemsPanelTemplate>"
                L"                    </ListView.ItemsPanel>"
                L"              </ListView>"
                L"         </SemanticZoom.ZoomedInView>"
                L"         <SemanticZoom.ZoomedOutView>"
                L"             <ListView x:Name='lv2' ShowsScrollingPlaceholders='True' "
                L"               ItemsSource='{Binding  Source={StaticResource cvs}}' >"
                L"                  <ListView.ItemsPanel>"
                L"                      <ItemsPanelTemplate>"
                L"                          <ItemsStackPanel Orientation='Vertical' CacheLength = '0' />"
                L"                      </ItemsPanelTemplate>"
                L"                   </ListView.ItemsPanel>"
                L"             </ListView>"
                L"         </SemanticZoom.ZoomedOutView>"
                L" </SemanticZoom>"));
            VERIFY_IS_NOT_NULL(sezo);
            cvs = safe_cast<xaml_data::CollectionViewSource^>(sezo->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            data = ref new Platform::Collections::Vector<int>();
            for (int i = 0; i < 10; i++)
            {
                data->Append(i);
            }

            cvs->Source = data;

            test_infra::TestServices::WindowHelper->WindowContent = sezo;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        // switch views and see if we hit the placeholder visual state
        RunOnUIThread([&]() {
            ::Tests::External::Controls::CustomTypes::CustomListViewItemPresenter::Set_PlaceholderStateReached(false);
            sezo->ToggleActiveView();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(::Tests::External::Controls::CustomTypes::CustomListViewItemPresenter::Get_PlaceholderStateReached());

        // resize the control and check if we hit the placeholder state
        RunOnUIThread([&]() {
            sezo->Height -= 50;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(::Tests::External::Controls::CustomTypes::CustomListViewItemPresenter::Get_PlaceholderStateReached());
    }

    void MoCoIntegrationTests::CanScrollIntoViewDuringStartup()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto CCCRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ContainerContentChanging);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, Loaded);

        RunOnUIThread([&] ()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();
            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;


            loadedRegistration.Attach(list,
            ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                MocoHelper::PopulateList(list, 2000, false /* isGrouped */, 0, nullptr, true /* isResettable */);

                collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);
                // interestingly, selector ignores scrollintoview if it is not fully in the tree with an attached ScrollViewer
                list->ScrollIntoView(collection->GetAt(200), xaml_controls::ScrollIntoViewAlignment::Leading);
            }));


            CCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [](xaml_controls::ListViewBase^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                // scrolls to top
                VERIFY_ARE_NOT_EQUAL(args->ItemIndex, 1);

                if (args->ItemIndex == 200)
                {
                    args->ItemContainer->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
                }
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&] ()
        {
            auto isp = static_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(isp->FirstVisibleIndex, 200);
        });
    }

    void MoCoIntegrationTests::CanScrollIntoViewDuringStartupHorizontally()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto listLayoutUpdatedRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, LayoutUpdated);
        auto CCCRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ContainerContentChanging);
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();
            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            listLayoutUpdatedRegistration.Attach(list, ref new wf::EventHandler<Platform::Object^>([&](Platform::Object^, Platform::Object^)
            {
                auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
                // it is possible that the layout updated is fired before the loaded event in which case
                // we will not have the panel yet.
                if (isp)
                {
                    listLayoutUpdatedRegistration.Detach();
                    isp->Orientation = xaml_controls::Orientation::Horizontal;
                    isp->CacheLength = 0;
                    isp->InvalidateMeasure();    // makes sure we don't exit layout and not get a chance to start scrolling
                }
            }));

            loadedRegistration.Attach(list,
                ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                MocoHelper::PopulateList(list, 2000, false /* isGrouped */, 0, nullptr, true /* isResettable */);


                collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);
                // interestingly, selector ignores scrollintoview if it is not fully in the tree with an attached ScrollViewer
                list->ScrollIntoView(collection->GetAt(200), xaml_controls::ScrollIntoViewAlignment::Leading);
            }));


            CCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [](xaml_controls::ListViewBase^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                // scrolls to top
                if (!args->InRecycleQueue)
                {
                    VERIFY_ARE_NOT_EQUAL(args->ItemIndex, 1);
                }

                if (args->ItemIndex == 200)
                {
                    args->ItemContainer->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
                }
            }));

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto isp = static_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(isp->FirstVisibleIndex, 200);
        });
    }

    void MoCoIntegrationTests::CanFocusWhileInPopupWithoutStackOverflow()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::UserControl^ uc = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"<Popup IsOpen='True'><UserControl x:Name='u' IsTabStop='False'>"
                L"    <ListView x:Name='listView' Width='400'>"
                L"        <ListViewItem Content='Item 1' IsEnabled='False'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"    </ListView>"
                L"</UserControl></Popup>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            uc = safe_cast<xaml_controls::UserControl^>(rootPanel->FindName(L"u"));
            VERIFY_IS_NOT_NULL(uc);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // no stackoverflow please!!
            uc->Focus(Microsoft::UI::Xaml::FocusState::Pointer);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateIsSwipeEnabledDeprecated()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml::DependencyObject^ listViewItemAsDO = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400'>"
                L"        <ListViewItem Content='Item 1'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItemAsDO = listView->ContainerFromIndex(0);
            VERIFY_IS_NOT_NULL(listViewItemAsDO);
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->Flick(safe_cast<xaml::FrameworkElement^>(listViewItemAsDO), FlickDirection::East);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateNewSelectionAndItemClickBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;

        auto listViewSelectionChangedEvent = std::make_shared<Event>();
        auto listViewSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        auto listViewItemClickEvent = std::make_shared<Event>();
        auto listViewItemClickRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ItemClick);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center'>"
                L"        <ListViewItem Content='Item 1'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"        <ListViewItem Content='Item 3'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewSelectionChangedRegistration.Attach(listView, ref new xaml_controls::SelectionChangedEventHandler(
                [listViewSelectionChangedEvent]
            (Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ args)
            {
                listViewSelectionChangedEvent->Set();
            }));

            listViewItemClickRegistration.Attach(listView, ref new xaml_controls::ItemClickEventHandler(
                [listViewItemClickEvent]
            (Platform::Object^ sender, xaml_controls::ItemClickEventArgs^ args)
            {
                listViewItemClickEvent->Set();
            }));

            listView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tests with IsItemClickEnabled false
        PerformSelectionAndItemClickFalseTests(listView, listViewSelectionChangedEvent, listViewItemClickEvent);

        // Tests with IsItemClickEnabled true
        PerformSelectionAndItemClickTrueTests(listView, listViewSelectionChangedEvent, listViewItemClickEvent, true);

        // Test with IsItemClickEnabled true and SingleSelectionFollowsFocus false
        PerformSelectionAndItemClickTrueTests(listView, listViewSelectionChangedEvent, listViewItemClickEvent, false);
    }

    void MoCoIntegrationTests::PerformSelectionAndItemClickFalseTests(
        Microsoft::UI::Xaml::Controls::ListView^ listView,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& itemClickEvent)
    {
        Microsoft::UI::Xaml::FrameworkElement^ item1AsFE = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ item2AsFE = nullptr;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Single;
            listView->IsItemClickEnabled = false;

            item1AsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(item1AsFE);

            item2AsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(1));
            VERIFY_IS_NOT_NULL(item2AsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        {
            // test space -> should only select (since IsItemClickEnabled is false)
            {
                LOG_OUTPUT(L"Test space");

                RunOnUIThread([&]()
                {
                    // set the focus on the listview
                    listView->Focus(Microsoft::UI::Xaml::FocusState::Programmatic);
                });
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                // select item 1
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ #$u$_ ");
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), false);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                    // item 1 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // test enter -> should only select (since IsItemClickEnabled is false)
            {
                LOG_OUTPUT(L"Test enter");

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                // move to item 2
                TestServices::KeyboardHelper->Down();
                TestServices::KeyboardHelper->Enter();
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), false);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                    // item 2 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(1));
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void MoCoIntegrationTests::PerformSelectionAndItemClickTrueTests(
        Microsoft::UI::Xaml::Controls::ListView^ listView,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& selectionChangedEvent,
        std::shared_ptr<Microsoft::UI::Xaml::Tests::Common::Event>& itemClickEvent,
        bool singleSelectionFollowsFocus)
    {
        Microsoft::UI::Xaml::FrameworkElement^ item1AsFE = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ item2AsFE = nullptr;

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Single;
            listView->SelectedIndex = 1;
            listView->IsItemClickEnabled = true;
            listView->SingleSelectionFollowsFocus = singleSelectionFollowsFocus;

            item1AsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(item1AsFE);

            item2AsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(1));
            VERIFY_IS_NOT_NULL(item2AsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test tap -> should invoke and select
        {
            LOG_OUTPUT(L"Test tap");

            selectionChangedEvent->Reset();
            itemClickEvent->Reset();

            // select item 1
            TestServices::InputHelper->Tap(item1AsFE);
            TestServices::WindowHelper->WaitForIdle();

            selectionChangedEvent->WaitForDefault();
            itemClickEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        {
            // test left-click -> should invoke and select
            {
                LOG_OUTPUT(L"Test left-click");

                RunOnUIThread([&]()
                {
                    listView->SelectedItem = -1;
                });
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                // select item 2
                TestServices::InputHelper->LeftMouseClick(item2AsFE);
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->WaitForDefault();
                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                    // item 2 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(1));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // test space
            {
                LOG_OUTPUT(L"Test space");

                // in single selection, moving to item 1 will select it
                TestServices::KeyboardHelper->Up();
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                // pressing space will invoke only (already selected)
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ #$u$_ ");
                TestServices::WindowHelper->WaitForIdle();

                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), !singleSelectionFollowsFocus);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                    // item 1 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // Ctrl + space -> should invoke and UNselect (toggle)
            {
                LOG_OUTPUT(L"Test Control + space in Single mode");

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#$d$_ #$u$_ #$u$_ctrl#");

                selectionChangedEvent->WaitForDefault();
                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));

                    VERIFY_ARE_EQUAL(listView->SelectedIndex, -1);
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // test enter
            {
                LOG_OUTPUT(L"Test enter");

                // in single selection, moving to item 2 will select it
                TestServices::KeyboardHelper->Down();
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                TestServices::KeyboardHelper->Enter();
                TestServices::WindowHelper->WaitForIdle();

                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), !singleSelectionFollowsFocus);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

                    // item 2 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(1));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // mod + arrows -> should only select
            {
                LOG_OUTPUT(L"Test mod + arrows in Extended mode");

                RunOnUIThread([&]()
                {
                    listView->SelectedItem = -1;

                    listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Extended;
                });
                TestServices::WindowHelper->WaitForIdle();

                // select the first item
                TestServices::InputHelper->LeftMouseClick(item1AsFE);
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");

                TestServices::KeyboardHelper->Down();
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");

                selectionChangedEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), false);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(2));

                    // item 1 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));

                    // item 2 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(1), listView->Items->GetAt(1));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // mod + left-click -> should invoke and select
            {
                LOG_OUTPUT(L"Test mod + left-click in Extended mode");

                RunOnUIThread([&]()
                {
                    listView->SelectedItem = -1;

                    listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Extended;
                });
                TestServices::WindowHelper->WaitForIdle();

                // select the first item
                TestServices::InputHelper->LeftMouseClick(item1AsFE);
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#");

                TestServices::InputHelper->LeftMouseClick(item2AsFE);
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl#");

                selectionChangedEvent->WaitForDefault();
                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(2));

                    // item 1 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));

                    // item 2 selected
                    VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(1), listView->Items->GetAt(1));
                });
                TestServices::WindowHelper->WaitForIdle();
            }

            // mod + left-click with selection mode = None -> should only invoke
            {
                LOG_OUTPUT(L"Test mod + left-click in None mode");

                RunOnUIThread([&]()
                {
                    listView->SelectedItem = -1;

                    listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::None;
                });
                TestServices::WindowHelper->WaitForIdle();

                selectionChangedEvent->Reset();
                itemClickEvent->Reset();

                TestServices::KeyboardHelper->PressKeySequence(L"$d$_ctrl#");

                TestServices::InputHelper->LeftMouseClick(item2AsFE);
                TestServices::WindowHelper->WaitForIdle();

                TestServices::KeyboardHelper->PressKeySequence(L"$u$_ctrl#");

                itemClickEvent->WaitForDefault();

                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), false);
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);

                RunOnUIThread([&]()
                {
                    VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));
                });
                TestServices::WindowHelper->WaitForIdle();
            }
        }
    }

    void MoCoIntegrationTests::ValidateSourceClearDuringSelection()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml::FrameworkElement^ item0 = nullptr;
        xaml::FrameworkElement^ item1 = nullptr;
        xaml::FrameworkElement^ item2 = nullptr;

        int selectionCount = 0;
        bool okayToClear = false;

        auto listViewItemClickRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ItemClick);
        auto listViewSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center'>"
                L"        <ListViewItem Content='Item 1'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"        <ListViewItem Content='Item 3'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewItemClickRegistration.Attach(listView, ref new xaml_controls::ItemClickEventHandler(
                [&listView, &okayToClear]
            (Platform::Object^ sender, xaml_controls::ItemClickEventArgs^ args)
            {
                if (okayToClear)
                {
                    listView->Items->Clear();
                }
            }));

            listViewSelectionChangedRegistration.Attach(listView, ref new xaml_controls::SelectionChangedEventHandler(
                [&selectionCount]
            (Platform::Object^ sender, xaml_controls::SelectionChangedEventArgs^ args)
            {
                selectionCount++;
            }));


            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Single;
            listView->IsItemClickEnabled = true;
            listView->Focus(xaml::FocusState::Programmatic);

            // We get our items here so that our input injection doesn't occur on the UI thread.  This allows our
            // mitigations for delayed dwm intialization and phantom light dismiss overlays to work.
            item0 = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            item1 = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(1));
            item2 = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(2));


        });
        TestServices::WindowHelper->WaitForIdle();

        // select item 1
        TestServices::InputHelper->LeftMouseClick(item0);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->LeftMouseClick(item1);
        TestServices::WindowHelper->WaitForIdle();

        okayToClear = true;
        TestServices::InputHelper->LeftMouseClick(item2);
        TestServices::WindowHelper->WaitForIdle();

        // first raise: when we selected item 0
        // second raise: when we unselected item 0 and selected 1
        // third raise: when we unselected item 1
        // but for item 2, we do nothing (since it couldn't have been selected)
        VERIFY_ARE_EQUAL(selectionCount, 3);
    }

    void MoCoIntegrationTests::BasicScrollOnGridView()
    {
        TestCleanupWrapper cleanup;
        auto list = MocoHelper::SetUpBasicEnvironment(MocoHelper::ListControlType::GridView, 100, 100);

        xaml_controls::ScrollViewer^ scrollViewer = nullptr;
        Platform::Collections::Vector<Object^>^ collection = nullptr;

        double previousVerticalOffset = 0;
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);
        auto viewChangedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(list, L"ScrollViewer"));
            collection = safe_cast<Platform::Collections::Vector<Object^>^>(list->ItemsSource);

            viewChangedRegistration.Attach(scrollViewer,
                ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                if (args->IsIntermediate == false)
                {
                    viewChangedEvent->Set();
                }
            }));

            VERIFY_IS_TRUE(scrollViewer->ExtentHeight > scrollViewer->ExtentWidth);
            previousVerticalOffset = scrollViewer->VerticalOffset;

            LOG_OUTPUT(L"Scrolling to the end");
            list->ScrollIntoView(collection->GetAt(collection->Size -1), xaml_controls::ScrollIntoViewAlignment::Leading);
        });

        LOG_OUTPUT(L"Waiting for ViewChanged event.");
        viewChangedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // verify that we scrolled vertically
            VERIFY_IS_TRUE(previousVerticalOffset < scrollViewer->VerticalOffset);
        });
    }

    void MoCoIntegrationTests::VerifyCacheExpansion()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ItemsStackPanel^ isp = nullptr;
        Platform::Collections::Vector<Object^>^ collection = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <Grid.Resources >" \
                L"        <Style TargetType='ListViewItem' x:Key='MyListViewItemStyle'>" \
                L"          <Setter Property='Margin' Value='0,0,0,0' />" \
                L"        </Style>" \
                L"   </Grid.Resources >" \
                L"   <ListView x:Name='list' Height='500' Width='500' ItemContainerStyle='{StaticResource MyListViewItemStyle}'>" \
                L"        <ListView.ItemsPanel>" \
                L"          <ItemsPanelTemplate>" \
                L"              <ItemsStackPanel CacheLength='10' />" \
                L"          </ItemsPanelTemplate>"\
                L"        </ListView.ItemsPanel>" \
                L"        <ListView.ItemTemplate>" \
                L"          <DataTemplate>" \
                L"            <Grid Height='50' >" \
                L"               <TextBlock Text='{Binding}' />" \
                L"            </Grid>" \
                L"          </DataTemplate>" \
                L"        </ListView.ItemTemplate>" \
                L"   </ListView>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"list"));

            MocoHelper::PopulateList(list, 200);
            collection = safe_cast<Platform::Collections::Vector<Object^>^>(list->ItemsSource);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        // wait for build tree work complete and idel - this should expand the entire
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            LOG_OUTPUT(L"first cache: %d, first visible: %d, lastvisible: %d, lastcache: %d", isp->FirstCacheIndex, isp->FirstVisibleIndex, isp->LastVisibleIndex, isp->LastCacheIndex);
            // 10 items in first page, + 10*10 items in the cache
            VERIFY_ARE_EQUAL(110, isp->LastCacheIndex);
        });
    }

    void MoCoIntegrationTests::ValidateMultipleSelectionModeRangeSelection()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 10;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ itemsAsFE[itemsCount];

        RunOnUIThread([&]()
        {
            //We need to put a Margin of 32 to the top of the ListView to prevent it from clipping with the Title Bar due to a known issue
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Margin='0,32,0,0' />"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<int>^ items = ref new Platform::Collections::Vector<int>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;

            for (unsigned int i = 0; i < itemsCount; ++i)
            {
                itemsAsFE[i] = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(itemsAsFE[i]);
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            safe_cast<xaml::Controls::Control^>(itemsAsFE[1])->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test case 1
        // select the first item
        // hold down the shift key and press down twice -> we should select 3 items
        // hold down the shift key and press up once -> selected items should remain
        {
            // Select the first item
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[0]);
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Hold down Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");
            TestServices::WindowHelper->WaitForIdle();

            // Press Down key twice
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            // Release Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(3));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Hold down Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");

            // Press Up key once
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();

            // Release Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(3));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test case 2
        // select first and third items
        // hold down the shift key and press down three times -> we should keep selecting the first item and select 3 new items
        {
            RunOnUIThread([&]()
            {
                // reset selection
                listView->SelectedIndex = -1;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select the first and third items
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[0]);
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[2]);
            TestServices::WindowHelper->WaitForIdle();

            // Hold down Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");

            // Press Down key three times
            TestServices::KeyboardHelper->Down();
            TestServices::KeyboardHelper->Down();
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            // Release Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(5));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test case 3
        // select first and third items
        // hold down the shift key and click the 8th item -> we should keep selecting the first item and select the range from third to eighth item
        // tap the fifth item to unselect
        // hold down the shift key and click the 7th item -> sixth and seventh item should be unselected
        {
            RunOnUIThread([&]()
            {
                // reset selection
                listView->SelectedIndex = -1;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Select the first and third items
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[0]);
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[2]);
            TestServices::WindowHelper->WaitForIdle();

            // Hold down Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");

            // Press Down key three times
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[7]);
            TestServices::WindowHelper->WaitForIdle();

            // Release Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(7));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Click the 5th item to unselect
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[4]);
            TestServices::WindowHelper->WaitForIdle();

            // Hold down Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");

            // Press Down key three times
            TestServices::InputHelper->LeftMouseClick(itemsAsFE[6]);
            TestServices::WindowHelper->WaitForIdle();

            // Release Shift key
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(4));
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoIntegrationTests::ValidatePhasingWithIDataTemplateExtensions()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);
        Platform::Collections::Vector<MoCoDataTemplateExtensionMock^>^ extensions = ref new Platform::Collections::Vector<MoCoDataTemplateExtensionMock^>();

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 2, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [extensions](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                auto root = dynamic_cast<xaml::FrameworkElement^>(args->ItemContainer->ContentTemplateRoot);
                Platform::Collections::Vector<uint32>^ phasesToCall = ref new Platform::Collections::Vector<uint32>();
                phasesToCall->Append(0);// todo: skip a phase somewhere when we support that
                phasesToCall->Append(1);
                phasesToCall->Append(2);
                phasesToCall->Append(5);
                phasesToCall->Append(8);

                auto extension = ref new MoCoDataTemplateExtensionMock(phasesToCall);

                extensions->Append(extension);

                // we're not testing x:bind functionality, but are interested in the ability for listview to properly communicate with
                // IDataTemplateExtensions
                Microsoft::UI::Xaml::DataTemplate::SetExtensionInstance(root, extension);

            }));

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(extensions->Size == 2);

        VERIFY_IS_TRUE(extensions->GetAt(0)->VerifyCalledForAllPhases());
        VERIFY_IS_TRUE(extensions->GetAt(1)->VerifyCalledForAllPhases());

        VERIFY_IS_FALSE(extensions->GetAt(0)->ContainerIsInRecycleQueue());

        RunOnUIThread([&]()
        {
            collection->Reset();
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        // todo: there appear to be gaps in our reset story that I will follow up on with a different workitem
        //   VERIFY_IS_TRUE(extensions->GetAt(0)->ContainerIsInRecycleQueue());
    }

    void MoCoIntegrationTests::ValidatePhasingWithIDataTemplateExtensionsAndRegularCCC()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);

        Platform::Collections::Vector<uint32>^ phasesToCall = ref new Platform::Collections::Vector<uint32>();
        phasesToCall->Append(0);// todo: skip a phase somewhere when we support that
        phasesToCall->Append(1);
        phasesToCall->Append(2);
        phasesToCall->Append(5);
        phasesToCall->Append(8);
        auto extension = ref new MoCoDataTemplateExtensionMock(phasesToCall);
        bool reachedPhase12 = false;

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 1, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [extension, &reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                auto root = dynamic_cast<xaml::FrameworkElement^>(args->ItemContainer->ContentTemplateRoot);

                Microsoft::UI::Xaml::DataTemplate::SetExtensionInstance(root, extension);

                args->RegisterUpdateCallback(3,
                    ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                    [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                {
                    // this will be in reaction to phase 3

                    VERIFY_IS_TRUE(args->Phase == 3);

                    args->RegisterUpdateCallback(4,
                        ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                        [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                    {
                        // this will be in reaction to phase 4

                        VERIFY_IS_TRUE(args->Phase == 4);

                        args->RegisterUpdateCallback(5,
                            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                            [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                        {
                            // this will be in reaction to phase 5

                            VERIFY_IS_TRUE(args->Phase == 5);

                            args->RegisterUpdateCallback(12,
                                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                                [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                            {
                                // this will be in reaction to phase 12

                                VERIFY_IS_TRUE(args->Phase == 12);
                                reachedPhase12 = true;

                            }));
                        }));

                    }));

                }));

                args->Handled = true;

            }));

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(reachedPhase12);
    }

    void MoCoIntegrationTests::ValidatePhasingWithRegularCCC()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        MocoResettableCollection^ collection = nullptr;
        auto cCCRegistration = CreateSafeEventRegistration(xaml_controls::IListViewBase, ContainerContentChanging);
        bool reachedPhase12 = false;
        bool inRecycleQueue = false;

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            list = ref new xaml_controls::ListView();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 1, false /* isGrouped */, 0, nullptr, true /* isResettable */);

            cCCRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                [&reachedPhase12, &inRecycleQueue](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                if (args->InRecycleQueue)
                {
                    inRecycleQueue = true;

                }
                else
                {
                    args->RegisterUpdateCallback(
                        ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                        [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                    {
                        // this will be in reaction to phase 1

                        VERIFY_IS_TRUE(args->Phase == 1);

                        args->RegisterUpdateCallback(2,
                            ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                            [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                        {
                            // this will be in reaction to phase 2

                            VERIFY_IS_TRUE(args->Phase == 2);

                            args->RegisterUpdateCallback(4,
                                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                                [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                            {
                                // this will be in reaction to phase 4

                                VERIFY_IS_TRUE(args->Phase == 4);

                                args->RegisterUpdateCallback(5,
                                    ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                                    [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                                {
                                    // this will be in reaction to phase 5

                                    VERIFY_IS_TRUE(args->Phase == 5);

                                    args->RegisterUpdateCallback(12,
                                        ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                                        [&reachedPhase12](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
                                    {
                                        // this will be in reaction to phase 12

                                        VERIFY_IS_TRUE(args->Phase == 12);
                                        reachedPhase12 = true;

                                    }));
                                }));

                            }));

                        }));

                    }));

                    args->Handled = true;
                }

            }));

            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_TRUE(reachedPhase12);
        VERIFY_IS_FALSE(inRecycleQueue);

        collection->Reset();
        test_infra::TestServices::WindowHelper->WaitForIdle();

        // todo: there appear to be gaps in our reset story that I will follow up on with a different workitem
     //   VERIFY_IS_TRUE(inRecycleQueue);

    }

    void MoCoIntegrationTests::ValidateSizingOfListViewItems()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ScrollViewer^ viewer;
        std::vector<ListViewItemMock^> containers;
        ::Windows::Foundation::Collections::IVector<Object^>^ collection = nullptr;
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = ref new xaml_controls::Grid();

            list = ref new xaml_controls::ListView();

            // allow horizontal scrolling, even though this is clearly a vertically virtualizing list
            xaml_controls::ScrollViewer::SetHorizontalScrollMode(list, xaml_controls::ScrollMode::Enabled);
            xaml_controls::ScrollViewer::SetHorizontalScrollBarVisibility(list, xaml_controls::ScrollBarVisibility::Visible);

            list->Height = 600;
            list->Width = 300;

            MocoHelper::PopulateList(list, 9, false /* isGrouped */, 0, nullptr, false /* isResettable */);

            choosingItemContainerRegistration.Attach(list,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [&containers](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                int index = args->ItemIndex;
                auto container = ref new ListViewItemMock();
                containers.push_back(container);
                args->ItemContainer = container;

                if (index < 4)
                {
                    container->ContentTemplate = dynamic_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                        L"  <TextBlock TextWrapping=\"Wrap\" Text=\"Short str\" />"
                        L"</DataTemplate>"
                        ));
                }
                else if (index < 8)
                {
                    container->ContentTemplate = dynamic_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                        L"  <TextBlock TextWrapping=\"Wrap\" Text=\"long string, long string, long string, long string, long string\" />"
                        L"</DataTemplate>"
                        ));
                }
                else // index 8
                {
                    container->ContentTemplate = dynamic_cast<xaml::DataTemplate^>(xaml_markup::XamlReader::Load(
                        L"<DataTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                        L"  <TextBlock Text=\"not wrapping long string, long string, long string, long string, long string, asdfasdfasdfasdfasasdf, asdfasdfasdfa asdf as dfasdf asdfasdf \" />"
                        L"</DataTemplate>"
                        ));
                }
                if (index % 4 == 0)
                {
                    container->HorizontalAlignment = HorizontalAlignment::Left;
                }
                if (index % 4 == 1)
                {
                    container->HorizontalAlignment = HorizontalAlignment::Center;
                }
                if (index % 4 == 2)
                {
                    container->HorizontalAlignment = HorizontalAlignment::Right;
                }
                if (index % 4 == 3)
                {
                    container->HorizontalAlignment = HorizontalAlignment::Stretch;
                }

                args->IsContainerPrepared = true;
            }));

            collection = safe_cast<::Windows::Foundation::Collections::IVector<Object^>^>(list->ItemsSource);

            rootPanel->Children->Append(list);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        test_infra::TestServices::WindowHelper->WaitForIdle(true);

        RunOnUIThread([&]()
        {
            auto current = dynamic_cast<FrameworkElement^>(list->ItemsPanelRoot);
            while (viewer == nullptr)
            {
                current = dynamic_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetParent(current));
                viewer = dynamic_cast<xaml_controls::ScrollViewer^>(current);
            }

            VERIFY_IS_GREATER_THAN(viewer->ScrollableWidth, 0);
            VERIFY_IS_GREATER_THAN(viewer->ExtentWidth, 300);

            VERIFY_IS_TRUE(viewer->ChangeView(100.0, 0.0, 1.0f, true));
        });

        test_infra::TestServices::WindowHelper->WaitForIdle(true);

        VERIFY_ARE_EQUAL(viewer->HorizontalOffset, 100);

        int smallFinalSizes = 0;
        int bigFinalSizes = 0;
        int hugeFinalSizes = 0;

        for (auto it = containers.begin(); it != containers.end(); ++it)
        {
            // were measured with this constraint
            VERIFY_ARE_EQUAL((*it)->GetPassedInAvailableSize().Width, 300);
            float finalWidth = (*it)->GetPassedInFinalSize().Width;

            if (finalWidth < 100.0)
            {
                // these are the non stretch short strings
                smallFinalSizes++;
            }
            if (finalWidth > 250.0 && finalWidth <= 300.0)
            {
                // these are the longer strings that wrapped around the 300 constraint (so they won't be exactly 300)
                // and the stretched containers
                bigFinalSizes++;
            }
            if (finalWidth > 800.0)
            {
                // this is the very long long string in the non wrapping textblock
                hugeFinalSizes++;
            }
        }

        VERIFY_ARE_EQUAL(smallFinalSizes, 3);
        VERIFY_ARE_EQUAL(bigFinalSizes, 5);
        VERIFY_ARE_EQUAL(hugeFinalSizes, 1);
    }

    void MoCoIntegrationTests::ValidateNoSelectionWithCVSAndNone()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListView^ listView = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Grid.Resources>"
                L"        <CollectionViewSource x:Name='cvs'/>"
                L"    </Grid.Resources>"
                L"    <ListView x:Name='listView' Height='400' Width='400' ItemsSource='{Binding Source={StaticResource cvs}}' SelectionMode='None'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            itemsSource->Append("Roger Federer");
            itemsSource->Append("Cristiano Ronaldo");
            itemsSource->Append("LeBron James");

            cvs->Source = itemsSource;
            cvs->IsSourceGrouped = false;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);

        RunOnUIThread([&]()
        {
            // regardless of SelectionMode, we are synced to the CVS by default
            // meaning there will always be a current item (selected)
            VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));

            // removing the synchronization should reset selection
            listView->IsSynchronizedWithCurrentItem = false;

            VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));

            listView->SelectionMode = xaml_controls::ListViewSelectionMode::Multiple;

            VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    int MoCoDataTemplateExtensionMock::ProcessBindings(Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ args)
    {
        if (args->InRecycleQueue)
        {
            inRecycle = true;
            return -1;
        }

        inRecycle = false;

        phasesCalled->Append(args->Phase);

        uint32 index = 0;

        if (phasesWeWantCallbacksFor->IndexOf(args->Phase, &index))
        {
            if (index < phasesWeWantCallbacksFor->Size - 1)
            {
                return phasesWeWantCallbacksFor->GetAt(index+1);
            }
        }
        else
        {
            // weird that we were called for a phase that we didn't want to be called for. Bad mock setup?
            throw ref new Platform::FailureException;
        }

        // no callback please
        return -1;
    }

    bool MoCoDataTemplateExtensionMock::VerifyCalledForAllPhases()
    {
        if (phasesCalled->Size != phasesWeWantCallbacksFor->Size)
            return false;

        // called at the end of a test to see whether we got called for all phases we wanted to be called for
        for (unsigned int outer = 0; outer < phasesCalled->Size; outer++)
        {
            // even verify order
            if (phasesCalled->GetAt(outer) != phasesWeWantCallbacksFor->GetAt(outer))
                return false;
        }
        return true;
    }

    void MoCoIntegrationTests::CanDragMulitpleItemsInListView()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 10;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ itemsAsFE[itemsCount];

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' ItemContainerStyle='{ThemeResource ListViewItemExpanded}' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<int>^ items = ref new Platform::Collections::Vector<int>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;

            for (unsigned int i = 0; i < itemsCount; ++i)
            {
                itemsAsFE[i] = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(i));
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // Select the first and third items
        TestServices::InputHelper->LeftMouseClick(itemsAsFE[0]);
        TestServices::InputHelper->LeftMouseClick(itemsAsFE[2]);
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item (along with the third)
        TestServices::InputHelper->DragFromCenter(itemsAsFE[0], 0, 100, 0.1);
        TestServices::WindowHelper->WaitForIdle();

        // no crash
    }

    void MoCoIntegrationTests::ValidateQuicklySwitchingSelectionModes()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 10;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        Microsoft::UI::Xaml::Controls::Button^ button = nullptr;

        auto buttonClickEvent = std::make_shared<Event>();
        auto buttonClickRegistration = CreateSafeEventRegistration(xaml_controls::Button, Click);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='400'/>"
                L"    <Button x:Name='button' Content='Click Me'/>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            Platform::Collections::Vector<int>^ items = ref new Platform::Collections::Vector<int>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            buttonClickRegistration.Attach(button, ref new xaml::RoutedEventHandler(
                [listView]
            (Platform::Object^ sender, xaml::IRoutedEventArgs^ args)
            {
                static bool click = false;

                click = !click;

                if (click)
                {
                    listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Multiple;
                }
                else
                {
                    listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Single;
                }
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // tap several times to switch between single and multiple selection mode quickly
        // this should end on multiple
        TestServices::InputHelper->Tap(button);
        TestServices::InputHelper->Tap(button);
        TestServices::InputHelper->Tap(button);
        TestServices::WindowHelper->WaitForIdle();

        // this time, we start with multiple and tap several times
        TestServices::InputHelper->Tap(button);
        TestServices::InputHelper->Tap(button);
        TestServices::InputHelper->Tap(button);
        TestServices::WindowHelper->WaitForIdle();

        // no crash
    }

    void MoCoIntegrationTests::ValidateCacheRenewal()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ list = nullptr;
        xaml_controls::ItemsStackPanel^ isp = nullptr;
        MocoResettableCollection^ collection = nullptr;
        xaml_controls::ScrollViewer^ sv = nullptr;

        RunOnUIThread([&]()
        {
            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>" \
                L"   <ListView x:Name='list' Height='500' Width='500'>" \
                L"        <ListView.ItemsPanel>" \
                L"          <ItemsPanelTemplate>" \
                L"              <ItemsStackPanel CacheLength='1' />" \
                L"          </ItemsPanelTemplate>"\
                L"        </ListView.ItemsPanel>" \
                L"        <ListView.ItemTemplate>" \
                L"          <DataTemplate>" \
                L"            <Grid Height='50' >" \
                L"               <TextBlock Text='{Binding}' />" \
                L"            </Grid>" \
                L"          </DataTemplate>" \
                L"        </ListView.ItemTemplate>" \
                L"   </ListView>" \
                L"</Grid>";

            xaml_controls::Grid^ rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            list = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"list"));

            // nice about a resettable collection: it implements change notification
            MocoHelper::PopulateList(list, 100, false /* isGrouped */, 0, nullptr, true /* isResettable */);
            collection = safe_cast<MocoResettableCollection^>(list->ItemsSource);

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        // wait for build tree work complete and idle - this should expand the entire viewport
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot);
            sv = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(list, L"ScrollViewer"));

            // we are scrolled to the start
            VERIFY_IS_TRUE(sv->VerticalOffset == 0);

            sv->ChangeView(0.0, sv->ScrollableHeight, 1.0f, true);
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(isp->LastVisibleIndex == 99);
            sv->ChangeView(0.0,0.0, 1.0f, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            for (int i = 0; i < 50; ++i)
            {
                collection->Append(i);
            };
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]() { sv->ChangeView(0.0, sv->ScrollableHeight, 1.0f, true); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(isp->LastVisibleIndex == 149);
            sv->ChangeView(0.0, 0.0, 1.0f, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            for (int i = 0; i < 50; ++i)
            {
                collection->RemoveAtEnd();
            };
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]() { sv->ChangeView(0.0, sv->ScrollableHeight, 1.0f, true); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(isp->LastVisibleIndex == 99);
            sv->ChangeView(0.0, 0.0, 1.0f, true);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            for (int i = 0; i < 5; ++i)
            {
                collection->RemoveAt(70 + i);
            };
            for (int i = 0; i < 10; ++i)
            {
                collection->InsertAt(40 + i, i);
            };
            for (int i = 0; i < 15; ++i)
            {
                collection->SetAt(30 + i, i);
            };
        });

        TestServices::WindowHelper->WaitForIdle();
        RunOnUIThread([&]() { sv->ChangeView(0.0, sv->ScrollableHeight, 1.0f, true); });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(isp->LastVisibleIndex == 104);
        });
    }

    void MoCoIntegrationTests::ValidateListItemRightTap()
    {
        TestCleanupWrapper cleanup;

        auto listViewItemRightTapEvent = std::make_shared<Event>();
        auto listViewItemRightTapRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, RightTapped);
        auto listViewItemClickRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ItemClick);

        xaml_controls::ListView^ listView = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center'>"
                L"        <ListViewItem x:Name='listViewItem' Content='Item'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            itemAsFE = safe_cast<Microsoft::UI::Xaml::FrameworkElement^>(rootPanel->FindName(L"listViewItem"));
            VERIFY_IS_NOT_NULL(itemAsFE);

            listViewItemRightTapRegistration.Attach(itemAsFE, ref new Microsoft::UI::Xaml::Input::RightTappedEventHandler(
                [listViewItemRightTapEvent]
            (Platform::Object^ sender, Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs^ args)
            {
                listViewItemRightTapEvent->Set();
                LOG_OUTPUT(L"Right Tap Event Raised");
            }));

            listView->IsItemClickEnabled = true;
            listViewItemClickRegistration.Attach(listView, ref new xaml_controls::ItemClickEventHandler([]
            (Platform::Object^ sender, xaml_controls::ItemClickEventArgs^ args)
            {
                VERIFY_FAIL(L"Should not get click in right tap test case");
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        LOG_OUTPUT(L"Testing Right Tap from Pen Hold");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->PenHold(itemAsFE);
        listViewItemRightTapEvent->WaitForDefault();

        listViewItemRightTapEvent->Reset();

        LOG_OUTPUT(L"Testing Right Tap from Touch Hold");
        TestServices::WindowHelper->WaitForIdle();
        TestServices::InputHelper->Hold(itemAsFE);
        listViewItemRightTapEvent->WaitForDefault();
    }

    void MoCoIntegrationTests::ValidateLVWithIWGAndInlineLVI()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"      xmlns:local='using:Tests.External.Controls.CustomTypes'>"
                L"    <ListView x:Name='listView' Width='300' SelectionMode='Multiple' Background='Green'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsWrapGrid MaximumRowsOrColumns='1' Orientation='Horizontal'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <local:CustomListViewItemPanel/>"
                L"            </DataTemplate>"
                L"        </ListView.ItemTemplate>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            auto itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            itemsSource->Append("");
            itemsSource->Append("");
            itemsSource->Append("");

            // set the items source
            listView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            auto itemAsFE = safe_cast<FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(itemAsFE);

            VERIFY_ARE_EQUAL(itemAsFE->ActualWidth, static_cast<unsigned int>(300));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateNavigationAndSelection()
    {
        TestCleanupWrapper cleanup;
        InputDevice device = InputDevice::Gamepad;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ container = nullptr;
        Microsoft::UI::Xaml::VisualStateGroup^ commonStatesVisualStateGroup = nullptr;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto listViewSelectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListView, SelectionChanged);

        auto itemClickEvent = std::make_shared<Event>();
        auto listViewItemClickRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ItemClick);
        KeyboardInjectionIgnoreEventWaitOverride keyboardEventsOverride(KeyboardWaitKind::None);

        RunOnUIThread([&]()
        {
            // Note that we are using ListViewItemExpanded for the visual state groups
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center'>"
                L"        <ListViewItem Content='Item 1' Style='{ThemeResource ListViewItemExpanded}'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Register for VisualStateGroup.CurrentStateChanged for the containers CommonStates VSG
        RunOnUIThread([&]()
        {
            container = (xaml_controls::ListViewItem^)listView->ContainerFromIndex(0);
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(container, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
            VERIFY_IS_TRUE(groups->Size > 0);

            // we want to find the CommonStates VSG
            for (unsigned int i = 0; i < groups->Size; ++i)
            {
                commonStatesVisualStateGroup = groups->GetAt(i);

                if (commonStatesVisualStateGroup->Name == L"CommonStates")
                {
                    break;
                }
            }

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                if (args->NewState->Name == L"Pressed")
                {
                    stateChangedEvent->Set();
                }
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewSelectionChangedRegistration.Attach(listView, [selectionChangedEvent]() { selectionChangedEvent->Set();});

            listViewItemClickRegistration.Attach(listView, [itemClickEvent]() {itemClickEvent->Set();});

            listView->Focus(xaml::FocusState::Keyboard);
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::Single;
            listView->IsItemClickEnabled = false;

        });
        TestServices::WindowHelper->WaitForIdle();

        // test selection
        {
            LOG_OUTPUT(L"Selection Test");

            selectionChangedEvent->Reset();
            itemClickEvent->Reset();

            if (device == InputDevice::Gamepad)
            {
                // Gamepad needs special verification and should not be generalized.

                // Press A
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_GamepadA");

                // In order to remedy a test reliability bug, Synchronously tick the UI to ensure the PointerPressed animation
                // caused by GamepadA completes before the MockDComp Comparison expecting the item in pressed state takes place.
                // Because this animation is based on pointer position, it continuously updates
                // with each UI tick therefore will not be waited for by WaitForIdle() or produce a completed event that can be listened to.
                // A single UI tick should be sufficient to ensure the ListViewItem reaches the correct visual state
                // though this is still a race and the exact number of ticks may need to be increased if tests resume failing.
                // This wait could be replaced with a mechanism that allows waiting on "forever" animations like PointerPressed.
                TestServices::WindowHelper->SynchronouslyTickUIThread(1);

                TestServices::WindowHelper->WaitForIdle();

                // selection changed should be fired on release of GamepadA
                VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), false);

                // verify that the item is in the Pressed state
                stateChangedEvent->WaitForDefault();

                // Release A
                TestServices::KeyboardHelper->PressKeySequence(L"$u$_GamepadA");
                TestServices::WindowHelper->WaitForIdle();
            }
            else
            {
                CommonInputHelper::Accept(device);
                TestServices::WindowHelper->WaitForIdle();
            }

            selectionChangedEvent->WaitForDefault();

            VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), true);
            VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), false);

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(1));
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            listView->SelectedIndex = -1;
            listView->SelectionMode = Microsoft::UI::Xaml::Controls::ListViewSelectionMode::None;
            listView->IsItemClickEnabled = true;
            listView->Focus(xaml::FocusState::Keyboard);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test ItemClick
        {
            LOG_OUTPUT(L"ItemClick Test");

            selectionChangedEvent->Reset();
            itemClickEvent->Reset();

            CommonInputHelper::Down(device);

            if (device == InputDevice::Gamepad)
            {
                // Gamepad needs special verification and should not be generalized.

                // Press A
                TestServices::KeyboardHelper->PressKeySequence(L"$d$_GamepadA");
                TestServices::WindowHelper->WaitForIdle();

                // item click event should be fired on release of GamepadA
                VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), false);

                // Release A
                TestServices::KeyboardHelper->PressKeySequence(L"$u$_GamepadA");
                TestServices::WindowHelper->WaitForIdle();
            }
            else
            {
                CommonInputHelper::Accept(device);
                TestServices::WindowHelper->WaitForIdle();
            }

            itemClickEvent->WaitForDefault();

            VERIFY_ARE_EQUAL(selectionChangedEvent->HasFired(), false);
            VERIFY_ARE_EQUAL(itemClickEvent->HasFired(), true);
        }

        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateKeyboardTabFocusWithFirstItemDisabled()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::Button^ dummyButton = nullptr;
        std::vector<xaml_controls::ListViewItem^> itemsAsLVI(3);

        auto listViewItem2GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, GotFocus);
        auto listViewItem2GotFocusEvent = std::make_shared<Event>();

        auto listViewItem3GotFocusRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, GotFocus);
        auto listViewItem3GotFocusEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Button x:Name='dummyButton' Content='Something'/>"
                L"    <ListView x:Name='listView' Width='300'  Background='Green'/>"
                L"    <Button Content='Something'/>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            dummyButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"dummyButton"));

            auto itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            itemsSource->Append("Item 1");
            itemsSource->Append("Item 2");
            itemsSource->Append("Item 3");

            // set the items source
            listView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        FocusTestHelper::EnsureFocus(dummyButton, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            for (unsigned int i = 0; i < 3; ++i)
            {
                itemsAsLVI[i] = safe_cast<xaml_controls::ListViewItem^>(listView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(itemsAsLVI[i]);
            }

            // set the first item to disabled
            itemsAsLVI[0]->IsEnabled = false;

            // attach the GotFocus event for second item
            listViewItem2GotFocusRegistration.Attach(itemsAsLVI[1], ref new xaml::RoutedEventHandler(
                [&listViewItem2GotFocusEvent]
            (Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                listViewItem2GotFocusEvent->Set();
            }));

            // attach the GotFocus event for third item
            listViewItem3GotFocusRegistration.Attach(itemsAsLVI[2], ref new xaml::RoutedEventHandler(
                [&listViewItem3GotFocusEvent]
            (Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                listViewItem3GotFocusEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // press tab to switch focus to the ListView
        // this should set focus on the second item since the first is disabled
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        // verify second item received focus
        listViewItem2GotFocusEvent->WaitForDefault();

        // reset event
        listViewItem2GotFocusEvent->Reset();

        // Press down to the move focus to the third item
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        // verify third item received focus
        listViewItem3GotFocusEvent->WaitForDefault();

        // tab out of the listview to the next button
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        // reset event
        listViewItem3GotFocusEvent->Reset();

        // shift+tab to go back to the listview
        // in this case, third item should have focus since it was the last item to have focus before tabbing out
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        // verify third item received focus
        listViewItem3GotFocusEvent->WaitForDefault();
    }

    static void ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigation(bool forward)
    {
        TestCleanupWrapper cleanup;
        // Run this test in compat mode so style is applied immediately (during CreationComplete) without needing to be added to the tree.
        VERIFY_IS_FALSE(xaml_settings::XamlOptionalChanges::IsChangeEnabled(xaml_settings::XamlChangeId::DelayApplyStyleOptimization));

        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_data::CollectionViewSource^ cvs = nullptr;
        xaml_controls::ListViewBase^ list = nullptr;

        xaml_controls::Button^ button = nullptr;
        auto buttonGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::Button, GotFocus);
        auto buttonGotFocusEvent = std::make_shared<Event>();

        xaml_controls::GridViewItem^ item = nullptr;
        auto itemGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::GridViewItem, GotFocus);
        auto itemGotFocusEvent = std::make_shared<Event>();

        xaml_controls::GridViewHeaderItem^ groupHeader = nullptr;
        auto groupHeaderGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::GridViewHeaderItem, GotFocus);
        auto groupHeaderGotFocusEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            WEX::Common::String xamlString = WEX::Common::String().Format(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <StackPanel.Resources>"
                L"      <CollectionViewSource x:Name='cvs'/>"
                L"    </StackPanel.Resources>"
                L"    %s"
                L"    <GridView x:Name='list' Height='500' Width='500' TabNavigation='Local' ItemsSource='{Binding Source={StaticResource cvs}}' SelectionMode='None'>"
                L"        <GridView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <Grid Background='Orange' Height='150' Width='150'>"
                L"                    <TextBlock Text='{Binding}' />"
                L"                </Grid>"
                L"            </DataTemplate>"
                L"        </GridView.ItemTemplate>"
                L"        <GridView.GroupStyle>"
                L"            <GroupStyle>"
                L"                <GroupStyle.HeaderTemplate>"
                L"                    <DataTemplate>"
                L"                        <Grid Background='Blue'>"
                L"                            <TextBlock Text='{Binding}' Foreground='White' FontSize='17' />"
                L"                        </Grid>"
                L"                    </DataTemplate>"
                L"                </GroupStyle.HeaderTemplate>"
                L"            </GroupStyle>"
                L"        </GridView.GroupStyle>"
                L"    </GridView>"
                L"    %s"
                L"</StackPanel>",
                (forward) ? L"<Button x:Name='button'/>" : L"",
                (!forward) ? L"<Button x:Name='button'/>" : L"");

            rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(Platform::StringReference((const wchar_t*)xamlString.GetBuffer())));

            cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            list = safe_cast<xaml_controls::ListViewBase^>(rootPanel->FindName(L"list"));
            MocoHelper::PopulateList(list, 1, true /*isGrouped*/, 1, cvs);

            list->Measure(wf::Size(500.0f, 500.0f));

            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button"));
            VERIFY_IS_NOT_NULL(button);

            buttonGotFocusRegistration.Attach(
                button,
                ref new xaml::RoutedEventHandler([&buttonGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Button got focus.");
                buttonGotFocusEvent->Set();
            }));

            item = safe_cast<xaml_controls::GridViewItem^>(list->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(item);

            itemGotFocusRegistration.Attach(
                item,
                ref new xaml::RoutedEventHandler([&itemGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Item got focus.");
                itemGotFocusEvent->Set();
            }));

            groupHeader = safe_cast<xaml_controls::GridViewHeaderItem^>(list->GroupHeaderContainerFromItemContainer(item));
            VERIFY_IS_NOT_NULL(groupHeader);

            groupHeaderGotFocusRegistration.Attach(
                groupHeader,
                ref new xaml::RoutedEventHandler([&groupHeaderGotFocusEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                LOG_OUTPUT(L"Group header got focus.");
                groupHeaderGotFocusEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        if (forward)
        {
            // start with button
            RunOnUIThread([&]()
            {
                button->Focus(xaml::FocusState::Keyboard);
            });
            LOG_OUTPUT(L"Waiting for button to get focus.");
            buttonGotFocusEvent->WaitForDefault();

            // move to group header
            groupHeaderGotFocusEvent->Reset();
            TestServices::KeyboardHelper->Tab();
            LOG_OUTPUT(L"Waiting for group header to get focus.");
            groupHeaderGotFocusEvent->WaitForDefault();

            // move to item
            itemGotFocusEvent->Reset();
            TestServices::KeyboardHelper->Tab();
            LOG_OUTPUT(L"Waiting for item to get focus.");
            itemGotFocusEvent->WaitForDefault();

            // move back to button
            buttonGotFocusEvent->Reset();
            TestServices::KeyboardHelper->Tab();
            LOG_OUTPUT(L"Waiting for button to get focus.");
            buttonGotFocusEvent->WaitForDefault();
        }
        else
        {
            // focus on item
            RunOnUIThread([&]()
            {
                item->Focus(xaml::FocusState::Keyboard);
            });
            LOG_OUTPUT(L"Waiting for item to get focus.");
            itemGotFocusEvent->WaitForDefault();

            // move to group header
            groupHeaderGotFocusEvent->Reset();
            TestServices::KeyboardHelper->ShiftTab();
            LOG_OUTPUT(L"Waiting for group header to get focus.");
            groupHeaderGotFocusEvent->WaitForDefault();

            // wrap to button
            buttonGotFocusEvent->Reset();
            TestServices::KeyboardHelper->ShiftTab();
            LOG_OUTPUT(L"Waiting for button to get focus.");
            buttonGotFocusEvent->WaitForDefault();

            // move to item
            itemGotFocusEvent->Reset();
            TestServices::KeyboardHelper->ShiftTab();
            LOG_OUTPUT(L"Waiting for item to get focus.");
            itemGotFocusEvent->WaitForDefault();
        }
    }

    void MoCoIntegrationTests::ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigationForward()
    {
        ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigation(true /*forward*/);
    }

    void MoCoIntegrationTests::ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigationBackward()
    {
        ValidateKeyboardTabNavigationWrapsInGroupingScenarioWithLocalNavigation(false /*forward*/);
    }

    void MoCoIntegrationTests::SuspendListViewBuildTreeWhileCollapsed()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::Grid^ rootPanel = nullptr;

        Microsoft::UI::Xaml::Controls::ListView^ lv = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Navy'/> "));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;

            lv = ref new xaml_controls::ListView();
        });

        auto cccEvent = std::make_shared<Event>();
        auto cccRegistration = CreateSafeEventRegistration(Microsoft::UI::Xaml::Controls::IListViewBase, ContainerContentChanging);
        cccRegistration.Attach(
            lv,
            ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::ListViewBase^, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^>(
            [cccEvent](Microsoft::UI::Xaml::Controls::ListViewBase^ sender, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ e)
        {
            if (!e->InRecycleQueue)
            {
                e->RegisterUpdateCallback(
                    ref new ::Windows::Foundation::TypedEventHandler<Microsoft::UI::Xaml::Controls::ListViewBase^, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^>(
                    [cccEvent](Microsoft::UI::Xaml::Controls::ListViewBase^ sender, Microsoft::UI::Xaml::Controls::ContainerContentChangingEventArgs^ e)
                {
                    VERIFY_IS_TRUE(e->Phase == 1);
                    cccEvent->Set();
                }));
            }
        }));

        // load into visual tree
        RunOnUIThread([&]()
        {
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;

            auto itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();

            // populate the items
            itemsSource->Append("ListViewItem for testing.");
            lv->ItemsSource = itemsSource;
            rootPanel->Children->Append(lv);
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(cccEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"measure to force it to register buildtree work.");
            lv->Measure(wf::Size(500.f, 500.f));
        });

        TestServices::WindowHelper->WaitForIdle();
        VERIFY_IS_FALSE(cccEvent->HasFired());

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"make parent visible");
            rootPanel->Visibility = Microsoft::UI::Xaml::Visibility::Visible;
        });

        cccEvent->WaitForDefault();
        VERIFY_IS_TRUE(cccEvent->HasFired());
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::SuspendMCBPBuildTreeWhileCollapsed()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::Grid^ rootPanel = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ lv = nullptr;
        xaml_controls::ItemsStackPanel^ isp = nullptr;

        RunOnUIThread([&]()
        {
            rootPanel = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' "
                L"      Width='400' Height='400' VerticalAlignment='Top' HorizontalAlignment='Left' Background='Navy'/> "));

            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;

            lv = dynamic_cast<xaml_controls::ListView^>(xaml_markup::XamlReader::Load(
                L"<ListView xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ListView.Resources>"
                L"    <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>"
                L"  </ListView.Resources>"
                L"</ListView>"
            ));

            // initial cache length is 0
            lv->ItemsPanel = dynamic_cast<xaml_controls::ItemsPanelTemplate^>(xaml_markup::XamlReader::Load(
                L"<ItemsPanelTemplate xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'>"
                L"  <ItemsStackPanel CacheLength='0' />"
                L"</ItemsPanelTemplate>"
                ));

            MocoHelper::PopulateList(lv, 1000, false /* isGrouped */, 0, nullptr, true /* isResettable */);
            rootPanel->Children->Append(lv);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(lv->ItemsPanelRoot);

            // cache is 0, we have only 10 children
            VERIFY_ARE_EQUAL(isp->Children->Size, 10u);

            isp->Visibility = Microsoft::UI::Xaml::Visibility::Collapsed;
            isp->CacheLength = 4;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // cache is 4 but panel is collapsed, we can't expand cache, still has 10 children
            VERIFY_ARE_EQUAL(isp->Children->Size, 10u);

            isp->Visibility = Microsoft::UI::Xaml::Visibility::Visible;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // panel is visible now, cache is being expanded, panel now has 46 children.
            VERIFY_ARE_EQUAL(isp->Children->Size, 46u);
        });

    }

    void MoCoIntegrationTests::VerifyTabOutOfListViewToAppBar()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::AppBar^ appBar = nullptr;

        auto appBarGotFocusRegistration = CreateSafeEventRegistration(xaml_controls::AppBar, GotFocus);
        auto appBarGotFocusEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Page^> (xaml_markup::XamlReader::Load(
                L"<Page xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' Height='400' VerticalAlignment='Center'>"
                L"        <ListViewItem Content='Item 1'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"        <ListViewItem Content='Item 3'/>"
                L"    </ListView>"
                L"    <Page.BottomAppBar>"
                L"        <AppBar x:Name='appBar'>"
                L"            <Button/>"
                L"        </AppBar>"
                L"    </Page.BottomAppBar>"
                L"</Page>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            appBar = safe_cast<xaml_controls::AppBar^>(rootPanel->FindName(L"appBar"));
            VERIFY_IS_NOT_NULL(appBar);

            // attach the GotFocus event for the AppBar
            appBarGotFocusRegistration.Attach(appBar, ref new xaml::RoutedEventHandler(
                [&appBarGotFocusEvent]
            (Platform::Object^ sender, xaml::RoutedEventArgs^ args)
            {
                appBarGotFocusEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Press tab to tab out of ListView
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        // Make sure the GotFocus event got fired
        appBarGotFocusEvent->WaitForDefault();
    }

    void MoCoIntegrationTests::VerifyPlaceholders()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Height='400' Width='400'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            itemsSource->Append("Roger Federer");
            itemsSource->Append(nullptr);
            itemsSource->Append("Cristiano Ronaldo");
            itemsSource->Append(nullptr);

            listView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void MoCoIntegrationTests::VerifyListViewKeyboardBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' Height='400' VerticalAlignment='Center'>"
                L"       <ListView.Resources>"
                L"           <x:Double x:Key='ListViewItemMinHeight'>44</x:Double>"
                L"       </ListView.Resources>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            for (int i = 0; i < 50; i++)
            {
                Platform::String^ itemText = L"Item ";
                itemText += (i + 1);

                itemsSource->Append(itemText);
            }

            listView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test space -> should only select
        {
            LOG_OUTPUT(L"Test space");

            // select item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ #$u$_ ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key down -> should move selection to second item
        {
            LOG_OUTPUT(L"Test key down");

            // move to item 2
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 2 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key up -> should move selection to first item
        {
            LOG_OUTPUT(L"Test key up");

            // move to item 1
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key right -> should keep selection
        {
            LOG_OUTPUT(L"Test key right");

            // stay on item 1
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key left -> should keep selection
        {
            LOG_OUTPUT(L"Test key left");

            // stay on item 1
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test end -> should move selection to last item
        {
            LOG_OUTPUT(L"Test end");

            // move to item 50
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 50 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 49);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test home -> should move selection to first item
        {
            LOG_OUTPUT(L"Test home");

            // move to item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pagedown -> should move selection to 9th item
        {
            LOG_OUTPUT(L"Test pagedown");

            // move to item 9
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 9 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 8);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pageup -> should move selection to second item
        {
            LOG_OUTPUT(L"Test pageup");

            // move to item 2
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 2 selected
                VERIFY_ARE_EQUAL(listView->SelectedIndex, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoIntegrationTests::VerifyGridViewKeyboardBehavior()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <GridView x:Name='gridView' Width='400' Height='400' VerticalAlignment='Center'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            for (int i = 0; i < 50; i++)
            {
                Platform::String^ itemText = L"Item ";
                itemText += (i + 1);

                itemsSource->Append(itemText);
            }

            gridView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            gridView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test space -> should only select
        {
            LOG_OUTPUT(L"Test space");

            // select item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ #$u$_ ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key down -> should move selection to ninth item
        {
            LOG_OUTPUT(L"Test key down");

            // move to item 9
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 9 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 8);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key up -> should move selection to first item
        {
            LOG_OUTPUT(L"Test key up");

            // move to item 1
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key right -> should move selection to second item
        {
            LOG_OUTPUT(L"Test key right");

            // move to item 2
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 2 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key left -> should move selection to first item
        {
            LOG_OUTPUT(L"Test key left");

            // move to item 1
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test end -> should move selection to last item
        {
            LOG_OUTPUT(L"Test end");

            // move to item 50
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 50 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 49);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test home -> should move selection to first item
        {
            LOG_OUTPUT(L"Test home");

            // move to item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pagedown -> should move selection to last item (since all items fit on one page)
        {
            LOG_OUTPUT(L"Test pagedown");

            // move to item 50
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 50 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 49);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pageup -> should move selection to first item
        {
            LOG_OUTPUT(L"Test pageup");

            // move to item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoIntegrationTests::VerifyGridViewKeyboardBehaviorWrapGrid()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;
        Platform::Collections::Vector<Platform::Object^>^ itemsSource = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <GridView x:Name='gridView' Width='400' Height='400' VerticalAlignment='Center'>"
                L"        <GridView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <WrapGrid Orientation = 'Horizontal'/>"
                L"            </ItemsPanelTemplate>"
                L"        </GridView.ItemsPanel>"
                L"    </GridView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            itemsSource = ref new Platform::Collections::Vector<Platform::Object^>();
            VERIFY_IS_NOT_NULL(itemsSource);

            // populate the items
            for (int i = 0; i < 50; i++)
            {
                Platform::String^ itemText = L"Item ";
                itemText += (i + 1);

                itemsSource->Append(itemText);
            }

            gridView->ItemsSource = itemsSource;

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            gridView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // test space -> should only select
        {
            LOG_OUTPUT(L"Test space");

            // select item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_ #$u$_ ");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key down -> should move selection to ninth item
        {
            LOG_OUTPUT(L"Test key down");

            // move to item 9
            TestServices::KeyboardHelper->Down();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 9 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 8);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key up -> should move selection to first item
        {
            LOG_OUTPUT(L"Test key up");

            // move to item 1
            TestServices::KeyboardHelper->Up();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key right -> should move selection to second item
        {
            LOG_OUTPUT(L"Test key right");

            // move to item 2
            TestServices::KeyboardHelper->Right();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 2 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 1);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test key left -> should move selection to first item
        {
            LOG_OUTPUT(L"Test key left");

            // move to item 1
            TestServices::KeyboardHelper->Left();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test end -> should move selection to last item
        {
            LOG_OUTPUT(L"Test end");

            // move to item 50
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 50 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 49);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test home -> should move selection to first item
        {
            LOG_OUTPUT(L"Test home");

            // move to item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_home#$u$_home");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pagedown -> should move selection to last item (since all items fit on one page)
        {
            LOG_OUTPUT(L"Test pagedown");

            // move to item 50
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pagedown#$u$_pagedown");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 50 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 49);
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // test pageup -> should move selection to first item
        {
            LOG_OUTPUT(L"Test pageup");

            // move to item 1
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_pageup#$u$_pageup");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // item 1 selected
                VERIFY_ARE_EQUAL(gridView->SelectedIndex, 0);
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoIntegrationTests::VerifyEdgeScrollingWithReorder()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='200' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item to the bottom
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        // verify that the first visible index is greater than 0
        // this means that we have scrolled to the bottom
        RunOnUIThread([&]()
        {
            auto itemsPanel = safe_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);

            VERIFY_IS_GREATER_THAN(itemsPanel->FirstVisibleIndex, 0);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::CanCalculateFutureRealizationWindowCorrectly()
    {
        TestCleanupWrapper cleanup;

        Microsoft::UI::Xaml::Controls::ListView^ list;
        ::Windows::Foundation::Collections::IVector<Object^>^ data;

        // Add a ListView populated with 15 items.
        RunOnUIThread([&]()
        {
            list = ref new xaml_controls::ListView();
            auto rootPanel = ref new xaml_controls::Grid();

            list->Height = 500;
            list->Width = 500;

            MocoHelper::PopulateList(list, 15, false /* isGrouped */);
            data = safe_cast<::Windows::Foundation::Collections::IVector<Object^>^>(list->ItemsSource);

            rootPanel->Children->Append(list);
            test_infra::TestServices::WindowHelper->WindowContent = rootPanel;
        });

        // Wait for the build tree service to finish growing the buffer cache.
        TestServices::WindowHelper->WaitForIdle();

        // Jump to the second item. This is going to be our tracked element.
        RunOnUIThread([&]()
        {
            list->ScrollIntoView(data->GetAt(1), xaml_controls::ScrollIntoViewAlignment::Leading);
        });

        TestServices::WindowHelper->WaitForIdle();

        // Insert 100 elements before item 1.
        // If we miscalculate the future realization window, item 1 will get recycled and we won't
        // successfully track it.
        RunOnUIThread([&]()
        {
            const int insertCount = 100;
            auto dataToInsert = MocoBasicDataSource::GetFlatDataCollection(insertCount);

            for (int i = 0; i < insertCount; ++i)
            {
                data->InsertAt(0, dataToInsert->GetAt(i));
            }

            // Previously, this call will crash with an access violation while trying to dereference
            // the tracked element because the latter got recycled (since we calculated the wrong realization window).
            list->UpdateLayout();

            VERIFY_ARE_EQUAL(1 + insertCount, safe_cast<xaml_controls::ItemsStackPanel^>(list->ItemsPanelRoot)->FirstVisibleIndex);
        });
    }

    void MoCoIntegrationTests::VerifyRemoveSelectedDraggedItemFromItemsList()
    {
        TestCleanupWrapper cleanup;

        xaml::FrameworkElement^ itemAsFE = nullptr;
        xaml_controls::ListView^ listView = nullptr;

        auto listViewDragItemsStartingEvent = std::make_shared<Event>();
        auto listViewDragItemsStartingRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragItemsStarting);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center' CanDragItems='True'"
                L"        ItemContainerStyle='{StaticResource ListViewItemExpanded}'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            for (int i = 0; i < 10; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // select the first item (one to be dragged)
            listView->SelectedIndex = 0;

            // get the first item
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));

            // register for the DragItemsStarting event
            listViewDragItemsStartingRegistration.Attach(listView, ref new xaml_controls::DragItemsStartingEventHandler(
                [listViewDragItemsStartingEvent]
            (Platform::Object^ sender, xaml_controls::DragItemsStartingEventArgs^ args)
            {
                safe_cast<xaml_controls::ListView^>(sender)->Items->RemoveAt(0);
                listViewDragItemsStartingEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        listViewDragItemsStartingEvent->WaitForDefault();

        // no crash
    }

    void MoCoIntegrationTests::CanInsertMultipleItems()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <GridView x:Name='gridView' Width='200' Height='300' HorizontalAlignment='Center' VerticalAlignment='Center'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            for (int i = 0; i < 6; ++i)
            {
                gridView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // insert items such that the last item and one new item are outside of the current bounds of the items
            for (int i = 0; i < 10; ++i)
            {
                gridView->Items->InsertAt(1, "new");
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // the last visible index should be the total number of elements
            // (since all elements should fit on the screen)
            auto panel = static_cast<xaml_controls::ItemsWrapGrid^>(gridView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(panel->LastVisibleIndex, 15);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateLargeFooterDoesNotRecycleElements()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <GridView x:Name='gridView' Width='200' Height='300' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                L"        <GridView.Footer>"
                L"            <Border Height='350' Background='Green'/>"
                L"        </GridView.Footer>"
                L"    </GridView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            for (int i = 0; i < 6; ++i)
            {
                gridView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // set the focus on the GridView
            gridView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Press End key twice
        // First time moves focus to the LastElement
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");
        // Second time moves scrolls to the end of the list
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_end#$u$_end");

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // nothing is visible so first and last visible index should be -1
            // and we should have cached items
            auto panel = static_cast<xaml_controls::ItemsWrapGrid^>(gridView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(panel->FirstVisibleIndex, -1);
            VERIFY_ARE_EQUAL(panel->LastVisibleIndex, -1);
            VERIFY_ARE_EQUAL(panel->FirstCacheIndex, 0);
            VERIFY_ARE_EQUAL(panel->LastCacheIndex, 5);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::VerifyDraggedItemsContentNotNullWithCCC()
    {
        TestCleanupWrapper cleanup;

        xaml::FrameworkElement^ itemAsFE = nullptr;
        xaml_controls::ListView^ listView = nullptr;

        auto listViewDragItemsStartingEvent = std::make_shared<Event>();
        auto listViewDragItemsStartingRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragItemsStarting);

        auto listViewContainerContentChangingRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ContainerContentChanging);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='200' Height='200' HorizontalAlignment='Center' VerticalAlignment='Center' CanDragItems='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            // attach to the CCC event
            listViewContainerContentChangingRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ContainerContentChangingEventArgs^>(
                    [](Platform::Object^ sender, xaml_controls::ContainerContentChangingEventArgs^ args)
            {
                args->Handled = true;
            }));

            for (int i = 0; i < 10; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the first item
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));

            // register for the DragItemsStarting event
            listViewDragItemsStartingRegistration.Attach(listView, ref new xaml_controls::DragItemsStartingEventHandler(
                [listViewDragItemsStartingEvent]
            (Platform::Object^ sender, xaml_controls::DragItemsStartingEventArgs^ args)
            {
                VERIFY_IS_TRUE(args->Items->Size > 0);

                // verify the dragged item is not null
                VERIFY_IS_NOT_NULL(args->Items->GetAt(0));

                listViewDragItemsStartingEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        listViewDragItemsStartingEvent->WaitForDefault();
    }

    void MoCoIntegrationTests::VerifyListViewKeyboardReorder()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='200' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        PerformKeyboardReorderTest(listView, 1 /* initialSelectedIndex */, 2 /* resultingSelectedIndex */);
    }

    void MoCoIntegrationTests::VerifyGridViewKeyboardReorder()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <GridView x:Name='gridView' Width='400' Height='200' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            gridView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        PerformKeyboardReorderTest(gridView, 8 /* initialSelectedIndex */, 9 /* resultingSelectedIndex */);
    }

    void MoCoIntegrationTests::VerifyGridViewKeyboardReorderWrapGrid()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::Controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <GridView x:Name='gridView' Width='400' Height='200' CanReorderItems='True' AllowDrop='True'>"
                L"        <GridView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <WrapGrid Orientation = 'Horizontal'/>"
                L"            </ItemsPanelTemplate>"
                L"        </GridView.ItemsPanel>"
                L"    </GridView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            gridView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        PerformKeyboardReorderTest(gridView, 8 /* initialSelectedIndex */, 9 /* resultingSelectedIndex */);
    }

    void MoCoIntegrationTests::PerformKeyboardReorderTest(
        Microsoft::UI::Xaml::Controls::ListViewBase^ listViewBase,
        int initialSelectedIndex,
        int resultingSelectedIndex)
    {
        RunOnUIThread([&]()
        {
            listViewBase->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // press down once to move to second item
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // make sure we have the right selected index
            VERIFY_ARE_EQUAL(listViewBase->SelectedIndex, initialSelectedIndex);
        });
        TestServices::WindowHelper->WaitForIdle();

        // hold down alt+shift
        TestServices::KeyboardHelper->PressKeySequence(L"$d$_alt#$d$_shift");

        // press down twice
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Down();
        TestServices::WindowHelper->WaitForIdle();

        // press up once
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        // release alt+shift
        TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#$u$_alt");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // selected index should have changed because of the reorder
            VERIFY_ARE_EQUAL(listViewBase->SelectedIndex, resultingSelectedIndex);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::VerifyDropIntoFolder()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        ::Windows::Foundation::Point itemCenter = {};
        ::Windows::Foundation::Point folderCenter = {};
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;
        Microsoft::UI::Xaml::FrameworkElement^ folderAsFE = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        auto dragEnterRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, DragEnter);
        auto dropRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, Drop);
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);
        auto dropEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='400' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            choosingItemContainerRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [dropEvent, &dragEnterRegistration, &dropRegistration](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                auto lvi = ref new xaml_controls::ListViewItem();

                // we will treat the first item as a folder
                if (args->ItemIndex == 0)
                {
                    lvi->AllowDrop = true;

                    dragEnterRegistration.Attach(lvi,
                        ref new DragEventHandler(
                        [](Platform::Object^ sender, DragEventArgs^ args)
                    {
                        // set it to something other than None
                        // that way, the container will receive the drop event
                        args->AcceptedOperation = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy;
                    }));

                    dropRegistration.Attach(lvi,
                        ref new DragEventHandler(
                        [dropEvent](Platform::Object^ sender, DragEventArgs^ args)
                    {
                        // no reorder should happen if this is set to true
                        args->Handled = true;

                        dropEvent->Set();
                    }));
                }

                args->ItemContainer = lvi;
                args->IsContainerPrepared = true;
            }));

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the 1st item (a folder)
            folderAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(folderAsFE);

            // get the 4th item (not a folder)
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(3));
            VERIFY_IS_NOT_NULL(itemAsFE);

            // get the elements' centers
            folderCenter = ControlHelper::GetCenterOfElement(folderAsFE);
            itemCenter = ControlHelper::GetCenterOfElement(itemAsFE);

            // set the selected index to the item that will be dragged
            listView->SelectedIndex = 3;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the item to the folder
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, static_cast<int>(folderCenter.Y - itemCenter.Y) /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        dropEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            // Verify that the SelectedIndex is still the same -> no reorder has happened
            VERIFY_ARE_EQUAL(listView->SelectedIndex, 3);
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::VerifyLiveReorderStoryboards()
    {
        TestCleanupWrapper cleanup;

        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;

        unsigned int storyboardStartedCount = 0;
        auto storyboardMonitor = ref new StoryboardMonitorWrapper();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='400' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(2));
            VERIFY_IS_NOT_NULL(itemAsFE);
        });
        TestServices::WindowHelper->WaitForIdle();

        storyboardMonitor->AttachStartedHandler(
            [&](xaml_animation::Storyboard^ storyboard, xaml::UIElement^ target)
        {
            xaml_controls::ListViewItem^ lvi = dynamic_cast<xaml_controls::ListViewItem^>(target);

            if (lvi != nullptr)
            {
                ++storyboardStartedCount;
            }
        });

        // Drag the item to the bottom
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.1 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // one item being dragged
            // and two items being translated
            VERIFY_ARE_EQUAL(storyboardStartedCount, static_cast<unsigned int>(3));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateRangeSelectionAfterSelectingProgrammatically()
    {
        TestCleanupWrapper cleanup;

        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center' SelectionMode='Multiple'>"
                L"        <ListViewItem Content='Item 1'/>"
                L"        <ListViewItem Content='Item 2'/>"
                L"        <ListViewItem Content='Item 3'/>"
                L"        <ListViewItem Content='Item 4'/>"
                L"        <ListViewItem Content='Item 5'/>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        FocusTestHelper::EnsureFocus(listView, FocusState::Keyboard);
        TestServices::WindowHelper->WaitForIdle();

        // testing SelectedItem
        {
            RunOnUIThread([&]()
            {
                // set the selected item programmatically
                listView->SelectedIndex = 0;

                itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(2));
                VERIFY_IS_NOT_NULL(itemAsFE);
            });
            TestServices::WindowHelper->WaitForIdle();

            // shift click the third item
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");
            TestServices::InputHelper->LeftMouseClick(itemAsFE);
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(3));

                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));

                // item 2 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(1), listView->Items->GetAt(1));

                // item 3 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(2), listView->Items->GetAt(2));
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // testing SelectRange
        {
            RunOnUIThread([&]()
            {
                // clear selection
                listView->SelectedIndex = -1;

                itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(3));
                VERIFY_IS_NOT_NULL(itemAsFE);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(0));
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                // select the first two elements
                listView->SelectRange(ref new Microsoft::UI::Xaml::Data::ItemIndexRange(0, 2));
            });
            TestServices::WindowHelper->WaitForIdle();

            // shift click the fourth item
            TestServices::KeyboardHelper->PressKeySequence(L"$d$_shift#");
            TestServices::InputHelper->LeftMouseClick(itemAsFE);
            TestServices::KeyboardHelper->PressKeySequence(L"$u$_shift#");
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(listView->SelectedItems->Size, static_cast<unsigned int>(4));

                // item 1 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(0), listView->Items->GetAt(0));

                // item 2 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(1), listView->Items->GetAt(1));

                // item 3 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(2), listView->Items->GetAt(2));

                // item 4 selected
                VERIFY_ARE_EQUAL(listView->SelectedItems->GetAt(3), listView->Items->GetAt(3));
            });
            TestServices::WindowHelper->WaitForIdle();
        }
    }

    void MoCoIntegrationTests::ValidateGlyphAndCaptionVisibilityDuringReorder()
    {
        TestCleanupWrapper cleanup;

        int dragDistance = 0;
        const double itemHeight = 50;
        const unsigned int itemsCount = 20;
        Microsoft::UI::Xaml::FrameworkElement^ itemAsFE = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        auto firstItemDragEnterRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, DragEnter);
        auto thirdItemDragEnterRegistration = CreateSafeEventRegistration(xaml_controls::ListViewItem, DragEnter);
        auto choosingItemContainerRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, ChoosingItemContainer);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='400' CanReorderItems='True' AllowDrop='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            choosingItemContainerRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::ChoosingItemContainerEventArgs^>(
                [&firstItemDragEnterRegistration, &thirdItemDragEnterRegistration, &itemHeight](xaml_controls::ListViewBase^ sender, xaml_controls::ChoosingItemContainerEventArgs^ args)
            {
                auto lvi = ref new xaml_controls::ListViewItem();
                lvi->Height = itemHeight;

                // First item will be used as a drop target to make sure that the visiblity remains as set by the third item
                // Third item will be used as a drop target to change the visiblity to true
                if (args->ItemIndex == 0)
                {
                    lvi->AllowDrop = true;

                    firstItemDragEnterRegistration.Attach(lvi,
                        ref new DragEventHandler(
                        [](Platform::Object^ sender, DragEventArgs^ args)
                    {
                        // verify that the ListView did not override the values
                        VERIFY_IS_TRUE(args->DragUIOverride->IsCaptionVisible);
                        VERIFY_IS_TRUE(args->DragUIOverride->IsGlyphVisible);
                    }));
                }
                else if (args->ItemIndex == 2)
                {
                    lvi->AllowDrop = true;

                    thirdItemDragEnterRegistration.Attach(lvi,
                        ref new DragEventHandler(
                        [](Platform::Object^ sender, DragEventArgs^ args)
                    {
                        // verifying the default value during a Reorder
                        VERIFY_IS_FALSE(args->DragUIOverride->IsCaptionVisible);
                        VERIFY_IS_FALSE(args->DragUIOverride->IsGlyphVisible);

                        // change the values to true
                        args->DragUIOverride->IsCaptionVisible = true;
                        args->DragUIOverride->IsGlyphVisible = true;
                    }));
                }

                args->ItemContainer = lvi;
                args->IsContainerPrepared = true;
            }));

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            int dragSource = 5;
            int dragTarget = 0;

            // get the 6th item (our drag source)
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(dragSource));
            VERIFY_IS_NOT_NULL(itemAsFE);

            dragDistance = static_cast<int>(itemHeight * (dragTarget - dragSource + 1));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag from the fourth item to the first
        // Passing through the third (where the visibility will be set to true)
        // Reaching the first item (where we make sure it is still true)
        TestServices::InputHelper->DragFromCenter(itemAsFE, 0 /* relX */, dragDistance /* relY */, 0.1 /* velocityFactor */);
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateScrollingWithLiveReorder()
    {
        TestCleanupWrapper cleanup;

        const int itemHeight = 50;
        const unsigned int itemsCount = 20;
        ::Windows::Foundation::Point mouseLocation = { 0, 0 };
        std::vector<Microsoft::UI::Xaml::FrameworkElement^> itemsAsFE;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;

        auto dragItemsCompletedRegistration = CreateSafeEventRegistration(xaml_controls::ListViewBase, DragItemsCompleted);
        auto dropCompletedEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ListView x:Name='listView' Width='400' Height='300' CanReorderItems='True' AllowDrop='True' CanDragItems='True'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='0'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemContainerStyle>"
                L"            <Style TargetType='ListViewItem'>"
                L"                <Setter Property='Height' Value='" + itemHeight.ToString() + L"'/>"
                L"            </Style>"
                L"        </ListView.ItemContainerStyle>"
                L"    </ListView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            dragItemsCompletedRegistration.Attach(listView,
                ref new wf::TypedEventHandler<xaml_controls::ListViewBase^, xaml_controls::DragItemsCompletedEventArgs^>(
                [dropCompletedEvent](xaml_controls::ListViewBase^ sender, xaml_controls::DragItemsCompletedEventArgs^ args)
            {
                dropCompletedEvent->Set();
            }));

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            itemsAsFE.push_back(safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0)));
            VERIFY_IS_NOT_NULL(itemsAsFE[0]);

            itemsAsFE.push_back(safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(3)));
            VERIFY_IS_NOT_NULL(itemsAsFE[1]);

            // get the fourth item's center (drag over item)
            // also since this item will move out of view after the scroll
            // we need the location to mouse up
            mouseLocation = ControlHelper::GetCenterOfElement(itemsAsFE[1]);
        });
        TestServices::WindowHelper->WaitForIdle();

        // click and drag first item to fourth item
        TestServices::InputHelper->MouseButtonDown(itemsAsFE[0], 0, 0, test_infra::MouseButton::Left);
        TestServices::InputHelper->MoveMouse(itemsAsFE[1]);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->ScrollMouseWheel(itemsAsFE[1], -3);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the panel
            auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);
            for (int i = isp->FirstVisibleIndex; i <= isp->LastVisibleIndex; ++i)
            {
                // get the container
                auto container = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(container);

                // get the container's location
                auto rect = Microsoft::UI::Xaml::Controls::Primitives::LayoutInformation::GetLayoutSlot(container);

                // verify that the container is where it should be
                VERIFY_ARE_EQUAL(i * itemHeight, static_cast<int>(rect.Y));
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // mouse up to complete drag
        TestServices::InputHelper->MouseButtonUp(mouseLocation, test_infra::MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        dropCompletedEvent->WaitForDefault();
    }

    void MoCoIntegrationTests::ValidateInsertFromOutsideAfterLastItem()
    {
        TestCleanupWrapper cleanup;

        const int itemHeight = 50;
        const unsigned int itemsCount = 5;
        ::Windows::Foundation::Point mouseLocation = { 0, 0 };
        Microsoft::UI::Xaml::FrameworkElement^ lastItemAsFE = nullptr;
        Microsoft::UI::Xaml::Controls::Border^ border = nullptr;
        Microsoft::UI::Xaml::Controls::ListView^ listView = nullptr;
        auto listViewDragEnterRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragEnter);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <Border x:Name='border' Background='Yellow' Width='200' Height='100' CanDrag='True'/>"
                L"    <ListView x:Name='listView' Width='400' Height='500' CanReorderItems='True' AllowDrop='True'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='0'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemContainerStyle>"
                L"            <Style TargetType='ListViewItem'>"
                L"                <Setter Property='Height' Value='" + itemHeight.ToString() + L"'/>"
                L"            </Style>"
                L"        </ListView.ItemContainerStyle>"
                L"    </ListView>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            border = safe_cast<xaml_controls::Border^>(rootPanel->FindName(L"border"));
            VERIFY_IS_NOT_NULL(border);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            Platform::Collections::Vector<Platform::Object^>^ items = ref new Platform::Collections::Vector<Platform::Object^>(itemsCount);
            VERIFY_IS_NOT_NULL(items);

            int count = 0;
            for (auto i : items)
            {
                i = count++;
            }

            listView->ItemsSource = items;
            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            listViewDragEnterRegistration.Attach(listView,
                ref new DragEventHandler(
                [](Platform::Object^ sender, DragEventArgs^ args)
            {
                // set the value to something other than none
                args->AcceptedOperation = ::Windows::ApplicationModel::DataTransfer::DataPackageOperation::Copy;
            }));

            lastItemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(itemsCount - 1));
            VERIFY_IS_NOT_NULL(lastItemAsFE);

            // get the fourth item's center (drag over item)
            // also since this item will move out of view after the scroll
            // we need the location to mouse up
            mouseLocation = ControlHelper::GetCenterOfElement(lastItemAsFE);

            // we want to arrive the top half of the item so it moves away
            // Y coordinate: 5 within the item
            mouseLocation.Y -= 20;
        });
        TestServices::WindowHelper->WaitForIdle();

        // click and drag first item to fourth item
        TestServices::InputHelper->MouseButtonDown(border, 0, 0, test_infra::MouseButton::Left);
        TestServices::WindowHelper->WaitForIdle();

        TestServices::InputHelper->MoveMouse(mouseLocation);
        TestServices::WindowHelper->WaitForIdle();

        // move the location to the bottom half of the empty space
        // Y coordinate: 35 > 25 (half) since size is 50
        mouseLocation.Y += 30;
        TestServices::InputHelper->MoveMouse(mouseLocation);
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the panel
            auto isp = dynamic_cast<xaml_controls::ItemsStackPanel^>(listView->ItemsPanelRoot);
            for (int i = isp->FirstVisibleIndex; i <= isp->LastVisibleIndex; ++i)
            {
                // get the container
                auto container = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(i));
                VERIFY_IS_NOT_NULL(container);

                // get the container's location
                auto rect = Microsoft::UI::Xaml::Controls::Primitives::LayoutInformation::GetLayoutSlot(container);

                // verify that the container is where it should be
                VERIFY_ARE_EQUAL(i * itemHeight, static_cast<int>(rect.Y));
            }
        });
        TestServices::WindowHelper->WaitForIdle();

        // mouse up and reset mouse location
        TestServices::InputHelper->MouseButtonUp(mouseLocation, test_infra::MouseButton::Left);
        TestServices::InputHelper->MoveMouse({ 0, 0 });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::ValidateItemsAfterCancellingDragWithTouch()
    {
        // MockDComp should be injected and detached per test, since it keeps information like the surfaces
        // that get created.
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        // Set a consistent window size and zoom scale, so that the DComp tree is predictable.
        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(400.0f, 600.0f));

        xaml::FrameworkElement^ itemAsFE = nullptr;
        xaml_controls::ListView^ listView = nullptr;

        auto listViewDragItemsStartingEvent = std::make_shared<Event>();
        auto listViewDragItemsStartingRegistration = CreateSafeEventRegistration(xaml_controls::ListView, DragItemsStarting);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='200' Height='400' HorizontalAlignment='Center' VerticalAlignment='Center' CanReorderItems='True' AllowDrop='True' CanDragItems='True'/>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            VERIFY_IS_NOT_NULL(listView);

            for (int i = 0; i < 5; ++i)
            {
                listView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // select the first item (one to be dragged)
            listView->SelectedIndex = 0;

            // get the first item
            itemAsFE = safe_cast<xaml::FrameworkElement^>(listView->ContainerFromIndex(0));

            // register for the DragItemsStarting event
            listViewDragItemsStartingRegistration.Attach(listView, ref new xaml_controls::DragItemsStartingEventHandler(
                [listViewDragItemsStartingEvent]
            (Platform::Object^ sender, xaml_controls::DragItemsStartingEventArgs^ args)
            {
                args->Cancel = true;
                listViewDragItemsStartingEvent->Set();
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Drag the first item
        TestServices::InputHelper->PressHoldAndPanFromCenter(itemAsFE, 0 /* relX */, 100 /* relY */, 0.5 /* velocityFactor */, 1000 /* holdTime */);
        TestServices::WindowHelper->WaitForIdle();

        listViewDragItemsStartingEvent->WaitForDefault();

        // verify the DComp visual
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
    }

    void MoCoIntegrationTests::ValidateReadOnlyMode()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ listView = nullptr;
        xaml_controls::ListViewItem^ listViewItem = nullptr;
        Microsoft::UI::Xaml::VisualStateGroup^ commonStatesVisualStateGroup = nullptr;
        auto stateChangedEvent = std::make_shared<Event>();
        auto stateChangedRegistration = CreateSafeEventRegistration(xaml::VisualStateGroup, CurrentStateChanged);

        RunOnUIThread([&]()
        {
            // Note that we are using ListViewItemExpanded for the visual state groups
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <ListView x:Name='listView' Width='400' VerticalAlignment='Center' SelectionMode='None' IsItemClickEnabled='False'>"
                L"        <ListViewItem Content='Item 1' Style='{ThemeResource ListViewItemExpanded}'/>"
                L"    </ListView>"
                L"</Grid>"));
            THROW_IF_NULL(rootPanel);

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            THROW_IF_NULL(listView);

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        // Register for VisualStateGroup.CurrentStateChanged for the containers CommonStates VSG
        RunOnUIThread([&]()
        {
            listViewItem = (xaml_controls::ListViewItem^)listView->ContainerFromIndex(0);
            auto templateRoot = safe_cast<FrameworkElement^>(xaml_media::VisualTreeHelper::GetChild(listViewItem, 0));
            auto groups = xaml::VisualStateManager::GetVisualStateGroups(templateRoot);
            VERIFY_IS_TRUE(groups->Size > 0);

            // we want to find the CommonStates VSG
            for (unsigned int i = 0; i < groups->Size; ++i)
            {
                commonStatesVisualStateGroup = groups->GetAt(i);

                if (commonStatesVisualStateGroup->Name == L"CommonStates")
                {
                    break;
                }
            }

            stateChangedRegistration.Attach(commonStatesVisualStateGroup, ref new xaml::VisualStateChangedEventHandler(
                [stateChangedEvent](Platform::Object^, xaml::VisualStateChangedEventArgs^ args)
            {
                if (args->NewState->Name == L"Pressed" || args->NewState->Name == L"PointerOver")
                {
                    stateChangedEvent->Set();
                }
            }));
        });
        TestServices::WindowHelper->WaitForIdle();

        // move the mouse over the item
        // PointerOver state should NOT be invoked
        TestServices::InputHelper->MoveMouse(listViewItem);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(stateChangedEvent->HasFired());

        // reset the mouse position
        TestServices::InputHelper->MoveMouse({ 0, 0 });
        TestServices::WindowHelper->WaitForIdle();

        // now we tap the item
        // Pressed state should NOT be invoked
        TestServices::InputHelper->Tap(listViewItem);
        TestServices::WindowHelper->WaitForIdle();

        VERIFY_IS_FALSE(stateChangedEvent->HasFired());
    }

    void MoCoIntegrationTests::ValidateArrowKeysDontAffectListViewBaseItemsWhenInHeaderFooter()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::GridView^ gridView = nullptr;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' >"
                L"    <GridView x:Name='gridView' Width='200' HorizontalAlignment='Center' VerticalAlignment='Center'>"
                L"        <GridView.Header>"
                L"            <Button/>"
                L"        </GridView.Header>"
                L"        <GridView.Footer>"
                L"            <Button/>"
                L"        </GridView.Footer>"
                L"        <GridView.ItemContainerStyle>"
                L"            <Style TargetType='GridViewItem'>"
                L"                <Setter Property='Width' Value='50'/>"
                L"                <Setter Property='Margin' Value='0'/>"
                L"            </Style>"
                L"        </GridView.ItemContainerStyle>"
                L"    </GridView>"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootPanel);

            gridView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            for (int i = 1; i <= 12; ++i)
            {
                gridView->Items->Append(i);
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // set the focus on the GridView
            gridView->Focus(xaml::FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Tab from the header to the items
        TestServices::KeyboardHelper->Tab();
        // Move to the 5th item
        TestServices::KeyboardHelper->Down();
        // Move to the 6th item
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // 6th item should have focus
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(gridView->ContainerFromIndex(5)));
        });
        TestServices::WindowHelper->WaitForIdle();

        // Shift+Tab to the header from the item
        TestServices::KeyboardHelper->ShiftTab();
        TestServices::WindowHelper->WaitForIdle();

        PerformArrowKeysInHeaderFooterFocusTest();

        // Tab twice to go into the footer
        TestServices::KeyboardHelper->Tab();
        TestServices::KeyboardHelper->Tab();
        TestServices::WindowHelper->WaitForIdle();

        PerformArrowKeysInHeaderFooterFocusTest();
    }

    void MoCoIntegrationTests::PerformArrowKeysInHeaderFooterFocusTest()
    {
        Platform::Object^ focusedElement = nullptr;

        RunOnUIThread([&]()
        {
            // save the current focused element
            focusedElement = xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
        });
        TestServices::WindowHelper->WaitForIdle();

        // Press all the arrow keys
        TestServices::KeyboardHelper->Up();
        TestServices::KeyboardHelper->Down();
        TestServices::KeyboardHelper->Left();
        TestServices::KeyboardHelper->Right();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // focus should still be on the saved element (Header or Footer)
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(focusedElement));
        });
        TestServices::WindowHelper->WaitForIdle();
    }

    void MoCoIntegrationTests::VerifyFocusCanBeChangedInListViewItemClickEventHandler()
    {
        TestCleanupWrapper cleanup;

        auto listViewItemClickEvent = std::make_shared<Event>();
        auto listViewItemClickRegistration = CreateSafeEventRegistration(xaml_controls::ListView, ItemClick);

        xaml_controls::ListView^ listView;
        xaml_controls::ListViewItem^ listViewItemToTap;
        xaml_controls::Button^ firstButton;
        xaml_controls::Button^ buttonToFocus;

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::Panel^> (xaml_markup::XamlReader::Load(
                LR"(<StackPanel xmlns="http://schemas.microsoft.com/winfx/2006/xaml/presentation" xmlns:x="http://schemas.microsoft.com/winfx/2006/xaml" >
                        <Button x:Name="firstButton" Content="Button" />
                        <ListView x:Name="listView" Width="400" VerticalAlignment="Center" IsItemClickEnabled="True" >
                            <ListViewItem Content="Item 1"/>
                            <ListViewItem x:Name="listViewItemToTap" Content="Item 2"/>
                            <ListViewItem Content="Item 3"/>
                        </ListView>
                        <Button x:Name="buttonToFocus" Content="Button" />
                    </StackPanel>)"));

            listView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listView"));
            listViewItemToTap = safe_cast<xaml_controls::ListViewItem^>(rootPanel->FindName(L"listViewItemToTap"));
            firstButton = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"firstButton"));
            buttonToFocus = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"buttonToFocus"));

            listViewItemClickRegistration.Attach(listView,[&]()
            {
                LOG_OUTPUT(L"Moving focus to button 'buttonToFocus'.");
                buttonToFocus->Focus(xaml::FocusState::Programmatic);
                listViewItemClickEvent->Set();
            });

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Tapping on ListViewItem.");
        TestServices::InputHelper->Tap(listViewItemToTap);
        listViewItemClickEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(buttonToFocus));
        });
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::Moco
