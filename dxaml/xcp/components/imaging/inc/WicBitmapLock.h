// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

struct WICRect;
struct IWICBitmap;
struct IWICBitmapLock;

// RAII class for IWICBitmap that does a FAILFAST if it cannot lock the buffer
// If FAILFAST is insufficient, the constructor should be modified to throw.
class WicBitmapLock
{
public:
    WicBitmapLock(
        _In_opt_ const WICRect* lockRect,
        WICBitmapLockFlags wicBitmapLockFlags,
        _In_ const wrl::ComPtr<IWICBitmap>& wicBitmap
        );

    // After calling Unlock, all other members are invalid and should not be called.
    void Unlock() { m_spWicBitmapLock = nullptr; }

    uint8_t* GetBuffer(){ return m_pBuffer; }
    const uint8_t* GetBuffer() const { return m_pBuffer; }

    uint32_t GetBufferSize() const { return m_bufferSize; }

    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    uint32_t GetStride() const { return m_stride; }

    // Typically the width is returned in pixels, but occasionally bytes are beneficial.
    uint32_t GetWidthInBytes() const { return m_width << 2; }

private:
    wrl::ComPtr<IWICBitmapLock> m_spWicBitmapLock;
    uint8_t* m_pBuffer = nullptr;
    uint32_t m_bufferSize = 0;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    uint32_t m_stride = 0;
};
