// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "OfferableMemory.h"
#include "OfferableSoftwareBitmap.h"
#include "OfferableSoftwareBitmapUnitTests.h"
#include "PixelFormat.h"
#include <wincodec.h>
#include <windows.graphics.imaging.h>
#include <CoInitHelper.h>
#include "WicBitmapLock.h"
#include "SoftwareBitmapUtility.h"

namespace Windows { namespace UI { namespace Xaml { namespace Tests {
    namespace Foundation { namespace Imaging {

void OfferableSoftwareBitmapUnitTests::AllocateWrite()
{
    uint32_t widths[] =
    {
        1,
        10,
        100,
        1000,
    };

    uint32_t heights[] =
    {
        1,
        10,
        100,
        1000,
    };

    for (uint32_t widthIndex = 0; widthIndex < ARRAYSIZE(widths); widthIndex++)
    {
        for (uint32_t heightIndex = 0; heightIndex < ARRAYSIZE(heights); heightIndex++)
        {
            OfferableSoftwareBitmap softwareBitmap(
                pixelColor32bpp_A8R8G8B8,
                widths[widthIndex],
                heights[heightIndex]);

            // Write to full buffer size
            memset(softwareBitmap.GetBuffer(), 1, softwareBitmap.GetBufferSize());

            // Verify the implied buffer size is the same as the returned buffer size.
            uint32_t impliedBufferSize = softwareBitmap.GetWidth() * softwareBitmap.GetHeight() * 4;
            VERIFY_ARE_EQUAL(impliedBufferSize, softwareBitmap.GetBufferSize());
        }
    }
}

void OfferableSoftwareBitmapUnitTests::WicLockMovesWithBuffer()
{
    CoInitHelper::EnsureCoInitialized();
    wrl::ComPtr<IWICImagingFactory> factory;
    VERIFY_SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)));
    wrl::ComPtr<IWICBitmap> bitmap;
    VERIFY_SUCCEEDED(factory->CreateBitmap(2, 2, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapCacheOnLoad, &bitmap));
    WicBitmapLock source(nullptr, WICBitmapLockWrite, bitmap);
    auto buffer = source.GetBuffer();
    WicBitmapLock destination(std::move(source));
    VERIFY_IS_NULL(source.GetBuffer());
    VERIFY_IS_TRUE(destination.GetBuffer() == buffer);
    destination.GetBuffer()[0] = 37;
    destination.Unlock();
    destination.Unlock();
    VERIFY_IS_NULL(destination.GetBuffer());
    WicBitmapLock relocked(nullptr, WICBitmapLockRead, bitmap);
    bitmap.Reset();
    VERIFY_ARE_EQUAL(static_cast<uint8_t>(37), relocked.GetBuffer()[0]);
}

void OfferableSoftwareBitmapUnitTests::WicEmptyLock()
{
    WICRect empty{0, 0, 0, 0};
    wrl::ComPtr<IWICBitmap> bitmap;
    WicBitmapLock lock(&empty, WICBitmapLockRead, bitmap);
    VERIFY_IS_NULL(lock.GetBuffer());
    VERIFY_ARE_EQUAL(0u, lock.GetBufferSize());
    lock.Unlock();
}

void OfferableSoftwareBitmapUnitTests::SoftwareBitmapLockMovesWithBuffer()
{
    CoInitHelper::EnsureCoInitialized();
    wrl::ComPtr<wgri::ISoftwareBitmapFactory> factory;
    VERIFY_SUCCEEDED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_Graphics_Imaging_SoftwareBitmap).Get(),
        &factory));
    wrl::ComPtr<wgri::ISoftwareBitmap> bitmap;
    VERIFY_SUCCEEDED(factory->CreateWithAlpha(wgri::BitmapPixelFormat_Bgra8, 2, 2,
        wgri::BitmapAlphaMode_Ignore, &bitmap));
    SoftwareBitmapLock source(bitmap, wgri::BitmapBufferAccessMode_ReadWrite);
    auto buffer = source.GetBuffer();
    SoftwareBitmapLock destination(std::move(source));
    VERIFY_IS_NULL(source.GetBuffer());
    VERIFY_IS_TRUE(destination.GetBuffer() == buffer);
    bitmap.Reset();
    destination.GetStartPtr()[0] = 41;
    VERIFY_ARE_EQUAL(static_cast<uint8_t>(41), destination.GetStartPtr()[0]);
}

} } } } } }
