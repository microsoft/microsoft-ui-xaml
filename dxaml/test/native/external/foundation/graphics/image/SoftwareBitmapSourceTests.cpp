// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "SoftwareBitmapSourceTests.h"
#include <cstdint>
#include <collection.h>
#include <ppltasks.h>
#include <XamlTailored.h>
#include <TestEvent.h>
#include <SafeEventRegistration.h>
#include <robuffer.h>
#include <Windows.Graphics.Imaging.Interop.h>
#include <FileLoader.h>
#include <TestCleanupWrapper.h>
#include <MUX-ETWEvents.h>
#include <ETWWaiterProxy.h>
#include <WUCRenderingScopeGuard.h>
#include <ImageEventWaitingContext.h>

// Privately declare the IClosable interface from windows.foundation.h
// Since this module was compiled with CX, the windows.foundation.h cannot be included without a lot of pain points.
#include "Closable.h"

using namespace Concurrency;
using namespace Microsoft::WRL;
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
using namespace ::Windows::Graphics::Imaging;

using namespace test_infra;
using namespace MockDComp;
using namespace ::Windows::Storage::Streams;

// OPTIMIZE: Some of these tests duplicate effort and can probably be trimmed or added to other test
//                     My only problem with this is that it is hard to describe what an individual test does.
//                     When the tests run in less than a couple seconds, I am not sure if it is worth it.

// TODO: Some of these tests use SynchronouslyTickUIThread to improve test stability to 100% because not
//                 all DComp primitives are submitted after WaitForIdle.  Determine if these are necessary or
//                 remove them and replace with an appropriate wait condition.

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

Platform::String^ SoftwareBitmapSourceTests::GetResourcesPath() const
{
    return GetPackageFolder() + L"resources\\native\\external\\foundation\\graphics\\image\\";
}

bool SoftwareBitmapSourceTests::ClassSetup()
{
    CommonTestSetupHelper::CommonTestClassSetup();
    return true;
}

bool SoftwareBitmapSourceTests::TestSetup()
{
    test_infra::TestServices::WindowHelper->InitializeXaml();
    return true;
}

bool SoftwareBitmapSourceTests::TestCleanup()
{
    test_infra::TestServices::WindowHelper->ShutdownXaml();
    TestServices::WindowHelper->VerifyTestCleanup();
    return true;
}

void SoftwareBitmapSourceTests::LoadBasicSourceBadData()
{
    SoftwareBitmap^ pSoftwareBitmap = nullptr;
    ComPtr<CustomWicBitmap> pCustomWicBitmap = nullptr;

    // Invalid pixel format
    pCustomWicBitmap = new CustomWicBitmap(
        false,
        100,
        100,
        OpaqueRed,
        OpaqueGreen);
    pCustomWicBitmap->OverridePixelFormat(GUID_WICPixelFormat8bppGray);
    pSoftwareBitmap = CreateCustomSoftwareBitmap(pCustomWicBitmap);

    SingleImageTestHelper(true, pSoftwareBitmap, nullptr, true);

    // Pre-multiplied alpha (not supported)
    pCustomWicBitmap = new CustomWicBitmap(
        false,
        100,
        100,
        OpaqueRed,
        OpaqueGreen);
    pCustomWicBitmap->OverridePixelFormat(GUID_WICPixelFormat32bppBGRA);
    pSoftwareBitmap = CreateCustomSoftwareBitmap(pCustomWicBitmap);

    SingleImageTestHelper(true, pSoftwareBitmap, nullptr, true);
}

void SoftwareBitmapSourceTests::Close()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;
    WriteableBitmap^ pWriteableBitmap = nullptr;

    xaml_controls::Image^ pTestImage = nullptr;

    SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(
        false,
        100,
        100,
        OpaqueRed,
        OpaqueGreen);

    RunOnUIThread([&]()
    {
        pTestImage = safe_cast<xaml_controls::Image^>(pRootGrid->FindName(L"imageElement"));

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        pSoftwareBitmapSource = ref new SoftwareBitmapSource();

        create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
            .then([=]()
        {
            LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
            pSoftwareBitmapSourceCompletionEvent->Set();
        });

        pTestImage->Source = pSoftwareBitmapSource;
    });
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    RunOnUIThread([&]()
    {
        ComPtr<IUnknown> spIUnknown(reinterpret_cast<IUnknown*>(pSoftwareBitmapSource));
        ComPtr<ABI::Windows::Foundation::IClosable> spClosable;

        VERIFY_SUCCEEDED(spIUnknown.As(&spClosable));

        VERIFY_SUCCEEDED(spClosable->Close());
    });
    TestServices::WindowHelper->WaitForIdle();

    // Verify the pixel width is 0 to ensure the SoftwareBitmap is closed.
    VERIFY_ARE_EQUAL(pSoftwareBitmap->PixelWidth, 0);

    // Verify the image is no longer visible
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, Platform::StringReference(L"Closed"));

    // Try to set the source again to the same SoftwareBitmap (it should fail because it is Closed)
    pSoftwareBitmapSourceCompletionEvent->Reset();
    RunOnUIThread([&]()
    {
        pTestImage = safe_cast<xaml_controls::Image^>(pRootGrid->FindName(L"imageElement"));

        bool exceptionHit = false;
        try
        {
            pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap);
        }
        catch (Platform::Exception^ e)
        {
            exceptionHit = true;
        }

        VERIFY_IS_TRUE(exceptionHit);
    });
    TestServices::WindowHelper->WaitForIdle();

    // Try to set the source again to a new SoftwareBitmap
    SoftwareBitmap^ pSoftwareBitmap2 = CreateCustomSoftwareBitmap(
        false,
        200,
        200,
        OpaqueGreen,
        OpaqueBlue);

    pSoftwareBitmapSourceCompletionEvent->Reset();
    RunOnUIThread([&]()
    {
        pTestImage = safe_cast<xaml_controls::Image^>(pRootGrid->FindName(L"imageElement"));

        create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap2))
            .then([=]()
        {
            LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
            pSoftwareBitmapSourceCompletionEvent->Set();
        });
    });
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, Platform::StringReference(L"New"));
}

void SoftwareBitmapSourceTests::SetBitmapAsyncNull()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    SetBitmapAsyncNull(true);
}

void SoftwareBitmapSourceTests::SetBitmapAsyncNull2()
{
    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    SetBitmapAsyncNull(false);
}

void SoftwareBitmapSourceTests::SetBitmapAsyncNull(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;
    SoftwareBitmap^ pSoftwareBitmap = nullptr;
    WriteableBitmap^ pWriteableBitmap = nullptr;

    xaml_controls::Image^ pTestImage = nullptr;

    if (isSoftwareBitmap)
    {
        pSoftwareBitmap = CreateCustomSoftwareBitmap(
            false,
            100,
            100,
            OpaqueRed,
            OpaqueGreen);
    }

    RunOnUIThread([&]()
    {
        pTestImage = safe_cast<xaml_controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });

            pTestImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    pSoftwareBitmapSourceCompletionEvent->Reset();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        if (isSoftwareBitmap)
        {
            create_task(pSoftwareBitmapSource->SetBitmapAsync(nullptr))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource null completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            pTestImage->Source = nullptr;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    if (isSoftwareBitmap)
    {
        // Verify the pixel width is still 100 to ensure the software bitmap has not closed.
        VERIFY_ARE_EQUAL(pSoftwareBitmap->PixelWidth, 100);
    }

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageControl()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);
    WriteableBitmap^ pWriteableBitmap = nullptr;
    RunOnUIThread([&]()
    {
        pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
    });

    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    SingleImageTestHelper(true, pSoftwareBitmap, pWriteableBitmap);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    SingleImageTestHelper(false, pSoftwareBitmap, pWriteableBitmap);
}

void SoftwareBitmapSourceTests::LoadImageControlAttachFirst()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageControlAttachFirst(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageControlAttachFirst(false);
}

void SoftwareBitmapSourceTests::LoadImageControlAttachFirst(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);


    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(
                false,
                100,
                100,
                OpaqueBlue,
                OpaqueGreen);

            SoftwareBitmapSource^ pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });

            pTestImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueBlue, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::NonTiledSegmentedCopy()
{
    NonTiledSegmentedCopyInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void SoftwareBitmapSourceTests::NonTiledSegmentedCopyWUCFull()
{
    NonTiledSegmentedCopyInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void SoftwareBitmapSourceTests::NonTiledSegmentedCopyInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup([] ()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 2000, 2000, OpaqueRed, OpaqueBlue);
    WriteableBitmap^ pWriteableBitmap = nullptr;
    RunOnUIThread([&]()
    {
        pWriteableBitmap = CreateWriteableBitmap(2000, 2000, OpaqueRed, OpaqueBlue);
    });

    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    SingleImageTestHelper(true, pSoftwareBitmap, pWriteableBitmap, false /* expectFailure */, MockDComp::SurfaceIdMode::XmlOrder, dcompRendering);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    SingleImageTestHelper(false, pSoftwareBitmap, pWriteableBitmap, false /* expectFailure */, MockDComp::SurfaceIdMode::XmlOrder, dcompRendering);
}

void SoftwareBitmapSourceTests::LoadLargeImage()
{
    LoadLargeImageInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void SoftwareBitmapSourceTests::LoadLargeImageWUCFull()
{
    LoadLargeImageInternal(DCompRendering::WUCCompleteSynchronousCompTree);
}

void SoftwareBitmapSourceTests::LoadLargeImageInternal(DCompRendering dcompRendering)
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    // TODO: WriteableBitmap WUCCompleteSynchronousCompTree variant runs out of memory on phone during BeginDraw if we don't use scoping {} to limit memory usage and
    //                 force resource cleanup.  This is a better way to write a test, but it does indicate that there might be a memory issue for XAML or DComp
    //                 to investigate.
    {
        SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 3000, 3000, OpaqueGreen, OpaqueBlue);

        LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
        SingleImageTestHelper(true, pSoftwareBitmap, nullptr, false /* expectFailure */, MockDComp::SurfaceIdMode::XmlOrder, dcompRendering);
    }

    {
        WriteableBitmap^ pWriteableBitmap = nullptr;
        RunOnUIThread([&] ()
        {
            pWriteableBitmap = CreateWriteableBitmap(3000, 3000, OpaqueGreen, OpaqueBlue);
        });

        LOG_OUTPUT(L"[[WriteableBitmap variation]]");
        SingleImageTestHelper(false, nullptr, pWriteableBitmap, false /* expectFailure */, MockDComp::SurfaceIdMode::XmlOrder, dcompRendering);
    }
}

void SoftwareBitmapSourceTests::LoadTranslucentImage()
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(true, 100, 100, TranslucentRed, TranslucentGreen);
    WriteableBitmap^ pWriteableBitmap = nullptr;
    RunOnUIThread([&]()
    {
        pWriteableBitmap = CreateWriteableBitmap(100, 100, TranslucentRed, TranslucentGreen);
    });

    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    SingleImageTestHelper(true, pSoftwareBitmap, pWriteableBitmap);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    SingleImageTestHelper(false, pSoftwareBitmap, pWriteableBitmap);
}

void SoftwareBitmapSourceTests::BitmapCache()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    BitmapCache(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    BitmapCache(false);
}

void SoftwareBitmapSourceTests::BitmapCache(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

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

    RunOnUIThread([&]()
    {
        SoftwareBitmapSource^ pSoftwareBitmapSource = ref new SoftwareBitmapSource();

        xaml_controls::Image^ pTestImage = ref new xaml_controls::Image();
        pTestImage->Width = 100;
        pTestImage->Height = 100;
        pTestImage->Stretch = Stretch::Fill;
        pTestImage->CacheMode = ref new xaml_media::BitmapCache();

        pRootPanel->Children->Append(pTestImage);

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });

            pTestImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }

    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageBrush()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageBrush(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageBrush(false);
}

void SoftwareBitmapSourceTests::LoadImageBrush(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

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

    RunOnUIThread([&]()
    {
        SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;
        WriteableBitmap^ pWriteableBitmap = nullptr;

        xaml_shapes::Ellipse^ pEllipse = ref new xaml_shapes::Ellipse();
        VERIFY_IS_NOT_NULL(pEllipse);

        xaml_media::ImageBrush^ pImageBrush = ref new xaml_media::ImageBrush();
        pImageBrush->Stretch = Stretch::Uniform;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();
            pImageBrush->ImageSource = pSoftwareBitmapSource;
        }

        pEllipse->Fill = pImageBrush;
        pEllipse->Width = 100;
        pEllipse->Height = 100;

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pImageBrush->ImageSource = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }

        pRootPanel->Children->Append(pEllipse);
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageBrushAttachFirst()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageBrushAttachFirst(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageBrushAttachFirst(false);
}

void SoftwareBitmapSourceTests::LoadImageBrushAttachFirst(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

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

    RunOnUIThread([&]()
    {
        SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

        xaml_shapes::Ellipse^ pEllipse = ref new xaml_shapes::Ellipse();
        VERIFY_IS_NOT_NULL(pEllipse);

        xaml_media::ImageBrush^ pImageBrush = ref new xaml_media::ImageBrush();
        pImageBrush->Stretch = Stretch::Uniform;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();
            pImageBrush->ImageSource = pSoftwareBitmapSource;
        }

        pEllipse->Fill = pImageBrush;
        pEllipse->Width = 100;
        pEllipse->Height = 100;

        pRootPanel->Children->Append(pEllipse);

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pImageBrush->ImageSource = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(2);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadMultipleImageElements()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadMultipleImageElements(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadMultipleImageElements(false);
}

void SoftwareBitmapSourceTests::LoadMultipleImageElements(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

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

    RunOnUIThread([&]()
    {
        // Create all the images and attach them to the panel
        SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

        xaml_controls::Image^ pBigImage = ref new xaml_controls::Image();
        pBigImage->Width = 100;
        pBigImage->Height = 100;
        pBigImage->Stretch = Stretch::Fill;

        xaml_controls::Image^ pMediumImage = ref new xaml_controls::Image();
        pMediumImage->Width = 50;
        pMediumImage->Height = 50;
        pMediumImage->Stretch = Stretch::Fill;

        xaml_controls::Image^ pSmallImage = ref new xaml_controls::Image();
        pSmallImage->Width = 25;
        pSmallImage->Height = 25;
        pSmallImage->Stretch = Stretch::Fill;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();
            pBigImage->Source = pSoftwareBitmapSource;
            pMediumImage->Source = pSoftwareBitmapSource;
            pSmallImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pBigImage->Source = pWriteableBitmap;
            pMediumImage->Source = pWriteableBitmap;
            pSmallImage->Source = pWriteableBitmap;
        }

        pRootPanel->Children->Append(pBigImage);
        pRootPanel->Children->Append(pMediumImage);
        pRootPanel->Children->Append(pSmallImage);

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            // Set all the source content
            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageElementAndBrush()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageElementAndBrush(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageElementAndBrush(false);
}

void SoftwareBitmapSourceTests::LoadImageElementAndBrush(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRedSourceCompletionEvent = std::make_shared<Event>();

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

    RunOnUIThread([&]()
    {
        SoftwareBitmapSource^ pRedSource = nullptr;
        WriteableBitmap^ pWriteableBitmap = nullptr;

        // Add the image element
        xaml_controls::Image^ pImage = ref new xaml_controls::Image();
        pImage->Stretch = Stretch::Fill;
        pImage->Width = 100;
        pImage->Height = 100;
        if (isSoftwareBitmap)
        {
            pRedSource = ref new SoftwareBitmapSource();
            pImage->Source = pRedSource;
        }
        else
        {
            pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pImage->Source = pWriteableBitmap;
        }
        pRootPanel->Children->Append(pImage);

        // Add the ellipse
        xaml_shapes::Ellipse^ pEllipse = ref new xaml_shapes::Ellipse();
        VERIFY_IS_NOT_NULL(pEllipse);

        xaml_media::ImageBrush^ pImageBrush = ref new xaml_media::ImageBrush();
        pImageBrush->Stretch = Stretch::Uniform;

        if (isSoftwareBitmap)
        {
            pImageBrush->ImageSource = pRedSource;
        }
        else
        {
            pImageBrush->ImageSource = pWriteableBitmap;
        }

        pEllipse->Fill = pImageBrush;
        pEllipse->Width = 100;
        pEllipse->Height = 100;

        pRootPanel->Children->Append(pEllipse);

        if (isSoftwareBitmap)
        {
            SoftwareBitmap^ pRedSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            // Set all the source content
            create_task(pRedSource->SetBitmapAsync(pRedSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"Red source completion fired [0x%p]", pRedSource);
                pRedSourceCompletionEvent->Set();
            });
        }
        else
        {
            pRedSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pRedSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageTwiceNoWait()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageTwiceNoWait(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageTwiceNoWait(false);
}

void SoftwareBitmapSourceTests::LoadImageTwiceNoWait(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            SoftwareBitmapSource^ pSoftwareBitmapSource = ref new SoftwareBitmapSource();
            pTestImage->Source = pSoftwareBitmapSource;

            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            // Call SetBitmapAsync twice back to back.
            pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            WriteWriteableBitmap(pWriteableBitmap, 100, 100, OpaqueRed, OpaqueGreen);
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LoadImageTwiceWithWait()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LoadImageTwiceWithWait(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LoadImageTwiceWithWait(false);
}

void SoftwareBitmapSourceTests::LoadImageTwiceWithWait(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    SoftwareBitmap^ pSoftwareBitmap = nullptr;
    WriteableBitmap^ pWriteableBitmap = nullptr;

    if (isSoftwareBitmap)
    {
        pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);
    }

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            pTestImage->Source = pSoftwareBitmapSource;

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    pSoftwareBitmapSourceCompletionEvent->Reset();

    RunOnUIThread([&]()
    {
        if (isSoftwareBitmap)
        {
            // Set the source async again to the same bitmap after the previous async completion fired
            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            WriteWriteableBitmap(pWriteableBitmap, 100, 100, OpaqueRed, OpaqueGreen);
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::LeaveTreeDataRestore()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    LeaveTreeDataRestore(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    LeaveTreeDataRestore(false);
}

void SoftwareBitmapSourceTests::LeaveTreeDataRestore(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;
    WriteableBitmap^ pWriteableBitmap = nullptr;

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            pTestImage->Source = pSoftwareBitmapSource;

            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));

        // Set the source to null and immediately back to bitmapImage
        pTestImage->Source = nullptr;

        TestServices::Utilities->OverrideTrimImageResourceDelay(true);

        pRootGrid->UpdateLayout();
    });

    // Give it a chance to run layout and clean up the memory
    TestServices::WindowHelper->WaitForIdle();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));

        // Set the bitmap image back to ensure it can reload
        if (isSoftwareBitmap)
        {
            pTestImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            pTestImage->Source = pWriteableBitmap;
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::DeviceLost()
{
    LOG_OUTPUT(L"[[SoftwareBitmapSource variation]]");
    DeviceLost(true);

    LOG_OUTPUT(L"[[WriteableBitmap variation]]");
    DeviceLost(false);
}

void SoftwareBitmapSourceTests::DeviceLost(bool isSoftwareBitmap)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            pTestImage->Source = pSoftwareBitmapSource;

            SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(false, 100, 100, OpaqueRed, OpaqueGreen);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([=]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        }
        else
        {
            WriteableBitmap^ pWriteableBitmap = CreateWriteableBitmap(100, 100, OpaqueRed, OpaqueGreen);
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

// Covers this specific scenario:
// 1. Use an image control with an SBS (this will cause it to decode to hardware and throwaway the software)
// 2. Remove the image control and trim the hardware resources.
// 3. Reload the image in an ellipse so it must load software and hardware resources.
// This causes the image to reload the content for software rendering which wasn't working properly because
// the hardware resources weren't available.
void SoftwareBitmapSourceTests::TFS_6246592()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    xaml_controls::Panel^ rootPanel = nullptr;
    xaml_imaging::SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

    RunOnUIThread([&]()
    {
        rootPanel = safe_cast<xaml_controls::Panel^> (
            xaml_markup::XamlReader::Load(
                L"<StackPanel xmlns='http://schemas.microsoft.com/winfx/2006/xaml/presentation'"
                L"            xmlns:x='http://schemas.microsoft.com/winfx/2006/xaml'"
                L"            HorizontalAlignment='Left'"
                L"            VerticalAlignment='Top'"
                L"            Orientation='Horizontal'/>"
            ));
        VERIFY_IS_NOT_NULL(rootPanel);

        TestServices::WindowHelper->WindowContent = rootPanel;
    });
    TestServices::WindowHelper->WaitForIdle();

    RunOnUIThread([&]()
    {
        pSoftwareBitmapSource = ref new SoftwareBitmapSource();

        auto image = ref new xaml_controls::Image();
        image->Source = pSoftwareBitmapSource;
        image->Stretch = Stretch::Uniform;
        image->Width = 100;
        image->Height = 100;

        SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(
            false,
            100,
            100,
            OpaqueRed,
            OpaqueGreen);

        create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
            .then([=]()
        {
            LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
            pSoftwareBitmapSourceCompletionEvent->Set();
        });

        rootPanel->Children->Append(image);
    });
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->WaitForIdle();

    // Remove the element and ensure the hardware resources are cleaned up via OverrideTrimImageResourceDelay
    RunOnUIThread([&]()
    {
        TestServices::Utilities->OverrideTrimImageResourceDelay(true);

        rootPanel->Children->Clear();
        rootPanel->UpdateLayout();
    });
    TestServices::WindowHelper->WaitForIdle();

    // Put the image back in as an ellipse so that it reloads the software surface
    ETWWaiterProxy imageEtwWaiter;

    imageEtwWaiter.Start(
        WINDOWS_UI_XAML_ETW_PROVIDER,
        ImageUpdateHardwareResourcesEnd_value);

    RunOnUIThread([&]()
    {
        auto ellipse = ref new xaml_shapes::Ellipse();

        xaml_media::ImageBrush^ imageBrush = ref new xaml_media::ImageBrush();
        imageBrush->Stretch = Stretch::Uniform;

        imageBrush->ImageSource = pSoftwareBitmapSource;

        ellipse->Fill = imageBrush;
        ellipse->Width = 100;
        ellipse->Height = 100;

        rootPanel->Children->Append(ellipse);
    });
    TestServices::WindowHelper->WaitForIdle();
    imageEtwWaiter.WaitForDefault();

    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

void SoftwareBitmapSourceTests::WriteableBitmapSetSource()
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    WriteableBitmap^ writeableBitmap;

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(rootGrid);

    LOG_OUTPUT(L"Getting stream of image");
    wsts::IRandomAccessStream^ bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.jpg");

    LOG_OUTPUT(L"Creating custom BitmapSource with SetStream");
    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        writeableBitmap = ref new xaml_imaging::WriteableBitmap(266, 150);
        VERIFY_IS_NOT_NULL(writeableBitmap);
        writeableBitmap->SetSource(bitmapStream);

        auto testImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(rootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(testImage);
        testImage->Stretch = Stretch::Fill;
        testImage->Source = writeableBitmap;
    });

    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource");

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource");

    LOG_OUTPUT(L"Overwrite pixels from SetSource with pixels from Buffer");
    RunOnUIThread([&]()
    {
        WriteWriteableBitmap(writeableBitmap, 266, 150, OpaqueRed, OpaqueGreen);
        pSoftwareBitmapSourceCompletionEvent->Set();
    });
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    pSoftwareBitmapSourceCompletionEvent->Reset();
    TestServices::WindowHelper->WaitForIdle();
    // The surface from SetStream has id 2, and the surface from Buffer has id 3. After we go through device lost,
    // we toss both surfaces and re-create the surface from Buffer, which will now have id 2. This means we can't
    // use the same baseline file for before/after device lost.
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"Buffer");

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"BufferDeviceLost");

    LOG_OUTPUT(L"Overwrite pixels from Buffer with different pixels from Buffer");
    RunOnUIThread([&]()
    {
        WriteWriteableBitmap(writeableBitmap, 266, 150, OpaqueGreen, OpaqueBlue);
        pSoftwareBitmapSourceCompletionEvent->Set();
    });
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    pSoftwareBitmapSourceCompletionEvent->Reset();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"Buffer2");

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"Buffer2");

    LOG_OUTPUT(L"Getting stream of image");
    bitmapStream = LoadBinaryFile(GetResourcesPath() + L"barcelona.jpg");

    LOG_OUTPUT(L"Overwrite different pixels from Buffer with pixels from SetSource");
    RunOnUIThread([&]()
    {
        writeableBitmap->SetSource(bitmapStream);
    });
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource2");

    LOG_OUTPUT(L"Simulating device lost");
    TestServices::WindowHelper->SimulateDeviceLost();
    TestServices::WindowHelper->WaitForIdle();
    TestServices::Utilities->ResetMockDCompSurfaceId();
    TestServices::Utilities->SetMockDCompSurfaceIdMode(MockDComp::SurfaceIdMode::XmlOrder);
    TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly, L"SetSource");
}

// Covers this specific scenario:
// 1. Load an image that is very large and will be broken into a lot of tiles.
// 2. During the asynchronous decode, get the UI thread to flush texture updates.
//    With an RS2 bug, this will result in the staging surface being released early
//    and the asynchronous decoding will queue an update with that null staging texture
//    which will result in a crash
//
// This the SoftwareBitmapSource version of the test.
void SoftwareBitmapSourceTests::TFS_9742148()
{
    // Test must run with SpriteVisuals enabled in order to use virtual surface tiling to reproduce
    // the issue.
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    auto rootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));

    SoftwareBitmap^ softwareBitmap = CreateCustomSoftwareBitmap(false, 3000, 3000, OpaqueGreen, OpaqueBlue);
    auto sbsCompletionEvent = std::make_shared<Event>();

    RunOnUIThread([&] ()
    {
        TestServices::WindowHelper->WindowContent = rootGrid;

        auto image = safe_cast<xaml_controls::Image^>(rootGrid->FindName(L"imageElement"));

        auto sbs = ref new SoftwareBitmapSource();
        image->Source = sbs;

        create_task(sbs->SetBitmapAsync(softwareBitmap))
            .then([=] ()
        {
            LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", sbs);
            sbsCompletionEvent->Set();
        });
    });

    // Intentionally tick the UI thread frequently to get it to flush updates in the middle of
    // decoding.  Do this until the image is opened at which point it doesn't need to be
    // ticked anymore since the point was to cause a race condition between both threads.
    auto maxIterations = 100;
    while ((maxIterations > 0) && !sbsCompletionEvent->HasFired())
    {
        TestServices::WindowHelper->SynchronouslyTickUIThread(1);
        maxIterations--;
    }

    VERIFY_IS_TRUE(sbsCompletionEvent->HasFired());
}

void SoftwareBitmapSourceTests::SingleImageTestHelper(
    bool isSoftwareBitmap,
    SoftwareBitmap^ pTestBitmap,
    WriteableBitmap^ pWriteableBitmap,
    bool expectFailure,
    MockDComp::SurfaceIdMode surfaceIdMode,
    DCompRendering dcompRendering
    )
{
    WUCRenderingScopeGuard guard(dcompRendering);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;
    });
    TestServices::WindowHelper->WaitForIdle();

    auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

    RunOnUIThread([&]()
    {
        auto pTestImage = safe_cast<Microsoft::UI::Xaml::Controls::Image^>(pRootGrid->FindName(L"imageElement"));
        VERIFY_IS_NOT_NULL(pTestImage);

        pTestImage->Stretch = Stretch::Fill;
        pTestImage->Width = 100;
        pTestImage->Height = 100;

        if (isSoftwareBitmap)
        {
            SoftwareBitmapSource^ pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            if (expectFailure)
            {
                VERIFY_THROWS_WINRT(pSoftwareBitmapSource->SetBitmapAsync(pTestBitmap), Platform::InvalidArgumentException^);
                pSoftwareBitmapSourceCompletionEvent->Set();
            }
            else
            {
                create_task(pSoftwareBitmapSource->SetBitmapAsync(pTestBitmap))
                    .then([=]()
                {
                    LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                    pSoftwareBitmapSourceCompletionEvent->Set();
                });
            }

            pTestImage->Source = pSoftwareBitmapSource;
        }
        else
        {
            pTestImage->Source = pWriteableBitmap;
            pSoftwareBitmapSourceCompletionEvent->Set();
        }
    });
    TestServices::WindowHelper->WaitForIdle();
    pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
    TestServices::WindowHelper->SynchronouslyTickUIThread(1);

    if (!expectFailure)
    {
        TestServices::WindowHelper->SynchronouslyTickUIThread(1);

        TestServices::Utilities->SetMockDCompSurfaceIdMode(surfaceIdMode);
        TestServices::Utilities->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
    }
}

// Stress tests Allocating, displaying in the visual tree and releasing all resources
void SoftwareBitmapSourceTests::AllocateDisplayReleaseStress(int iterationCount)
{
    TestCleanupWrapper cleanup([]()
    {
        TestServices::WindowHelper->ResetWindowContentAndWaitForIdle();
    });

    ::Windows::Foundation::Size size(400, 300);
    TestServices::WindowHelper->SetWindowSizeOverride(size);

    auto pRootGrid = safe_cast<Grid^>(LoadXamlFileOnUIThread(GetResourcesPath() + L"SimpleImage.xaml"));
    VERIFY_IS_NOT_NULL(pRootGrid);

    xaml_controls::Image^ pTestImage = nullptr;

    RunOnUIThread([&]()
    {
        TestServices::WindowHelper->WindowContent = pRootGrid;

        pTestImage = safe_cast<xaml_controls::Image^>(pRootGrid->FindName(L"imageElement"));
    });
    TestServices::WindowHelper->WaitForIdle();

    for (int i = 0; i < iterationCount; i++)
    {
        SoftwareBitmap^ pSoftwareBitmap = CreateCustomSoftwareBitmap(
            false,
            1000,
            1000,
            OpaqueRed,
            OpaqueGreen);

        LOG_OUTPUT(L"SoftwareBitmap created [0x%p]", pSoftwareBitmap);

        SoftwareBitmapSource^ pSoftwareBitmapSource = nullptr;

        auto pSoftwareBitmapSourceCompletionEvent = std::make_shared<Event>();

        RunOnUIThread([&]()
        {
            pSoftwareBitmapSource = ref new SoftwareBitmapSource();

            LOG_OUTPUT(L"SoftwareBitmapSource created [0x%p]", pSoftwareBitmapSource);

            pTestImage->Source = pSoftwareBitmapSource;

            LOG_OUTPUT(L"SoftwareBitmapSource [0x%p] assigned to Image element [0x%p]",
                pSoftwareBitmapSource,
                pTestImage);

            LOG_OUTPUT(L"SoftwareBitmap [0x%p] set on SoftwareBitmapSource [0x%p]",
                pSoftwareBitmap,
                pSoftwareBitmapSource);

            create_task(pSoftwareBitmapSource->SetBitmapAsync(pSoftwareBitmap))
                .then([&pSoftwareBitmapSourceCompletionEvent, &pSoftwareBitmapSource]()
            {
                LOG_OUTPUT(L"SoftwareBitmapSource completion fired [0x%p]", pSoftwareBitmapSource);
                pSoftwareBitmapSourceCompletionEvent->Set();
            });
        });
        pSoftwareBitmapSourceCompletionEvent->WaitForDefault();
        TestServices::WindowHelper->WaitForIdle();

        RunOnUIThread([&]()
        {
            LOG_OUTPUT(L"Image element [0x%p] set to null", pTestImage);
            pTestImage->Source = nullptr;

            LOG_OUTPUT(L"SoftwareBitmapSource [0x%p] set to null", pSoftwareBitmapSource);
            pSoftwareBitmapSource = nullptr;
        });
        TestServices::WindowHelper->WaitForIdle();

        LOG_OUTPUT(L"SoftwareBitmap [0x%p] set to null", pSoftwareBitmap);
        pSoftwareBitmap = nullptr;
    }
}

::Windows::Graphics::Imaging::SoftwareBitmap^ SoftwareBitmapSourceTests::CreateCustomSoftwareBitmap(
    Microsoft::WRL::ComPtr<CustomWicBitmap> customWicBitmap
    )
{
    SoftwareBitmap^ pSoftwareBitmap = nullptr;

    ComPtr<ISoftwareBitmapNativeFactory> factoryNative;
    VERIFY_SUCCEEDED(CoCreateInstance(CLSID_SoftwareBitmapNativeFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factoryNative)));

    // This may seem a bit strange, but ISoftwareBitmap cannot be declared as a ComPtr with CX
    // enabled, it must be declared as ISoftwareBitmap^.
    VERIFY_SUCCEEDED(factoryNative->CreateFromWICBitmap(
        customWicBitmap.Get(),
        false,
        __uuidof(SoftwareBitmap),
        reinterpret_cast<void**>(&pSoftwareBitmap)));

    return pSoftwareBitmap;
}

::Windows::Graphics::Imaging::SoftwareBitmap^ SoftwareBitmapSourceTests::CreateCustomSoftwareBitmap(
    bool supportsTransparency,
    unsigned int width,
    unsigned int height,
    uint32_t colorValue1,
    uint32_t colorValue2
    )
{
    return CreateCustomSoftwareBitmap(
        new CustomWicBitmap(supportsTransparency, width, height, colorValue1, colorValue2));
}

xaml_imaging::WriteableBitmap^ SoftwareBitmapSourceTests::CreateWriteableBitmap(
    unsigned int width,
    unsigned int height,
    uint32_t colorValue1,
    uint32_t colorValue2
    )
{
    xaml_imaging::WriteableBitmap^ bitmap = ref new xaml_imaging::WriteableBitmap(width, height);
    WriteWriteableBitmap(bitmap, width, height, colorValue1, colorValue2);
    return bitmap;
}

void SoftwareBitmapSourceTests::WriteWriteableBitmap(
    xaml_imaging::WriteableBitmap^ bitmap,
    unsigned int width,
    unsigned int height,
    uint32_t colorValue1,
    uint32_t colorValue2
    )
{
    ComPtr<IUnknown> pBuffer(reinterpret_cast<IUnknown*>(bitmap->PixelBuffer));

    ComPtr<IBufferByteAccess> pBufferByteAccess;
    pBuffer.As(&pBufferByteAccess);

    byte* pPixel;
    pBufferByteAccess->Buffer(&pPixel);

    CustomWicBitmap::InitializeWithGradientPattern(
        reinterpret_cast<uint32_t*>(pPixel),
        width,
        height,
        colorValue1,
        colorValue2
        );

    bitmap->Invalidate();
}

void SoftwareBitmapSourceTests::SetBitmapAsync_DeviceLost()
{
    LOG_OUTPUT(L"> Variation with delay before inserting SoftwareBitmapSource into tree");
    SetBitmapAsync_DeviceLostHelper(true);

    LOG_OUTPUT(L"> Variation that immediately puts SoftwareBitmapSource into tree");
    SetBitmapAsync_DeviceLostHelper(false);
}

void SoftwareBitmapSourceTests::SetBitmapAsync_DeviceLostHelper(bool delay)
{
    WUCRenderingScopeGuard guard(DCompRendering::WUCCompleteSynchronousCompTree);

    const auto& wh = TestServices::WindowHelper;
    const auto& u = TestServices::Utilities;

    wh->SetWindowSizeOverrideWithScale(wf::Size(400, 300), 1.0f);

    auto softwareBitmapSourceCompletionEvent = std::make_shared<Event>();
    SoftwareBitmap^ softwareBitmap = CreateCustomSoftwareBitmap(
        false,
        100,
        100,
        OpaqueRed,
        OpaqueGreen);

    // Prime Xaml to return a device lost error when we do an off-thread upload
    u->SimulateDeviceLostOnOffThreadImageUpload();

    Canvas^ canvas;

    RunOnUIThread([&]()
    {
        SoftwareBitmapSource^ softwareBitmapSource = ref new SoftwareBitmapSource();

        LOG_OUTPUT(L"> SoftwareBitmapSource.SetBitmapAsync");
        auto setBitmapAsyncTask = create_task(softwareBitmapSource->SetBitmapAsync(softwareBitmap));

        setBitmapAsyncTask
            .then([=](task<void> result)
        {
            try
            {
                result.get();
                LOG_OUTPUT(L"  > SoftwareBitmapSource completion succeeded");
                softwareBitmapSourceCompletionEvent->Set();
            }
            catch (Platform::COMException^ e)
            {
                LOG_OUTPUT(L"  > SoftwareBitmapSource failed!");
                LOG_OUTPUT(L"    %s", e->Message->Data());
            }
        });

        xaml_controls::Image^ image = ref new xaml_controls::Image();
        image->Stretch = Stretch::Fill;
        image->Width = 100;
        image->Height = 100;
        image->Source = softwareBitmapSource;

        canvas = ref new Canvas();
        canvas->Children->Append(image);
        if (!delay)
        {
            wh->WindowContent = canvas;
        }
    });

    if (delay)
    {
        wh->WaitForIdle();
        TestServices::WindowHelper->SynchronouslyTickUIThread(2);

        RunOnUIThread([&]()
        {
            wh->WindowContent = canvas;
        });
    }

    wh->WaitForIdle();
    softwareBitmapSourceCompletionEvent->WaitForDefault();
    softwareBitmapSourceCompletionEvent->Reset();

    u->VerifyMockDCompOutput(MockDComp::SurfaceComparison::ReferencedOnly);
}

} } } } } } }
