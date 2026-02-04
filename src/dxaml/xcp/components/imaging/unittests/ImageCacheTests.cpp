// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageCacheTests.h"
#include "ImageTestHelper.h"
#include <CoInitHelper.h>

#include <ImageCache.h>
#include <ImageDecodeParams.h>
#include <ImageTaskDispatcher.h>
#include <imaginginterfaces.h>
#include <PixelFormat.h>

using namespace ::Windows::UI::Xaml::Tests::Foundation::Imaging;
using namespace ImageTestHelper;

namespace
{
    struct ImageAvailableTestCallback : public CXcpObjectBase<IImageAvailableCallback>
    {
        bool callbackCalled = false;

        std::function<HRESULT(IImageAvailableResponse*)> onImageAvailable;
        _Check_return_ HRESULT OnImageAvailable(_In_ IImageAvailableResponse* response) override
        {
            callbackCalled = true;
            return onImageAvailable ? onImageAvailable(response) : S_OK;
        }
    };
}

static void WaitForDefault(_In_ IPALExecuteOnUIThread *executeOnUIThread, ImageAvailableTestCallback *cb1, ImageAvailableTestCallback *cb2 = nullptr)
{
    cb1->callbackCalled = false;
    if (cb2 != nullptr)
    {
        cb2->callbackCalled = false;
    }

    // TODO: Use event based waiting instead of polling. Current values 500*10 are chosen to limit the wait
    //       time to 5 seconds similar to Tests::Common::Event::WaitForDefault
    for(int i = 0; i < 500; ++i)
    {
        HRESULT executeHR = executeOnUIThread->Execute();
        if (FAILED(executeHR)) // to avoid noise in output
        {
            VERIFY_SUCCEEDED(executeHR);
        }

        if (!cb1->callbackCalled ||
            (cb2 && !cb2->callbackCalled))
        {
            Sleep(10);
            continue;
        }

        return; // all callbacks were called
    }
    WEX::Common::Throw::Exception(E_FAIL, L"Timed out or failed to wait for the event");
}

bool ImageCacheTests::TestSetup()
{
    CoInitHelper::EnsureCoInitialized();
    m_dispatcher = make_xref<ImageTaskDispatcher>(nullptr /* core */);
    m_decodeParams16x16 = make_xref<ImageDecodeParams>(pixelColor32bpp_A8R8G8B8, 16, 16, false /* isLoadedImageSurface */, 0 /* imageId */, xstring_ptr::EmptyString());
    return true;
}

bool ImageCacheTests::TestCleanup()
{
    m_dispatcher.reset();
    m_decodeParams16x16.reset();
    return true;
}

xref_ptr<ImageCache> ImageCacheTests::MakeImageCache()
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

    auto fileName = GetImageResourcesPath() + L"Smiley.bmp";
    auto encodedImageData = GetFileEncodedData(fileName);

    imageCache->SetEncodedImageData(encodedImageData);

    return imageCache;
}

void ImageCacheTests::DecodeOne()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    auto callback = make_xref<ImageAvailableTestCallback>();
    callback->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        VERIFY_SUCCEEDED(response->GetDecodingResult());
        return S_OK;
    };

    xref_ptr<IAbortableImageOperation> abortableImageOperation;
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback, abortableImageOperation));

    WaitForDefault(m_dispatcher, callback);
}

void ImageCacheTests::DecodeTwo()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    // first
    xref_ptr<IAbortableImageOperation> abortableImageOperation1;
    auto callback1 = make_xref<ImageAvailableTestCallback>();
    callback1->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        VERIFY_SUCCEEDED(response->GetDecodingResult());
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

    // second
    xref_ptr<IAbortableImageOperation> abortableImageOperation2;
    auto callback2 = make_xref<ImageAvailableTestCallback>();
    callback2->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        VERIFY_SUCCEEDED(response->GetDecodingResult());
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

    // wait both
    WaitForDefault(m_dispatcher, callback1, callback2);
}

void ImageCacheTests::DecodeTwoCancelFirst()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    // first
    xref_ptr<IAbortableImageOperation> abortableImageOperation1;
    auto callback1 = make_xref<ImageAvailableTestCallback>();
    callback1->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        VERIFY_SUCCEEDED(E_FAIL, L"Released request should not call the callback");
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

    // second
    xref_ptr<IAbortableImageOperation> abortableImageOperation2;
    auto callback2 = make_xref<ImageAvailableTestCallback>();
    callback2->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        VERIFY_SUCCEEDED(response->GetDecodingResult());
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

    // cancel first
    abortableImageOperation1.reset();

    // wait second
    WaitForDefault(m_dispatcher, callback2);

    VERIFY_IS_FALSE(callback1->callbackCalled);
}

void ImageCacheTests::ReleaseRequestInCallback()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    // first
    xref_ptr<IAbortableImageOperation> abortableImageOperation1;
    auto callback1 = make_xref<ImageAvailableTestCallback>();
    callback1->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        abortableImageOperation1.reset();
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

    // second
    xref_ptr<IAbortableImageOperation> abortableImageOperation2;
    auto callback2 = make_xref<ImageAvailableTestCallback>();
    callback2->onImageAvailable = [&](IImageAvailableResponse *response)
    {
        abortableImageOperation2.reset();
        return S_OK;
    };
    VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

    // wait all
    WaitForDefault(m_dispatcher, callback1, callback2);
}

void ImageCacheTests::RedecodeOtherRelease()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    {
        // first
        xref_ptr<IAbortableImageOperation> abortableImageOperation1;
        auto callback1 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

        // second
        xref_ptr<IAbortableImageOperation> abortableImageOperation2;
        auto callback2 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

        // wait both
        WaitForDefault(m_dispatcher, callback1, callback2);

        // release first
        abortableImageOperation1.reset();

        // repeat second
        VERIFY_SUCCEEDED(abortableImageOperation2->SetDecodeParams(callback2, m_decodeParams16x16));
        WaitForDefault(m_dispatcher, callback2);
    }

    // ...and in reverse order
    {
        // first
        xref_ptr<IAbortableImageOperation> abortableImageOperation1;
        auto callback1 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

        // second
        xref_ptr<IAbortableImageOperation> abortableImageOperation2;
        auto callback2 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

        // wait both
        WaitForDefault(m_dispatcher, callback1, callback2);

        // release second
        abortableImageOperation2.reset();

        // repeat first
        VERIFY_SUCCEEDED(abortableImageOperation1->SetDecodeParams(callback1, m_decodeParams16x16));
        WaitForDefault(m_dispatcher, callback1);
    }
}

void ImageCacheTests::RedecodeOtherDisconnect()
{
    xref_ptr<ImageCache> imageCache = MakeImageCache();

    {
        // first
        xref_ptr<IAbortableImageOperation> abortableImageOperation1;
        auto callback1 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

        // second
        xref_ptr<IAbortableImageOperation> abortableImageOperation2;
        auto callback2 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

        // wait both
        WaitForDefault(m_dispatcher, callback1, callback2);

        // disconnect first
        abortableImageOperation1->DisconnectImageOperation();

        // repeat second
        VERIFY_SUCCEEDED(abortableImageOperation2->SetDecodeParams(callback2, m_decodeParams16x16));
        WaitForDefault(m_dispatcher, callback2);
    }

    // ...and in reverse order
    {
        // first
        xref_ptr<IAbortableImageOperation> abortableImageOperation1;
        auto callback1 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback1, abortableImageOperation1));

        // second
        xref_ptr<IAbortableImageOperation> abortableImageOperation2;
        auto callback2 = make_xref<ImageAvailableTestCallback>();
        VERIFY_SUCCEEDED(imageCache->GetImage(m_decodeParams16x16, callback2, abortableImageOperation2));

        // wait both
        WaitForDefault(m_dispatcher, callback1, callback2);

        // disconnect second
        abortableImageOperation2->DisconnectImageOperation();

        // repeat first
        VERIFY_SUCCEEDED(abortableImageOperation1->SetDecodeParams(callback1, m_decodeParams16x16));
        WaitForDefault(m_dispatcher, callback1);
    }
}
