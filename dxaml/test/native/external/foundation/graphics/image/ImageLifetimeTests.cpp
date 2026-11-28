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

                auto testUri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
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

                auto testUri = ref new Uri(GetResourcesPath() + L"rainier_2048x1536.png");
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

        //------------------------------------------------------------------------
        // Test case: an Image that was asked for its casting source hands each new software image to its image sharing
        //            engine without failing, and keeps returning the same casting source.
        //------------------------------------------------------------------------
        void ImageLifetimeTests::CastingSourceFollowsWriteableBitmapSources()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(::Windows::Foundation::Size(400, 300));

            auto isSameObject = [](Platform::Object^ first, Platform::Object^ second)
            {
                Microsoft::WRL::ComPtr<IUnknown> firstIdentity;
                Microsoft::WRL::ComPtr<IUnknown> secondIdentity;
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(first)->QueryInterface(IID_PPV_ARGS(&firstIdentity)));
                VERIFY_SUCCEEDED(reinterpret_cast<IInspectable*>(second)->QueryInterface(IID_PPV_ARGS(&secondIdentity)));
                return firstIdentity.Get() == secondIdentity.Get();
            };

            Microsoft::UI::Xaml::Controls::Image^ image = nullptr;
            ::Windows::Media::Casting::CastingSource^ castingSource = nullptr;
            RunOnUIThread([&]()
            {
                auto root = ref new Grid();
                image = ref new Microsoft::UI::Xaml::Controls::Image();
                image->Width = 64;
                image->Height = 64;
                root->Children->Append(image);
                TestServices::WindowHelper->WindowContent = root;

                // Creates the image sharing engine of the element.
                castingSource = image->GetAsCastingSource();
                VERIFY_IS_NOT_NULL(castingSource);
            });
            TestServices::WindowHelper->WaitForIdle();

            // A WriteableBitmap keeps its pixels in a software surface from the start, so setting it as the source passes
            // the pixels to the image sharing engine right away. The engine is internal: the test checks that the hand-off
            // keeps the element's casting source and source, and survives Invalidate + a UI tick without failing.
            for (auto size : { std::make_pair(32, 16), std::make_pair(8, 8) })
            {
                LOG_OUTPUT(L"Source: %dx%d WriteableBitmap", size.first, size.second);
                WriteableBitmap^ bitmap = nullptr;
                RunOnUIThread([&]()
                {
                    bitmap = ref new WriteableBitmap(size.first, size.second);
                    image->Source = bitmap;
                    VERIFY_IS_TRUE(isSameObject(castingSource, image->GetAsCastingSource()), L"The casting source is kept when the source changes");
                    bitmap->Invalidate();
                });
                TestServices::WindowHelper->WaitForIdle();
                RunOnUIThread([&]()
                {
                    VERIFY_IS_TRUE(isSameObject(bitmap, image->Source), L"The image keeps the WriteableBitmap source");
                    VERIFY_ARE_EQUAL(size.first, bitmap->PixelWidth);
                    VERIFY_ARE_EQUAL(size.second, bitmap->PixelHeight);
                });
            }

            RunOnUIThread([&]()
            {
                image->Source = nullptr;
                VERIFY_IS_TRUE(isSameObject(castingSource, image->GetAsCastingSource()), L"The casting source is kept without a source");
                image = nullptr;
                castingSource = nullptr;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

    } } }
} } } }
