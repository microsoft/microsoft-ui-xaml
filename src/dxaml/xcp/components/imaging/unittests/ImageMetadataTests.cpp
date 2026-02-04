// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageMetadataTests.h"
#include "ImageTestHelper.h"
#include <CoInitHelper.h>

#include <ImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageMetadataView.h>
#include <ImageTaskDispatcher.h>
#include <ImageViewListener.h>


using namespace ::Windows::UI::Xaml::Tests::Foundation::Imaging;
using namespace ImageTestHelper;


bool ImageMetadataTests::TestSetup()
{
    CoInitHelper::EnsureCoInitialized();
    m_dispatcher = make_xref<ImageTaskDispatcher>(nullptr /* core */);
    m_decodeParams16x16 = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 16, 16, false /* isLoadedImageSurface */, 0 /* imageId */, xstring_ptr::EmptyString());
    return true;
}

bool ImageMetadataTests::TestCleanup()
{
    m_dispatcher.reset();
    m_decodeParams16x16.reset();
    return true;
}

xref_ptr<ImageCache> ImageMetadataTests::MakeImageCache()
{
    auto imageCache = make_xref<ImageCache>(
        xstring_ptr::NullString() /* strCacheKey */,
        xstring_ptr::NullString() /* strUri */,
        nullptr /* absoluteUri */,
        false /* isSvg */,
        nullptr /* core */,
        m_dispatcher,
        true /* ignoreNetworkCache */,
        nullptr /* invalidateCallback */);
    return imageCache;
}

void ImageMetadataTests::GetMetadataViewNoData()
{
    auto imageCache = MakeImageCache();
    VERIFY_IS_NOT_NULL(imageCache->GetMetadataView(0));
    VERIFY_IS_NULL(imageCache->GetMetadataView(0)->GetImageMetadata());
}

void ImageMetadataTests::GetMetadataAfterDownload()
{
    auto imageCache = MakeImageCache();

    auto fileName = GetImageResourcesPath() + L"Smiley.bmp";
    auto encodedImageData = GetFileEncodedData(fileName, false /* parse */);
    imageCache->SetEncodedImageData(encodedImageData);

    VERIFY_IS_NOT_NULL(imageCache->GetMetadataView(0));
    VERIFY_IS_NOT_NULL(imageCache->GetMetadataView(0)->GetImageMetadata());
}

void ImageMetadataTests::DownloadAddsMetadata()
{
    struct TestListener : public IImageViewListener
    {
        int imageViewUpdatedCount = 0;
        ImageViewBase* lastSender = nullptr;

        _Check_return_ HRESULT OnImageViewUpdated(ImageViewBase& sender) override
        {
            lastSender = &sender;
            imageViewUpdatedCount++;
            return S_OK;
        }
    };

    auto imageCache = MakeImageCache();
    auto metadataView = imageCache->GetMetadataView(0);

    TestListener testListener;
    metadataView->AddImageViewListener(testListener);

    VERIFY_ARE_EQUAL(0, testListener.imageViewUpdatedCount, L"Expect no callback during AddImageViewListener");

    auto fileName = GetImageResourcesPath() + L"Smiley.bmp";
    auto encodedImageData = GetFileEncodedData(fileName, false /* parse */);
    imageCache->SetEncodedImageData(encodedImageData);

    VERIFY_ARE_EQUAL(1, testListener.imageViewUpdatedCount);
    VERIFY_ARE_EQUAL(static_cast<ImageViewBase*>(metadataView.get()), testListener.lastSender);

    metadataView->RemoveImageViewListener(testListener);

    VERIFY_ARE_EQUAL(1, testListener.imageViewUpdatedCount, L"Expect no callback during RemoveImageViewListener");

    VERIFY_IS_NOT_NULL(metadataView->GetImageMetadata());
}
