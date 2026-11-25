// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollContentPresenterPublicApiTests.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <TreeHelper.h>
#include <cmath>
#include <limits>
#include <vector>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        // ScrollViewer_Partial.h documents the wheel step: max(48, 15% of the viewport) pixels per
        // 120 wheel delta, capped by the viewport. The public parameterless MouseWheel* methods use
        // the default 120 delta, and a 200px viewport keeps 15% (30px) below the 48px floor.
        static const double c_expectedWheelStep = 48.0;
        static const double c_tolerance = 0.5;

        static bool AreClose(double expected, double actual)
        {
            return std::abs(expected - actual) <= c_tolerance;
        }

        // ScrollContentPresenter offsets are requested synchronously but published (HorizontalOffset /
        // VerticalOffset, i.e. the computed offsets) by its next arrange pass, so every scroll action is
        // followed by WaitForIdle before its result is observed.
        static void ActThenWaitForLayout(std::function<void()> action)
        {
            RunOnUIThread([&]() { action(); });
            TestServices::WindowHelper->WaitForIdle();
        }

        static void VerifyOffsets(
            xaml_controls::ScrollContentPresenter^ presenter,
            xaml_controls::ScrollViewer^ scrollViewer,
            double expectedHorizontal,
            double expectedVertical)
        {
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Presenter offsets H=%f V=%f, ScrollViewer offsets H=%f V=%f, expected H=%f V=%f",
                    presenter->HorizontalOffset, presenter->VerticalOffset,
                    scrollViewer->HorizontalOffset, scrollViewer->VerticalOffset,
                    expectedHorizontal, expectedVertical);
                VERIFY_IS_TRUE(AreClose(expectedHorizontal, presenter->HorizontalOffset));
                VERIFY_IS_TRUE(AreClose(expectedVertical, presenter->VerticalOffset));
                VERIFY_IS_TRUE(AreClose(expectedHorizontal, scrollViewer->HorizontalOffset));
                VERIFY_IS_TRUE(AreClose(expectedVertical, scrollViewer->VerticalOffset));
            });
        }

        bool ScrollContentPresenterPublicApiTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool ScrollContentPresenterPublicApiTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ScrollContentPresenterPublicApiTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        xaml_controls::ScrollContentPresenter^ ScrollContentPresenterPublicApiTests::LoadScrollViewer(
            bool canScrollVertically,
            _Out_ xaml_controls::ScrollViewer^* scrollViewer,
            _Out_ xaml::FrameworkElement^* root)
        {
            xaml_controls::ScrollContentPresenter^ presenter = nullptr;
            auto loadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(xaml::FrameworkElement, Loaded);

            Platform::String^ verticalVisibility = canScrollVertically ? Platform::StringReference(L"Auto") : Platform::StringReference(L"Disabled");
            Platform::String^ xamlPrefix =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"  <ScrollViewer x:Name='sv' Width='200' Height='200' HorizontalAlignment='Left' VerticalAlignment='Top'"
                L"      HorizontalScrollBarVisibility='Auto' VerticalScrollBarVisibility='";
            Platform::String^ xamlSuffix =
                L"'>"
                L"    <Canvas x:Name='canvas' Width='1000' Height='1000' Background='LightGray'>"
                L"      <Border x:Name='nearTarget' Canvas.Left='10' Canvas.Top='10' Width='50' Height='50' Background='Green'/>"
                L"      <Border x:Name='farTarget' Canvas.Left='600' Canvas.Top='600' Width='50' Height='50' Background='Red'/>"
                L"    </Canvas>"
                L"  </ScrollViewer>"
                L"</Grid>";
            Platform::String^ xamlString = xamlPrefix + verticalVisibility + xamlSuffix;

            RunOnUIThread([&]()
            {
                *root = safe_cast<xaml::FrameworkElement^>(xaml_markup::XamlReader::Load(xamlString));
                VERIFY_IS_NOT_NULL(*root);
                loadedRegistration.Attach(*root, [loadedEvent]() { loadedEvent->Set(); });
                TestServices::WindowHelper->WindowContent = *root;
            });

            loadedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                *scrollViewer = safe_cast<xaml_controls::ScrollViewer^>((*root)->FindName(L"sv"));
                VERIFY_IS_NOT_NULL(*scrollViewer);
                presenter = TreeHelper::GetVisualChildByType<xaml_controls::ScrollContentPresenter>(*scrollViewer);
                VERIFY_IS_NOT_NULL(presenter);

                // Preconditions: the presenter is the scroll client, content is scrollable and the view starts at the origin.
                VERIFY_IS_TRUE(presenter->ExtentWidth > presenter->ViewportWidth);
                VERIFY_IS_TRUE(presenter->ViewportHeight >= c_expectedWheelStep);
                VERIFY_IS_TRUE(AreClose(0.0, presenter->HorizontalOffset));
                VERIFY_IS_TRUE(AreClose(0.0, presenter->VerticalOffset));
                VERIFY_ARE_EQUAL(canScrollVertically, presenter->CanVerticallyScroll);
                VERIFY_IS_TRUE(presenter->CanHorizontallyScroll);
                if (canScrollVertically)
                {
                    VERIFY_IS_TRUE(presenter->ExtentHeight > presenter->ViewportHeight);
                }
            });

            return presenter;
        }

        void ScrollContentPresenterPublicApiTests::MouseWheelMethodsScrollContent()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml::FrameworkElement^ root = nullptr;

            LOG_OUTPUT(L"Vertically and horizontally scrollable content.");
            auto presenter = LoadScrollViewer(true /*canScrollVertically*/, &scrollViewer, &root);

            LOG_OUTPUT(L"Lower bound: wheeling up/left at the origin does not move the view.");
            ActThenWaitForLayout([&]() { presenter->MouseWheelUp(); presenter->MouseWheelLeft(); });
            VerifyOffsets(presenter, scrollViewer, 0.0, 0.0);

            LOG_OUTPUT(L"MouseWheelDown scrolls vertically by one wheel step.");
            ActThenWaitForLayout([&]() { presenter->MouseWheelDown(); });
            VerifyOffsets(presenter, scrollViewer, 0.0, c_expectedWheelStep);

            LOG_OUTPUT(L"MouseWheelRight scrolls horizontally by one wheel step.");
            ActThenWaitForLayout([&]() { presenter->MouseWheelRight(); });
            VerifyOffsets(presenter, scrollViewer, c_expectedWheelStep, c_expectedWheelStep);

            LOG_OUTPUT(L"MouseWheelUp and MouseWheelLeft are the inverse operations.");
            ActThenWaitForLayout([&]() { presenter->MouseWheelUp(); });
            VerifyOffsets(presenter, scrollViewer, c_expectedWheelStep, 0.0);
            ActThenWaitForLayout([&]() { presenter->MouseWheelLeft(); });
            VerifyOffsets(presenter, scrollViewer, 0.0, 0.0);

            LOG_OUTPUT(L"Upper bound: a wheel step near the end clamps at the scrollable maximum.");
            double maxHorizontalOffset = 0.0;
            double maxVerticalOffset = 0.0;
            ActThenWaitForLayout([&]()
            {
                maxHorizontalOffset = presenter->ExtentWidth - presenter->ViewportWidth;
                maxVerticalOffset = presenter->ExtentHeight - presenter->ViewportHeight;
                presenter->SetHorizontalOffset(maxHorizontalOffset - 10.0);
                presenter->SetVerticalOffset(maxVerticalOffset - 10.0);
            });
            VerifyOffsets(presenter, scrollViewer, maxHorizontalOffset - 10.0, maxVerticalOffset - 10.0);
            ActThenWaitForLayout([&]() { presenter->MouseWheelDown(); presenter->MouseWheelRight(); });
            VerifyOffsets(presenter, scrollViewer, maxHorizontalOffset, maxVerticalOffset);

            LOG_OUTPUT(L"Vertical scrolling disabled: the vertical wheel methods scroll horizontally instead.");
            presenter = LoadScrollViewer(false /*canScrollVertically*/, &scrollViewer, &root);

            ActThenWaitForLayout([&]() { presenter->MouseWheelDown(); });
            VerifyOffsets(presenter, scrollViewer, c_expectedWheelStep, 0.0);
            ActThenWaitForLayout([&]() { presenter->MouseWheelUp(); });
            VerifyOffsets(presenter, scrollViewer, 0.0, 0.0);
        }

        void ScrollContentPresenterPublicApiTests::OffsetRequestsBeyondTheEndAreClampedWhenRequested()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml::FrameworkElement^ root = nullptr;
            auto presenter = LoadScrollViewer(true /*canScrollVertically*/, &scrollViewer, &root);
            auto requestedHorizontalOffsets = std::make_shared<std::vector<double>>();
            auto viewChangingRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanging);
            double maxHorizontalOffset = 0.0;

            // ScrollViewer.ViewChanging is raised synchronously by the presenter's offset request (ScrollData::put_OffsetX ->
            // IScrollOwner::NotifyHorizontalOffsetChanging) with the requested target, before any layout coerces it.
            RunOnUIThread([&]()
            {
                maxHorizontalOffset = presenter->ExtentWidth - presenter->ViewportWidth;
                viewChangingRegistration.Attach(scrollViewer, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangingEventArgs^>(
                    [requestedHorizontalOffsets](Platform::Object^, xaml_controls::ScrollViewerViewChangingEventArgs^ args)
                    {
                        requestedHorizontalOffsets->push_back(args->NextView->HorizontalOffset);
                    }));
            });

            LOG_OUTPUT(L"A request past the end is clamped to ExtentWidth - ViewportWidth when it is made.");
            ActThenWaitForLayout([&]() { presenter->SetHorizontalOffset(maxHorizontalOffset + 500.0); });
            VerifyOffsets(presenter, scrollViewer, maxHorizontalOffset, 0.0);
            std::vector<double> clampedRequests;
            RunOnUIThread([&]()
            {
                clampedRequests = *requestedHorizontalOffsets;
                requestedHorizontalOffsets->clear();
            });
            LOG_OUTPUT(L"%u ViewChanging event(s) for the request past the end (max horizontal offset %f).", static_cast<unsigned>(clampedRequests.size()), maxHorizontalOffset);
            VERIFY_IS_FALSE(clampedRequests.empty());
            for (double requested : clampedRequests)
            {
                LOG_OUTPUT(L"ViewChanging NextView.HorizontalOffset=%f", requested);
                VERIFY_IS_TRUE(AreClose(maxHorizontalOffset, requested));
            }

            LOG_OUTPUT(L"At the end, wheeling right or requesting a larger offset does not request a view past the end.");
            ActThenWaitForLayout([&]() { presenter->MouseWheelRight(); presenter->SetHorizontalOffset(maxHorizontalOffset + 10.0); });
            VerifyOffsets(presenter, scrollViewer, maxHorizontalOffset, 0.0);
            std::vector<double> requestsAtTheEnd;
            RunOnUIThread([&]() { requestsAtTheEnd = *requestedHorizontalOffsets; });
            LOG_OUTPUT(L"%u ViewChanging event(s) for the requests at the end.", static_cast<unsigned>(requestsAtTheEnd.size()));
            for (double requested : requestsAtTheEnd)
            {
                LOG_OUTPUT(L"ViewChanging NextView.HorizontalOffset=%f", requested);
                VERIFY_IS_TRUE(AreClose(maxHorizontalOffset, requested));
            }
        }

        void ScrollContentPresenterPublicApiTests::MakeVisibleSimpleOverloadBringsElementIntoView()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml::FrameworkElement^ root = nullptr;
            auto presenter = LoadScrollViewer(true /*canScrollVertically*/, &scrollViewer, &root);
            xaml::FrameworkElement^ farTarget = nullptr;
            xaml::FrameworkElement^ nearTarget = nullptr;
            double offsetXAfterFirstCall = 0.0;
            double offsetYAfterFirstCall = 0.0;

            auto verifyFullyInViewport = [&](xaml::FrameworkElement^ target)
            {
                auto bounds = target->TransformToVisual(scrollViewer)->TransformBounds(wf::Rect(0, 0, 50, 50));
                LOG_OUTPUT(L"%s bounds in viewport {%f,%f,%f,%f}; offsets H=%f V=%f",
                    target->Name->Data(), bounds.X, bounds.Y, bounds.Width, bounds.Height,
                    presenter->HorizontalOffset, presenter->VerticalOffset);
                VERIFY_IS_TRUE(bounds.X >= -c_tolerance && bounds.X + bounds.Width <= presenter->ViewportWidth + c_tolerance);
                VERIFY_IS_TRUE(bounds.Y >= -c_tolerance && bounds.Y + bounds.Height <= presenter->ViewportHeight + c_tolerance);
            };

            ActThenWaitForLayout([&]()
            {
                farTarget = safe_cast<xaml::FrameworkElement^>(root->FindName(L"farTarget"));
                nearTarget = safe_cast<xaml::FrameworkElement^>(root->FindName(L"nearTarget"));
                VERIFY_IS_NOT_NULL(farTarget);
                VERIFY_IS_NOT_NULL(nearTarget);

                wf::Rect made = presenter->MakeVisible(farTarget, wf::Rect(0, 0, 50, 50));
                LOG_OUTPUT(L"MakeVisible returned {%f,%f,%f,%f}", made.X, made.Y, made.Width, made.Height);
                VERIFY_IS_TRUE(AreClose(50.0, made.Width));
                VERIFY_IS_TRUE(AreClose(50.0, made.Height));
            });

            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(presenter->HorizontalOffset > 0.0);
                VERIFY_IS_TRUE(presenter->VerticalOffset > 0.0);
                verifyFullyInViewport(farTarget);
                offsetXAfterFirstCall = presenter->HorizontalOffset;
                offsetYAfterFirstCall = presenter->VerticalOffset;
            });

            LOG_OUTPUT(L"Boundary: making an already fully visible element visible does not scroll.");
            ActThenWaitForLayout([&]() { presenter->MakeVisible(farTarget, wf::Rect(0, 0, 50, 50)); });
            VerifyOffsets(presenter, scrollViewer, offsetXAfterFirstCall, offsetYAfterFirstCall);

            LOG_OUTPUT(L"Scrolling back to an element near the origin brings it into view.");
            ActThenWaitForLayout([&]() { presenter->MakeVisible(nearTarget, wf::Rect(0, 0, 50, 50)); });
            RunOnUIThread([&]()
            {
                VERIFY_IS_TRUE(presenter->HorizontalOffset < offsetXAfterFirstCall);
                VERIFY_IS_TRUE(presenter->VerticalOffset < offsetYAfterFirstCall);
                verifyFullyInViewport(nearTarget);
            });
        }

        void ScrollContentPresenterPublicApiTests::SetOffsetWithNaNThrowsAndStaticPropertiesExist()
        {
            TestCleanupWrapper cleanup;
            xaml_controls::ScrollViewer^ scrollViewer = nullptr;
            xaml::FrameworkElement^ root = nullptr;
            auto presenter = LoadScrollViewer(true /*canScrollVertically*/, &scrollViewer, &root);
            const double nan = std::numeric_limits<double>::quiet_NaN();

            ActThenWaitForLayout([&]() { presenter->SetHorizontalOffset(30.0); presenter->SetVerticalOffset(40.0); });
            VerifyOffsets(presenter, scrollViewer, 30.0, 40.0);

            LOG_OUTPUT(L"NaN offsets are rejected with E_FAIL and leave the current offsets untouched.");
            ActThenWaitForLayout([&]()
            {
                VERIFY_THROWS_WINRT(presenter->SetHorizontalOffset(nan), Platform::FailureException^);
                VERIFY_THROWS_WINRT(presenter->SetVerticalOffset(nan), Platform::FailureException^);
            });
            VerifyOffsets(presenter, scrollViewer, 30.0, 40.0);

            LOG_OUTPUT(L"The static dependency property identifiers address the instance properties.");
            RunOnUIThread([&]()
            {
                auto canRenderOutsideProperty = xaml_controls::ScrollContentPresenter::CanContentRenderOutsideBoundsProperty;
                auto sizesContentProperty = xaml_controls::ScrollContentPresenter::SizesContentToTemplatedParentProperty;
                VERIFY_IS_NOT_NULL(canRenderOutsideProperty);
                VERIFY_IS_NOT_NULL(sizesContentProperty);
                VERIFY_IS_FALSE(canRenderOutsideProperty->Equals(sizesContentProperty));

                const bool originalCanRenderOutside = presenter->CanContentRenderOutsideBounds;
                const bool originalSizesContent = presenter->SizesContentToTemplatedParent;

                presenter->CanContentRenderOutsideBounds = !originalCanRenderOutside;
                VERIFY_ARE_EQUAL(!originalCanRenderOutside, safe_cast<bool>(presenter->GetValue(canRenderOutsideProperty)));
                VERIFY_ARE_EQUAL(originalSizesContent, safe_cast<bool>(presenter->GetValue(sizesContentProperty)));

                presenter->SetValue(sizesContentProperty, !originalSizesContent);
                VERIFY_ARE_EQUAL(!originalSizesContent, presenter->SizesContentToTemplatedParent);

                presenter->CanContentRenderOutsideBounds = originalCanRenderOutside;
                presenter->SizesContentToTemplatedParent = originalSizesContent;
            });
        }

    } } }
} } } }
