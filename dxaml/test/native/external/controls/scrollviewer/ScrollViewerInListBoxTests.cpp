// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ScrollViewerInListBoxTests.h"
#include <XamlTailored.h>
#include <TreeHelper.h>
#include <TestEvent.h>
#include <FileLoader.h>
#include <SafeEventRegistration.h>
#include <TestCleanupWrapper.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Controls { namespace ScrollViewer {

    bool ScrollViewerInListBoxTests::ClassSetup()
    {
        CommonTestSetupHelper::CommonTestClassSetup();
        return true;
    }

    bool ScrollViewerInListBoxTests::ClassCleanup()
    {
        return true;
    }

    bool ScrollViewerInListBoxTests::TestSetup()
    {
        test_infra::TestServices::WindowHelper->InitializeXaml();
        return true;
    }

    bool ScrollViewerInListBoxTests::TestCleanup()
    {
        test_infra::TestServices::WindowHelper->ShutdownXaml();
        TestServices::WindowHelper->VerifyTestCleanup();
        return true;
    }

    void ScrollViewerInListBoxTests::ChangeZoomFactorOfVSP()
    {
        ScrollViewerInListBoxTests::ChangeZoomFactorOfVSP(false /*usePen*/);
    }

    void ScrollViewerInListBoxTests::ChangeZoomFactorOfVSPWithPen()
    {
        ScrollViewerInListBoxTests::ChangeZoomFactorOfVSP(true /*usePen*/);
    }

    // Validates that ZoomToFactor properly changes the view for a ScrollViewer hosting a VirtualizingStackPanel.
    void ScrollViewerInListBoxTests::ChangeZoomFactorOfVSP(bool usePen)
    {
        WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

        ::Windows::Foundation::Size size(300, 300);
        TestServices::WindowHelper->SetWindowSizeOverride(size);

        auto loadedEvent = std::make_shared<Event>();
        auto loadedRegistration = CreateSafeEventRegistration(xaml_controls::ListBox, Loaded);

        auto viewChangedEvent = std::make_shared<Event>();
        auto viewChangedRegistration = CreateSafeEventRegistration(xaml_controls::ScrollViewer, ViewChanged);

        auto selectionChangedEvent = std::make_shared<Event>();
        auto selectionChangedRegistration = CreateSafeEventRegistration(xaml_controls::ListBox, SelectionChanged);

        Platform::String^ xamlFile = GetPackageFolder() + L"resources\\native\\controls\\ScrollViewer\\ScrollViewerInListBox.xaml";

        xaml_controls::ScrollViewer^ scrollViewerInListBox = nullptr;
        xaml_controls::ListBox^ listBox = nullptr;
        xaml_controls::Grid^ rootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(xamlFile));
        VERIFY_IS_NOT_NULL(rootGrid);

        RunOnUIThread([&]()
        {
            TestServices::WindowHelper->WindowContent = rootGrid;

            listBox = safe_cast<xaml_controls::ListBox^>(rootGrid->FindName(L"listBox"));
            VERIFY_IS_NOT_NULL(listBox);

            loadedRegistration.Attach(
                listBox,
                ref new RoutedEventHandler([loadedEvent](Platform::Object^ sender, RoutedEventArgs^)
            {
                loadedEvent->Set();
            }));

            selectionChangedRegistration.Attach(
                listBox,
                ref new xaml_controls::SelectionChangedEventHandler([selectionChangedEvent](Platform::Object^, xaml_controls::SelectionChangedEventArgs^)
            {
                LOG_OUTPUT(L"ListBox SelectionChanged event handler.");
                selectionChangedEvent->Set();
            }));
        });

        loadedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            scrollViewerInListBox = safe_cast<xaml_controls::ScrollViewer^>(TreeHelper::GetVisualChildByName(listBox, L"ScrollViewer"));
            VERIFY_IS_NOT_NULL(scrollViewerInListBox);

            viewChangedRegistration.Attach(scrollViewerInListBox, ref new wf::EventHandler<xaml_controls::ScrollViewerViewChangedEventArgs^>(
                [viewChangedEvent](Platform::Object^ sender, xaml_controls::ScrollViewerViewChangedEventArgs^ args)
            {
                LOG_OUTPUT(L"ViewChanged raised. IsIntermediate = %d", args->IsIntermediate);

                if (!args->IsIntermediate)
                {
                    viewChangedEvent->Set();
                }
            }));

            LOG_OUTPUT(L"Changing vertical offset with ScrollToVerticalOffset");
            scrollViewerInListBox->ScrollToVerticalOffset(6.0);
        });

        LOG_OUTPUT(L"Waiting for ScrollViewer's view to change...");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final view of ScrollViewer is (x, y, z) = (%f, %f, %f).", scrollViewerInListBox->HorizontalOffset, scrollViewerInListBox->VerticalOffset, scrollViewerInListBox->ZoomFactor);
            VERIFY_ARE_EQUAL(scrollViewerInListBox->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewerInListBox->VerticalOffset, 6.0);
            VERIFY_ARE_EQUAL(scrollViewerInListBox->ZoomFactor, 1.0);

            viewChangedEvent->Reset();
            LOG_OUTPUT(L"Changing zoom factor with ZoomToFactor");
            scrollViewerInListBox->ZoomToFactor(1.1f);
        });

        LOG_OUTPUT(L"Waiting for ScrollViewer's view to change...");
        viewChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Final view of ScrollViewer is (x, y, z) = (%f, %f, %f).", scrollViewerInListBox->HorizontalOffset, scrollViewerInListBox->VerticalOffset, scrollViewerInListBox->ZoomFactor);
            VERIFY_ARE_EQUAL(scrollViewerInListBox->HorizontalOffset, 0.0);
            VERIFY_ARE_EQUAL(scrollViewerInListBox->VerticalOffset, 6.0);
            VERIFY_IS_TRUE(scrollViewerInListBox->ZoomFactor > 1.0999);
            VERIFY_IS_TRUE(scrollViewerInListBox->ZoomFactor < 1.1001);
        });

        LOG_OUTPUT(L"Select item.");
        if (usePen)
        {
            TestServices::InputHelper->PenTap(scrollViewerInListBox);
        }
        else
        {
            TestServices::InputHelper->Tap(scrollViewerInListBox);
        }
        LOG_OUTPUT(L"Waiting for selection change.");
        selectionChangedEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"Press item.");
        for (INT dx = 0; dx < 10; dx++)
        {
            if (usePen)
            {
                TestServices::InputHelper->DynamicPenPressCenter(scrollViewerInListBox, dx, 0);
            }
            else
            {
                TestServices::InputHelper->DynamicPressCenter(scrollViewerInListBox, dx, 0, PointerFinger::Finger1);
            }
            TestServices::WindowHelper->WaitForIdle();
            Sleep(100);
        }

        LOG_OUTPUT(L"Recording DComp tree during press.");
        TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

        LOG_OUTPUT(L"Release item.");
        if (usePen)
        {
            TestServices::InputHelper->DynamicPenRelease();
        }
        else
        {
            TestServices::InputHelper->DynamicRelease(PointerFinger::Finger1);
        }
    }
} } } } } } // Microsoft::UI::Xaml::Tests::Controls::ScrollViewer
