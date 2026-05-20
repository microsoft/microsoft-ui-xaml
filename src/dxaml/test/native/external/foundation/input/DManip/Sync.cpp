// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Copyright (c) Microsoft Corporation.  All rights reserved.

#include "pch.h"
#include "Sync.h"
#include "RuntimeEnabledFeaturesEnum.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <FileLoader.h>
#include <WUCRenderingScopeGuard.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace MockDComp;
using namespace test_infra;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Input { namespace DManip {

        bool SyncTest::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool SyncTest::ClassCleanup()
        {
            return true;
        }

        bool SyncTest::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        Platform::String^ SyncTest::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\input\\dmanip\\";
        }

        ScrollViewer^ SyncTest::SetupUI(
            _In_ Platform::String^ filename,
            _In_ std::shared_ptr<Event>& viewChangedEvent,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanging)& viewChangingRegistration,
            _In_ SafeEventRegistrationType(ScrollViewer, ViewChanged)& viewChangedRegistration)
        {
            TextBox^ textBox = nullptr;
            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                textBox = safe_cast<TextBox^>(root->FindName(L"myTextBox"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            ScrollViewer^ sv = nullptr;
            std::shared_ptr<Event> spHasLoadedEvent = std::make_shared<Event>();
            auto loadedRegistration = CreateSafeEventRegistration(ScrollViewer, Loaded);

            RunOnUIThread([&]()
            {
                auto controlTemplateRoot = safe_cast<FrameworkElement^>(VisualTreeHelper::GetChild(textBox, 0));
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

        void SyncTest::SyncRectangle()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->SetPostTickCallback(nullptr);
            });

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            int viewChangeCount = 0;
            int tickCount = 0;
            std::shared_ptr<Event> stopTestEvent = std::make_shared<Event>();
            auto viewChangedRegistration = CreateSafeEventRegistration(ScrollViewer, ViewChanged);
            Platform::String^ filename = GetResourcesPath() + L"SyncRectangle.xaml";

            ScrollViewer^ sv = nullptr;
            xaml_shapes::Rectangle^ rectangle = nullptr;

            auto root = safe_cast<FrameworkElement^>(LoadXamlFileOnUIThread(filename));

            RunOnUIThread([&]()
            {
                sv = safe_cast<ScrollViewer^>(root->FindName(L"myScrollViewer"));
                rectangle = safe_cast<xaml_shapes::Rectangle^>(root->FindName(L"myRectangle"));
                TestServices::WindowHelper->WindowContent = root;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                sv->IsScrollInertiaEnabled = false;
                sv->IsZoomInertiaEnabled = false;
                sv->HorizontalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->VerticalScrollBarVisibility = ScrollBarVisibility::Hidden;
                sv->HorizontalScrollMode = ScrollMode::Disabled;

                viewChangedRegistration.Attach(sv, ref new wf::EventHandler<ScrollViewerViewChangedEventArgs^>(
                    [&](Platform::Object^, ScrollViewerViewChangedEventArgs^ args)
                {
                    if (!args->IsIntermediate && viewChangeCount == 0)
                    {
                        viewChangeCount++;
                        TestServices::WindowHelper->SetPostTickCallback(ref new PostTickCallback([&]()
                        {
                            tickCount++;
                            if (tickCount < 3)
                            {
                                TestServices::Utilities->VerifyMockDCompOutput(SurfaceComparison::NoComparison);
                                if (tickCount == 2)
                                {
                                    stopTestEvent->Set();
                                }
                            }
                        }));
                        sv->ChangeView(nullptr, 2500.0, nullptr, true);
                    }
                }));
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::InputHelper->PanFromCenter(sv, 0 /*relX*/, -100 /*relY*/, 0.66 /*velocityFactor*/);
            stopTestEvent->WaitForDefault();
        }

    } } }
} } } }
