// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "MemoryInfoTests.h"
#include <XamlTailored.h>
#include <TestCleanupWrapper.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <Microsoft.UI.Xaml.hosting.referencetracker.h>
#include <Collection.h>

using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Markup;
using namespace Microsoft::UI::Xaml::Tests::Common;
using namespace test_infra;
using namespace ::Windows::Foundation;
using namespace ::Windows::Foundation::Collections;

namespace Private { namespace Infrastructure {

    struct HdrOutputOverrideHelper
    {
        HdrOutputOverrideHelper()
        {
            test_infra::TestServices::WindowHelper->SetHdrOutputOverride(true);
        }

        ~HdrOutputOverrideHelper()
        {
            test_infra::TestServices::WindowHelper->SetHdrOutputOverride(false);
        }
    };

}}

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Tools {
    namespace MemoryDiagnostics {

        bool MemoryInfoTests::ClassSetup()
        {
            CommonTestSetupHelper::CommonTestClassSetup();
            return true;
        }

        bool MemoryInfoTests::ClassCleanup()
        {
            return true;
        }

        bool MemoryInfoTests::TestSetup()
        {
            test_infra::TestServices::WindowHelper->InitializeXaml();
            return true;
        }

        bool MemoryInfoTests::TestCleanup()
        {
            test_infra::TestServices::WindowHelper->ShutdownXaml();
            TestServices::WindowHelper->VerifyTestCleanup();
            return true;
        }

        #pragma region Test Methods

        void MemoryInfoTests::TestGetCountOfDescendantUIElements()
        {
            TestCleanupWrapper cleanup;

            static const wchar_t markup[] =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootGrid'>"
                L"    <Grid.RowDefinitions>"
                L"        <RowDefinition Height='Auto' />"
                L"        <RowDefinition Height='*' />"
                L"    </Grid.RowDefinitions>"
                L"    <RelativePanel Grid.Row='0' Background='Red' x:Name='relativePanel'>"
                L"        <Border>"
                L"            <TextBlock>Foobar</TextBlock>"
                L"        </Border>"
                L"        <TextBlock>Hello world</TextBlock>"
                L"    </RelativePanel>"
                L"    <StackPanel Grid.Row='1' x:Name='stackPanel'>"
                L"        <Border Background='Blue'>"
                L"            <Rectangle Fill='Yellow' Opacity='0.5' />"
                L"        </Border>"
                L"        <TextBlock>More text</TextBlock>"
                L"        <TextBlock>Yet more text</TextBlock>"
                L"    </StackPanel>"
                L"</Grid>";

            xaml_controls::Grid^ rootGrid;
            RunOnUIThread([&]() {
                rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(Platform::StringReference(markup)));
                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                Microsoft::WRL::ComPtr<IReferenceTracker> gridReferenceTracker = nullptr;
                THROW_IF_FAILED(reinterpret_cast<IUnknown*>(rootGrid)->QueryInterface(IID_PPV_ARGS(gridReferenceTracker.ReleaseAndGetAddressOf())));
                Microsoft::WRL::ComPtr<IReferenceTrackerManager> manager = nullptr;
                THROW_IF_FAILED(gridReferenceTracker->GetReferenceTrackerManager(manager.ReleaseAndGetAddressOf()));

                auto gridMemoryInfo = safe_cast<xaml::IMemoryInfoPrivate^>(rootGrid);
                {
                    LOG_OUTPUT(L"Verifying UIElement count starting at root");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(gridMemoryInfo->GetCountOfDescendantUIElements(), 10);
                }

                auto relativePanel = safe_cast<xaml_controls::RelativePanel^>(rootGrid->FindName(Platform::StringReference(L"relativePanel")));
                auto relativePanelMemoryInfo = safe_cast<xaml::IMemoryInfoPrivate^>(relativePanel);
                {
                    LOG_OUTPUT(L"Verifying UIElement count starting at root of subtree");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(relativePanelMemoryInfo->GetCountOfDescendantUIElements(), 4);
                }

                // Invoke again on same object; things shouldn't have changed
                {
                    LOG_OUTPUT(L"Verifying UIElement count starting at root of subtree (this is to verify that the count doesn't change unexpectedly)");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(relativePanelMemoryInfo->GetCountOfDescendantUIElements(), 4);
                }
            });
        }

        void MemoryInfoTests::TestGetCountOfDescendantUIElementsWithVirtualization()
        {
            TestCleanupWrapper cleanup;

            static const wchar_t markup[] =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation' xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml' x:Name='rootGrid'>"
                L"    <ListView x:Name='listview' Height='20'>"
                L"        <ListView.ItemsPanel>"
                L"            <ItemsPanelTemplate>"
                L"                <ItemsStackPanel CacheLength='4'/>"
                L"            </ItemsPanelTemplate>"
                L"        </ListView.ItemsPanel>"
                L"        <ListView.ItemTemplate>"
                L"            <DataTemplate>"
                L"                <TextBlock Text='{Binding}' />"
                L"            </DataTemplate>"
                L"        </ListView.ItemTemplate>"
                L"    </ListView>"
                L"</Grid>";

            xaml_controls::Grid^ rootGrid;
            RunOnUIThread([&]() {
                rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(Platform::StringReference(markup)));

                // Populate the ListView's ItemsSource
                auto itemList = ref new Platform::Collections::Vector<int>();
                for (int i = 0; i < 500; i++)
                {
                    itemList->Append(i);
                }
                auto listview = safe_cast<xaml_controls::ListView^>(rootGrid->FindName(Platform::StringReference(L"listview")));
                listview->ItemsSource = itemList;

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                Microsoft::WRL::ComPtr<IReferenceTracker> gridReferenceTracker = nullptr;
                THROW_IF_FAILED(reinterpret_cast<IUnknown*>(rootGrid)->QueryInterface(IID_PPV_ARGS(gridReferenceTracker.ReleaseAndGetAddressOf())));
                Microsoft::WRL::ComPtr<IReferenceTrackerManager> manager = nullptr;
                THROW_IF_FAILED(gridReferenceTracker->GetReferenceTrackerManager(manager.ReleaseAndGetAddressOf()));

                auto gridMemoryInfo = safe_cast<xaml::IMemoryInfoPrivate^>(rootGrid);
                {
                    LOG_OUTPUT(L"Verifying UIElement count starting at root");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    // Over half of the descendant count belongs to just the scrollbars...
                    VERIFY_ARE_EQUAL(gridMemoryInfo->GetCountOfDescendantUIElements(), 58);
                }

                // Invoke again on same object; things shouldn't have changed
                {
                    LOG_OUTPUT(L"Verifying UIElement count starting at root");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(gridMemoryInfo->GetCountOfDescendantUIElements(), 58);
                }
            });
        }

        void MemoryInfoTests::TestGetEstimatedSizeOfDescendantImages()
        {
            TestCleanupWrapper cleanup;

            TestGetEstimatedSizeOfDescendantImages_Helper(false);
        }

        void MemoryInfoTests::TestGetEstimatedSizeOfDescendantImagesHDR()
        {
            TestCleanupWrapper cleanup;

            TestGetEstimatedSizeOfDescendantImages_Helper(true);
        }

        void MemoryInfoTests::TestGetEstimatedSizeOfDescendantImagesSoftwareRasterization()
        {
            TestCleanupWrapper cleanup;

            static const wchar_t markup[] =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Ellipse x:Name='ellipse' Height='200' Width='200'>"
                L"        <Ellipse.Fill>"
                L"            <ImageBrush>"
                L"                <ImageBrush.ImageSource>"
                L"                    <BitmapImage DecodePixelWidth='512' DecodePixelHeight='384' UriSource='ms-appx:///resources/native/external/foundation/graphics/image/Rainier_444_2048x1536.jpg' />"
                L"                </ImageBrush.ImageSource>"
                L"            </ImageBrush>"
                L"        </Ellipse.Fill>"
                L"    </Ellipse>"
                L"</Grid>";

            // Fill image: 2-pixel gutters on hardware surface, has no software surface (due to D2D), raw data size is 611714
            int fillImage_EstimatedSize = (170074 + ((512 + 2) * (384 + 2) * 4));

            LOG_OUTPUT(L"Verifying estimated size of descendant images when software rasterization is used.");

            xaml_controls::Grid^ rootGrid;
            auto openedRegistration = CreateSafeEventRegistration(xaml_imaging::BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent = std::make_shared<Event>();
            RunOnUIThread([&]() {
                rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(Platform::StringReference(markup)));
                auto ellipse = safe_cast<xaml_shapes::Ellipse^>(rootGrid->FindName(L"ellipse"));
                VERIFY_IS_NOT_NULL(ellipse);

                auto bitmapImage = safe_cast<xaml_imaging::BitmapImage^>(safe_cast<xaml_media::ImageBrush^>(ellipse->Fill)->ImageSource);

                openedRegistration.Attach(
                    bitmapImage,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            bitmapImageOpenedEvent->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                Microsoft::WRL::ComPtr<IReferenceTracker> gridReferenceTracker = nullptr;
                THROW_IF_FAILED(reinterpret_cast<IUnknown*>(rootGrid)->QueryInterface(IID_PPV_ARGS(gridReferenceTracker.ReleaseAndGetAddressOf())));
                Microsoft::WRL::ComPtr<IReferenceTrackerManager> manager = nullptr;
                THROW_IF_FAILED(gridReferenceTracker->GetReferenceTrackerManager(manager.ReleaseAndGetAddressOf()));

                auto ellipse = safe_cast<xaml_shapes::Ellipse^>(rootGrid->FindName(L"ellipse"));
                auto ellipseMemoryInfo = dynamic_cast<xaml::IMemoryInfoPrivate^>(ellipse);
                {
                    LOG_OUTPUT(L"Verifying estimated size of the ellipse with an ImageBrush fill");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(ellipseMemoryInfo->GetEstimatedSizeOfDescendantImages(), fillImage_EstimatedSize);
                }

            });

        }

        #pragma endregion

        void MemoryInfoTests::TestGetEstimatedSizeOfDescendantImages_Helper(bool useHdr)
        {
            static const wchar_t markup[] =
                L"<Grid xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"      xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'>"
                L"    <Grid.RowDefinitions>"
                L"        <RowDefinition Height='Auto' />"
                L"        <RowDefinition Height='*' />"
                L"    </Grid.RowDefinitions>"
                L"    <Image Grid.Row='0' x:Name='imageElement0'>"
                L"        <Image.Source>"
                L"            <BitmapImage DecodePixelWidth='512' DecodePixelHeight='384' UriSource='ms-appx:///resources/native/external/foundation/graphics/image/Rainier_444_2048x1536.jpg' />"
                L"        </Image.Source>"
                L"    </Image>"
                L"    <Image Grid.Row='1' x:Name='imageElement1'>"
                L"        <Image.Source>"
                L"            <BitmapImage DecodePixelWidth='384' DecodePixelHeight='216' UriSource='ms-appx:///resources/native/external/foundation/graphics/image/journey.jxr' />"
                L"        </Image.Source>"
                L"    </Image>"
                L"</Grid>";

            // First image: 2-pixel gutters, no software surface, raw data size is 611714
            int imageElement0_EstimatedSize = (170074 + ((512 + 2) * (384 + 2) * 4));
            // Second image: 2-pixel gutters, no software surface, supports HDR (64bpp), raw data size is 8146769
            int imageElement1_EstimatedSize = (8146769 + ((384 + 2) * (216 + 2) * (useHdr ? 8 : 4)));

            auto hdrOverrideGuard = wil::scope_exit([]
            {
                test_infra::TestServices::WindowHelper->SetHdrOutputOverride(false);
            });
            if (useHdr)
            {
                test_infra::TestServices::WindowHelper->SetHdrOutputOverride(true);
            }
            else
            {
                hdrOverrideGuard.release();
            }

            LOG_OUTPUT(L"Verifying estimated size of descendant images. HDR is: %s", useHdr ? L"enabled" : L"disabled");

            xaml_controls::Grid^ rootGrid;
            auto openedRegistration0 = CreateSafeEventRegistration(xaml_imaging::BitmapImage, ImageOpened);
            auto openedRegistration1 = CreateSafeEventRegistration(xaml_imaging::BitmapImage, ImageOpened);
            auto bitmapImageOpenedEvent0 = std::make_shared<Event>();
            auto bitmapImageOpenedEvent1 = std::make_shared<Event>();
            RunOnUIThread([&]() {
                rootGrid = safe_cast<xaml_controls::Grid^>(xaml_markup::XamlReader::Load(Platform::StringReference(markup)));
                auto testImage0 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement0"));
                auto testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));
                VERIFY_IS_NOT_NULL(testImage0);
                VERIFY_IS_NOT_NULL(testImage1);

                testImage0->Stretch = xaml_media::Stretch::Fill;
                testImage1->Stretch = xaml_media::Stretch::Fill;

                auto bitmapImage0 = safe_cast<xaml_imaging::BitmapImage^>(testImage0->Source);
                auto bitmapImage1 = safe_cast<xaml_imaging::BitmapImage^>(testImage1->Source);

                openedRegistration0.Attach(
                    bitmapImage0,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent0](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent0->Set();
                }));
                openedRegistration1.Attach(
                    bitmapImage1,
                    ref new xaml::RoutedEventHandler([bitmapImageOpenedEvent1](Platform::Object^ sender, xaml::RoutedEventArgs^)
                {
                    LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                    bitmapImageOpenedEvent1->Set();
                }));

                TestServices::WindowHelper->WindowContent = rootGrid;
            });
            bitmapImageOpenedEvent0->WaitForDefault();
            bitmapImageOpenedEvent1->WaitForDefault();
            TestServices::WindowHelper->WaitForIdle();

            RunOnUIThread([&]() {
                Microsoft::WRL::ComPtr<IReferenceTracker> gridReferenceTracker = nullptr;
                THROW_IF_FAILED(reinterpret_cast<IUnknown*>(rootGrid)->QueryInterface(IID_PPV_ARGS(gridReferenceTracker.ReleaseAndGetAddressOf())));
                Microsoft::WRL::ComPtr<IReferenceTrackerManager> manager = nullptr;
                THROW_IF_FAILED(gridReferenceTracker->GetReferenceTrackerManager(manager.ReleaseAndGetAddressOf()));

                auto gridMemoryInfo = safe_cast<xaml::IMemoryInfoPrivate^>(rootGrid);
                {
                    LOG_OUTPUT(L"Verifying estimated image size starting at root");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(gridMemoryInfo->GetEstimatedSizeOfDescendantImages(), imageElement0_EstimatedSize + imageElement1_EstimatedSize);
                }

                auto testImage0 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement0"));
                auto testImage0MemoryInfo = dynamic_cast<xaml::IMemoryInfoPrivate^>(testImage0);
                {
                    LOG_OUTPUT(L"Verifying estimated size of just the first image (no HDR support)");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(testImage0MemoryInfo->GetEstimatedSizeOfDescendantImages(), imageElement0_EstimatedSize);
                }

                auto testImage1 = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement1"));
                auto testImage1MemoryInfo = dynamic_cast<xaml::IMemoryInfoPrivate^>(testImage1);
                {
                    LOG_OUTPUT(L"Verifying estimated size of just the second image (HDR is supported)");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(testImage1MemoryInfo->GetEstimatedSizeOfDescendantImages(), imageElement1_EstimatedSize);
                }

                // Invoke again on same object; things shouldn't have changed
                {
                    LOG_OUTPUT(L"Verifying estimated size of just the second image (HDR is supported). Should not change unexpectedly");
                    THROW_IF_FAILED(manager->ReferenceTrackingStarted());
                    auto guard = wil::scope_exit([manager] { THROW_IF_FAILED(manager->ReferenceTrackingCompleted()); });
                    VERIFY_ARE_EQUAL(testImage1MemoryInfo->GetEstimatedSizeOfDescendantImages(), imageElement1_EstimatedSize);
                }
            });
        }

    }
} } } } }
