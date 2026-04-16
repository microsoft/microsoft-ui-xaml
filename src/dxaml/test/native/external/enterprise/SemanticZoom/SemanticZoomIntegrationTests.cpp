// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SemanticZoomIntegrationTests.h"

#include <TestEvent.h>
#include <TestCleanupWrapper.h>
#include <XamlTailored.h>

#include <generic\DependencyObjectTests.h>
#include <generic\FrameworkElementTests.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Enterprise { namespace SemanticZoom {

    bool SemanticZoomIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SemanticZoomIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void SemanticZoomIntegrationTests::CanInstantiate()
    {
        Generic::DependencyObjectTests<xaml_controls::SemanticZoom>::CanInstantiate();
    }

    void SemanticZoomIntegrationTests::CanEnterAndLeaveLiveTree()
    {
        Generic::FrameworkElementTests<xaml_controls::SemanticZoom>::CanEnterAndLeaveLiveTree();
    }

    void SemanticZoomIntegrationTests::CanZoomOutToKeysList()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        std::shared_ptr<Event> viewChangeStartedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();

        auto viewChangeStartedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeStarted);
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            semanticZoom = SetupSemanticZoomTest();
            VERIFY_IS_NOT_NULL(semanticZoom);

            viewChangeStartedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeStartedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeStarted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);
                VERIFY_IS_TRUE(args->IsSourceZoomedInView);

                viewChangeStartedEvent->Set();
            }));


            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeCompleted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);
                VERIFY_IS_TRUE(args->IsSourceZoomedInView);

                viewChangeCompletedEvent->Set();
            }));

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        DoToggleActiveView(semanticZoom, viewChangeStartedEvent, viewChangeCompletedEvent);
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });
    }

    void SemanticZoomIntegrationTests::CanZoomInToKey()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        std::shared_ptr<Event> viewChangeStartedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();

        auto viewChangeStartedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeStarted);
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            semanticZoom = SetupSemanticZoomTest();
            VERIFY_IS_NOT_NULL(semanticZoom);

            viewChangeStartedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeStartedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeStarted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);

                viewChangeStartedEvent->Set();
            }));


            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeCompleted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);

                viewChangeCompletedEvent->Set();
            }));

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        DoToggleActiveView(semanticZoom, viewChangeStartedEvent, viewChangeCompletedEvent);
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

        DoToggleActiveView(semanticZoom, viewChangeStartedEvent, viewChangeCompletedEvent);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });
    }

    void SemanticZoomIntegrationTests::CanSetAndGetProperties()
    {
        TestCleanupWrapper cleanup;

        RunOnUIThread([&]
        {
            auto semanticZoom = ref new xaml_controls::SemanticZoom();

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            semanticZoom->IsZoomedInViewActive = false;
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);

            VERIFY_IS_TRUE(semanticZoom->CanChangeViews);
            semanticZoom->CanChangeViews = false;
            VERIFY_IS_FALSE(semanticZoom->CanChangeViews);

            VERIFY_IS_TRUE(semanticZoom->IsZoomOutButtonEnabled);
            semanticZoom->IsZoomOutButtonEnabled = false;
            VERIFY_IS_FALSE(semanticZoom->IsZoomOutButtonEnabled);

            VERIFY_IS_NULL(semanticZoom->ZoomedInView);
            VERIFY_IS_NULL(semanticZoom->ZoomedOutView);
        });
    }

    void SemanticZoomIntegrationTests::SezoBasicWithDefaultTemplateGrouped()
    {
        TestCleanupWrapper cleanup;
        SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate::Default, true /*grouped*/);
    }

    void SemanticZoomIntegrationTests::SezoBasicWithSezoTemplateGrouped()
    {
        TestCleanupWrapper cleanup;
        SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate::SemanticZoom_WinBlue, true /*grouped */);
    }

    void SemanticZoomIntegrationTests::SezoBasicWithDefaultTemplateUnGrouped()
    {
        TestCleanupWrapper cleanup;
        SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate::Default, false /*grouped*/);
    }

    void SemanticZoomIntegrationTests::SezoBasicWithSezoTemplateUnGrouped()
    {
        TestCleanupWrapper cleanup;
        SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate::SemanticZoom_WinBlue, false /*grouped */);
    }

    void SemanticZoomIntegrationTests::ValidateToggleActiveViewWithKeyboard()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ sezoZoomedInView = nullptr;
        xaml_controls::GridView^ sezoZoomedOutView = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate::Default, true /*grouped */, sezoZoomedInView, sezoZoomedOutView);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(sezoZoomedInView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, isp->FirstVisibleIndex);

            sezoZoomedInView->Focus(FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        // shift the focus to the group header
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        // Press enter
        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        // Verify we're in the ZoomedOutView
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
            auto iwg = safe_cast<xaml_controls::ItemsWrapGrid^>(sezoZoomedOutView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, iwg->FirstVisibleIndex);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // go back to zoomed in view
            semanticZoom->ToggleActiveView();
            semanticZoom->CanChangeViews = false;
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            sezoZoomedInView->Focus(FocusState::Programmatic);
        });

        TestServices::WindowHelper->WaitForIdle();

        // shift the focus to the group header
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        // Press enter
        TestServices::KeyboardHelper->Enter();
        TestServices::WindowHelper->WaitForIdle();

        // verify that we are still in zoomed in view and we didn't
        // switch views because we set CanChangeViews to false
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            semanticZoom->CanChangeViews = true;
        });

        // press space key to toggle
        TestServices::KeyboardHelper->PressKeySequence(L" ");
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });
    }

    void SemanticZoomIntegrationTests::ValidateToggleActiveViewWithGamePad()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ sezoZoomedInView = nullptr;
        xaml_controls::GridView^ sezoZoomedOutView = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate::Default, true /*grouped */, sezoZoomedInView, sezoZoomedOutView);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(sezoZoomedInView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, isp->FirstVisibleIndex);

            sezoZoomedInView->Focus(FocusState::Keyboard);
        });

        TestServices::WindowHelper->WaitForIdle();

        // shift the focus to the group header
        TestServices::KeyboardHelper->Up();
        TestServices::WindowHelper->WaitForIdle();

        // press gamepad 'A' to zoom out
        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        // verify that we are in zoomed out view
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

        // press gamepad 'A' to zoom back in
        TestServices::KeyboardHelper->GamepadA();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

    }

    void SemanticZoomIntegrationTests::ValidateToggleActiveViewWithHeaderTap()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListViewHeaderItem^ listViewHeaderItem = nullptr;
        xaml_controls::ListView^ sezoZoomedInView = nullptr;
        xaml_controls::GridView^ sezoZoomedOutView = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate::Default, true /*grouped */, sezoZoomedInView, sezoZoomedOutView);

        RunOnUIThread([&]()
        {
            semanticZoom->Margin = xaml::Thickness({ 0, 50, 0, 0 });

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(sezoZoomedInView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, isp->FirstVisibleIndex);

            sezoZoomedInView->Focus(FocusState::Programmatic);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // get the second group header container
            listViewHeaderItem = safe_cast<xaml_controls::ListViewHeaderItem^>(sezoZoomedInView->GroupHeaderContainerFromItemContainer(sezoZoomedInView->ContainerFromIndex(7)));
        });
        TestServices::WindowHelper->WaitForIdle();

        // tap the group header
        TestServices::InputHelper->Tap(listViewHeaderItem);
        TestServices::WindowHelper->WaitForIdle();

        // Verify we're in the ZoomedOutView
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
            auto iwg = safe_cast<xaml_controls::ItemsWrapGrid^>(sezoZoomedOutView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, iwg->FirstVisibleIndex);

            // verify the currently focused item in the zoomed out view
            auto focusedElement = (DependencyObject^)xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot);
            int index = sezoZoomedOutView->IndexFromContainer(focusedElement);
            VERIFY_ARE_EQUAL(1, index);
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            // go back to zoomed in view
            semanticZoom->ToggleActiveView();
            semanticZoom->CanChangeViews = false;
        });
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        TestServices::WindowHelper->WaitForIdle();

        // tap the group header
        TestServices::InputHelper->Tap(listViewHeaderItem);
        TestServices::WindowHelper->WaitForIdle();

        // verify that we are still in zoomed in view and we didn't
        // switch views because we set CanChangeViews to false
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });
    }

    void SemanticZoomIntegrationTests::SezoZovItemHitTestableWhenGroupIsEmpty()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::ListView^ sezoZoomedInView = nullptr;
        xaml_controls::GridView^ sezoZoomedOutView = nullptr;

        // sezo with one group containing 0 items //
        xaml_controls::SemanticZoom^ semanticZoom = CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate::Default, true /*grouped */, sezoZoomedInView, sezoZoomedOutView, 1, 0);

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(sezoZoomedInView->ItemsPanelRoot);
            semanticZoom->ToggleActiveView();
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
            auto itemContainer = (xaml_controls::GridViewItem^)sezoZoomedOutView->ContainerFromIndex(0);

            // make sure that the container in the zoomed out view is hit testable.
            VERIFY_IS_TRUE(itemContainer->IsHitTestVisible);
        });
    }

    void SemanticZoomIntegrationTests::SezoBasicWithTemplate(SemanticZoomTemplate sTemplate, bool grouped)
    {
        TestCleanupWrapper cleanup;
        xaml_controls::ListView^ sezoZoomedInView = nullptr;
        xaml_controls::GridView^ sezoZoomedOutView = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = CreateSemanticZoomWithTemplateAndPopulate(sTemplate, grouped, sezoZoomedInView, sezoZoomedOutView);
        std::shared_ptr<Event> viewChangeStartedEvent = std::make_shared<Event>();
        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();

        auto viewChangeStartedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeStarted);
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            viewChangeStartedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeStartedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeStarted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);

                viewChangeStartedEvent->Set();
            }));


            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChangeCompleted raised.");

                VERIFY_IS_NOT_NULL(args->SourceItem);
                VERIFY_IS_NOT_NULL(args->DestinationItem);

                viewChangeCompletedEvent->Set();
            }));

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            auto isp = safe_cast<xaml_controls::ItemsStackPanel^>(sezoZoomedInView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, isp->FirstVisibleIndex);
        });

        DoToggleActiveView(semanticZoom, viewChangeStartedEvent, viewChangeCompletedEvent);

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
            auto iwg = safe_cast<xaml_controls::ItemsWrapGrid^>(sezoZoomedOutView->ItemsPanelRoot);
            VERIFY_ARE_EQUAL(0, iwg->FirstVisibleIndex);
        });

        DoToggleActiveView(semanticZoom, viewChangeStartedEvent, viewChangeCompletedEvent);
        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);

            if (sTemplate == SemanticZoomTemplate::Default)
            {
                // The converged threshold template should not show the button by default
                VERIFY_IS_FALSE(semanticZoom->IsZoomOutButtonEnabled);

                // The converged threshold template does not allow zooming by default
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(semanticZoom, 0));
                xaml_controls::ScrollViewer^ scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ScrollViewer"));
                VERIFY_IS_NOT_NULL(scrollViewer);
                VERIFY_ARE_EQUAL(xaml_controls::ZoomMode::Disabled, scrollViewer->ZoomMode);
            }
        });
    }


    // Validates that ListView in Semantic Zoom can switch views using UIAutomation's Invoke Pattern
    void SemanticZoomIntegrationTests::SwitchActiveViewUsingUiaInvokePatternOnListView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        xaml_controls::ListView^ listViewZoomedIn = nullptr;
        xaml_controls::ListView^ listViewZoomedOut = nullptr;

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                    L"<StackPanel.Resources>"
                        L"<CollectionViewSource x:Name='cvs' IsSourceGrouped='True' />"
                    L"</StackPanel.Resources>"
                    L"<SemanticZoom x:Name='semanticZoom' >"
                        L"<SemanticZoom.ZoomedInView>"
                            L"<ListView x:Name='listViewZoomedIn' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                                L"<ListView.ItemTemplate>"
                                    L"<DataTemplate>"
                                        L"<TextBlock Text='{Binding}'/>"
                                    L"</DataTemplate>"
                                L"</ListView.ItemTemplate>"
                                L"<ListView.GroupStyle>"
                                    L"<GroupStyle>"
                                        L"<GroupStyle.HeaderTemplate>"
                                            L"<DataTemplate>"
                                                L"<TextBlock Text='{Binding}' />"
                                            L"</DataTemplate>"
                                        L"</GroupStyle.HeaderTemplate>"
                                    L"</GroupStyle>"
                                L"</ListView.GroupStyle>"
                            L"</ListView>"
                        L"</SemanticZoom.ZoomedInView>"
                        L"<SemanticZoom.ZoomedOutView>"
                            L"<ListView x:Name='listViewZoomedOut' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                                L"<ListView.ItemTemplate>"
                                    L"<DataTemplate>"
                                        L"<TextBlock Text='{Binding}'/>"
                                    L"</DataTemplate>"
                                L"</ListView.ItemTemplate>"
                            L"</ListView>"
                        L"</SemanticZoom.ZoomedOutView>"
                    L"</SemanticZoom>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;
            loadedRegistration.Attach(rootPanel, [loadedEvent](){ loadedEvent->Set();});

            LOG_OUTPUT(L"Get Semantic Zoom");
            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(rootPanel->FindName(L"semanticZoom"));
            VERIFY_IS_NOT_NULL(semanticZoom);
            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"SemanticZoom ViewChangeCompleted event raised.");
                    viewChangeCompletedEvent->Set();
                }));

            LOG_OUTPUT(L"Get views");
            listViewZoomedIn = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listViewZoomedIn"));
            VERIFY_IS_NOT_NULL(listViewZoomedIn);

            listViewZoomedOut = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"listViewZoomedOut"));
            VERIFY_IS_NOT_NULL(listViewZoomedOut);

            LOG_OUTPUT(L"Load grouped data");
            auto cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);
            auto data = GetGroupedData(2, 2);
            cvs->Source = data;

        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoke UIA action on zoomed in view's listview header item, which will switch to zoomed out view");

            xaml_controls::ListViewItem^ listViewItem = safe_cast<xaml_controls::ListViewItem^>(listViewZoomedIn->ContainerFromIndex(0));
            xaml_controls::ListViewHeaderItem^ listViewHeaderItem = safe_cast<xaml_controls::ListViewHeaderItem^>(listViewZoomedIn->GroupHeaderContainerFromItemContainer(listViewItem));
            VERIFY_IS_NOT_NULL(listViewHeaderItem);

            xaml_automation_peers::AutomationPeer^ listViewHeaderItemAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(listViewHeaderItem);
            xaml_automation::Provider::IInvokeProvider^ invokeProvider = static_cast<xaml_automation::Provider::IInvokeProvider^>(listViewHeaderItemAP->GetPattern(xaml_automation_peers::PatternInterface::Invoke));
            invokeProvider->Invoke();
        });

        viewChangeCompletedEvent->WaitForDefault();
        viewChangeCompletedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoke UIA action on zoomed out view's listview item, which will switch to zoomed in view");

            xaml_controls::ListViewItem^ listViewItem = safe_cast<xaml_controls::ListViewItem^>(listViewZoomedOut->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(listViewItem);

            xaml_automation_peers::AutomationPeer^ listViewItemAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(listViewItem);
            xaml_automation::Provider::IInvokeProvider^ invokeProvider =

                static_cast<xaml_automation::Provider::IInvokeProvider^>(listViewItemAP->GetPattern(xaml_automation_peers::PatternInterface::Invoke));
            invokeProvider->Invoke();
        });

        viewChangeCompletedEvent->WaitForDefault();
        viewChangeCompletedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });
    }

    // Validates that GridView in Semantic Zoom can switch views using UIAutomation's Invoke Pattern
    void SemanticZoomIntegrationTests::SwitchActiveViewUsingUiaInvokePatternOnGridView()
    {
        TestCleanupWrapper cleanup;
        xaml_controls::StackPanel^ rootPanel = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        xaml_controls::GridView^ gridViewZoomedIn = nullptr;
        xaml_controls::GridView^ gridViewZoomedOut = nullptr;

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::StackPanel, Loaded);
        std::shared_ptr<Event> viewChangeCompletedEvent = std::make_shared<Event>();
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            auto rootPanel = dynamic_cast<xaml_controls::StackPanel^> (xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                    L"<StackPanel.Resources>"
                        L"<CollectionViewSource x:Name='cvs' IsSourceGrouped='True' />"
                    L"</StackPanel.Resources>"
                    L"<SemanticZoom x:Name='semanticZoom' >"
                        L"<SemanticZoom.ZoomedInView>"
                            L"<GridView x:Name='gridViewZoomedIn' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                                L"<GridView.ItemTemplate>"
                                    L"<DataTemplate>"
                                        L"<TextBlock Text='{Binding}'/>"
                                    L"</DataTemplate>"
                                L"</GridView.ItemTemplate>"
                                L"<GridView.GroupStyle>"
                                    L"<GroupStyle>"
                                        L"<GroupStyle.HeaderTemplate>"
                                            L"<DataTemplate>"
                                                L"<TextBlock Text='{Binding}' />"
                                            L"</DataTemplate>"
                                        L"</GroupStyle.HeaderTemplate>"
                                    L"</GroupStyle>"
                                L"</GridView.GroupStyle>"
                            L"</GridView>"
                        L"</SemanticZoom.ZoomedInView>"
                        L"<SemanticZoom.ZoomedOutView>"
                            L"<GridView x:Name='gridViewZoomedOut' ItemsSource='{Binding Source={StaticResource cvs}}'>"
                                L"<GridView.ItemTemplate>"
                                    L"<DataTemplate>"
                                        L"<TextBlock Text='{Binding}'/>"
                                    L"</DataTemplate>"
                                L"</GridView.ItemTemplate>"
                            L"</GridView>"
                        L"</SemanticZoom.ZoomedOutView>"
                    L"</SemanticZoom>"
                L"</StackPanel>"));
            VERIFY_IS_NOT_NULL(rootPanel);
            TestServices::WindowHelper->WindowContent = rootPanel;
            loadedRegistration.Attach(rootPanel, [loadedEvent](){ loadedEvent->Set();});

            LOG_OUTPUT(L"Get Semantic Zoom");
            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(rootPanel->FindName(L"semanticZoom"));
            VERIFY_IS_NOT_NULL(semanticZoom);
            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
                {
                    LOG_OUTPUT(L"SemanticZoom ViewChangeCompleted event raised.");
                    viewChangeCompletedEvent->Set();
                }));

            LOG_OUTPUT(L"Get views");
            gridViewZoomedIn = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridViewZoomedIn"));
            VERIFY_IS_NOT_NULL(gridViewZoomedIn);

            gridViewZoomedOut = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"gridViewZoomedOut"));
            VERIFY_IS_NOT_NULL(gridViewZoomedOut);

            LOG_OUTPUT(L"Load grouped data");
            auto cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);
            auto data = GetGroupedData(2, 2);
            cvs->Source = data;

        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoke UIA action on zoomed in view's gridview header item, which will switch to zoomed out view");

            xaml_controls::GridViewItem^ gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridViewZoomedIn->ContainerFromIndex(0));
            xaml_controls::GridViewHeaderItem^ gridViewHeaderItem = safe_cast<xaml_controls::GridViewHeaderItem^>(gridViewZoomedIn->GroupHeaderContainerFromItemContainer(gridViewItem));
            VERIFY_IS_NOT_NULL(gridViewHeaderItem);

            xaml_automation_peers::AutomationPeer^ gridViewHeaderItemAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(gridViewHeaderItem);
            xaml_automation::Provider::IInvokeProvider^ invokeProvider =

                static_cast<xaml_automation::Provider::IInvokeProvider^>(gridViewHeaderItemAP->GetPattern(xaml_automation_peers::PatternInterface::Invoke));
            invokeProvider->Invoke();
        });

        viewChangeCompletedEvent->WaitForDefault();
        viewChangeCompletedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoke UIA action on zoomed out view's gridview item, which will switch to zoomed in view");

            xaml_controls::GridViewItem^ gridViewItem = safe_cast<xaml_controls::GridViewItem^>(gridViewZoomedOut->ContainerFromIndex(0));
            VERIFY_IS_NOT_NULL(gridViewItem);

            xaml_automation_peers::AutomationPeer^ gridViewItemAP = xaml_automation_peers::FrameworkElementAutomationPeer::CreatePeerForElement(gridViewItem);
            xaml_automation::Provider::IInvokeProvider^ invokeProvider = static_cast<xaml_automation::Provider::IInvokeProvider^>(gridViewItemAP->GetPattern(xaml_automation_peers::PatternInterface::Invoke));
            invokeProvider->Invoke();
        });

        viewChangeCompletedEvent->WaitForDefault();
        viewChangeCompletedEvent->Reset();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });
    }


    void SemanticZoomIntegrationTests::SetSkipFocusSubtreeOnEnteringVisualTree()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::StackPanel^ stackPanel = nullptr;
        xaml_controls::SemanticZoom^ semanticZoom = nullptr;
        xaml_controls::Button^ targetButton = nullptr;

        std::shared_ptr<Event> loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::Grid, Loaded);

        RunOnUIThread([&]()
        {
            auto rootGrid = dynamic_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid \r\n"
                L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> \r\n"
                L"      <Button x:Name='targetButton'/> \r\n"
                L"      <StackPanel x:Name='stackPanel'> \r\n"
                L"        <SemanticZoom x:Name='semanticZoom' Width='100' Height='400'> \r\n"
                L"          <SemanticZoom.ZoomedOutView> \r\n"
                L"            <ListView> \r\n"
                L"              <ListViewItem>A</ListViewItem> \r\n"
                L"              <ListViewItem>B</ListViewItem> \r\n"
                L"              <ListViewItem>C</ListViewItem> \r\n"
                L"            </ListView> \r\n"
                L"          </SemanticZoom.ZoomedOutView> \r\n"
                L"          <SemanticZoom.ZoomedInView> \r\n"
                L"            <ListView> \r\n"
                L"              <ListViewItem>1</ListViewItem> \r\n"
                L"              <ListViewItem>2</ListViewItem> \r\n"
                L"              <ListViewItem>3</ListViewItem> \r\n"
                L"            </ListView> \r\n"
                L"          </SemanticZoom.ZoomedInView> \r\n"
                L"        </SemanticZoom> \r\n"
                L"      </StackPanel> \r\n"
                L"</Grid>"));
            VERIFY_IS_NOT_NULL(rootGrid);
            TestServices::WindowHelper->WindowContent = rootGrid;
            loadedRegistration.Attach(rootGrid, [loadedEvent]() { loadedEvent->Set(); });

            LOG_OUTPUT(L"Get Button");
            targetButton = safe_cast<xaml_controls::Button^>(rootGrid->FindName(L"targetButton"));
            VERIFY_IS_NOT_NULL(targetButton);

            LOG_OUTPUT(L"Get Stack Panel");
            stackPanel = safe_cast<xaml_controls::StackPanel^>(rootGrid->FindName(L"stackPanel"));
            VERIFY_IS_NOT_NULL(stackPanel);

            LOG_OUTPUT(L"Get Semantic Zoom");
            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(stackPanel->FindName(L"semanticZoom"));
            VERIFY_IS_NOT_NULL(semanticZoom);
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();
        }

        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(targetButton));
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Remove and replace the SemanticZoom");
            stackPanel->Children->RemoveAt(0);
            stackPanel->Children->Append(semanticZoom);
        });

        TestServices::WindowHelper->WaitForIdle();

        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadUp();
            TestServices::WindowHelper->WaitForIdle();
        }

        for (int i = 0; i < 3; i++)
        {
            TestServices::KeyboardHelper->GamepadDpadDown();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(xaml_input::FocusManager::GetFocusedElement(TestServices::WindowHelper->WindowContent->XamlRoot)->Equals(targetButton));
        });

        TestServices::WindowHelper->WaitForIdle();
    }

    xaml_controls::SemanticZoom^ SemanticZoomIntegrationTests::SetupSemanticZoomTest()
    {
        auto gridRoot = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
            L"<Grid \r\n"
            L"  xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' \r\n"
            L"  x:Name='root' Background='#000000' Width='480' Height='768' HorizontalAlignment='Left' VerticalAlignment='Top'> \r\n"
            L"      <SemanticZoom Name='semanticZoom' Width='100' Height='400'> \r\n"
            L"          <SemanticZoom.ZoomedOutView> \r\n"
            L"              <ListView> \r\n"
            L"                  <ListViewItem>A</ListViewItem> \r\n"
            L"                  <ListViewItem>B</ListViewItem> \r\n"
            L"                  <ListViewItem>C</ListViewItem> \r\n"
            L"              </ListView> \r\n"
            L"          </SemanticZoom.ZoomedOutView> \r\n"
            L"          <SemanticZoom.ZoomedInView> \r\n"
            L"              <ListView> \r\n"
            L"                  <ListViewItem>1</ListViewItem> \r\n"
            L"                  <ListViewItem>2</ListViewItem> \r\n"
            L"                  <ListViewItem>3</ListViewItem> \r\n"
            L"              </ListView> \r\n"
            L"          </SemanticZoom.ZoomedInView> \r\n"
            L"      </SemanticZoom> \r\n"
            L"</Grid>"));
        TestServices::WindowHelper->WindowContent = gridRoot;

        xaml_controls::SemanticZoom^ semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(gridRoot->FindName(L"semanticZoom"));
        return semanticZoom;
    }


    xaml_controls::SemanticZoom^ SemanticZoomIntegrationTests::CreateSemanticZoomWithTemplateAndPopulate(
        SemanticZoomTemplate sTemplate,
        bool grouped,
        xaml_controls::ListView^& sezoZoomedInView,
        xaml_controls::GridView^& sezoZoomedOutView)
    {
        return CreateSemanticZoomWithTemplateAndPopulate(sTemplate, grouped, sezoZoomedInView, sezoZoomedOutView, 3, 5);
    }

    xaml_controls::SemanticZoom^ SemanticZoomIntegrationTests::CreateSemanticZoomWithTemplateAndPopulate(SemanticZoomTemplate sTemplate, bool grouped, xaml_controls::ListView^& sezoZoomedInView, xaml_controls::GridView^& sezoZoomedOutView, int numGroups, int numItemsInGroup)
    {
        xaml_controls::SemanticZoom^ sezo;

        RunOnUIThread([&]()
        {
            auto rootPanel = safe_cast<xaml_controls::StackPanel^>(xaml_markup::XamlReader::Load(GetSezoWithTemplate(sTemplate)));
            VERIFY_IS_NOT_NULL(rootPanel);

            sezo = safe_cast<xaml_controls::SemanticZoom^>(rootPanel->FindName(L"semanticZoom"));
            VERIFY_IS_NOT_NULL(sezo);

            auto cvs = safe_cast<xaml_data::CollectionViewSource^>(rootPanel->FindName(L"cvs"));
            VERIFY_IS_NOT_NULL(cvs);

            auto data = GetGroupedData(numGroups, numItemsInGroup);
            if (!grouped)
            {
                cvs->IsSourceGrouped = false;
            }
            cvs->Source = data;

            sezoZoomedOutView = safe_cast<xaml_controls::GridView^>(rootPanel->FindName(L"sezoZov"));
            VERIFY_IS_NOT_NULL(sezoZoomedOutView);

            sezoZoomedInView = safe_cast<xaml_controls::ListView^>(rootPanel->FindName(L"sezoZiv"));
            VERIFY_IS_NOT_NULL(sezoZoomedInView);

            if (grouped)
            {
                sezoZoomedOutView->ItemsSource = cvs->View->CollectionGroups;
            }
            else
            {
                sezoZoomedOutView->ItemsSource = data;
            }

            TestServices::WindowHelper->WindowContent = rootPanel;
        });

        TestServices::WindowHelper->WaitForIdle();

        return sezo;
    }

    void SemanticZoomIntegrationTests::DoToggleActiveView(xaml_controls::SemanticZoom^ semanticZoom,
        std::shared_ptr<Event> viewChangeStartedEvent,
        std::shared_ptr<Event> viewChangeCompletedEvent)
    {
        RunOnUIThread([&]()
        {
            semanticZoom->ToggleActiveView();
        });

        viewChangeStartedEvent->WaitForDefault();
        viewChangeCompletedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    String^ SemanticZoomIntegrationTests::GetSezoWithTemplate(SemanticZoomTemplate sTemplate)
    {
        String^ semanticZoomTemplate = L"";

        if (sTemplate == SemanticZoomTemplate::SemanticZoom_WinBlue)
        {
            semanticZoomTemplate = sezoTemplate;
        }

        return     L"         <StackPanel                                                                                                                        "\
                   L"           xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'                                                                "\
                   L"           xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'                                                                           "\
                   L"           Background='Black'>                                                                                                              "\
                   L"           <StackPanel.Resources>                                                                                                           "\
                   L"               <CollectionViewSource x:Name='cvs' IsSourceGrouped='true' />                                                                 "\
                   L"            " + semanticZoomTemplate + L" "\
                   L"           </StackPanel.Resources>                                                                                                          "\
                   L"           <SemanticZoom x:Name='semanticZoom' Width='300' Height='400' >                                                                   "\
                   L"               <SemanticZoom.ZoomedOutView>                                                                                                 "\
                   L"                   <GridView ScrollViewer.IsHorizontalScrollChainingEnabled='False' Background='Beige' x:Name='sezoZov'>                    "\
                   L"                       <GridView.ItemContainerTransitions><TransitionCollection /></GridView.ItemContainerTransitions> "\
                   L"                       <GridView.ItemTemplate>                                                                                              "\
                   L"                           <DataTemplate>                                                                                                   "\
                   L"                                   <TextBlock                                                                                               "\
                   L"                                       Text='{Binding Group}'                                                                           "\
                   L"                                         Foreground='White'                                                                               "\
                   L"                                           FontFamily='Segoe UI' FontWeight='Light'                                                         "\
                   L"                                           FontSize='24'/>                                                                                  "\
                   L"                           </DataTemplate>                                                                                                  "\
                   L"                       </GridView.ItemTemplate>                                                                                             "\
                   L"                   </GridView>                                                                                                              "\
                   L"               </SemanticZoom.ZoomedOutView>                                                                                                "\
                   L"               <SemanticZoom.ZoomedInView>                                                                                                  "\
                   L"                   <ListView    x:Name='sezoZiv'                                                                                   "\
                   L"                                 ItemsSource='{Binding Source={StaticResource cvs}}'                                                        "\
                   L"                                 ShowsScrollingPlaceholders='False'                                                                         "\
                   L"                                 IsSwipeEnabled='False'                                                                                     "\
                   L"                                 IsTapEnabled='False'                                                                                       "\
                   L"                                 IsItemClickEnabled='False'                                                                                 "\
                   L"                                 SelectionMode='None'                                                                                       "\
                   L"                                 ScrollViewer.IsHorizontalScrollChainingEnabled='False' Background='Gray'>                                  "\
                   L"                       <ListView.ItemContainerTransitions><TransitionCollection /></ListView.ItemContainerTransitions> "\
                   L"                       <ListView.ItemTemplate>                                                                                              "\
                   L"                           <DataTemplate>                                                                                                   "\
                   L"                               <TextBlock Text='{Binding}' Height='25'  FontFamily='Segoe UI' VerticalAlignment='Center' />     "\
                   L"                           </DataTemplate>                                                                                                  "\
                   L"                       </ListView.ItemTemplate>                                                                                             "\
                   L"                       <ListView.GroupStyle>                                                                                                "\
                   L"                           <GroupStyle>                                                                                                     "\
                   L"                               <GroupStyle.HeaderTemplate>                                                                                  "\
                   L"                                   <DataTemplate>                                                                                           "\
                   L"                                           <TextBlock Text='{Binding}'                                                                      "\
                   L"                                                         Margin='2' FontSize='18'                                                           "\
                   L"                                                          FontFamily='Segoe UI'                                                             "\
                   L"                                                          FontWeight='Light' />                                                             "\
                   L"                                   </DataTemplate>                                                                                          "\
                   L"                               </GroupStyle.HeaderTemplate>                                                                                 "\
                   L"                           </GroupStyle>                                                                                                    "\
                   L"                       </ListView.GroupStyle>                                                                                               "\
                   L"                   </ListView>                                                                                                              "\
                   L"               </SemanticZoom.ZoomedInView>                                                                                                 "\
                   L"           </SemanticZoom>                                                                                                                  "\
                   L"       </StackPanel>";

    }
} } } } } } // Microsoft::UI::Xaml::Tests::Enterprise::SemanticZoom
