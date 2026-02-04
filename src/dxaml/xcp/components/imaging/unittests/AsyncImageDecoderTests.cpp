// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// tests
#include "ImageTestHelper.h"
#include "AsyncImageDecoderTests.h"
#include <TestEvent.h>
#include <CoInitHelper.h>

// imaging
#include "AsyncImageDecoder.h"
#include "WicService.h"
#include "EncodedImageData.h"
#include "ImageDecodeParams.h"
#include "ImageProviderInterfaces.h"
#include "imaginginterfaces.h"
#include "OfferableSoftwareBitmap.h"

// base
#include <weakref_ptr.h>

class TestImageDecodeCallback : public IImageDecodeCallback
{
public:
    std::function<HRESULT(const xref_ptr<IImageAvailableResponse>& spResponse)> onDecode;

    // IImageDecodeCallback
    _Check_return_ HRESULT OnDecode(
        _In_ xref_ptr<IImageAvailableResponse> spResponse,
        _In_ uint64_t requestId
        ) override
    {
        return onDecode(spResponse);
    }
};


using namespace ImageTestHelper;

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

bool AsyncImageDecoderTests::ClassSetup()
{
    CoInitHelper::EnsureCoInitialized();
    return true;
}

void AsyncImageDecoderTests::InitialState()
{
    auto fileName = GetImageResourcesPath() + L"Smiley.bmp";
    auto spEncodedImageData = GetFileEncodedData(fileName);

    auto spAsyncDecoder = std::make_unique<AsyncImageDecoder>(
        WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata()),
        std::move(spEncodedImageData),
        false /* isAutoPlay */,
        xref::get_weakref<IImageDecodeCallback>(nullptr));

    spAsyncDecoder->CleanupDeviceRelatedResources();
}

void AsyncImageDecoderTests::ChangeDecodeSizeStatic()
{
    auto fileName = GetImageResourcesPath() + L"Smiley.bmp";
    auto spEncodedImageData = GetFileEncodedData(fileName);
    auto spDecodeCallback = make_xref<TestImageDecodeCallback>();

    auto onDecodeEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    xref_ptr<OfferableSoftwareBitmap> decodedSurface;

    spDecodeCallback->onDecode = [onDecodeEvent,&decodedSurface](const xref_ptr<IImageAvailableResponse>& spResponse)
    {
        VERIFY_SUCCEEDED(spResponse->GetDecodingResult());
        VERIFY_ARE_NOT_EQUAL(nullptr, spResponse->GetSurface());
        decodedSurface = spResponse->GetSurface();
        onDecodeEvent->Set();
        return S_OK;
    };

    auto spAsyncDecoder = std::make_unique<AsyncImageDecoder>(
        WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata()),
        std::move(spEncodedImageData),
        false /* isAutoPlay */,
        xref::get_weakref(spDecodeCallback));

    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeStatic_natural",
        0x3fa918f5));

    onDecodeEvent->Reset();

    // expect the new decode event after each SetDecodeParams
    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 16, 16, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeStatic_16x16",
        0xb60cf5af));
}

void AsyncImageDecoderTests::ChangeDecodeSizeAnimatedPlaying()
{
    // We need to replace decode params between the first and the second frames so that the second frame is decoded into the new size.
    // This test is time sensitive since there is no wait to tell the decoder to wait after the first frame.
    // We assume the delay between frames is long enough to reliably validate the first frame and then update the decode params.

    auto fileName = GetImageResourcesPath() + L"animatedgif\\08bS4.gif";
    auto spEncodedImageData = GetFileEncodedData(fileName);
    auto spDecodeCallback = make_xref<TestImageDecodeCallback>();

    auto onDecodeEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    xref_ptr<OfferableSoftwareBitmap> decodedSurface;

    spDecodeCallback->onDecode = [onDecodeEvent, &decodedSurface](const xref_ptr<IImageAvailableResponse>& spResponse)
    {
        VERIFY_SUCCEEDED(spResponse->GetDecodingResult());
        VERIFY_ARE_NOT_EQUAL(nullptr, spResponse->GetSurface());
        decodedSurface = spResponse->GetSurface();
        onDecodeEvent->Set();
        return S_OK;
    };

    auto spAsyncDecoder = std::make_unique<AsyncImageDecoder>(
        WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata()),
        std::move(spEncodedImageData),
        true /* isAutoPlay */,
        xref::get_weakref(spDecodeCallback));

    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeAnimatedPlaying_natural",
        0xfea6ba63));

    onDecodeEvent->Reset();

    // expect the new decode event after each SetDecodeParams
    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 16, 16, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));

    // Expect the scaled down version of the second frame
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeAnimatedPlaying_16x16",
        0x156755d3));
}

void AsyncImageDecoderTests::ChangeDecodeSizeAnimatedStopped()
{
    auto fileName = GetImageResourcesPath() + L"animatedgif\\08bS4.gif";
    auto spEncodedImageData = GetFileEncodedData(fileName);
    auto spDecodeCallback = make_xref<TestImageDecodeCallback>();

    auto onDecodeEvent = std::make_shared<Microsoft::UI::Xaml::Tests::Common::Event>();
    xref_ptr<OfferableSoftwareBitmap> decodedSurface;

    spDecodeCallback->onDecode = [onDecodeEvent, &decodedSurface](const xref_ptr<IImageAvailableResponse>& spResponse)
    {
        VERIFY_SUCCEEDED(spResponse->GetDecodingResult());
        VERIFY_ARE_NOT_EQUAL(nullptr, spResponse->GetSurface());
        decodedSurface = spResponse->GetSurface();
        onDecodeEvent->Set();
        return S_OK;
    };

    auto spAsyncDecoder = std::make_unique<AsyncImageDecoder>(
        WicService::GetInstance().CreateDefaultDecoder(spEncodedImageData->GetMetadata()),
        std::move(spEncodedImageData),
        false /* isAutoPlay */,
        xref::get_weakref(spDecodeCallback));

    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 0, 0, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeAnimatedStopped_natural",
        0xfea6ba63));

    onDecodeEvent->Reset();

    // expect the new decode event after each SetDecodeParams
    VERIFY_SUCCEEDED(spAsyncDecoder->SetDecodeParams(make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 16, 16, false, 0 /* imageId */, xstring_ptr::EmptyString()), 0));

    // Note that we should receive the scaled down version of the first frame since the decoder is in the stopped state.
    onDecodeEvent->WaitForDefault();
    VERIFY_IS_TRUE(ValidateImageCrc(
        decodedSurface,
        L"AsyncImageDecoderTests_ChangeDecodeSizeAnimatedStopped_16x16",
        0x1d9542f8));
}


} } } } } }
