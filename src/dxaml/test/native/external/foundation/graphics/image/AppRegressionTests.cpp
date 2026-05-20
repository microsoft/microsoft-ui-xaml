// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ImageEventWaitingContext.h"
#include <MUX-ETWEvents.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "ImageTestEngine.h"
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "AppRegressionTests.h"
#include "ETWWaiterProxy.h"
#include <WUCRenderingScopeGuard.h>

using namespace concurrency;
using namespace ::Windows::Storage;
using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        Platform::String^ AppRegressionTests::GetResourcesPath() const
        {
            return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
        }

        bool AppRegressionTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool AppRegressionTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool AppRegressionTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        // Renders two hub elements with background images.
        // Expected results is that the application should not crash.
        // This bug is specific to the Phone code path for Hub but the test runs on Any platform
        void AppRegressionTests::TFS_895642()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pRootPanel = safe_cast<xaml_controls::Panel^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TFS_895642.xaml"));
            VERIFY_IS_NOT_NULL(pRootPanel);

            auto loadedRegistration = CreateSafeEventRegistration(Panel, Loaded);
            auto pStackPanelLoadedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                loadedRegistration.Attach(
                    pRootPanel,
                    ref new xaml::RoutedEventHandler([pStackPanelLoadedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"StackPanel Loaded event fired");
                    pStackPanelLoadedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = pRootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Waiting for StackPanel Loaded event");
            pStackPanelLoadedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = nullptr;
            });
            pRootPanel = nullptr;
            TestServices::WindowHelper->WaitForIdle();
        }

        // Covers very specific case when all of the following conditions are met:
        //  1. The same image URI is used in two or more elements.
        //  2. One of the elements requires decoding to software surface. A border with round corners is an example.
        //  3. Another element which would normally use hardware surface only is hidden.
        void AppRegressionTests::TFS_2144093()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);
            ::Windows::Foundation::Size size(300, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto pRootGrid = safe_cast<xaml_controls::Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TFS_2144093.xaml"));
            VERIFY_IS_NOT_NULL(pRootGrid);

            auto imageOpenedRegistration = CreateSafeEventRegistration(ImageBrush, ImageOpened);
            auto imageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                auto imageBrush = safe_cast<ImageBrush^>(pRootGrid->FindName(L"TheBrush"));
                VERIFY_IS_NOT_NULL(imageBrush);

                imageOpenedRegistration.Attach(
                    imageBrush,
                    ref new xaml::RoutedEventHandler([imageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"ImageOpened Event Fired");
                    imageOpenedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = pRootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            LOG_OUTPUT(L"Waiting for ImageOpened event");
            imageOpenedEvent->WaitForDefault();

            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        // Covers this specific scenario:
        //  1. Image is opened with background thread image loading and the image is cached
        //  2. Image changes size and must be rasterized with software (such as an ellipse).
        // For this bug, the app would crash.  This test verifies that it does not.
        void AppRegressionTests::TFS_2529029()
        {
            TestCleanupWrapper cleanup([]()
            {
                TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
            });

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TFS_2529029.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ bitmapImage = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(testImage);

                bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);

                testImage->Source = bitmapImage;

                auto testUri = ref new Uri(GetResourcesPath() + L"windows.png");
                VERIFY_IS_NOT_NULL(testUri);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                bitmapImage->UriSource = testUri;
            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            bitmapImageOpenedEvent->Reset();

            RunOnUIThread([&]()
            {
                auto ellipseBrush = safe_cast<xaml_media::ImageBrush^>(rootGrid->FindName(L"ellipseBrush"));
                VERIFY_IS_NOT_NULL(ellipseBrush);

                ellipseBrush->ImageSource = bitmapImage;
            });
            TestServices::WindowHelper->WaitForIdle();
        }

        // This scenario repros in the following way:
        //  1. Image A loads an image with a URI at a given size.
        //  2. Image B loads an image with the same URI and the same size (resulting in the same cache lookup).
        //  3. Image A is removed from the tree right away (so that decode was started while it was in the tree, but will not be updated because
        //     it is outside the tree).
        //  4. Image B is in the tree and should render normally despite referencing the hardware cache of A.
        void AppRegressionTests::TFS_2187595()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TwoImages.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration1 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImage1OpenedEvent = std::make_shared<Event>();
            auto bitmapImage2OpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));
                VERIFY_IS_NOT_NULL(testImage1);

                auto testImage2 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement2"));
                VERIFY_IS_NOT_NULL(testImage2);

                testImage1->Stretch = Stretch::Fill;
                testImage2->Stretch = Stretch::Fill;

                auto bitmapImage1 = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage1);

                auto bitmapImage2 = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage2);

                testImage1->Source = bitmapImage1;
                testImage2->Source = bitmapImage2;

                openedRegistration1.Attach(
                    bitmapImage1,
                    ref new xaml::RoutedEventHandler([bitmapImage1OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage1 Opened Event Fired");
                    bitmapImage1OpenedEvent->Set();
                }));

                openedRegistration2.Attach(
                    bitmapImage2,
                    ref new xaml::RoutedEventHandler([bitmapImage2OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage2 Opened Event Fired");
                    bitmapImage2OpenedEvent->Set();
                }));
            });
            TestServices::WindowHelper->WaitForIdle();

            // Must let the UI thread make everything active in the tree so the URI source will be set immediately

            RunOnUIThread([&]()
            {
                auto testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));
                auto testImage2 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement2"));

                auto bitmapImage1 = safe_cast<BitmapImage^>(testImage1->Source);
                auto bitmapImage2 = safe_cast<BitmapImage^>(testImage2->Source);

                auto testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
                VERIFY_IS_NOT_NULL(testUri);

                bitmapImage1->UriSource = testUri;
                bitmapImage2->UriSource = testUri;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Let DecodeToRenderSize take effect and initiate the downloads

            RunOnUIThread([&]()
            {
                auto testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));

                testImage1->Visibility = Visibility::Collapsed;
            });
            bitmapImage1OpenedEvent->WaitForDefault();
            bitmapImage2OpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        // This scenario happens under these conditions:
        // 1) The app has specified DecodePixelWidth/Height and DecodePixelType = Logical
        // 2) The plateau scale is changed
        // The test ensures that when this sequence happens with these properties set, we do not
        // generate a new hardware surface with the image scaled to the new plateau scale factor,
        // instead we should be simply reusing the existing surface.
        void AppRegressionTests::TFS_2524409()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TFS_2524409.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ bitmapImage = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto ellipseBrush = safe_cast<xaml_media::ImageBrush^>(rootGrid->FindName(L"ellipseBrush"));
                VERIFY_IS_NOT_NULL(ellipseBrush);

                bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);
                bitmapImage->DecodePixelWidth = 100;
                bitmapImage->DecodePixelHeight = 100;
                bitmapImage->DecodePixelType = DecodePixelType::Logical;

                create_task(StorageFile::GetFileFromPathAsync(GetResourcesPath() + L"TestPattern.png"))
                    .then([=](StorageFile^ pFile)
                {
                    VERIFY_IS_NOT_NULL(pFile);

                    create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                        .then([=](IRandomAccessStream^ pFileStream)
                    {
                        VERIFY_IS_NOT_NULL(pFileStream);

                        RunOnUIThread([=]()
                        {
                            auto pAsyncAction = bitmapImage->SetSourceAsync(pFileStream);
                            VERIFY_IS_NOT_NULL(pAsyncAction);
                        });
                    });
                });

                ellipseBrush->ImageSource = bitmapImage;

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

            bitmapImageOpenedEvent->Reset();

            TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

            // Now change the plateau scale factor to 1.5
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 1.5f);
            TestServices::WindowHelper->WaitForIdle();
            TestServices::WindowHelper->SynchronouslyTickUIThread(1);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"2");
        }


        // Covers this specific scenario:
        // 1. Decode a large image using background thread image loading to full size.
        // 2. Set DecodePixelWidth/DecodePixelHeight to force a specific image size.
        // 3. Decode a smaller image to the same BitmapImage object as soon as the image from #1 has started decoding.
        // 4. Image from #3 should start and finish decoding before the image from #1 completes.
        // 5. Image will then crash while copying to a hardware surface.
        // This requires the introduction of a race condition with a very large image first image and very small second image.
        void AppRegressionTests::TFS_1715872()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ bitmapImage = nullptr;

            ETWWaiterProxy decodeEtwWaiter;

            decodeEtwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                DecodeToSurfaceBegin_value);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(testImage);

                // Use NineGrid to force DecodeToRenderSize off
                testImage->NineGrid = xaml::Thickness({ 1.0, 1.0, 1.0, 1.0 });

                bitmapImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage);

                testImage->Source = bitmapImage;

                SetSourceAsyncFromFile(bitmapImage, GetResourcesPath() + L"CN-EOS5DMarkII-ORI-BottomUp.jpg");
            });
            TestServices::WindowHelper->WaitForIdle();

            // This waits for the decode ETW event to fire from a background thread letting the test
            // know that the decode has started (helps with generating the race condition).
            decodeEtwWaiter.WaitForDefault();

            RunOnUIThread([&]()
            {
                bitmapImage->DecodePixelWidth = 4;
                bitmapImage->DecodePixelHeight = 4;

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                SetSourceAsyncFromFile(bitmapImage, GetResourcesPath() + L"VerySmall.bmp");
            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
        }

        // Covers this specific scenario:
        // 1. Use an image that has implicit rotation information in the EXIF information
        // 2. Set DecodePixelWidth or DecodePixelHeight but not both.
        // 3. Layout size must be conditional to the decoded size of the image.
        // This causes the image size to be rotated twice without rotating the image twice causing
        // a landscape size for a portrait rotated image making people appear wider than normal.
        void AppRegressionTests::TFS_6697031()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree, false /*resizeWindow*/);

            ::Windows::Foundation::Size size(100, 75);
            TestServices::WindowHelper->SetWindowSizeOverrideWithWindowScale(size, 4.0f);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ bitmapImage = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
                testImage->Stretch = Stretch::Uniform;
                testImage->Height = size.Height;

                bitmapImage = ref new BitmapImage();
                bitmapImage->DecodePixelHeight = static_cast<int>(size.Width);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                testImage->Source = bitmapImage;

                SetSourceAsyncFromFile(bitmapImage, GetResourcesPath() + L"messaging_test.jpg");
            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        // Covers this specific scenario:
        // 1. Set an image to a URI
        // 2. Subscribe to PointerPressed/PointerReleased and change the URI in those events.
        // 3. Tap the image control
        // 4. Wait for the Tapped event of the control to fire
        // Pre-RS1, we left the natural size on the object alone when changing image sources which
        // still allowed hit-testing to proceed.  Applications took a dependency on this "bug" which
        // means we need to preserve the size of the previously set image until the new image is done.
        void AppRegressionTests::TFS_7302806()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));

            auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto tappedRegistration = CreateSafeEventRegistration(UIElement, Tapped);
            auto pointerPressedRegistration = CreateSafeEventRegistration(UIElement, PointerPressed);
            auto pointerReleasedRegistration = CreateSafeEventRegistration(UIElement, PointerReleased);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();
            auto imageTappedEvent = std::make_shared<Event>();

            xaml_controls::Image^ testImage = nullptr;
            BitmapImage^ bitmapImage = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                testImage = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));
                testImage->Stretch = Stretch::Uniform;
                testImage->Width = 100;
                testImage->Height = 100;

                bitmapImage = ref new BitmapImage();
                testImage->Source = bitmapImage;

                tappedRegistration.Attach(
                    testImage,
                    ref new xaml_input::TappedEventHandler([imageTappedEvent](Platform::Object^ sender, xaml_input::TappedRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Image.Tapped Event Fired");
                    imageTappedEvent->Set();
                }));

                pointerPressedRegistration.Attach(
                    testImage,
                    ref new xaml_input::PointerEventHandler([&](Platform::Object^ sender, xaml_input::PointerRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Image.PointerPressed Event Fired, Changing URI to SmileyWink");
                    bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"SmileyWink.png");
                }));

                pointerReleasedRegistration.Attach(
                    testImage,
                    ref new xaml_input::PointerEventHandler([&](Platform::Object^ sender, xaml_input::PointerRoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Image.PointerReleased Event Fired, Changing URI to no Smiley");
                    bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Smiley.bmp");
                }));

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage.ImageOpened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Smiley.bmp");
            });
            bitmapImageOpenedEvent->WaitForDefault();

            TestServices::InputHelper->Tap(testImage);

            imageTappedEvent->WaitForDefault();
        }


        // Covers this specific scenario:
        // 1. Set an image to a URI, make sure it shows up
        // 2. Have another image use the same URI and size.
        // 3. Hide the first image (Visibility=Collapsed).
        // 4. Simulate app suspend.
        // 5. Simulate discarded hardware surface.
        // 6. Simulate app resume.
        // 7. Wait for surface update (indicating a surface update to repopulate the surface happened).
        // 8. Check the MockDComp image content is available.
        //
        // Pre-RS2 the image would not be recovered because the main surface responsible for updates is
        // hidden and because it is not walked, it will not detect that it is discarded and issue the
        // necessary work to recover the surface.
        void AppRegressionTests::TFS_4354828()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"TwoImages.xaml"));

            auto openedRegistration1 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent1 = std::make_shared<Event>();

            auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent2 = std::make_shared<Event>();

            Uri^ testUri = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");

            // Load the first image first to ensure it is the "baseline" holding the surface cache entry.
            xaml_controls::Image^ testImage1 = nullptr;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));

                auto bitmapImage1 = ref new BitmapImage();

                openedRegistration1.Attach(
                    bitmapImage1,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent1](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage 1 Opened Event Fired");
                    bitmapImageOpenedEvent1->Set();
                }));

                testImage1->Source = bitmapImage1;
                bitmapImage1->UriSource = testUri;
            });
            bitmapImageOpenedEvent1->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Load the second image next to ensure it references the previous surface cache entry.
            RunOnUIThread([&]()
            {
                auto testImage2 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement2"));

                auto bitmapImage2 = ref new BitmapImage();

                openedRegistration2.Attach(
                    bitmapImage2,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent2](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage 2 Opened Event Fired");
                    bitmapImageOpenedEvent2->Set();
                }));

                testImage2->Source = bitmapImage2;
                bitmapImage2->UriSource = testUri;
            });
            bitmapImageOpenedEvent2->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            // Collapse the first element so it isn't render walked.
            RunOnUIThread([&]()
            {
                testImage1->Visibility = Visibility::Collapsed;
            });
            TestServices::WindowHelper->WaitForIdle();

            // Suspend, Discard, Resume and wait for an image update to fire indicating the image was reloaded.
            // Then do MockDComp comparison to make sure the image content is valid.
            ETWWaiterProxy decodeEtwWaiter;
            decodeEtwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                ImageUpdateHardwareResourcesEnd_value);

            TestServices::WindowHelper->TriggerSuspend(false /* isTriggeredByResourceTimer */, true /* allowOfferResources */);
            TestServices::Utilities->SimulateAllSurfacesDiscarded();
            TestServices::WindowHelper->TriggerResume();

            decodeEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        // Set the URI on the BitmapImage after it has been used with stream.
        void AppRegressionTests::TFS_9797797()
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto stream = LoadBinaryFile(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
            ImageEventWaitingContext waiter;

            BitmapImage ^bitmapImage;

            RunOnUIThread([&]()
            {
                auto image = ref new xaml_controls::Image();
                TestServices::WindowHelper->WindowContent = image;

                bitmapImage = ref new BitmapImage();
                image->Source = bitmapImage;

                waiter.Attach(bitmapImage);

                bitmapImage->SetSourceAsync(stream);
                bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"Rainier_444_2048x1536.jpg");
            });

            waiter.WaitOpened();
        }

        // Covers this specific scenario:
        // 1. Load an image that is very large and will be broken into a lot of tiles.
        // 2. During the asynchronous decode, get the UI thread to flush texture updates.
        //    With an RS2 bug, this will result in the staging surface being released early
        //    and the asynchronous decoding will queue an update with that null staging texture
        //    which will result in a crash
        //
        // This the generic BitmapImage version of the test.
        void AppRegressionTests::TFS_9742148()
        {
            // Test must run with SpriteVisuals enabled in order to use virtual surface tiling to reproduce
            // the issue.
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            ImageEventWaitingContext waiter;

            auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));

            RunOnUIThread([&] ()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;

                auto image = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));

                auto bitmapImage = ref new BitmapImage();
                image->Stretch = Stretch::None;
                image->Source = bitmapImage;

                waiter.Attach(bitmapImage);

                bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"CN-EOS5DMarkII-ORI-BottomUp.JPG");
            });

            // Intentionally tick the UI thread frequently to get it to flush updates in the middle of
            // decoding.  Do this until the image is opened at which point it doesn't need to be
            // ticked anymore since the point was to cause a race condition between both threads.
            auto maxIterations = 100;
            while ((maxIterations > 0) && !waiter.IsOpened())
            {
                TestServices::WindowHelper->SynchronouslyTickUIThread(1);
                maxIterations--;
            }

            VERIFY_IS_TRUE(waiter.IsOpened());
        }

        // Setting the explicit decode size could have caused allocation of an upscaled surface.
        // Now we limit the decode size to the natural size to reduce the chance of OOM.
        void AppRegressionTests::TFS_9062885()
        {
            TestCleanupWrapper cleanup;
            TestServices::WindowHelper->SetWindowSizeOverride(::Windows::Foundation::Size(400, 300));

            ImageEventWaitingContext waiter;

            BitmapImage ^bitmapImage;

            RunOnUIThread([&]()
            {
                auto image = ref new xaml_controls::Image();

                TestServices::WindowHelper->WindowContent = image;

                bitmapImage = ref new BitmapImage();
                image->Source = bitmapImage;

                waiter.Attach(bitmapImage);

                bitmapImage->DecodePixelHeight = 10000;
                bitmapImage->UriSource = ref new Uri(GetResourcesPath() + L"LargeHorizontalStrip_4640x168.JPG");
            });

            waiter.WaitOpened();

            RunOnUIThread([&]()
            {
                VERIFY_ARE_EQUAL(4640, bitmapImage->PixelWidth);
                VERIFY_ARE_EQUAL(168, bitmapImage->PixelHeight);
            });
        }

        private ref class MeasureOverrideSetsImageUri sealed: public xaml_controls::Grid
        {
        public:
            MeasureOverrideSetsImageUri(Platform::String^ uri)
                : m_uri(uri)
            {
                Children->Append(m_image);
            }

        protected:
            ::Windows::Foundation::Size MeasureOverride(::Windows::Foundation::Size size) override
            {
                auto bitmapImage = ref new BitmapImage();
                bitmapImage->UriSource = ref new Uri(m_uri);
                m_image->Source = bitmapImage;

                return Grid::MeasureOverride(size);
            }

        private:
            Platform::String^ m_uri;
            xaml_controls::Image ^m_image = ref new xaml_controls::Image();
        };

        void AppRegressionTests::TFS_12066837()
        {
            WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

            ETWWaiterProxy decodeEtwWaiter;
            decodeEtwWaiter.Start(
                WINDOWS_UI_XAML_ETW_PROVIDER,
                ImageUpdateHardwareResourcesEnd_value);

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = ref new MeasureOverrideSetsImageUri(GetResourcesPath() + L"barcelona.jpg");
            });

            decodeEtwWaiter.WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();
            TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
        }

        void AppRegressionTests::SetSourceAsyncFromFile(
            xaml_imaging::BitmapImage^ bi,
            Platform::String^ filePath
            )
        {
            create_task(StorageFile::GetFileFromPathAsync(filePath))
                .then([=](StorageFile^ pFile)
            {
                VERIFY_IS_NOT_NULL(pFile);

                create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                    .then([=](IRandomAccessStream^ pFileStream)
                {
                    VERIFY_IS_NOT_NULL(pFileStream);

                    RunOnUIThread([=]()
                    {
                        auto pAsyncAction = bi->SetSourceAsync(pFileStream);
                        VERIFY_IS_NOT_NULL(pAsyncAction);
                    });
                });
            });
        }

    } } }
} } } }
