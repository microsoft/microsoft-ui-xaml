// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <wincodec.h>

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics { namespace Image {

class IUnlockable
{
public:

    virtual void Unlock() = 0;
};

class CustomWicBitmapLock
    : public IWICBitmapLock
{
public:

    CustomWicBitmapLock(
        IUnknown* pOwner,
        IUnlockable* pUnlockable,
        unsigned int width,
        unsigned int height,
        WICPixelFormatGUID wicPixelFormat,
        uint32_t* pBuffer
        )
        : m_refCounter(0)
        , m_pOwner(pOwner)
        , m_pUnlockable(pUnlockable)
        , m_width(width)
        , m_height(height)
        , m_wicPixelFormat(wicPixelFormat)
        , m_pBuffer(pBuffer)
    {
    }

    ~CustomWicBitmapLock()
    {
    }

    // IUnknown Implementation
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == __uuidof(IWICBitmapLock))
        {
            *ppv = static_cast<IWICBitmapLock*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        m_pOwner->AddRef();

        return InterlockedIncrement(&m_refCounter);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        LONG newRefCounter = InterlockedDecrement(&m_refCounter);
        ASSERT(newRefCounter >= 0);

        m_pOwner->Release();

        if (newRefCounter == 0)
        {
            m_pUnlockable->Unlock();
            delete this;
        }

        return newRefCounter;
    }

    // IWICBitmapLock Implementation
    STDMETHODIMP GetSize(
        UINT *pWidth,
        UINT *pHeight
        ) override
    {
        *pWidth = m_width;
        *pHeight = m_height;

        return S_OK;
    }

    STDMETHODIMP GetStride(
        UINT *pStride
        ) override
    {
        *pStride = m_width * sizeof(m_pBuffer[0]);

        return S_OK;
    }

    STDMETHODIMP GetDataPointer(
        UINT *pBufferSize,
        WICInProcPointer *ppData
        ) override
    {
        VERIFY_IS_NOT_NULL(m_pBuffer);

        // Only 32bpp supported for this test
        *pBufferSize = m_width * m_height * sizeof(m_pBuffer[0]);
        *ppData = reinterpret_cast<WICInProcPointer>(m_pBuffer);

        return S_OK;
    }

    STDMETHODIMP GetPixelFormat(
        WICPixelFormatGUID *pPixelFormat
        ) override
    {
        *pPixelFormat = m_wicPixelFormat;

        return S_OK;
    }

private:

    CustomWicBitmapLock();

    long m_refCounter;
    IUnknown* m_pOwner;
    IUnlockable* m_pUnlockable;
    unsigned int m_width;
    unsigned int m_height;
    uint32_t* m_pBuffer;
    WICPixelFormatGUID m_wicPixelFormat;
};

class CustomWicBitmap
    : public IWICBitmap
    , private IUnlockable
{
public:

    CustomWicBitmap(
        bool supportsTransparency,
        unsigned int width,
        unsigned int height,
        uint32_t colorValue1,
        uint32_t colorValue2
        );

    ~CustomWicBitmap()
    {
        delete[] m_pBuffer;
    }

    void OverridePixelFormat(
        WICPixelFormatGUID wicPixelFormat
        )
    {
        m_wicPixelFormat = wicPixelFormat;
    }

    void SetPendingLock(
        bool pendingLock
        )
    {
        m_pendingLock = pendingLock;
    }

    bool IsLocked() { return m_locked; }

    // IUnknown Implementation
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) override
    {
        if (riid == __uuidof(IWICBitmap))
        {
            *ppv = static_cast<IWICBitmap*>(this);
            AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef() override
    {
        return InterlockedIncrement(&m_refCounter);
    }

    STDMETHODIMP_(ULONG) Release() override
    {
        LONG newRefCounter = InterlockedDecrement(&m_refCounter);
        ASSERT(newRefCounter >= 0);

        if (newRefCounter == 0)
        {
            delete this;
        }

        return newRefCounter;
    }

    // IWICBitmapSource Implementation
    STDMETHODIMP GetSize(
        UINT *pWidth,
        UINT *pHeight
        ) override
    {
        *pWidth = m_width;
        *pHeight = m_height;

        return S_OK;
    }

    STDMETHODIMP GetPixelFormat(
        WICPixelFormatGUID *pPixelFormat
        ) override
    {
        // NOTE: If this is GUID_WICPixelFormat32bppBGR then it indicates that alpha should be ignored
        *pPixelFormat = m_wicPixelFormat;

        return S_OK;
    }

    STDMETHODIMP GetResolution(
        double *pDpiX,
        double *pDpiY
        ) override
    {
        *pDpiX = m_dpiX;
        *pDpiY = m_dpiY;

        return S_OK;
    }

    STDMETHODIMP CopyPalette(
        IWICPalette*
        ) override
    {
        // Don't need this
        ASSERT(false);
        return E_NOTIMPL;
    }

    STDMETHODIMP CopyPixels(
        const WICRect*,
        UINT,
        UINT,
        BYTE*
        ) override
    {
        // Don't need this
        ASSERT(false);
        return E_NOTIMPL;
    }

    // IWICBitmap Implementation
    STDMETHODIMP Lock(
        const WICRect*,
        DWORD flags,
        IWICBitmapLock **ppILock
        ) override
    {
        if (m_pendingLock)
        {
            // Test case where the data is unavailable and will be set again at a later time
            return E_PENDING;
        }

        // Rect is ignored for testing, but XAML could use it for a segmented copy.
        // XAML will only read
        // This operation should be very fast
        VERIFY_IS_TRUE(flags == WICBitmapLockRead);

        VERIFY_IS_NOT_NULL(m_pBuffer);

        VERIFY_IS_FALSE(m_locked);
        if (m_locked)
        {
            return E_FAIL;
        }

        CustomWicBitmapLock* pCustomWicBitmapLock = new CustomWicBitmapLock(
            this,
            this,
            m_width,
            m_height,
            m_wicPixelFormat,
            m_pBuffer);
        pCustomWicBitmapLock->AddRef();
        *ppILock = pCustomWicBitmapLock;

        m_locked = true;

        return S_OK;
    }

    STDMETHODIMP SetPalette(
        IWICPalette*
        ) override
    {
        // Don't need this
        ASSERT(false);
        return E_NOTIMPL;
    }

    STDMETHODIMP SetResolution(
        double,
        double
        ) override
    {
        // Don't need this
        ASSERT(false);
        return E_NOTIMPL;
    }

    static void InitializeWithGradientPattern(
        uint32_t* pPixel,
        unsigned int width,
        unsigned int height,
        uint32_t colorValue1,
        uint32_t colorValue2
        );

private:

    static uint32_t GetAlpha(uint32_t color)
    {
        return ((color & 0xFF000000) >> 24);
    }

    static uint32_t GetRed(uint32_t color)
    {
        return ((color & 0x00FF0000) >> 16);
    }

    static uint32_t GetGreen(uint32_t color)
    {
        return ((color & 0x0000FF00) >> 8);
    }

    static uint32_t GetBlue(uint32_t color)
    {
        return (color & 0x000000FF);
    }

    static uint32_t BuildColor(
        uint32_t alpha,
        uint32_t red,
        uint32_t green,
        uint32_t blue
        )
    {
        return
            (blue +
            (green << 8) +
            (red << 16) +
            (alpha << 24));
    }

    // IUnlockable
    void Unlock() override
    {
        m_locked = false;
    }

    long m_refCounter;
    bool m_supportsTransparency;
    unsigned int m_width;
    unsigned int m_height;
    double m_dpiX;
    double m_dpiY;
    WICPixelFormatGUID m_wicPixelFormat;
    uint32_t* m_pBuffer;
    bool m_locked;
    bool m_pendingLock;
};

} } } } } } }
