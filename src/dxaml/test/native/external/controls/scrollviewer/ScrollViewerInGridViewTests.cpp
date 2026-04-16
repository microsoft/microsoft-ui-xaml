// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollViewerInGridViewTests.h"
#include <XamlTailored.h>
#include <TreeHelper.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    bool ScrollViewerInGridViewTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ScrollViewerInGridViewTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ScrollViewerInGridViewTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ScrollViewerInGridViewTests::AttemptHorizontalOffsetChange(bool isHorizontalScrollBarEnabled, bool useChangeView, bool usePen)
    {
        TestCleanupWrapper cleanup;

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::GridView, Loaded);

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\ScrollViewer\\ScrollViewerInGridView.xaml";

        xaml_controls::ScrollViewer^ scrollViewerInGridView = nullptr;
        xaml_controls::GridView^ gridView = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(xamlFile));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootGrid;

            gridView = safe_cast<xaml_controls::GridView^>(rootGrid->FindName(L"gridView"));
            VERIFY_IS_NOT_NULL(gridView);

            loadedRegistration.Attach(
                gridView,
                ref new RoutedEventHandler([loadedEvent](Platform::Object^ sender, RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));
        });

        loadedEvent->WaitForDefault();

        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewerInGridView = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(gridView, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewerInGridView);

            scrollViewerInGridView->HorizontalScrollBarVisibility = isHorizontalScrollBarEnabled ? xaml_controls::ScrollBarVisibility::Auto : xaml_controls::ScrollBarVisibility::Disabled;

            viewChangedRegistration.Attach(scrollViewerInGridView, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. IsIntermediate = %d", args->IsIntermediate);

                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            if (useChangeView)
            {
                LOG_OUTPUT(L"Attempting to change horizontal offset with ChangeView");
                scrollViewerInGridView->ChangeView(3.0 /*horizontalOffset*/, nullptr /*verticalOffset*/, nullptr /*zoomFactor*/, true /*disableAnimation*/);
            }
            else if (!usePen)
            {
                LOG_OUTPUT(L"Attempting to change horizontal offset with ScrollToHorizontalOffset");
                scrollViewerInGridView->ScrollToHorizontalOffset(3.0);
            }
        });

        if (usePen)
        {
            LOG_OUTPUT(L"Attempting to change horizontal offset with pen input");
            TestServices::InputHelper->PenStrokeFromCenter(scrollViewerInGridView, -50 /*relX*/, 0 /*relY*/, 1.0 /*velocityFactor*/);
        }

        // ScrollViewer.ChangeView does not change the horizontal offset when ScrollViewer.HorizontalScrollBarVisibility==ScrollBarVisibility::Disabled
        if (isHorizontalScrollBarEnabled || !useChangeView)
        {
            LOG_OUTPUT(L"Waiting for ScrollViewer's view to change...");
            viewChangedEvent->WaitForDefault();
        }

        TestServices::WindowHelper->WaitForIdle();
        LOG_OUTPUT(L"Idling again.");

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final view of ScrollViewer is (x, y, z) = (%f, %f, %f).", scrollViewerInGridView->HorizontalOffset, scrollViewerInGridView->VerticalOffset, scrollViewerInGridView->ZoomFactor);
            if (usePen)
            {
                //The Pen Scroll ammount is not consistent so we just check to make sure it has moved
                VERIFY_IS_GREATER_THAN(scrollViewerInGridView->HorizontalOffset, 1.0);
            }
            else if (isHorizontalScrollBarEnabled || !useChangeView)
            {
                VERIFY_ARE_EQUAL(scrollViewerInGridView->HorizontalOffset, 3.0);
            }
            else
            {
                VERIFY_ARE_EQUAL(scrollViewerInGridView->HorizontalOffset, 2.0);
            }
            VERIFY_ARE_EQUAL(scrollViewerInGridView->VerticalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewerInGridView->ZoomFactor, 1.0);
        });
    }

    //
    // Test Cases
    //
    void ScrollViewerInGridViewTests::CanScrollToHorizontalOffsetWithScrollBarEnabled()
    {
        AttemptHorizontalOffsetChange(true /*isHorizontalScrollBarEnabled*/, false /*useChangeView*/, false /*usePen*/);
    }

    void ScrollViewerInGridViewTests::CanScrollToHorizontalOffsetWithScrollBarDisabled()
    {
        AttemptHorizontalOffsetChange(false /*isHorizontalScrollBarEnabled*/, false /*useChangeView*/, false /*usePen*/);
    }

    void ScrollViewerInGridViewTests::CanScrollWithPenToHorizontalOffsetWithScrollBarEnabled()
    {
        AttemptHorizontalOffsetChange(true /*isHorizontalScrollBarEnabled*/, false /*useChangeView*/, true /*usePen*/);
    }

    void ScrollViewerInGridViewTests::CanScrollWithPenToHorizontalOffsetWithScrollBarDisabled()
    {
        AttemptHorizontalOffsetChange(false /*isHorizontalScrollBarEnabled*/, false /*useChangeView*/, true /*usePen*/);
    }

    void ScrollViewerInGridViewTests::CanChangeViewWithScrollBarEnabled()
    {
        AttemptHorizontalOffsetChange(true /*isHorizontalScrollBarEnabled*/, true /*useChangeView*/, false /*usePen*/);
    }

    void ScrollViewerInGridViewTests::CannotChangeViewWithScrollBarDisabled()
    {
        AttemptHorizontalOffsetChange(false /*isHorizontalScrollBarEnabled*/, true /*useChangeView*/, false /*usePen*/);
    }

} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ScrollViewer
