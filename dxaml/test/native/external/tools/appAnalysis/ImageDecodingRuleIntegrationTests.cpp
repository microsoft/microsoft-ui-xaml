// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include "FileLoader.h"
#include <SafeEventRegistration.h>
#include "TestCleanupWrapper.h"
#include "ImageDecodingRuleIntegrationTests.h"
#include "MUX-ETWEvents.h"
#include "RuleTesterHelper.h"
#include "resource.h"
#include "ValueConverters.h"
#include "CustomUserControl.h"
#include "MainPage.xaml.h"
#include "CustomTypes.XamlTypeInfo.g.h"
#include "TestHelpers.h"
#include <CustomMetadataRegistrar.h>

using namespace ::Windows::Foundation;
using namespace ::Windows::UI;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace Microsoft::UI::Xaml::Media;
using namespace Microsoft::UI::Xaml::Media::Imaging;

using namespace test_infra;

using namespace ::Windows::Storage::Streams;
using namespace Microsoft::Diagnostics::AppAnalysis;

namespace shared_types = ::Tests::Tools::Shared;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Tools { namespace AppAnalysis {

        Platform::String^ ImageDecodingRuleIntegrationTests::GetResourcesPath() const
        {
            return "ms-appx:///resources/native/external/foundation/graphics/image/";
        }

        Concurrency::task<void> ImageDecodingRuleIntegrationTests::OpenImageSync(BitmapImage^ bitmapImage, Platform::String^ file)
        {
            Platform::String^ path = GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\" + file;

            return Concurrency::create_task([bitmapImage, path]
            {
                return ::Windows::Storage::StorageFile::GetFileFromPathAsync(path);
            }).then([bitmapImage](::Windows::Storage::StorageFile^ picture)
            {
                return picture->OpenAsync(::Windows::Storage::FileAccessMode::Read);
            }).then([bitmapImage](IRandomAccessStream^ stream)
            {
                RunOnUIThread([&]()
                {
                    bitmapImage->SetSource(stream);
                });
            });
        }

        Concurrency::task<void> ImageDecodingRuleIntegrationTests::OpenImageAsync(BitmapImage^ bitmapImage, Platform::String^ file)
        {
            Platform::String^ path = GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\" + file;

            return Concurrency::create_task([bitmapImage, path]
            {
                return ::Windows::Storage::StorageFile::GetFileFromPathAsync(path);
            }).then([bitmapImage](::Windows::Storage::StorageFile^ picture)
            {
                return picture->OpenAsync(::Windows::Storage::FileAccessMode::Read);
            }).then([bitmapImage](IRandomAccessStream^ stream)
            {
                RunOnUIThread([&]()
                {
                    bitmapImage->SetSourceAsync(stream);
                });
            });
        }


        bool ImageDecodingRuleIntegrationTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();

            return true;
        }

        bool ImageDecodingRuleIntegrationTests::ClassCleanup()
        {
            return true;
        }

        bool ImageDecodingRuleIntegrationTests::TestSetup()
        {
            TestServices::WindowHelper->InitializeXaml(ref new MetadataProvider(), ref new CustomMetadataRegistrar<shared_types::CustomUserControl>());

            return true;
        }

        bool ImageDecodingRuleIntegrationTests::TestCleanup()
        {
            TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        //------------------------------------------------------------------------
        // Test case: Verifies the App Analysis rule engine catches the case where
        // an image is loaded synchronously, preventing us from using the right
        // size decoding feature
        //------------------------------------------------------------------------
        void ImageDecodingRuleIntegrationTests::VerifiesCatchesSyncronousDecode()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            RuleTesterHelper helper(L"AA0001", L"VerifiesCatchesSyncronousDecode");

            Platform::String^ fileName = GetResourcesPath() + L"SimpleImage.xaml";
            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Grid>(fileName);

            auto rainierOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto rainierImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ rainierImage;

            Controls::Image^ testImage;
            Concurrency::task<void> pictureTask;

            RunOnUIThread([&]()
            {
                testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(testImage);

                // Set height and width to values smaller than images to ensure the event fires
                // The expected sizes for the images are 2048x1536 (rainier), 2000x1500 (animal_cats)
                testImage->Height = 100;
                testImage->Width = 100;

                rainierImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(rainierImage);
                rainierOpenedRegistration.Attach(
                    rainierImage,
                    ref new xaml::RoutedEventHandler([rainierImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rainier Image Opened Event Fired");
                    rainierImageOpenedEvent->Set();
                }));

                pictureTask = OpenImageSync(rainierImage, L"Rainier_444_2048x1536.jpg");
                testImage->Source = rainierImage;
            });
            pictureTask.wait();
            TestServices::WindowHelper->WaitForIdle();

            rainierImageOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(rainierImageOpenedEvent->HasFired());

            helper.VerifyRuleTriggered(1u);
            helper.VerifySourceInfo(0, fileName, 3u, 12u);
            helper.VerifyDescription(0, IMAGE_RULE_SYNCDECODE, L"imageElement");
        }

        //------------------------------------------------------------------------
        // Test case: Verifies the App Analysis rule engine catches the case where
        // an image brush is used to fill an ellipse. A case where we can't use
        // Right Size Decoding
        //------------------------------------------------------------------------
        void ImageDecodingRuleIntegrationTests::VerifyCatchesImageBrushInEllipse()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            RuleTesterHelper helper(L"AA0001", L"VerifyCatchesImageBrushInEllipse");

            Platform::String^ file = "ms-appx:///resources/native/tools/MainPage.xaml";
            auto page = AppAnalysisTestHelpers::LoadXaml<shared_types::MainPage>(file);

            helper.VerifyRuleTriggered(1u);
            helper.VerifyDescription(0, IMAGE_RULE_SOFTWARERENDERING, L"ellipse");

            // The ellipse is 30x30, so verify the memory bloat for the image (which is 2048x1536).
            unsigned int extraWidth = 2048 - 30;
            unsigned int extraHeight = 1536 - 30;
            double extraWaste = (4 * extraHeight*extraWidth) / 1000.00; // divide by 1000 for kilobytes
            helper.VerifyMeasurement(0, MeasurementUnit::Kilobytes, extraWaste);
        }

        //------------------------------------------------------------------------
        // Test case: Verifies the App Analysis rule engine doesn't mistakenly
        // catch the case where an image brush is used to fill an ellipse, but the
        // user specified the decode params
        //------------------------------------------------------------------------
        void ImageDecodingRuleIntegrationTests::VerifyNoFireImageBrushInEllipseWithDecodeSize()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            RuleTesterHelper helper(L"AA0001", L"VerifyNoFireImageBrushInEllipseWithDecodeSize");

            Platform::String^ file = "ms-appx:///resources/native/tools/MainPage.xaml";
            auto page = AppAnalysisTestHelpers::LoadXaml<shared_types::MainPage>(file, AppAnalysisTestHelpers::LoadComponentOptions::DoNotPlaceInTree);

            RunOnUIThread([&page]()
            {
                auto ellipse = safe_cast<Microsoft::UI::Xaml::Shapes::Ellipse^>(page->FindName(L"ellipse"));
                WEX::Common::Throw::If(ellipse == nullptr, E_POINTER, L"Could not find ellipse");

                auto imageBrush = safe_cast<Microsoft::UI::Xaml::Media::ImageBrush^>(ellipse->Fill);
                auto image = safe_cast<xaml_media::Imaging::BitmapImage^>(imageBrush->ImageSource);
                image->DecodePixelHeight = static_cast<int>(ellipse->Height);
                image->DecodePixelWidth = static_cast<int>(ellipse->Width);

                TestServices::WindowHelper->WindowContent = page;
            });

            TestServices::WindowHelper->WaitForIdle();

            helper.VerifyRuleNotTriggered();
        }

        //------------------------------------------------------------------------
        // Test case: Verifies the ImageDecodingRule fires when we redecode an image
        //------------------------------------------------------------------------
        void ImageDecodingRuleIntegrationTests::VerifyCatchesRedecodeAfterDecodeSizeSpecified()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            RuleTesterHelper helper(L"AA0001", L"VerifyCatchesRedecodeAfterDecodeSizeSpecified");

            Platform::String^ file = "ms-appx:///resources/native/tools/MainPage.xaml";
            auto page = AppAnalysisTestHelpers::LoadXaml<shared_types::MainPage>(file, AppAnalysisTestHelpers::LoadComponentOptions::DoNotPlaceInTree);
            xaml_media::Imaging::BitmapImage^ image = nullptr;
            RunOnUIThread([&page, &image]()
            {
                auto ellipse = safe_cast<Microsoft::UI::Xaml::Shapes::Ellipse^>(page->FindName(L"ellipse"));
                WEX::Common::Throw::If(ellipse == nullptr, E_POINTER, L"Failed to find ellipse in page");

                auto imageBrush = safe_cast<Microsoft::UI::Xaml::Media::ImageBrush^>(ellipse->Fill);
                image = safe_cast<xaml_media::Imaging::BitmapImage^>(imageBrush->ImageSource);
                image->DecodePixelHeight = static_cast<int>(ellipse->Height);
                image->DecodePixelWidth = static_cast<int>(ellipse->Width);

                TestServices::WindowHelper->WindowContent = page;
            });

            TestServices::WindowHelper->WaitForIdle();

            // now explicitly set the decode size to something greater and make sure the rule fires
            const int extraHeight = 100;
            const int extraWidth = 100;
            RunOnUIThread([&image, extraHeight, extraWidth]()
            {
                image->DecodePixelHeight += extraHeight;
                image->DecodePixelWidth += extraWidth;
            });

            TestServices::WindowHelper->WaitForIdle();

            helper.VerifyRuleTriggered(2u);
            helper.VerifyDescription(0, IMAGE_RULE_DECODESIZESPECIFIED, L"ellipse");
            helper.VerifyDescription(1, IMAGE_RULE_SOFTWARERENDERING, L"ellipse");

            double extraWaste = (4 * extraHeight*extraWidth) / 1000.00; // divide by 1000 for kilobytes
            helper.VerifyMeasurement(0, MeasurementUnit::Kilobytes, extraWaste);
            helper.VerifyMeasurement(1, MeasurementUnit::Kilobytes, extraWaste);

            helper.VerifySourceInfo(0, file, 15u, 16u);
        }

        //------------------------------------------------------------------------
        // Test case: Verifies the ImageDecodingRule doesn't fire in the scenario
        // when the DecodeSize was specified at a time when the scaling was 1.0x and
        // then the scaling changed to 0.5x. In this scenario, we don't attempt to
        // redecode the image and we don't want the rule firing in this scenario.
        //------------------------------------------------------------------------
        void ImageDecodingRuleIntegrationTests::VerifyNoFireAfterPlateauScalingChangedSmaller()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0001", L"VerifyNoFireAfterPlateauScalingChangedSmaller");
            auto filePath = GetResourcesPath() + L"SimpleImage.xaml";

            // Set the decode pixel width to be larger than the scaled image will be. The framework
            // still wouldn't redecode in this scenario
            PlateuScaleTestHelper(filePath, 0.5f, true);

            helper.VerifyRuleNotTriggered();
        }

        void ImageDecodingRuleIntegrationTests::VerifyNoFireAfterPlateauScalingChangedLarger()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0001", L"VerifyNoFireAfterPlateauScalingChangedLarger");
            auto filePath = GetResourcesPath() + L"SimpleImage.xaml";

            // Don't set decode pixel width/height so we redecode the image. Makes sure the rule
            // doesn't cache any previous requests at a smaller size, and complain once this happens
            PlateuScaleTestHelper(filePath, 1.5f, false);

            helper.VerifyRuleNotTriggered();
        }

        void ImageDecodingRuleIntegrationTests::VerifyCatchesSecondIncorrectDecode()
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            RuleTesterHelper helper(L"AA0001", L"VerifyCatchesRedecode");
            Platform::String^ fileName = GetResourcesPath() + L"SimpleImage.xaml";

            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Grid>(fileName);

            auto rainierOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto rainierImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ image;

            Controls::Image^ testImage;
            Concurrency::task<void> pictureTask;

            RunOnUIThread([&]()
            {
                testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(testImage);

                // Set height and width to values smaller than images to ensure the event fires
                // The expected sizes for the images are 2048x1536 (rainier), 2000x1500 (animal_cats)
                testImage->Height = 100;
                testImage->Width = 100;

                image = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(image);
                rainierOpenedRegistration.Attach(
                    image,
                    ref new xaml::RoutedEventHandler([rainierImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rainier Image Opened Event Fired");
                    rainierImageOpenedEvent->Set();
                }));

                pictureTask = OpenImageAsync(image, L"Rainier_444_2048x1536.jpg");
                testImage->Source = image;
            });

            pictureTask.wait();
            TestServices::WindowHelper->WaitForIdle();
            rainierImageOpenedEvent->WaitForDefault();

            auto catOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto catImageOpenedEvent = std::make_shared<Event>();

            RunOnUIThread([&]()
            {
                catOpenedRegistration.Attach(
                    image,
                    ref new xaml::RoutedEventHandler([catImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Cat Image Opened. Meooow!!");
                    catImageOpenedEvent->Set();
                }));

                pictureTask = OpenImageSync(image, L"animal_cats.jpg");
            });

            pictureTask.wait();
            TestServices::WindowHelper->WaitForIdle();
            catImageOpenedEvent->WaitForDefault();

            helper.VerifyRuleTriggered(1u);
            helper.VerifyDescription(0, IMAGE_RULE_SYNCDECODE, L"imageElement");

            int extraHeight = 1500 - 100;
            int extraWidth = 2000 - 100;
            double extraWaste = (4 * extraHeight*extraWidth) / 1000.00; // divide by 1000 for kilobytes
            helper.VerifyMeasurement(0, MeasurementUnit::Kilobytes, extraWaste);
            helper.VerifySourceInfo(0, fileName, 3u, 12u);
        }


        void ImageDecodingRuleIntegrationTests::VerifyMultipleImagesDifferentSizeSameUri()
        {
            TestCleanupWrapper cleanup;

            RuleTesterHelper helper(L"AA0001", L"VerifyMultipleImagesDifferentSizeSameUri");

            MultipleImagesTestHelper(
                GetResourcesPath() + L"TwoImagesDifferentSizes.xaml",
                GetResourcesPath() + L"Rainier_444_2048x1536.jpg"
            );

            helper.VerifyRuleNotTriggered();
        }

        void ImageDecodingRuleIntegrationTests::MultipleImagesTestHelper(Platform::String^ pXamlPath, Platform::String^ pImagePath)
        {
            TestCleanupWrapper cleanup;

            ::Windows::Foundation::Size size(400, 300);
            TestServices::WindowHelper->SetWindowSizeOverride(size);

            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Grid>(pXamlPath, AppAnalysisTestHelpers::LoadComponentOptions::DoNotPlaceInTree);
            VERIFY_IS_NOT_NULL(rootGrid);

            auto openedRegistration1 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto openedRegistration2 = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto bitmapImage1OpenedEvent = std::make_shared<Event>();
            auto bitmapImage2OpenedEvent = std::make_shared<Event>();

            ::Windows::Foundation::Uri^ testUri;

            RunOnUIThread([&]()
            {
                TestServices::WindowHelper->WindowContent = rootGrid;
                auto testImage2 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement2"));
                VERIFY_IS_NOT_NULL(testImage2);

                testImage2->Stretch = Stretch::Fill;

                auto bitmapImage2 = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage2);

                testImage2->Source = bitmapImage2;

                testUri = ref new Uri(pImagePath);
                VERIFY_IS_NOT_NULL(testUri);

                openedRegistration2.Attach(
                    bitmapImage2,
                    ref new xaml::RoutedEventHandler([bitmapImage2OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage2 Opened Event Fired");
                    bitmapImage2OpenedEvent->Set();
                }));

                bitmapImage2->UriSource = testUri;
            });

            bitmapImage2OpenedEvent->WaitForDefault();

            RunOnUIThread([&]()
            {
                auto testImage1 = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement1"));
                VERIFY_IS_NOT_NULL(testImage1);

                testImage1->Stretch = Stretch::Fill;

                auto bitmapImage1 = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(bitmapImage1);

                testImage1->Source = bitmapImage1;

                openedRegistration1.Attach(
                    bitmapImage1,
                    ref new xaml::RoutedEventHandler([bitmapImage1OpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage1 Opened Event Fired");
                    bitmapImage1OpenedEvent->Set();
                }));

                bitmapImage1->UriSource = testUri;
            });

            bitmapImage1OpenedEvent->WaitForDefault();


            TestServices::WindowHelper->WaitForIdle();
        }

        void ImageDecodingRuleIntegrationTests::PlateuScaleTestHelper(Platform::String^ xamlPath, float scaleFactor, bool setDecodePixelSize)
        {
            TestCleanupWrapper cleanup;

            std::shared_ptr<Event> notificationEvent = std::make_shared<Event>();

            ::Windows::Foundation::Size size(400, 400);
            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, 1.0f);

            auto rootGrid = AppAnalysisTestHelpers::LoadXaml<Grid>(xamlPath);

            auto rainierOpenedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
            auto rainierImageOpenedEvent = std::make_shared<Event>();

            BitmapImage^ rainierImage;

            Controls::Image^ testImage;
            Concurrency::task<void> pictureTask;

            RunOnUIThread([&]()
            {
                testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
                VERIFY_IS_NOT_NULL(testImage);

                testImage->Height = size.Height;
                testImage->Width = size.Width;

                rainierImage = ref new BitmapImage();
                VERIFY_IS_NOT_NULL(rainierImage);

                if (setDecodePixelSize)
                {
                    rainierImage->DecodePixelHeight = static_cast<int>(testImage->Height);
                    rainierImage->DecodePixelWidth = static_cast<int>(testImage->Width);
                }

                rainierOpenedRegistration.Attach(
                    rainierImage,
                    ref new xaml::RoutedEventHandler([rainierImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"Rainier Image Opened Event Fired");
                    rainierImageOpenedEvent->Set();
                }));

                pictureTask = OpenImageAsync(rainierImage, L"Rainier_444_2048x1536.jpg");
                testImage->Source = rainierImage;
            });

            pictureTask.wait();
            TestServices::WindowHelper->WaitForIdle();

            rainierImageOpenedEvent->WaitForDefault();
            VERIFY_IS_TRUE(rainierImageOpenedEvent->HasFired());

            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(size, scaleFactor);
            TestServices::WindowHelper->WaitForIdle();
        }

    } }
} } } }
