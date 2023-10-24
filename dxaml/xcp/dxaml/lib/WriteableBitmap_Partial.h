// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines the WriteableBitmap type.
//  Notes:
//      The PixelBuffer class is an internal implementation of the
//      IBuffer and IBufferByteAccess interfaces, which together provide the
//      only mechanism for exposing a pointer in WinRT.
//
//      The WriteableBitmap's PixelBuffer property exposes its PixelBuffer
//      as an IBuffer interface, which must then be cast to an
//      IBufferByteAccess in order to get the actual pointer.

#pragma once

#include "WriteableBitmap.g.h"
#include <fwd/windows.storage.h>

namespace DirectUI
{
    class PixelBuffer;
    PARTIAL_CLASS(WriteableBitmap)
    {
        friend class WriteableBitmapFactory;

    public:
        WriteableBitmap();
        ~WriteableBitmap() override;

        _Check_return_ HRESULT get_PixelBufferImpl(_Outptr_ wsts::IBuffer** pValue);
        _Check_return_ HRESULT InvalidateImpl();
        _Check_return_ HRESULT RefreshPixelBuffer();

    protected:
        _Check_return_ HRESULT OnStreamReadCompleted(_In_ wsts::IStreamReadOperation *pOperation, wf::AsyncStatus status) override;

    private:
        ctl::ComPtr<PixelBuffer> m_spPixelBuffer;
    };

    class PixelBuffer :
        public wsts::IBuffer,
        public ::Windows::Storage::Streams::IBufferByteAccess,
        public ctl::ComBase
    {
        BEGIN_INTERFACE_MAP(PixelBuffer, ctl::ComBase)
            INTERFACE_ENTRY(PixelBuffer, wsts::IBuffer)
            INTERFACE_ENTRY(PixelBuffer, ::Windows::Storage::Streams::IBufferByteAccess)
        END_INTERFACE_MAP(PixelBuffer, ctl::ComBase)

    public:
        PixelBuffer();
        ~PixelBuffer() override;

        // IBuffer methods
        IFACEMETHOD(get_Capacity)(_Out_ UINT32 *value) override;
        IFACEMETHOD(get_Length)(_Out_ UINT32 *value) override;
        IFACEMETHOD(put_Length)(_In_ UINT32 value) override;

        // IBufferByteAccess methods
        IFACEMETHOD(Buffer)(_Outptr_ BYTE **ppBuffer) override;

        // Memory management methods
        _Check_return_ HRESULT InitializeBuffer(_In_ UINT32 length, _In_ WriteableBitmap *pWriteableBitmap);
        void ReleaseBuffer();
        void MarkDirty() { m_fDirty = TRUE; }

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject) override;

    private:
        XUINT32 m_length;
        _Field_size_(m_length) XBYTE *m_pBuffer;
        WriteableBitmap *m_pWriteableBitmap;
        bool m_fDirty;
    };
}
