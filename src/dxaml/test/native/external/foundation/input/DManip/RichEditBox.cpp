// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RichEditBox.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Text;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool RichEditBoxTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool RichEditBoxTest::ClassCleanup()
        {
            return true;
        }

        bool RichEditBoxTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ RichEditBoxTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ScrollViewer^ RichEditBoxTest::SetupUI(
            _In_ Platform::String^ filename,
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            RichEditBox^ richEditBox = nullptr;
            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                richEditBox = safe_cast<RichEditBox^>(root->FindName(L"myRichEditBox"));
                richEditBox->Document->SetText(TextSetOptions::None, L"This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text This is the richedit text");
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            ScrollViewer^ sv = nullptr;
            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

            RunOnUIThread([&]()
            {
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(richEditBox, 0));
                sv = safe_cast<ScrollViewer^>(controlTemplateRoot->FindName(L"ContentElement"));
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->HorizontalScrollMode = ScrollMode::Disabled;

                viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                    [viewChangedEvent](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                {
                    if (!args->IsIntermediate)
                    {
                        viewChangedEvent->Set();
                    }
                }));

                viewChangingRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangingEventArgs^>(
                    [](Platform::Object^ sender, ScrollViewerViewChangingEventArgs^ args)
                {
                    LOG_OUTPUT(L"ViewChanging, NextView: %f, %f, %f, FinalView: %f, %f, %f, IsInertial: %d",
                        args->NextView->HorizontalOffset,
                        args->NextView->VerticalOffset,
                        args->NextView->ZoomFactor,
                        args->FinalView->HorizontalOffset,
                        args->FinalView->VerticalOffset,
                        args->FinalView->ZoomFactor,
                        args->IsInertial);
                }));
            });

            return sv;
        }

        void RichEditBoxTest::Basics()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // Scenario 1:  Load the XAML markup and verify the DComp visual tree.
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"RichEditBox.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Scenario 2:  Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as after scenario 1
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

        void RichEditBoxTest::BasicsRTL()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // Scenario 1:  Load the XAML markup and verify the DComp visual tree.
            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"RichEditBoxRTL.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            RichEditBox^ richEditBox = nullptr;

            // First popuplate the RichEditBox with English text and scroll to the end.
            // In this case since the text is RTL, we'll end up with a positive scroll offset.
            // Note carefully that the scroll offset is in an RTL coordinate space.
            RunOnUIThread([&]()
            {
                auto root = safe_cast<FrameworkElement^>(TestServices::WindowHelper->WindowContent);
                richEditBox = safe_cast<RichEditBox^>(root->FindName(L"myRichEditBox"));
                richEditBox->Document->SetText(TextSetOptions::None, L"abc def ghi jkl mno pqr stu vwx yz");
                richEditBox->Document->GetRange(24, 25)->ScrollIntoView(PointOptions::Start);
            });
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as before device lost
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"1");

            // Now populate the RichEditBox with Hebrew text and scroll it.
            // In this case since the text is RTL we'll end up with a negative scroll offset.
            RunOnUIThread([&]()
            {
                richEditBox->Document->SetText(TextSetOptions::None, L"שנב גקכ עין חלך צמם פ/ר דאו ה'ס טז");
                richEditBox->Document->GetRange(24, 25)->ScrollIntoView(PointOptions::Start);
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();

            // The visual tree should look exactly the same as before device lost
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison, L"2");

            RunOnUIThread([&]()
            {
                // Test out transforming (0,0) up to global.  This should flip to the right side of the RichEditBox.
                auto transformToRoot = richEditBox->TransformToVisual(nullptr);
                auto pos = transformToRoot->TransformPoint(wf::Point(0, 0));
                VERIFY_IS_TRUE(pos.X == 300.0f);
            });

        }

        void RichEditBoxTest::Pan()
        {
            WUCRenderingScopeGuardWithDManipHitTestVisual guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            std::shared_ptr<Event> viewChangedEvent = std::make_shared<Event>();
            auto viewChangingRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanging);
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"RichEditBox.xaml";
            ScrollViewer^ sv = SetupUI(filename, viewChangedEvent, viewChangingRegistration, viewChangedRegistration);

            TestServices::WindowHelper->WaitForIdle();
            LOG_OUTPUT(L"Launching horizontal pan operation.");
            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -150 /*relY*/, 1.0 /*velocityFactor*/);
            viewChangedEvent->WaitForDefault();

            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);

            // Simulate a device lost.  This will tear down and rebuild the DComp visual tree.
            // The visual tree should look exactly the same as before device lost.
            LOG_OUTPUT(L"Simulating device lost");
            TestServices::WindowHelper->SimulateDeviceLost();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
        }

    } } }
} } } }
