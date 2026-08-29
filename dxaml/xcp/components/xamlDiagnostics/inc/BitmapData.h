// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "includes.h"

class BitmapData :
    public wrl::RuntimeClass<
    wrl::RuntimeClassFlags<wrl::ClassicCom>,
    IBitmapData>
{
public:
    BitmapData();

    HRESULT RuntimeClassInitialize(
        _In_ unsigned int capacity,
        _In_ unsigned int stride,
        _In_ BitmapDescription bitmapDescription,
        _In_ BitmapDescription bitmapSourceDescription);

    ~BitmapData() override;

    BYTE* GetBytes() const;

    IFACEMETHOD(CopyBytesTo)(
        _In_ unsigned int sourceOffsetInBytes,
        _In_ unsigned int maxBytesToCopy,
        _Out_writes_bytes_(maxBytesToCopy) BYTE *pvBytes,
        _Out_ _Pre_defensive_ unsigned int *numberOfBytesCopied);

    IFACEMETHOD(GetStride)(
        _Out_ _Pre_defensive_ unsigned int* pStride);

    IFACEMETHOD(GetBitmapDescription)(
        _Out_ _Pre_defensive_ BitmapDescription *pBitmapDescription);

    IFACEMETHOD(GetSourceBitmapDescription)(
        _Out_ _Pre_defensive_ BitmapDescription *pBitmapDescription);

private:
    BYTE* m_bytes;
    unsigned int m_capacity;
    unsigned int m_stride;
    BitmapDescription m_bitmapDescription;
    BitmapDescription m_bitmapSourceDescription;
};
