// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <collection.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "ImageLifetimeTests.h"
#include "MUX-ETWEvents.h"
#include "ETWWaiterProxy.h"
#include <WUCRenderingScopeGuard.h>

using namespace Platform::Collections;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;
using namespace MockDComp;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        Platform::String^ ImageLifetimeTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
        }

        bool ImageLifetimeTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool ImageLifetimeTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool ImageLifetimeTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Simple test to reset the source of an image element and
        //            ensure no extra decode occurs.
        //------------------------------------------------------------------------
        void ImageLifetimeTests::SourceRefresh()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            // Setup the Window
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // Load the base xaml
            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            // Setup events
            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            // Fire up the UI thread
            RunOnUIThread([&]()
            {
                // Attach the grid to the window
                TestServices::WindowHelper->WindowContent = rootGrid;

                // Get the image element
                auto imageElement = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(imageElement);

                // Create the BitmapImage
                auto bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);

                auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
                VERIFY_IS_NOT_NULL(testUri);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    // Make sure the event only gets fired once
                    VERIFY_IS_FALSE(bitmapImageOpenedEvent->HasFired());

                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                bitmapImage->UriSource = testUri;

                imageElement->Stretch = Stretch::Fill;

                // Set the bitmap source and make sure it fired ImageOpened
                imageElement->Source = bitmapImage;
            });

            TestServices::WindowHelper->WaitForIdle();

            // Make sure the bitmap opened event occurred
            bitmapImageOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(bitmapImageOpenedEvent->HasFired());

            // Reset the event for negative testing
            bitmapImageOpenedEvent->Reset();

            RunOnUIThread([&]()
            {
                auto imageElement = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(imageElement);

                // Set the source to null and immediately back to bitmapImage
                auto bitmapImage = imageElement->Source;
                imageElement->Source = nullptr;
                imageElement->Source = bitmapImage;
            });

            // todo: When realtime ETW is supported, this test should check to ensure that only 1 DecodeToSurface event was issued.

            TestServices::WindowHelper->WaitForIdle();

            // Verify DComp output
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);
        }

        //------------------------------------------------------------------------
        // Test case: Sets the source of an image element to null and ensure the
        //            hardware resources are released.  After it sets the source
        //            to the same element and ensures that a decode occurred.
        //------------------------------------------------------------------------
        void ImageLifetimeTests::SourceReleaseRefresh()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            // Setup the window for the test
            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            // Load the base xaml
            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            BitmapImage^ bitmapImage;

            // Setup events
            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            // Fire up the UI thread
            RunOnUIThread([&]()
            {
                // Attach the grid to the window
                TestServices::WindowHelper->WindowContent = rootGrid;

                // Get the image element
                auto imageElement = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(imageElement);

                // Create the BitmapImage
                bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);

                auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
                VERIFY_IS_NOT_NULL(testUri);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    // Make sure the event only gets fired once
                    VERIFY_IS_FALSE(bitmapImageOpenedEvent->HasFired());

                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                bitmapImage->UriSource = testUri;

                imageElement->Stretch = Stretch::Fill;

                // Set the bitmap source and make sure it fired ImageOpened
                imageElement->Source = bitmapImage;
            });

            // Make sure the bitmap opened event occurred
            bitmapImageOpenedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            // Test DComp comparison here instead of at the end of the file since there is no mechanism to force a frame update and wait for
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::AllSurfaces);

            // Reset the event for negative testing
            bitmapImageOpenedEvent->Reset();

            RunOnUIThread([&]()
            {
                auto imageElement = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));

                // Set the source to null and immediately back to bitmapImage
                LOG_OUTPUT(L"Set the source to null");
                imageElement->Source = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Use the ETW event to verify there was another image decode and update
            ETWWaiterProxy imageEtwWaiter;

            imageEtwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                ImageUpdateHardwareResourcesEnd_value);

            // Override the trim setting to trim the resources immediately and tick
            // the UI thread once so it does the trim (this occurs during the UI thread tick)
            RunOnUIThread([&]()
            {
                TestServices::Utilities->OverrideTrimImageResourceDelay(true);
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]()
            {
                auto imageElement = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));

                LOG_OUTPUT(L"Set the bitmap image back to ensure it can reload");
                imageElement->Source = bitmapImage;
            });
            TestServices::WindowHelper->WaitForIdle();
            imageEtwWaiter.WaitForDefault();

            VERIFY_IS_FALSE(bitmapImageOpenedEvent->HasFired());
        }

    } } }
} } } }
