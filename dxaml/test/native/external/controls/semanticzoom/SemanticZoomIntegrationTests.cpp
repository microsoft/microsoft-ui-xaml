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

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace SemanticZoom {

    bool SemanticZoomIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool SemanticZoomIntegrationTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool SemanticZoomIntegrationTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
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

        TestServices::WindowHelper->WaitForIdle();
    }

    void SemanticZoomIntegrationTests::VerifyBackButtonExitsZoomedOutView()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SemanticZoom^ semanticZoom = nullptr;

        auto viewChangeCompletedEvent = std::make_shared<Event>();
        auto viewChangeCompletedRegistration = CreateSafeEventRegistration(xaml_controls::SemanticZoom, ViewChangeCompleted);

        RunOnUIThread([&]()
        {
            semanticZoom = SetupSemanticZoomTest();

            viewChangeCompletedRegistration.Attach(semanticZoom,
                ref new xaml_controls::SemanticZoomViewChangedEventHandler(
                [viewChangeCompletedEvent](Platform::Object^ sender, xaml_controls::SemanticZoomViewChangedEventArgs^ args)
            {
                viewChangeCompletedEvent->Set();
            }));

            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
            semanticZoom->ToggleActiveView();
        });

        viewChangeCompletedEvent->WaitForDefault();
        RunOnUIThread([&]()
        {
            VERIFY_IS_FALSE(semanticZoom->IsZoomedInViewActive);
        });

        LOG_OUTPUT(L"Exit zoomed out mode using Back button");
        viewChangeCompletedEvent->Reset();
        bool backButtonPressHandled = false;
        TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
        VERIFY_IS_TRUE(backButtonPressHandled);
        viewChangeCompletedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        LOG_OUTPUT(L"After exiting zoomed out mode, further back button presses should not get handled");
        TestServices::Utilities->InjectBackButtonPress(&backButtonPressHandled);
        VERIFY_IS_FALSE(backButtonPressHandled);

        TestServices::WindowHelper->WaitForIdle();
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

    void SemanticZoomIntegrationTests::CanChangeAlignment()
    {
        TestCleanupWrapper cleanup;

        xaml_controls::SemanticZoom^ semanticZoom = nullptr;

        RunOnUIThread([&]()
        {
            auto gridRoot = safe_cast<xaml_controls::Grid^> (xaml_markup::XamlReader::Load(
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'> \r\n"
                L"  <SemanticZoom x:Name='semanticZoom'> \r\n"
                L"    <SemanticZoom.ZoomedInView> \r\n"
                L"      <GridView/> \r\n"
                L"    </SemanticZoom.ZoomedInView> \r\n"
                L"    <SemanticZoom.ZoomedOutView> \r\n"
                L"      <GridView/> \r\n"
                L"    </SemanticZoom.ZoomedOutView> \r\n"
                L"  </SemanticZoom> \r\n"
                L"</Grid>"));
            TestServices::WindowHelper->WindowContent = gridRoot;

            semanticZoom = safe_cast<xaml_controls::SemanticZoom^>(gridRoot->FindName(L"semanticZoom"));

            VERIFY_IS_NOT_NULL(semanticZoom);
            VERIFY_IS_TRUE(semanticZoom->IsZoomedInViewActive);
        });

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Changing SemanticZoom.VerticalAlignment to Top...");
            semanticZoom->VerticalAlignment = VerticalAlignment::Top;
        });

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Change successful");
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
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::SemanticZoom
