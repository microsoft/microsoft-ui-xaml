// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Description:
//      DirectX device PAL interface implementation

#pragma once
#include "LockableGraphicsPointer.h"

struct ID3D11Texture2D;
struct ID3D11ShaderResourceView;
struct ID3D11RenderTargetView;
class IWarpPrivateAPI;
interface ID3D11DeviceContext;
class CD3D11Device;
class DCompTreeHost;
struct IDCompositionSurfaceFactory;

#include "xref_ptr.h"

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Base class that represents a system memory surface
//
//-------------------------------------------------------------------------
class SystemMemoryBits : public CXcpObjectBase<>
{
public:
    void Lock(
        _Outptr_result_buffer_((*pnStride)*(*puHeight)) void **ppAddress,
        _Out_ XINT32 *pnStride,
        _Out_ XUINT32 *puWidth,
        _Out_ UINT32 *puHeight
        );

    void Unlock();

    IntrusiveList<SystemMemoryBits>::ListEntry *GetPoolLink()
    {
        return &m_poolLink;
    }

    static XUINT32 PoolLinkOffset()
    {
        return OFFSET(SystemMemoryBits, m_poolLink);
    }

    XUINT32 GetWidth() const { return m_width; }
    XUINT32 GetHeight() const { return m_height; }
    DXGI_FORMAT GetDxgiFormat() const { return m_dxgiFormat; }

    XUINT32 GetPixelStride() const
    {
        return GetPixelStrideForFormat(m_dxgiFormat);
    }

    static XUINT32 GetPixelStrideForFormat(DXGI_FORMAT dxgiFormat);

    void SetTimeStamp(XUINT64 timeStamp) { m_timeStamp = timeStamp; }
    XUINT64 GetTimeStamp() { return m_timeStamp; }

    virtual bool IsDriverVisible() = 0;

    virtual void *GetBits() = 0;
    virtual XUINT32 GetStride() = 0;

protected:
    SystemMemoryBits(
        XUINT32 width,
        XUINT32 height,
        DXGI_FORMAT dxgiFormat
        );

    ~SystemMemoryBits() override;

private:
    XUINT32 m_width;
    XUINT32 m_height;
    DXGI_FORMAT m_dxgiFormat;

    XUINT64 m_timeStamp;

    IntrusiveList<SystemMemoryBits>::ListEntry m_poolLink;

#if DBG
    volatile LONG m_dbgOustandingLockCount;
#endif /* DBG */
};

//-------------------------------------------------------------------------
//
//  Synopsis:
//      A system memory surface that is allocated on the heap
//
//-------------------------------------------------------------------------
class SystemMemoryBitsHeap : public SystemMemoryBits
{
public:
    SystemMemoryBitsHeap(
        XUINT32 width,
        XUINT32 height,
        DXGI_FORMAT dxgiFormat,
        XUINT32 stride,
        _In_reads_bytes_(allocationSize) void *pAllocation,
        XUINT32 allocationSize
        );

    ~SystemMemoryBitsHeap() override;

    static _Check_return_ HRESULT Create(
        XUINT32 width,
        XUINT32 height,
        DXGI_FORMAT dxgiFormat,
        _Outptr_ SystemMemoryBits **ppSystemMemoryBits
        );

    bool IsDriverVisible() override { return false; }

    void *GetBits() override { return m_pAllocation; }
    XUINT32 GetStride() override { return m_stride; }

private:
    XUINT32 m_stride;

    _Notnull_ _Field_size_bytes_(m_allocationSize) void *m_pAllocation;
    XUINT32 m_allocationSize;
};

//-------------------------------------------------------------------------
//
//  Synopsis:
//      A system memory surface that is allocated by the driver
//
//-------------------------------------------------------------------------
class SystemMemoryBitsDriver : public SystemMemoryBits
{
public:
    SystemMemoryBitsDriver(
        _In_ CLockableGraphicsPointer<ID3D11DeviceContext> & deviceContext,
        _In_ ID3D11Texture2D *pTexture,
        XUINT32 width,
        XUINT32 height,
        DXGI_FORMAT dxgiFormat
        );

    ~SystemMemoryBitsDriver() override;

    static _Check_return_ HRESULT Create(
        _In_ ID3D11Device *pDevice,
        _In_ CLockableGraphicsPointer<ID3D11DeviceContext> & deviceContext,
        XUINT32 width,
        XUINT32 height,
        DXGI_FORMAT dxgiFormat,
        _Outptr_ SystemMemoryBits **ppSystemMemoryBits
        );

    bool IsDriverVisible() override { return true; }

    _Check_return_ HRESULT EnsureMapped(bool allowWait);

    void EnsureUnmapped();

    ID3D11Texture2D *GetTexture()
    {
        ASSERT(!m_isMapped);
        return m_pTexture;
    }

    void *GetBits() override
    {
        ASSERT(m_isMapped);
        return m_mappedSubresource.pData;
    }

    XUINT32 GetStride() override
    {
        ASSERT(m_isMapped);
        return m_mappedSubresource.pitch;
    }

private:
    CLockableGraphicsPointer<ID3D11DeviceContext> m_deviceContext;
    ID3D11Texture2D *m_pTexture;
    bool m_isMapped;
    struct
    {
        void *pData;
        UINT pitch;
    } m_mappedSubresource;
};

