// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// TODO: Use this class for SoftwareBitmapSource and delete
//                 any unneeded methods since OfferableSoftwareBitmap
//                 is the bitmap receiver now.

namespace SoftwareBitmapUtility
{
    static _Check_return_ HRESULT CreateSoftwareBitmap(
        uint32_t width,
        uint32_t height,
        _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
        );

    static _Check_return_ HRESULT CreateSoftwareBitmap(
        uint32_t width,
        uint32_t height,
        wgri::BitmapPixelFormat pixelFormat,
        wgri::BitmapAlphaMode alphaMode,
        _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
        );

    static _Check_return_ HRESULT CreateSoftwareBitmap(
        _In_ const wrl::ComPtr<IWICBitmap>& spWicBitmap,
        bool readOnly,
        _Out_ wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
        );

    // Validates the SoftwareBitmap fulfills the requirements for use in XAML
    static HRESULT ValidateSoftwareBitmap(
        _In_ const wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap
        );
};

// RAII class for SoftwareBitmap that does a FAILFAST if it cannot lock the buffer
// If fail fast is insufficient in the future, then throw an error which can be caught.
// Note that this only supports lock of a single plane (which allows for simplification).
// Any future support for multiple planes would need to be added (Probably in a specialized class).
class SoftwareBitmapLock
{
public:
    SoftwareBitmapLock(
        _In_ const wrl::ComPtr<wgri::ISoftwareBitmap>& spSoftwareBitmap,
        wgri::BitmapBufferAccessMode accessMode
        );

    uint8_t* GetBuffer() { return m_pBuffer; }
    const uint8_t* GetBuffer() const { return m_pBuffer; }

    uint32_t GetBufferSize() const { return m_bufferSize; }

    uint8_t* GetStartPtr() { return m_pBuffer + m_bitmapPlaneDescription.StartIndex; }
    const uint8_t* GetStartPtr() const { return m_pBuffer + m_bitmapPlaneDescription.StartIndex; }

    uint32_t GetWidth() const { return m_bitmapPlaneDescription.Width; }
    uint32_t GetHeight() const { return m_bitmapPlaneDescription.Height; }
    uint32_t GetStride() const { return m_bitmapPlaneDescription.Stride; }
    
    // Typically the width is returned in pixels, but occasionally bytes are beneficial.
    uint32_t GetWidthInBytes() const { return m_bitmapPlaneDescription.Width << 2; }

private:
    wrl::ComPtr<wf::IMemoryBufferReference> m_spMemoryBufferReference;
    uint8_t* m_pBuffer = nullptr;
    uint32_t m_bufferSize = 0;
    wgri::BitmapPlaneDescription m_bitmapPlaneDescription = {};
};
