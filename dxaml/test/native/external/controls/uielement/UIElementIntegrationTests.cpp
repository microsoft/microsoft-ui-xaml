// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "UIElementIntegrationTests.h"

#include <TestCleanupWrapper.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <WUCRenderingScopeGuard.h>

using namespace ::Windows::Foundation;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace UIElement {

    bool UIElementIntegrationTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool UIElementIntegrationTests::TestSetup()
    {
        TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool UIElementIntegrationTests::TestCleanup()
    {
        TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    //
    // Test Cases
    //
    void UIElementIntegrationTests::CanBringIntoView()
    {
        BringIntoViewVerticallyWithOptions(nullptr, L"button10", 1500.0 /*expectedVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithOptions()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            VERIFY_IS_FALSE(options->AnimationDesired);
            VERIFY_IS_TRUE(_isnan(options->HorizontalAlignmentRatio));
            VERIFY_IS_TRUE(_isnan(options->VerticalAlignmentRatio));
            VERIFY_ARE_EQUAL(options->HorizontalOffset, 0.0f);
            VERIFY_ARE_EQUAL(options->VerticalOffset, 0.0f);
            options->TargetRect = {};
        });

        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithSmallTargetRect()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 45.0f, 50.0f, 100.0f, 90.0f });
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 440.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 440.0 /*expectedVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithLargeTargetRect()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -45.0f, -50.0f, 300.0f, 310.0f });
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 560.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 560.0 /*expectedVerticalOffset*/);
    }

    void UIElementIntegrationTests::CannotBringIntoViewWithTargetRect()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 0.0f, 110.0f, 200.0f, 150.0f });
        });

        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 0.0f, -25.0f, 200.0f, 60.0f });
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 0.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithVerticalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 800.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 650.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 500.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CannotBringIntoViewWithVerticalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button9", 1500.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 0.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithTargetRectAndVerticalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 100.0f, -20.0f, 50.0f, 60.0f });
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 780.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 560.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 340.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithSmallTargetRectAndVerticalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 45.0f, 50.0f, 100.0f, 90.0f });
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 850.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 645.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 440.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithLargeTargetRectAndVerticalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -45.0f, -50.0f, 300.0f, 310.0f });
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 750.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 655.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 560.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoZoomedViewWithHorizontalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 800.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"1" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1600.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 650.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"3" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1550.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"4" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 1.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 500.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"5" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1500.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"6" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 800.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"1" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1600.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 650.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"3" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1550.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"4" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 1.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 500.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"5" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1500.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"6" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoZoomedViewWithTargetRectAndHorizontalAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -20.0f, 100.0f, 60.0f, 50.0f });
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 780.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"1" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1560.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 560.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"3" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1370.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"4" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 1.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 340.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"5" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1180.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"6" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 780.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"1" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1560.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 560.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"3" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1370.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"4" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalAlignmentRatio = 1.0;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 340.0 /*expectedHorizontalOffset*/, 1.0f /*zoomFactor*/, L"5" /*outputSuffix*/);
        BringIntoViewHorizontallyWithOptions(options, L"button5", 1180.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"6" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalOffset = -10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 510.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);
        BringIntoViewVerticallyWithOptions(options, L"button5", 810.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/, true /*startAtMaxOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 490.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
        BringIntoViewVerticallyWithOptions(options, L"button5", 790.0 /*expectedVerticalOffset*/, L"4" /*outputSuffix*/, true /*startAtMaxOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoZoomedViewWithOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalOffset = -10.0f;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 1510.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalOffset = 10.0f;
        });

        BringIntoViewHorizontallyWithOptions(options, L"button5", 1490.0 /*expectedHorizontalOffset*/, 2.0f /*zoomFactor*/, L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithTargetRectAndOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 0.0f, 30.0f, 200.0f, 55.0f });
            options->VerticalOffset = -10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 395.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 375.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewWithAlignmentRatioAndOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalOffset = -10.0f;
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 810.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 660.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 510.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0f;
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 790.0 /*expectedVerticalOffset*/, L"4" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 640.0 /*expectedVerticalOffset*/, L"5" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button5", 490.0 /*expectedVerticalOffset*/, L"6" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CannotBringIntoViewWithOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalOffset = -10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 10.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);
        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 0.0 /*expectedVerticalOffset*/, L"3" /*outputSuffix*/);
        BringIntoViewVerticallyWithOptions(options, L"button10", 1490.0 /*expectedVerticalOffset*/, L"4" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CannotBringIntoViewWithAlignmentRatioAndOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalOffset = -10.0f;
            options->VerticalAlignmentRatio = 0.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringIntoViewVerticallyWithOptions(options, L"button10", 1500.0 /*expectedVerticalOffset*/, L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0f;
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 0.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewVerticallyWithOptions(options, L"button1", 0.0 /*expectedVerticalOffset*/, L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInOrthoScrollViewers()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            200.0 /*expectedInnerHorizontalOffset*/,
            400.0 /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            200.0 /*expectedInnerHorizontalOffset*/,
            400.0 /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInOrthoScrollViewersWithOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalOffset = -10.0f;
            options->VerticalOffset = -20.0f;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            210.0 /*expectedInnerHorizontalOffset*/,
            420.0 /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            210.0 /*expectedInnerHorizontalOffset*/,
            420.0 /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInOrthoScrollViewersWithAlignmentRatios()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalAlignmentRatio = 0.0;
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInOrthoScrollViewersWithTargetRect()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -25.0f, -10.0f, 150.0f, 220.0f });
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            225.0 /*expectedInnerHorizontalOffset*/,
            400.0 /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            225.0 /*expectedInnerHorizontalOffset*/,
            400.0 /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInOrthoScrollViewersWithTargetRectAndAlignmentRatios()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -25.0f, -10.0f, 150.0f, 220.0f });
            options->HorizontalAlignmentRatio = 0.0;
            options->VerticalAlignmentRatio = 1.0;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            375.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInOrthoScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            375.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewers()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            200.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/,
            L"1"  /*outputSuffix*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterHorizontalOffset*/,
            L"2"  /*outputSuffix*/,
            true  /*startInnerAtMaxOffset*/,
            false /*startOuterAtMaxOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            200.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/,
            L"1"  /*outputSuffix*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterHorizontalOffset*/,
            L"2"  /*outputSuffix*/,
            true  /*startInnerAtMaxOffset*/,
            false /*startOuterAtMaxOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewersWithOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalOffset = -10.0f;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            210.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/,
            L"1"  /*outputSuffix*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button39",
            600.0 /*expectedInnerHorizontalOffset*/,
            710.0 /*expectedOuterHorizontalOffset*/,
            L"2"  /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalOffset = 10.0f;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            390.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterHorizontalOffset*/,
            L"3"  /*outputSuffix*/,
            true  /*startInnerAtMaxOffset*/,
            false /*startOuterAtMaxOffset*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button31",
            0.0   /*expectedInnerHorizontalOffset*/,
            490.0 /*expectedOuterHorizontalOffset*/,
            L"4"  /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
            options->HorizontalOffset = -10.0f;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            210.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/,
            L"1"  /*outputSuffix*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button39",
            600.0 /*expectedInnerHorizontalOffset*/,
            710.0 /*expectedOuterHorizontalOffset*/,
            L"2"  /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->HorizontalOffset = 10.0f;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            390.0 /*expectedInnerHorizontalOffset*/,
            500.0 /*expectedOuterHorizontalOffset*/,
            L"3"  /*outputSuffix*/,
            true  /*startInnerAtMaxOffset*/,
            false /*startOuterAtMaxOffset*/);

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button31",
            0.0   /*expectedInnerHorizontalOffset*/,
            490.0 /*expectedOuterHorizontalOffset*/,
            L"4"  /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewersWithAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            400.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = false;
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            300.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            300.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewersWithAlignmentRatioAndOffset()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->HorizontalAlignmentRatio = 0.0;
            options->HorizontalOffset = -10.0f;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            410.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            410.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = false;
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            310.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            310.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewersWithTargetRect()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -25.0f, -10.0f, 150.0f, 220.0f });
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            225.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            225.0 /*expectedInnerHorizontalOffset*/,
            700.0 /*expectedOuterHorizontalOffset*/);
    }

    void UIElementIntegrationTests::CanBringIntoViewInParallelScrollViewersWithTargetRectAndAlignmentRatio()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ -25.0f, -10.0f, 150.0f, 220.0f });
            options->HorizontalAlignmentRatio = 0.0;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            375.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            375.0 /*expectedInnerHorizontalOffset*/,
            600.0 /*expectedOuterHorizontalOffset*/,
            L"1" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = false;
            options->HorizontalAlignmentRatio = 0.5;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            300.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringIntoViewInParallelScrollViewers(
            options,
            L"innerScrollViewer3",
            L"button35",
            300.0 /*expectedInnerHorizontalOffset*/,
            650.0 /*expectedOuterHorizontalOffset*/,
            L"2" /*outputSuffix*/);
    }

    void UIElementIntegrationTests::CanBringScrollViewerContentIntoView()
    {
        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
            options->VerticalAlignmentRatio = 0.0;
        });

        BringScrollViewerContentIntoView(options, 0.0 /*expectedVerticalOffset*/, true /*startAtMaxOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.5;
        });

        BringScrollViewerContentIntoView(options, 300.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringScrollViewerContentIntoView(options, 600.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->AnimationDesired = true;
        });

        BringScrollViewerContentIntoView(options, 600.0 /*expectedVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.0;
        });

        BringScrollViewerContentIntoView(options, 0.0 /*expectedVerticalOffset*/, true /*startAtMaxOffset*/);
    }

    void UIElementIntegrationTests::CanBringTextBoxIntoView()
    {
        BringTextBoxIntoView(
            nullptr     /*options*/,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/);

        BringTextBoxIntoView(
            nullptr     /*options*/,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            404.0       /*expectedOuterVerticalOffset*/,
            false       /*startInnerAtMaxOffset*/,
            true        /*startOuterAtMaxOffset*/);

        BringIntoViewOptions^ options = nullptr;

        RunOnUIThread([&]()
        {
            options = ref new BringIntoViewOptions();
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 0.0;
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            404.0       /*expectedOuterVerticalOffset*/);

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            404.0       /*expectedOuterVerticalOffset*/,
            true        /*startInnerAtMaxOffset*/,
            true        /*startOuterAtMaxOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 0.0f, 0.0f, 150.0f, 140.0f });
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            50.0        /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = std::numeric_limits<double>::quiet_NaN();
            options->TargetRect = ref new Platform::Box<::Windows::Foundation::Rect>({ 0.0f, 52.0f, 150.0f, 88.0f });
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            50.0        /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = 1.0;
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            50.0        /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/,
            true        /*startInnerAtMaxOffset*/,
            true        /*startOuterAtMaxOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalAlignmentRatio = std::numeric_limits<double>::quiet_NaN();
            options->TargetRect = nullptr;
            options->VerticalOffset = -10.0;
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            10.0        /*expectedInnerVerticalOffset*/,
            194.0       /*expectedOuterVerticalOffset*/);

        RunOnUIThread([&]()
        {
            options->VerticalOffset = 10.0;
        });

        BringTextBoxIntoView(
            options,
            L"textBox5" /*targetName*/,
            0.0         /*expectedInnerVerticalOffset*/,
            184.0       /*expectedOuterVerticalOffset*/);
    }

    void UIElementIntegrationTests::BringIntoViewVerticallyWithOptions(
        BringIntoViewOptions^ options,
        Platform::String^ targetName,
        double expectedVerticalOffset,
        Platform::String^ outputSuffix,
        bool startAtMaxOffset)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        double currentVerticalOffset = 0.0;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"  <ScrollViewer x:Name='scrollViewer' Width='500' Height='500' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"    <StackPanel>"
                L"      <Button x:Name='button1' Content='Button1' Width='200' Height='200' Background='Blue'/>"
                L"      <Button x:Name='button2' Content='Button2' Width='200' Height='200' Background='Gray'/>"
                L"      <Button x:Name='button3' Content='Button3' Width='200' Height='200' Background='Red'/>"
                L"      <Button x:Name='button4' Content='Button4' Width='200' Height='200' Background='Blue'/>"
                L"      <Button x:Name='button5' Content='Button5' Width='200' Height='200' Background='Gray'/>"
                L"      <Button x:Name='button6' Content='Button6' Width='200' Height='200' Background='Red'/>"
                L"      <Button x:Name='button7' Content='Button7' Width='200' Height='200' Background='Blue'/>"
                L"      <Button x:Name='button8' Content='Button8' Width='200' Height='200' Background='Gray'/>"
                L"      <Button x:Name='button9' Content='Button9' Width='200' Height='200' Background='Red'/>"
                L"      <Button x:Name='button10' Content='Button10' Width='200' Height='200' Background='Blue'/>"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(targetName));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for ScrollViewer to be loaded...");
        loadedEvent->WaitForDefault();

        if (startAtMaxOffset)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking ScrollViewer.ChangeView(null, 1500, null, true)");
                scrollViewer->ChangeView(nullptr /*horizontalOffset*/, 1500.0 /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the ScrollViewer to finish view change...");
            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ScrollViewer View=(%.3f, %.3f, %.3f).", scrollViewer->HorizontalOffset, currentVerticalOffset = scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            });
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoking StartBringIntoView...");
            if (options)
            {
                button->StartBringIntoView(options);
            }
            else
            {
                button->StartBringIntoView();
            }
        });

        if (expectedVerticalOffset != currentVerticalOffset)
        {
            LOG_OUTPUT(L"Waiting for the ScrollViewer to finish scrolling...");
            viewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final ScrollViewer View=(%.3f, %.3f, %.3f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, expectedVerticalOffset - 0.0001);
            VERIFY_IS_LESS_THAN(scrollViewer->VerticalOffset, expectedVerticalOffset + 0.0001);
        });

        LOG_OUTPUT(L"Verifying resulting DComp output...");
        if (outputSuffix == L"")
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }
        else
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, outputSuffix);
        }
    }

    void UIElementIntegrationTests::BringIntoViewHorizontallyWithOptions(
        BringIntoViewOptions^ options,
        Platform::String^ targetName,
        double expectedHorizontalOffset,
        float zoomFactor,
        Platform::String^ outputSuffix)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        xaml_controls::Button^ button = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"  <ScrollViewer x:Name='scrollViewer' Width='500' Height='500' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"    <ScrollViewer HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"      <StackPanel Orientation='Horizontal'>"
                L"        <Button x:Name='button1' Content='Button1' Width='200' Height='200' Background='Blue'/>"
                L"        <Button x:Name='button2' Content='Button2' Width='200' Height='200' Background='Gray'/>"
                L"        <Button x:Name='button3' Content='Button3' Width='200' Height='200' Background='Red'/>"
                L"        <Button x:Name='button4' Content='Button4' Width='200' Height='200' Background='Blue'/>"
                L"        <Button x:Name='button5' Content='Button5' Width='200' Height='200' Background='Gray'/>"
                L"        <Button x:Name='button6' Content='Button6' Width='200' Height='200' Background='Red'/>"
                L"        <Button x:Name='button7' Content='Button7' Width='200' Height='200' Background='Blue'/>"
                L"        <Button x:Name='button8' Content='Button8' Width='200' Height='200' Background='Gray'/>"
                L"        <Button x:Name='button9' Content='Button9' Width='200' Height='200' Background='Red'/>"
                L"        <Button x:Name='button10' Content='Button10' Width='200' Height='200' Background='Blue'/>"
                L"      </StackPanel>"
                L"    </ScrollViewer>"
                L"  </ScrollViewer>"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(targetName));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for ScrollViewer to be loaded...");
        loadedEvent->WaitForDefault();

        if (zoomFactor != 1.0f)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking ScrollViewer.ChangeView(null, null, %f, true /*disableAnimation*/).", zoomFactor);
                scrollViewer->ChangeView(nullptr, nullptr, zoomFactor, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for ScrollViewer view change completion...");
            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();
            TestServices::WindowHelper->WaitForIdle();
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoking StartBringIntoView...");
            if (options)
            {
                button->StartBringIntoView(options);
            }
            else
            {
                button->StartBringIntoView();
            }
        });

        if (expectedHorizontalOffset != 0.0)
        {
            LOG_OUTPUT(L"Waiting for the ScrollViewer to finish scrolling...");
            viewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final ScrollViewer View=(%.3f, %.3f, %.3f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(scrollViewer->HorizontalOffset, expectedHorizontalOffset - 0.0001);
            VERIFY_IS_LESS_THAN(scrollViewer->HorizontalOffset, expectedHorizontalOffset + 0.0001);
        });

        LOG_OUTPUT(L"Verifying resulting DComp output...");
        if (outputSuffix == L"")
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }
        else
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, outputSuffix);
        }
    }

    void UIElementIntegrationTests::BringIntoViewInOrthoScrollViewers(
        BringIntoViewOptions^ options,
        Platform::String^ innerScrollViewerName,
        Platform::String^ targetName,
        double expectedInnerHorizontalOffset,
        double expectedOuterVerticalOffset,
        Platform::String^ outputSuffix)
    {
        BringIntoViewInNestedScrollViewers(
            true /*areScrollViewersOrtho*/,
            options,
            innerScrollViewerName,
            targetName,
            expectedInnerHorizontalOffset,
            expectedOuterVerticalOffset,
            outputSuffix,
            false /*startInnerAtMaxOffset*/,
            false /*startOuterAtMaxOffset*/);
    }

    void UIElementIntegrationTests::BringIntoViewInParallelScrollViewers(
        BringIntoViewOptions^ options,
        Platform::String^ innerScrollViewerName,
        Platform::String^ targetName,
        double expectedInnerHorizontalOffset,
        double expectedOuterHorizontalOffset,
        Platform::String^ outputSuffix,
        bool startInnerAtMaxOffset,
        bool startOuterAtMaxOffset)
    {
        BringIntoViewInNestedScrollViewers(
            false /*areScrollViewersOrtho*/,
            options,
            innerScrollViewerName,
            targetName,
            expectedInnerHorizontalOffset,
            expectedOuterHorizontalOffset,
            outputSuffix,
            startInnerAtMaxOffset,
            startOuterAtMaxOffset);
    }

    void UIElementIntegrationTests::BringIntoViewInNestedScrollViewers(
        bool areScrollViewersOrtho,
        BringIntoViewOptions^ options,
        Platform::String^ innerScrollViewerName,
        Platform::String^ targetName,
        double expectedInnerOffset,
        double expectedOuterOffset,
        Platform::String^ outputSuffix,
        bool startInnerAtMaxOffset,
        bool startOuterAtMaxOffset)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        double currentInnerOffset = 0.0;
        double currentOuterOffset = 0.0;

        xaml_controls::Button^ button = nullptr;
        xaml_controls::ScrollViewer^ innerScrollViewer = nullptr;
        xaml_controls::ScrollViewer^ outerScrollViewer = nullptr;
        xaml_controls::StackPanel^ outerStackPanel = nullptr;

        auto innerViewChangedEvent = std::make_shared<Event>();
        auto innerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto outerViewChangedEvent = std::make_shared<Event>();
        auto outerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto innerLoadedEvent = std::make_shared<Event>();
        auto innerLoadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        auto outerLoadedEvent = std::make_shared<Event>();
        auto outerLoadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"  <ScrollViewer x:Name='outerScrollViewer' Width='200' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"    <StackPanel x:Name='outerStackPanel' Orientation='Horizontal'>"
                L"      <ScrollViewer x:Name='innerScrollViewer1' Width='300' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel Orientation='Horizontal'>"
                L"          <Button x:Name='button11' Content='Button11' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button12' Content='Button12' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button13' Content='Button13' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button14' Content='Button14' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button15' Content='Button15' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button16' Content='Button16' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button17' Content='Button17' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button18' Content='Button18' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button19' Content='Button19' Width='100' Height='200' Background='Red'/>"
                L"        </StackPanel>"
                L"      </ScrollViewer>"
                L"      <ScrollViewer x:Name='innerScrollViewer2' Width='300' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel Orientation='Horizontal'>"
                L"          <Button x:Name='button21' Content='Button21' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button22' Content='Button22' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button23' Content='Button23' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button24' Content='Button24' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button25' Content='Button25' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button26' Content='Button26' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button27' Content='Button27' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button28' Content='Button28' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button29' Content='Button29' Width='100' Height='200' Background='Red'/>"
                L"        </StackPanel>"
                L"      </ScrollViewer>"
                L"      <ScrollViewer x:Name='innerScrollViewer3' Width='300' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel Orientation='Horizontal'>"
                L"          <Button x:Name='button31' Content='Button31' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button32' Content='Button32' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button33' Content='Button33' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button34' Content='Button34' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button35' Content='Button35' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button36' Content='Button36' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button37' Content='Button37' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button38' Content='Button38' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button39' Content='Button39' Width='100' Height='200' Background='Red'/>"
                L"        </StackPanel>"
                L"      </ScrollViewer>"
                L"      <ScrollViewer x:Name='innerScrollViewer4' Width='300' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel Orientation='Horizontal'>"
                L"          <Button x:Name='button41' Content='Button41' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button42' Content='Button42' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button43' Content='Button43' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button44' Content='Button44' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button45' Content='Button45' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button46' Content='Button46' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button47' Content='Button47' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button48' Content='Button48' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button49' Content='Button49' Width='100' Height='200' Background='Red'/>"
                L"        </StackPanel>"
                L"      </ScrollViewer>"
                L"      <ScrollViewer x:Name='innerScrollViewer5' Width='300' Height='200' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel Orientation='Horizontal'>"
                L"          <Button x:Name='button51' Content='Button51' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button52' Content='Button52' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button53' Content='Button53' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button54' Content='Button54' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button55' Content='Button55' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button56' Content='Button56' Width='100' Height='200' Background='Red'/>"
                L"          <Button x:Name='button57' Content='Button57' Width='100' Height='200' Background='Blue'/>"
                L"          <Button x:Name='button58' Content='Button58' Width='100' Height='200' Background='Gray'/>"
                L"          <Button x:Name='button59' Content='Button59' Width='100' Height='200' Background='Red'/>"
                L"        </StackPanel>"
                L"      </ScrollViewer>"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            outerScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"outerScrollViewer"));
            innerScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(innerScrollViewerName));
            button = safe_cast<xaml_controls::Button^>(innerScrollViewer->FindName(targetName));
            outerStackPanel = safe_cast<xaml_controls::StackPanel^>(outerScrollViewer->FindName(L"outerStackPanel"));

            if (areScrollViewersOrtho)
            {
                outerScrollViewer->Width = 300.0;
                outerScrollViewer->Height = 100.0;

                outerStackPanel->Orientation = xaml_controls::Orientation::Vertical;
            }

            innerViewChangedRegistration.Attach(innerScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [innerScrollViewer, innerViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"Inner ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    innerScrollViewer->HorizontalOffset, innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    innerViewChangedEvent->Set();
                }
            }));

            innerLoadedRegistration.Attach(innerScrollViewer, ref new xaml::RoutedEventHandler([innerLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                innerLoadedEvent->Set();
            }));

            outerViewChangedRegistration.Attach(outerScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [outerScrollViewer, outerViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"Outer ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    outerScrollViewer->HorizontalOffset, outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    outerViewChangedEvent->Set();
                }
            }));

            outerLoadedRegistration.Attach(outerScrollViewer, ref new xaml::RoutedEventHandler([outerLoadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                outerLoadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for ScrollViewers to be loaded...");
        innerLoadedEvent->WaitForDefault();
        outerLoadedEvent->WaitForDefault();

        if (startInnerAtMaxOffset)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking inner ScrollViewer.ChangeView(600, null, null, true)");
                innerScrollViewer->ChangeView(600.0 /*horizontalOffset*/, nullptr /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the inner ScrollViewer to finish view change...");
            innerViewChangedEvent->WaitForDefault();
            innerViewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Inner ScrollViewer View=(%.3f, %.3f, %.3f).", currentInnerOffset = innerScrollViewer->HorizontalOffset, innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor);
            });
        }

        if (startOuterAtMaxOffset)
        {
            // areScrollViewersOrtho == false.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking outer ScrollViewer.ChangeView(1300, null, null, true)");
                outerScrollViewer->ChangeView(1300.0 /*horizontalOffset*/, nullptr /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the outer ScrollViewer to finish view change...");
            outerViewChangedEvent->WaitForDefault();
            outerViewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Outer ScrollViewer View=(%.3f, %.3f, %.3f).", currentOuterOffset = outerScrollViewer->HorizontalOffset, outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor);
            });
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoking StartBringIntoView...");
            if (options)
            {
                button->StartBringIntoView(options);
            }
            else
            {
                button->StartBringIntoView();
            }
        });

        if (expectedInnerOffset != currentInnerOffset)
        {
            LOG_OUTPUT(L"Waiting for the inner ScrollViewer to finish scrolling...");
            innerViewChangedEvent->WaitForDefault();
        }

        if (expectedOuterOffset != currentOuterOffset)
        {
            LOG_OUTPUT(L"Waiting for the outer ScrollViewer to finish scrolling...");
            outerViewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final inner ScrollViewer View=(%.3f, %.3f, %.3f).", innerScrollViewer->HorizontalOffset, innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(innerScrollViewer->HorizontalOffset, expectedInnerOffset - 0.0001);
            VERIFY_IS_LESS_THAN(innerScrollViewer->HorizontalOffset, expectedInnerOffset + 0.0001);

            LOG_OUTPUT(L"Final outer ScrollViewer View=(%.3f, %.3f, %.3f).", outerScrollViewer->HorizontalOffset, outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor);
            double outerOffset = areScrollViewersOrtho ? outerScrollViewer->VerticalOffset : outerScrollViewer->HorizontalOffset;
            VERIFY_IS_GREATER_THAN(outerOffset, expectedOuterOffset - 0.0001);
            VERIFY_IS_LESS_THAN(outerOffset, expectedOuterOffset + 0.0001);
        });

        LOG_OUTPUT(L"Verifying resulting DComp output...");
        if (outputSuffix == L"")
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }
        else
        {
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison, outputSuffix);
        }
    }

    void UIElementIntegrationTests::BringScrollViewerContentIntoView(
        BringIntoViewOptions^ options,
        double expectedVerticalOffset,
        bool startAtMaxOffset)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        double currentVerticalOffset = 0.0;

        xaml_shapes::Rectangle^ rectangle = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"  <ScrollViewer x:Name='scrollViewer' Width='400' Height='500' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"    <Rectangle x:Name='rectangle' Fill='Red' Width='700' Height='1100'/>"
                L"  </ScrollViewer>"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            rectangle = safe_cast<xaml_shapes::Rectangle^>(rootPanel->FindName(L"rectangle"));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

            viewChangedRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [scrollViewer, viewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for ScrollViewer to be loaded...");
        loadedEvent->WaitForDefault();

        if (startAtMaxOffset)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking ScrollViewer.ChangeView(null, 600, null, true)");
                scrollViewer->ChangeView(nullptr /*horizontalOffset*/, 600.0 /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the ScrollViewer to finish view change...");
            viewChangedEvent->WaitForDefault();
            viewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"ScrollViewer View=(%.3f, %.3f, %.3f).", scrollViewer->HorizontalOffset, currentVerticalOffset = scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            });
        }

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Invoking StartBringIntoView...");
            if (options)
            {
                rectangle->StartBringIntoView(options);
            }
            else
            {
                rectangle->StartBringIntoView();
            }
        });

        if (expectedVerticalOffset != currentVerticalOffset)
        {
            LOG_OUTPUT(L"Waiting for the ScrollViewer to finish scrolling...");
            viewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final ScrollViewer View=(%.3f, %.3f, %.3f).", scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset, scrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(scrollViewer->VerticalOffset, expectedVerticalOffset - 0.0001);
            VERIFY_IS_LESS_THAN(scrollViewer->VerticalOffset, expectedVerticalOffset + 0.0001);
        });
    }


    void UIElementIntegrationTests::VerifyBringIntoviewRequestedEventArgs()
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        xaml_controls::Button^ button = nullptr;
        xaml_controls::Button^ button2 = nullptr;
        xaml_controls::ScrollViewer^ scrollViewer = nullptr;

        auto bringIntoViewRequestedEvent = std::make_shared<Event>();
        auto bringIntoViewRequestedRegistration = CreateSafeEventRegistration(xaml::UIElement, BringIntoViewRequested);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"    <ScrollViewer x:Name='scrollViewer' Height='500' MaxHeight='500' VerticalScrollBarVisibility='Hidden'>"
                L"        <StackPanel >"
                L"          <Button x:Name='button1' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button2' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button3' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button3' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button4' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button5' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button6' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button7' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button8' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button9' Content='Button' Width='200' Height='200'/> "
                L"          <Button x:Name='button10' Content='Button' Width='200' Height='200'/> "
                L"        </StackPanel >"
                L"    </ScrollViewer >"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            button = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button1"));
            button2 = safe_cast<xaml_controls::Button^>(rootPanel->FindName(L"button2"));
            scrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

            bringIntoViewRequestedRegistration.Attach(button, ref new wf::TypedEventHandler<xaml::UIElement^, xaml::BringIntoViewRequestedEventArgs^>(
                [&](Platform::Object^ sender, xaml::BringIntoViewRequestedEventArgs^ args)
            {
                bringIntoViewRequestedEvent->Set();
            }));

            loadedRegistration.Attach(scrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^ e)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for ScrollViewer to be loaded...");
        loadedEvent->WaitForDefault();

        RunOnUIThread([&]()
        {
            BringIntoViewOptions^ options = ref new BringIntoViewOptions();
            options->AnimationDesired = true;
            options->TargetRect = {};
            button->StartBringIntoView(options);
        });
        LOG_OUTPUT(L"Waiting for the scrollViewer to finish scrolling.");
        bringIntoViewRequestedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();
    }

    void UIElementIntegrationTests::BringTextBoxIntoView(
        BringIntoViewOptions^ options,
        Platform::String^ targetName,
        double expectedInnerVerticalOffset,
        double expectedOuterVerticalOffset,
        bool startInnerAtMaxOffset,
        bool startOuterAtMaxOffset)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        TestServices::WindowHelper->SetWindowSizeOverride(wf::Size(800, 600));

        double currentInnerVerticalOffset = 0.0;
        double currentOuterVerticalOffset = 0.0;

        xaml_controls::TextBox^ textBox = nullptr;
        xaml_controls::ScrollViewer^ innerScrollViewer = nullptr;
        xaml_controls::ScrollViewer^ outerScrollViewer = nullptr;

        auto innerViewChangedEvent = std::make_shared<Event>();
        auto innerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto outerViewChangedEvent = std::make_shared<Event>();
        auto outerViewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, Loaded);

        RunOnUIThread([&]()
        {
            xaml_controls::Grid^ rootPanel = nullptr;

            Platform::String^ xamlText = L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='root'>"
                L"  <ScrollViewer x:Name='scrollViewer' Width='150' Height='300' HorizontalScrollBarVisibility='Hidden' VerticalScrollBarVisibility='Hidden'>"
                L"    <StackPanel>"
                L"      <StackPanel.Resources>"
                L"        <Style TargetType='TextBox'>"
                L"          <Setter Property='FontSize' Value='15'/>"
                L"          <Setter Property='Padding' Value='10,3,6,5'/>"
                L"        </Style>"
                L"      </StackPanel.Resources>"
                L"      <TextBox x:Name='textBox1' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox2' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox3' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox4' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox5' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox6' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox7' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox8' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"      <TextBox x:Name='textBox9' TextWrapping='Wrap' Text='Using long words is important however using longer words is of utmost importance, because they allow the text to scroll.' Width='150' Height='100'/>"
                L"    </StackPanel>"
                L"  </ScrollViewer>"
                L"</Grid>";

            rootPanel = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(xamlText));
            textBox = safe_cast<xaml_controls::TextBox^>(rootPanel->FindName(targetName));
            outerScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(rootPanel->FindName(L"scrollViewer"));

            outerViewChangedRegistration.Attach(outerScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [outerScrollViewer, outerViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"Outer ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    outerScrollViewer->HorizontalOffset, outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    outerViewChangedEvent->Set();
                }
            }));

            loadedRegistration.Attach(outerScrollViewer, ref new xaml::RoutedEventHandler([loadedEvent](Platform::Object^, xaml::RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            TestServices::WindowHelper->WindowContent = rootPanel;
        });
        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Waiting for outer ScrollViewer to be loaded...");
        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            xaml::FrameworkElement^ controlTemplateRoot = safe_cast<xaml::FrameworkElement^>(Microsoft::UI::Xaml::Media::VisualTreeHelper::GetChild(textBox, 0));
            innerScrollViewer = safe_cast<xaml_controls::ScrollViewer^>(controlTemplateRoot->FindName(L"ContentElement"));

            innerViewChangedRegistration.Attach(innerScrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [innerScrollViewer, innerViewChangedEvent](Platform::Object^, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"Inner ViewChanged raised. View=(%.3f, %.3f, %.3f), IsIntermediate=%d.",
                    innerScrollViewer->HorizontalOffset, innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor, args->IsIntermediate);
                if (!args->IsIntermediate)
                {
                    innerViewChangedEvent->Set();
                }
            }));
        });

        if (startInnerAtMaxOffset)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking inner ScrollViewer.ChangeView(null, 600, null, true)");
                innerScrollViewer->ChangeView(nullptr /*horizontalOffset*/, 600.0 /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the inner ScrollViewer to finish view change...");
            innerViewChangedEvent->WaitForDefault();
            innerViewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Inner ScrollViewer View=(%.3f, %.3f, %.3f).", innerScrollViewer->HorizontalOffset, currentInnerVerticalOffset = innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor);
            });
        }

        if (startOuterAtMaxOffset)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Invoking outer ScrollViewer.ChangeView(null, 600, null, true)");
                outerScrollViewer->ChangeView(nullptr /*horizontalOffset*/, 600.0 /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            });

            LOG_OUTPUT(L"Waiting for the outer ScrollViewer to finish view change...");
            outerViewChangedEvent->WaitForDefault();
            outerViewChangedEvent->Reset();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Outer ScrollViewer View=(%.3f, %.3f, %.3f).", outerScrollViewer->HorizontalOffset, currentOuterVerticalOffset = outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor);
            });
        }

        RunOnUIThread([&]()
        {
            xaml::UIElement^ textBoxContent = static_cast<xaml::UIElement^>(innerScrollViewer->Content);
            VERIFY_IS_NOT_NULL(textBoxContent);

            LOG_OUTPUT(L"Invoking StartBringIntoView...");
            if (options)
            {
                textBoxContent->StartBringIntoView(options);
            }
            else
            {
                textBoxContent->StartBringIntoView();
            }
        });

        if (expectedInnerVerticalOffset != currentInnerVerticalOffset)
        {
            LOG_OUTPUT(L"Waiting for the inner ScrollViewer to finish scrolling...");
            innerViewChangedEvent->WaitForDefault();
        }

        if (expectedOuterVerticalOffset != currentOuterVerticalOffset)
        {
            LOG_OUTPUT(L"Waiting for the outer ScrollViewer to finish scrolling...");
            outerViewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final inner ScrollViewer View=(%.3f, %.3f, %.3f).", innerScrollViewer->HorizontalOffset, innerScrollViewer->VerticalOffset, innerScrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(innerScrollViewer->VerticalOffset, expectedInnerVerticalOffset - 0.0001);
            VERIFY_IS_LESS_THAN(innerScrollViewer->VerticalOffset, expectedInnerVerticalOffset + 0.0001);

            LOG_OUTPUT(L"Final outer ScrollViewer View=(%.3f, %.3f, %.3f).", outerScrollViewer->HorizontalOffset, outerScrollViewer->VerticalOffset, outerScrollViewer->ZoomFactor);
            VERIFY_IS_GREATER_THAN(outerScrollViewer->VerticalOffset, expectedOuterVerticalOffset - 0.0001);
            VERIFY_IS_LESS_THAN(outerScrollViewer->VerticalOffset, expectedOuterVerticalOffset + 0.0001);
        });
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::UIElement
