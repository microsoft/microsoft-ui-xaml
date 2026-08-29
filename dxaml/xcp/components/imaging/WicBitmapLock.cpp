// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "WicBitmapLock.h"

WicBitmapLock::WicBitmapLock(
    _In_opt_ const WICRect* lockRect,
    WICBitmapLockFlags wicBitmapLockFlags,
    _In_ const wrl::ComPtr<IWICBitmap>& wicBitmap
    )
{
    if (lockRect == nullptr || (lockRect->Width != 0 && lockRect->Height != 0))
    {
        IFCFAILFAST(wicBitmap->Lock(lockRect, wicBitmapLockFlags, &m_spWicBitmapLock));
        IFCFAILFAST(m_spWicBitmapLock->GetDataPointer(&m_bufferSize, &m_pBuffer));
        IFCFAILFAST(m_spWicBitmapLock->GetSize(&m_width, &m_height));
        IFCFAILFAST(m_spWicBitmapLock->GetStride(&m_stride));
    }
}
