// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "WicBitmapLock.h"

WicBitmapLock::BufferTraits::Handle WicBitmapLock::BufferTraits::Acquire(
    IWICBitmapLock& bitmapLock, uint32_t& size)
{
    uint8_t* buffer = nullptr;
    IFCFAILFAST(bitmapLock.GetDataPointer(&size, &buffer));
    return Handle::Adopt(bitmapLock, buffer);
}

WicBitmapLock::WicBitmapLock(
    _In_opt_ const WICRect* lockRect,
    WICBitmapLockFlags wicBitmapLockFlags,
    _In_ const wrl::ComPtr<IWICBitmap>& wicBitmap
    )
{
    if (lockRect == nullptr || (lockRect->Width != 0 && lockRect->Height != 0))
    {
        wrl::ComPtr<IWICBitmapLock> bitmapLock;
        IFCFAILFAST(wicBitmap->Lock(lockRect, wicBitmapLockFlags, &bitmapLock));
        m_buffer = BufferTraits::Acquire(*bitmapLock.Get(), m_bufferSize);
        IFCFAILFAST(bitmapLock->GetSize(&m_width, &m_height));
        IFCFAILFAST(bitmapLock->GetStride(&m_stride));
    }
}
