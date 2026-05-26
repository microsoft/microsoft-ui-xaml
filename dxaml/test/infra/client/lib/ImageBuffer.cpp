// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <memory>
#include <wincodec.h>
#include "ImageBuffer.h"

using namespace WEX::Common;

void ImageBuffer::Clear()
{
    m_width = 0;
    m_height = 0;
    m_pixels.reset(nullptr);
}

void ImageBuffer::SetSize(uint32_t width, uint32_t height)
{
    Clear();

    m_width = width;
    m_height = height;
}

void ImageBuffer::Init()
{
    m_pixels = std::unique_ptr<uint32_t[]>(new uint32_t[m_width * m_height]);
    memset(m_pixels.get(), 0, m_width * m_height * 4);
}

void ImageBuffer::EnsureInit()
{
    if (!IsInitialized())
    {
        Init();
    }
}

uint32_t* ImageBuffer::GetPixelBuffer()
{
    EnsureInit();
    return m_pixels.get();
}

void ImageBuffer::SetPixel(int x, int y, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t pixelValue =
        static_cast<uint32_t>(b) +
        (static_cast<uint32_t>(g) << 8) +
        (static_cast<uint32_t>(r) << 16) + 
        (static_cast<uint32_t>(a) << 24);

    SetPixel(x, y, pixelValue);
}

void ImageBuffer::SetPixel(int x, int y, uint32_t pixelValue)
{
    EnsureInit();

    m_pixels[y * m_width + x] = pixelValue;
}

void ImageBuffer::SaveToFile(wchar_t* fileName)
{
    EnsureInit();

    // Setup WIC and write the data to a file
    wrl::ComPtr<IWICImagingFactory> spWICFactory;
    LogThrow_IfFailed(CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&spWICFactory)
    ));

    wrl::ComPtr<IWICBitmap> spWICBitmap;
    LogThrow_IfFailed(spWICFactory->CreateBitmapFromMemory(
        m_width,
        m_height,
        GUID_WICPixelFormat32bppBGRA,
        m_width * 4,
        m_width * m_height * 4,
        reinterpret_cast<uint8_t*>(m_pixels.get()),
        &spWICBitmap
    ));

    wrl::ComPtr<IWICStream> spWicStream;
    LogThrow_IfFailed(spWICFactory->CreateStream(&spWicStream));
    LogThrow_IfFailed(spWicStream->InitializeFromFilename(fileName, GENERIC_WRITE));

    wrl::ComPtr<IWICBitmapEncoder> spWicBitmapEncoder;
    LogThrow_IfFailed(spWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &spWicBitmapEncoder));
    LogThrow_IfFailed(spWicBitmapEncoder->Initialize(spWicStream.Get(), WICBitmapEncoderNoCache));

    wrl::ComPtr<IWICBitmapFrameEncode> spWicBitmapFrameEncode;
    LogThrow_IfFailed(spWicBitmapEncoder->CreateNewFrame(&spWicBitmapFrameEncode, NULL));
    LogThrow_IfFailed(spWicBitmapFrameEncode->Initialize(NULL));

    WICPixelFormatGUID encoderPixelFormat = GUID_WICPixelFormatDontCare;
    LogThrow_IfFailed(spWicBitmapFrameEncode->SetPixelFormat(&encoderPixelFormat));
    LogThrow_IfFailed(spWicBitmapFrameEncode->SetSize(m_width, m_height));
    LogThrow_IfFailed(spWicBitmapFrameEncode->WriteSource(spWICBitmap.Get(), NULL));
    LogThrow_IfFailed(spWicBitmapFrameEncode->Commit());
    LogThrow_IfFailed(spWicBitmapEncoder->Commit());
}
