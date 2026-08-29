// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include "FileLoader.h"
#include "TestCleanupWrapper.h"
#include "ImageTestEngine.h"
#include <WUCRenderingScopeGuard.h>

using namespace concurrency;
using namespace Platform;
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
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Graphics { namespace Image {

        // Initializes the default values for all test properties
        // Care when changing default values as tests rely on them
        TestImage::TestImage()
            : m_pImagePath(nullptr)
            , m_stretchMode(xaml_media::Stretch::UniformToFill)
            , m_autoPlay(true)
            , m_bitmapCache(false)
            , m_decodeToRenderSize(true)
            , m_decodePixelWidth(0)
            , m_decodePixelHeight(0)
            , m_decodePixelType(xaml_imaging::DecodePixelType::Physical)
            , m_opacity(1.0)
            , m_nineGrid(xaml::Thickness({ 0,0,0,0 }))
            , m_elementSize(wf::Size(100,100))
            , m_loadApi(TestImageEnums::LoadApi::Uri)
            , m_parentElement(TestImageEnums::ParentElement::Image)
            , m_createOptions(xaml_imaging::BitmapCreateOptions::None)
            , m_trimAndRestoreHardwareResources(false)
            , m_imageEventWaitTime(2000)
            , m_failureExpected(false)
        {
        }

        ImageTestEngine::ImageTestEngine()
            : m_windowSize(wf::Size(400,300))
            , m_zoomScale(1.0f)
            , m_dcompRenderingMode(DCompRendering::WUCCompleteSynchronousCompTree)
            , m_mockDCompVerification(MockDComp::SurfaceComparison::ReferencedOnly)
            , m_mockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder)
        {
        }

        void ImageTestEngine::Execute()
        {
            WUCRenderingScopeGuard guard(m_dcompRenderingMode, false /*resizeWindow*/);

            TestServices::WindowHelper->SetWindowSizeOverrideWithScale(m_windowSize, m_zoomScale);

            xaml_controls::Panel^ pRootPanel = nullptr;

            RunOnUIThread([&]()
            {
                pRootPanel = safe_cast<xaml_controls::Panel^> (
                    xaml_markup::XamlReader::Load(
                        L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                        L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                        L"            HorizontalAlignment='Left'"
                        L"            VerticalAlignment='Top'"
                        L"            Orientation='Horizontal'/>"
                    ));
                VERIFY_IS_NOT_NULL(pRootPanel);

                TestServices::WindowHelper->WindowContent = pRootPanel;
            });
            TestServices::WindowHelper->WaitForIdle();

            for (TestImage^ pTestImage : m_testImages)
            {
                xaml_imaging::BitmapImage^ pBitmapImage = nullptr;
                xaml::FrameworkElement^ pFrameworkElement = nullptr;

                auto openedRegistration = CreateSafeEventRegistration(BitmapImage, ImageOpened);
                auto pBitmapImageOpenedEvent = std::make_shared<Event>();

                auto failedRegistration = CreateSafeEventRegistration(BitmapImage, ImageFailed);
                auto pBitmapImageFailedEvent = std::make_shared<Event>();

                RunOnUIThread([&]()
                {
                    pBitmapImage = SetupBitmapImage(pTestImage);
                    VERIFY_IS_NOT_NULL(pBitmapImage);

                    openedRegistration.Attach(
                        pBitmapImage,
                        ref new xaml::RoutedEventHandler([pBitmapImageOpenedEvent](Platform::Object^ sender, xaml::RoutedEventArgs^)
                    {
                        // Make sure the event only gets fired once
                        VERIFY_IS_FALSE(pBitmapImageOpenedEvent->HasFired());

                        LOG_OUTPUT(L"BitmapImage Opened Event Fired");
                        pBitmapImageOpenedEvent->Set();
                    }));
                    LOG_OUTPUT(L"BitmapImage Opened Event [0x%p] created and bound to BitmapImage [0x%p]", pBitmapImageOpenedEvent.get(), pBitmapImage);
                    VERIFY_IS_NOT_NULL(pBitmapImageOpenedEvent);

                    failedRegistration.Attach(
                        pBitmapImage,
                        ref new xaml::ExceptionRoutedEventHandler([pBitmapImageFailedEvent](Platform::Object^ sender, xaml::ExceptionRoutedEventArgs^)
                    {
                        // Make sure the event only gets fired once
                        VERIFY_IS_FALSE(pBitmapImageFailedEvent->HasFired());

                        LOG_OUTPUT(L"BitmapImage Failed Event Fired");
                        pBitmapImageFailedEvent->Set();
                    }));
                    LOG_OUTPUT(L"BitmapImage Failed Event [0x%p] created and bound to BitmapImage [0x%p]", pBitmapImageFailedEvent.get(), pBitmapImage);
                    VERIFY_IS_NOT_NULL(pBitmapImageFailedEvent);

                    pFrameworkElement = SetupFrameworkElement(pTestImage, pBitmapImage);
                    VERIFY_IS_NOT_NULL(pFrameworkElement);

                    // DecodeToRenderSize can very easily be disabled (by design) if the image source
                    // is set before the UI element is in the live tree
                    if (pTestImage->DecodeToRenderSize)
                    {
                        pRootPanel->Children->Append(pFrameworkElement);
                    }
                    else
                    {
                        SetupSource(pTestImage, pBitmapImage);
                    }
                });
                TestServices::WindowHelper->WaitForIdle();

                RunOnUIThread([&]()
                {
                    if (pTestImage->DecodeToRenderSize)
                    {
                        SetupSource(pTestImage, pBitmapImage);
                    }
                    else
                    {
                        pRootPanel->Children->Append(pFrameworkElement);
                    }
                });
                TestServices::WindowHelper->WaitForIdle();

                if (pTestImage->FailureExpected)
                {
                    LOG_OUTPUT(L"Verifying BitmapImage::ImageFailed event fired [0x%p] on BitmapImage [0x%p]", pBitmapImageFailedEvent.get(), pBitmapImage);
                    pBitmapImageFailedEvent->WaitFor(std::chrono::milliseconds(pTestImage->ImageEventWaitTime));

                    VERIFY_IS_FALSE(pBitmapImageOpenedEvent->HasFired());
                }
                else
                {
                    LOG_OUTPUT(L"Verifying BitmapImage::ImageOpened event fired [0x%p] on BitmapImage [0x%p]", pBitmapImageOpenedEvent.get(), pBitmapImage);
                    pBitmapImageOpenedEvent->WaitFor(std::chrono::milliseconds(pTestImage->ImageEventWaitTime));

                    VERIFY_IS_FALSE(pBitmapImageFailedEvent->HasFired());
                }
                TestServices::WindowHelper->WaitForIdle();

                if (pTestImage->TrimAndRestoreHardwareResources)
                {
                    // This will remove the image from the tree, force a cleanup of the hardware resources
                    // and then add the element back to the tree.  The image should be visible for comparison after.
                    // TODO: This currently only works with very small images like smiley.bmp that can
                    //                 decode quickly.  To support this with larger images, an ETW event waiter needs
                    //                 to be introduced that can wait on a decode operation to complete.

                    RunOnUIThread([&]()
                    {
                        TestServices::Utilities->OverrideTrimImageResourceDelay(true);

                        pRootPanel->Children->Clear();

                        pRootPanel->UpdateLayout();
                    });
                    TestServices::WindowHelper->WaitForIdle();
                    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

                    RunOnUIThread([&]()
                    {
                        pFrameworkElement = SetupFrameworkElement(pTestImage, pBitmapImage);
                        VERIFY_IS_NOT_NULL(pFrameworkElement);

                        pRootPanel->Children->Append(pFrameworkElement);
                        pRootPanel->UpdateLayout();

                        TestServices::Utilities->OverrideTrimImageResourceDelay(false);
                    });
                    TestServices::WindowHelper->WaitForIdle();
                    TestServices::WindowHelper->SynchronouslyTickUIThread(1);
                }
            }

            TestServices::Utilities->SetMockDCompSurfaceIdMode(m_mockDCompSurfaceIdMode);

            TestServices::Utilities->VerifyMockDCompOutput(MockDCompVerification);
        }

        xaml_imaging::BitmapImage^ ImageTestEngine::SetupBitmapImage(
            TestImage^ pTestImage
            )
        {
            xaml_imaging::BitmapImage^ pBitmapImage = ref new xaml_imaging::BitmapImage();

            pBitmapImage->AutoPlay = pTestImage->AutoPlay;

            if (pTestImage->DecodePixelWidth != 0)
            {
                pBitmapImage->DecodePixelWidth = pTestImage->DecodePixelWidth;
            }

            if (pTestImage->DecodePixelHeight != 0)
            {
                pBitmapImage->DecodePixelHeight = pTestImage->DecodePixelHeight;
            }

            if (pTestImage->DecodePixelType != xaml_imaging::DecodePixelType::Physical)
            {
                pBitmapImage->DecodePixelType = pTestImage->DecodePixelType;
            }

            if (pTestImage->BitmapCreateOptions != xaml_imaging::BitmapCreateOptions::None)
            {
                pBitmapImage->CreateOptions = pTestImage->BitmapCreateOptions;
            }

            LOG_OUTPUT(L"BitmapImage Created [0x%p]", pBitmapImage);

            return pBitmapImage;
        }

        xaml::FrameworkElement^ ImageTestEngine::SetupFrameworkElement(
            TestImage^ pTestImage,
            xaml_imaging::BitmapImage^ pBitmapImage
            )
        {
            xaml::FrameworkElement^ pFrameworkElement = nullptr;

            switch (pTestImage->ParentElement)
            {
            case TestImageEnums::ParentElement::Image:
                {
                    xaml_controls::Image^ pImage = ref new xaml_controls::Image();
                    VERIFY_IS_NOT_NULL(pImage);

                    pFrameworkElement = pImage;

                    pImage->Stretch = pTestImage->Stretch;
                    pImage->NineGrid = pTestImage->NineGrid;
                    pImage->Source = pBitmapImage;
                }
                break;
            case TestImageEnums::ParentElement::Border:
                {
                    xaml_controls::Border^ pBorder = ref new xaml_controls::Border();
                    VERIFY_IS_NOT_NULL(pBorder);

                    pFrameworkElement = pBorder;

                    xaml_media::ImageBrush^ pImageBrush = ref new xaml_media::ImageBrush();

                    pImageBrush->Stretch = pTestImage->Stretch;
                    pImageBrush->ImageSource = pBitmapImage;

                    pBorder->Background = pImageBrush;
                }
                break;
            case TestImageEnums::ParentElement::Ellipse:
                {
                    xaml_shapes::Ellipse^ pEllipse = ref new xaml_shapes::Ellipse();
                    VERIFY_IS_NOT_NULL(pEllipse);

                    pFrameworkElement = pEllipse;

                    xaml_media::ImageBrush^ pImageBrush = ref new xaml_media::ImageBrush();

                    pImageBrush->Stretch = pTestImage->Stretch;
                    pImageBrush->ImageSource = pBitmapImage;

                    pEllipse->Fill = pImageBrush;
                }
                break;
            default:
                // Invalid enum, should never happen
                VERIFY_FAIL();
            }

            // Set common properties
            pFrameworkElement->Opacity = pTestImage->Opacity;
            pFrameworkElement->Width = pTestImage->ElementSize.Width;
            pFrameworkElement->Height = pTestImage->ElementSize.Height;

            if (pTestImage->BitmapCache)
            {
                pFrameworkElement->CacheMode = ref new BitmapCache();
            }

            LOG_OUTPUT(L"FrameworkElement [0x%p] created with BitmapImage [0x%p]", pFrameworkElement, pBitmapImage);

            return pFrameworkElement;
        }

        void ImageTestEngine::SetupSource(
            TestImage^ pTestImage,
            xaml_imaging::BitmapSource^ pBitmapSource
            )
        {
            switch (pTestImage->LoadApi)
            {
            case TestImageEnums::LoadApi::SetSource:
                {
                    LOG_OUTPUT(L"Setting synchronous source on BitmapImage [0x%p]", pBitmapSource);

                    // Note: It is very important to use [=] capture-by-value (copy) for lambda
                    //       referencing here so that the CX ^ pointers are reference counted appropriately
                    //       and their lifetime guaranteed across all threads
                    create_task(StorageFile::GetFileFromPathAsync(pTestImage->ImagePath))
                        .then([=](StorageFile^ pFile)
                    {
                        VERIFY_IS_NOT_NULL(pFile);

                        create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                            .then([=](IRandomAccessStream^ pFileStream)
                        {
                            VERIFY_IS_NOT_NULL(pFileStream);

                            RunOnUIThread([=]()
                            {
                                pBitmapSource->SetSource(pFileStream);
                            });
                        });
                    });
                }
                break;
            case TestImageEnums::LoadApi::SetSourceAsync:
            case TestImageEnums::LoadApi::SetSourceAsyncTwice:
                {
                    LOG_OUTPUT(L"Setting asynchronous source on BitmapImage [0x%p]", pBitmapSource);

                    // See above comment about lambda [=] capture
                    create_task(StorageFile::GetFileFromPathAsync(pTestImage->ImagePath))
                        .then([=](StorageFile^ pFile)
                    {
                        VERIFY_IS_NOT_NULL(pFile);

                        create_task(pFile->OpenAsync(::Windows::Storage::FileAccessMode::Read))
                            .then([=](IRandomAccessStream^ pFileStream)
                        {
                            VERIFY_IS_NOT_NULL(pFileStream);

                            RunOnUIThread([=]()
                            {
                                // Todo: Could do some interesting tests with this async object in the future
                                //                 ie/ cancellation
                                if (pTestImage->LoadApi == TestImageEnums::LoadApi::SetSourceAsyncTwice)
                                {
                                    auto pAsyncAction = pBitmapSource->SetSourceAsync(pFileStream->CloneStream());
                                    VERIFY_IS_NOT_NULL(pAsyncAction);
                                }
                                auto pAsyncAction = pBitmapSource->SetSourceAsync(pFileStream);
                                VERIFY_IS_NOT_NULL(pAsyncAction);
                            });
                        });
                    });
                }
                break;
            case TestImageEnums::LoadApi::Uri:
                {
                    LOG_OUTPUT(L"Setting URI source on BitmapImage [0x%p]", pBitmapSource);

                    Uri^ pUri = ref new Uri(pTestImage->ImagePath);
                    BitmapImage^ pBitmapImage = safe_cast<BitmapImage^>(pBitmapSource);

                    VERIFY_IS_NOT_NULL(pBitmapImage);

                    pBitmapImage->UriSource = pUri;
                }
                break;
            default:
                // Invalid enum, should never happen
                VERIFY_FAIL();
            }
        }

    } } }
} } } }
