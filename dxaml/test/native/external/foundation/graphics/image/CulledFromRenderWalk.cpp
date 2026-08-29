// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <collection.h>
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <robuffer.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "CulledFromRenderWalk.h"
#include "MUX-ETWEvents.h"
#include "ETWWaiterProxy.h"
#include <WUCRenderingScopeGuard.h>

using namespace Concurrency;
using namespace ::Windows::Foundation;
using namespace ::Windows::Graphics::Imaging;
using namespace Platform;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;
using namespace ::Windows::Media::Casting;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace RuntimeFeatureBehavior;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        Platform::String^ CulledFromRenderWalk::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
        }

        bool CulledFromRenderWalk::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            // TODO:  Background Thread Image Loading is causing WaitForIdle() stability issues, so disabling it for these tests.
            // Remove this workaround after addressing this issue, tracked by:
            // WaitForIdle() reliability issue: Background Thread Image Loading is not tracked by WaitForIdle
            m_backgroundThreadImageLoadingFeature.Initialize(RuntimeEnabledFeature::BackgroundThreadImageLoading, false);
            return true;
        }

        bool CulledFromRenderWalk::TestCleanup()
        {
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Test case: Creates an Image element that is culled from the RenderWalk and verifies it still uses DecodeToRenderSize feature
        void CulledFromRenderWalk::Basics()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk1.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"myImage"));
                VERIFY_IS_NOT_NULL(testImage);

                auto bitmapImage = safe_cast<BitmapImage^>(testImage->Source);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        // Test case: Creates a Canvas element with background image that is culled from the RenderWalk and verifies it still uses DecodeToRenderSize feature
        void CulledFromRenderWalk::Basics2()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk2.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                auto brush = safe_cast<ImageBrush^>(myCanvas->Background);
                auto bitmapImage = safe_cast<BitmapImage^>(brush->ImageSource);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        void CulledFromRenderWalk::Basics3WUCFull()
        {
            Basics3(DCompRendering::WUCCompleteSynchronousCompTree);
        }

        void CulledFromRenderWalk::Basics3(DCompRendering dcompRendering)
        {
            WUCRenderingScopeGuard guard(dcompRendering, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk3.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"myImage"));
                VERIFY_IS_NOT_NULL(testImage);

                auto bitmapImage = safe_cast<BitmapImage^>(testImage->Source);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        void CulledFromRenderWalk::Basics4()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk4.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testEllipse = safe_cast<Microsoft::UI::Xaml::Shapes::Ellipse^>(rootGrid->FindName(L"myEllipse"));
                VERIFY_IS_NOT_NULL(testEllipse);

                auto imageBrush = safe_cast<ImageBrush^>(testEllipse->Fill);
                auto bitmapImage = safe_cast<BitmapImage^>(imageBrush->ImageSource);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        void CulledFromRenderWalk::Advanced1()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk5.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage1 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"myImage1"));
                auto testImage2 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"myImage2"));
                VERIFY_IS_NOT_NULL(testImage1);
                VERIFY_IS_NOT_NULL(testImage2);

                auto bitmapImage = safe_cast<BitmapImage^>(testImage1->Source);
                testImage2->Source = bitmapImage;

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        void CulledFromRenderWalk::Advanced2()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"CulledFromRenderWalk6.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"myImage"));
                VERIFY_IS_NOT_NULL(testImage);

                auto bitmapImage = safe_cast<BitmapImage^>(testImage->Source);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // At this point, since there's a Clip preventing the Image from being rendered,
            // we should not see any surface or primitive for this image yet.
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"1");

            RunOnUIThread([&]()
            {
                auto myCanvas = safe_cast<Microsoft::UI::Xaml::Controls::Canvas^>(rootGrid->FindName(L"myCanvas"));
                VERIFY_IS_NOT_NULL(myCanvas);

                // Now remove the clip from the Canvas and wait for another tick.
                // This time we'll produce a DComp surface which we can verify is the expected size.
                myCanvas->Clip = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }

        // Test case:  In this edge case, with the following setup:
        // -Two Image elements share the same BitmapImage
        // -One of the Images is not in the live tree at all
        // -The other Image is in the live tree but is culled
        // Validate the ImageDecodeBoundsFinder code properly skips over the Image that's not in the live tree.
        void CulledFromRenderWalk::EdgeCase1()
        {
            xaml_controls::Image^ image1 = nullptr;
            xaml_controls::Image^ image2 = nullptr;

            TestCleanupWrapper cleanup([&]()
            {
                RunOnUIThread([&]()
                {
                    image1 = nullptr;
                    image2 = nullptr;
                });
                TestServices::WindowHelper->WaitForIdle();
            });

            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto imageOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto imageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Creating BitmapImage");
                BitmapImage^ bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);
                auto uri = ref new Uri(GetResourcesPath() + L"Smiley.bmp");
                VERIFY_IS_NOT_NULL(uri);
                bitmapImage->UriSource = uri;

                imageOpenedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([&](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    imageOpenedEvent->Set();
                }));

                LOG_OUTPUT(L"Creating image1");
                image1 = ref new xaml_controls::Image();
                VERIFY_IS_NOT_NULL(image1);
                image1->Width = 200;
                image1->Height = 200;
                image1->Opacity = 0;
                image1->Source = bitmapImage;
                TestServices::WindowHelper->WindowContent = image1;

                LOG_OUTPUT(L"Creating image2");
                image2 = ref new xaml_controls::Image();
                VERIFY_IS_NOT_NULL(image2);
                image2->Source = bitmapImage;
            });

            LOG_OUTPUT(L"Invoking WaitForIdle, expect no crash");
            TestServices::WindowHelper->WaitForIdle();

            imageOpenedEvent->WaitForDefault();

            // For good measure, go ahead and set the Opacity to 1 so we actually render image1,
            // and validate that we generated a primitive for it.
            RunOnUIThread([&]()
            {
                LOG_OUTPUT(L"Setting Opacity to 1");
                image1->Opacity = 1;
            });

            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::NoComparison);
        }

    } } }
} } } }
