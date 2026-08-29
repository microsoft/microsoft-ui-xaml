// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      An IPALSurface wrapper around a raw memory pointer.

#pragma once

class CMemorySurface : public CXcpObjectBase<IPALSurface>
{
public:
#if DBG
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IPALSurface>);
#endif /* DBG */

    CMemorySurface();

public:
    ~CMemorySurface() override
    {
    }

    _Check_return_ static HRESULT Create(
        PixelFormat pixelFormat,
        XUINT32 nWidth,
        XUINT32 nHeight,
        XINT32  nStride,
        XUINT32 cAddressSize,
        _In_reads_bytes_(cAddressSize) void* pAddress,
        _Outptr_ CMemorySurface** ppMemorySurface
        );


    //--------------------------------------------------------------------------
    //
    //  IPALSurface interface implementation
    //
    //--------------------------------------------------------------------------

    _Check_return_ HRESULT Lock(
        _Outptr_result_bytebuffer_(*pnStride * *puHeight) void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ XUINT32 *puHeight
        ) final;

    _Check_return_ HRESULT Unlock() final;

    XUINT32 GetWidth() const final
    {
        return m_nWidth;
    }

    XUINT32 GetHeight() const final
    {
        return m_nHeight;
    }

    PixelFormat GetPixelFormat() final
    {
        return m_pixelFormat;
    }

    bool IsOpaque() final
    {
        return m_fIsOpaque;
    }

    bool IsVirtual() final
    {
        return false;
    }

    void SetIsOpaque(bool fIsOpaque) final
    {
        m_fIsOpaque = fIsOpaque;
    }

    HRESULT GetNotifyOnDelete(
        _Outptr_opt_result_maybenull_ CNotifyOnDelete **ppNotifyOnDelete
        ) override
    {
        IFCPTR_RETURN(ppNotifyOnDelete);

        *ppNotifyOnDelete = NULL;

        return S_OK;
    }

    // Offer and Reclaim can't be supported on CMemorySurface directly
    // because the buffer is passed in and we can only Offer memory that has been
    // specially allocated.  Use LocalMemorySurface if you want to offer the memory.
    HRESULT Offer() override { return S_OK; }
    HRESULT Reclaim(_Out_ bool *pWasDiscarded) override { *pWasDiscarded = FALSE; return S_OK; }

protected:
    bool m_fIsOpaque : 1;
    XUINT32 m_nWidth = 0;
    XUINT32 m_nHeight = 0;
    XINT32  m_nStride = 0;
    void*   m_pBits = nullptr;
    PixelFormat m_pixelFormat;
};
