// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <MUX-ETWEvents.h>

#include "XcpAllocation.h"

//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
SystemMemoryBits::SystemMemoryBits(
    XUINT32 width,
    XUINT32 height,
    DXGI_FORMAT dxgiFormat
    )
    : m_width(width)
    , m_height(height)
    , m_dxgiFormat(dxgiFormat)
    , m_timeStamp(0L)
#if DBG
    , m_dbgOustandingLockCount(0)
#endif /* DBG */
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
SystemMemoryBits::~SystemMemoryBits()
{
    // Trace the destroy surface event
    TraceSystemMemorySurfaceFreeInfo(reinterpret_cast<ULONG_PTR>(this));
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Locks the system memory buffer.
//
//------------------------------------------------------------------------------
void
SystemMemoryBits::Lock(
    _Outptr_result_buffer_((*pnStride)*(*puHeight)) void **ppAddress,
    _Out_ XINT32 *pnStride,
    _Out_ XUINT32 *puWidth,
    _Out_ UINT32 *puHeight
    )
{
    *ppAddress = GetBits();
    *pnStride = GetStride();
    *puWidth = m_width;
    *puHeight = m_height;

#if DBG
    ASSERT(InterlockedIncrement(&m_dbgOustandingLockCount) < 1000);
#endif
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Unlocks the system memory buffer.
//
//------------------------------------------------------------------------------
void
SystemMemoryBits::Unlock()
{
#if DBG
    ASSERT(InterlockedDecrement(&m_dbgOustandingLockCount) >= 0);
#endif
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the number of bytes per pixel for a given format.
//
//------------------------------------------------------------------------------
/* static */ XUINT32
SystemMemoryBits::GetPixelStrideForFormat(DXGI_FORMAT dxgiFormat)
{
    XUINT32 result = 0;

    switch (dxgiFormat)
    {
    case DXGI_FORMAT_A8_UNORM:
        result = 1;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        result = 4;
        break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        result = 8;
        break;

    default:
        ASSERT(FALSE); // unknown pixel format
    }

    return result;
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
SystemMemoryBitsHeap::SystemMemoryBitsHeap(
    XUINT32 width,
    XUINT32 height,
    DXGI_FORMAT dxgiFormat,
    XUINT32 stride,
    _In_reads_bytes_(allocationSize) void *pAllocation,
    XUINT32 allocationSize
    )
    : SystemMemoryBits(width, height, dxgiFormat)
    , m_stride(stride)
    , m_pAllocation(pAllocation)
    , m_allocationSize(allocationSize)
{
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
SystemMemoryBitsHeap::~SystemMemoryBitsHeap()
{
    XcpAllocation::OSMemoryFree(m_pAllocation);
    m_pAllocation = NULL;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a SystemMemoryBitsHeap.
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
SystemMemoryBitsHeap::Create(
    XUINT32 width,
    XUINT32 height,
    DXGI_FORMAT dxgiFormat,
    _Outptr_ SystemMemoryBits **ppSystemMemoryBits
    )
{
    *ppSystemMemoryBits = NULL;

    HRESULT hr = S_OK;

    SystemMemoryBitsHeap *pSystemMemoryBits = NULL;

    XUINT32 pixelSize = 0;

    switch (dxgiFormat)
    {
    case DXGI_FORMAT_A8_UNORM:
        pixelSize = 1;
        break;

    case DXGI_FORMAT_B8G8R8A8_UNORM:
        pixelSize = 4;
        break;

    case DXGI_FORMAT_R16G16B16A16_FLOAT:
        pixelSize = 8;
        break;

    default:
        ASSERT(FALSE); // other pixel formats are not supported
    }

    const XUINT32 stride = width * pixelSize;

    const XUINT32 allocationSize = stride * height;

    void *pAllocation =
        XcpAllocation::OSMemoryAllocateNoFailFast(allocationSize);
    IFCOOM_RETURN(pAllocation);

    pSystemMemoryBits =
        new SystemMemoryBitsHeap(
            width,
            height,
            dxgiFormat,
            stride,
            pAllocation,
            allocationSize);

    // Trace the allocation
    TraceSystemMemorySurfaceAllocateInfo(
        reinterpret_cast<ULONG_PTR>(pSystemMemoryBits),
        width,
        height,
        pixelSize,
        FALSE /* isDriverVisible */
        );

    *ppSystemMemoryBits = pSystemMemoryBits;

    RRETURN(hr);//RRETURN_REMOVAL
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//-------------------------------------------------------------------------
SystemMemoryBitsDriver::SystemMemoryBitsDriver(
    _In_ CLockableGraphicsPointer<ID3D11DeviceContext> & deviceContext,
    _In_ ID3D11Texture2D *pTexture,
    XUINT32 width,
    XUINT32 height,
    DXGI_FORMAT dxgiFormat
    )
    : SystemMemoryBits(width, height, dxgiFormat)
    , m_deviceContext(deviceContext)
    , m_pTexture(pTexture)
    , m_isMapped(FALSE)
{
    m_pTexture->AddRef();
    XINT64 size = GetWidth() * GetHeight() * GetPixelStride();
    
    TraceMemoryUpdateAllocationSystemMemoryBitsInfo(static_cast<int32_t>(size));
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//-------------------------------------------------------------------------
SystemMemoryBitsDriver::~SystemMemoryBitsDriver()
{
    ReleaseInterface(m_pTexture);
    XINT64 size = GetWidth() * GetHeight() * GetPixelStride();
    
    TraceMemoryUpdateAllocationSystemMemoryBitsInfo(static_cast<int32_t>(-size));
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a SystemMemoryBitsDriver.
//
//-------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
SystemMemoryBitsDriver::Create(
    _In_ ID3D11Device *pDevice,
    _In_ CLockableGraphicsPointer<ID3D11DeviceContext> & deviceContext,
    XUINT32 width,
    XUINT32 height,
    DXGI_FORMAT dxgiFormat,
    _Outptr_ SystemMemoryBits **ppSystemMemoryBits
    )
{
    *ppSystemMemoryBits = NULL;

    HRESULT hr = S_OK;
    ID3D11Texture2D *pTexture = NULL;
    SystemMemoryBitsDriver *pSystemMemoryBits = NULL;

    D3D11_TEXTURE2D_DESC desc =
    {
        width,
        height,
        1,          // MipLevels
        1,          // ArraySize
        dxgiFormat,
        { 1, 0 },   // SampleDesc
        D3D11_USAGE_STAGING,
        0,          // BindFlags
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE,
        0           // MiscFlags
    };

    IFC(pDevice->CreateTexture2D(&desc, NULL, &pTexture));

    pSystemMemoryBits =
        new SystemMemoryBitsDriver(
        deviceContext,
        pTexture,
        width,
        height,
        dxgiFormat
        );

    // Trace the allocation
    if (EventEnabledSystemMemorySurfaceAllocateInfo())
    {
        XUINT32 pixelSize = 0;

        switch (dxgiFormat)
        {
        case DXGI_FORMAT_A8_UNORM:
            pixelSize = 1;
            break;

        case DXGI_FORMAT_B8G8R8A8_UNORM:
            pixelSize = 4;
            break;

        case DXGI_FORMAT_R16G16B16A16_FLOAT:
            pixelSize = 8;
            break;

        default:
            ASSERT(FALSE); // other pixel formats are not supported
        }

        TraceSystemMemorySurfaceAllocateInfo(
            reinterpret_cast<ULONG_PTR>(pSystemMemoryBits),
            width,
            height,
            pixelSize,
            TRUE /* isDriverVisible */
            );
    }

    *ppSystemMemoryBits = pSystemMemoryBits;
    pSystemMemoryBits = NULL;

Cleanup:
    ReleaseInterface(pTexture);
    ReleaseInterface(pSystemMemoryBits);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Maps the underlying D3D texture if necessary so that the UI thread
//      has access to the pixels in the surface.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
SystemMemoryBitsDriver::EnsureMapped(bool allowWait)
{
    HRESULT hr = S_OK;

    D3D11_MAPPED_SUBRESOURCE mappedSubresource;

    if (!m_isMapped)
    {
        hr = m_deviceContext.GetLocked()->Map(
            m_pTexture,
            0,
            D3D11_MAP_WRITE,
            allowWait ? 0 : D3D11_MAP_FLAG_DO_NOT_WAIT,
            &mappedSubresource
            );

        if (!allowWait && DXGI_ERROR_WAS_STILL_DRAWING == hr)
        {
            // This failure case is expected, so do not trace.
            // When callers pass FALSE for allowWait they must
            // allow for the function to fail.
            IFC_NOTRACE(hr);
        }
        else
        {
            IFC(hr);
        }

        m_mappedSubresource.pData = mappedSubresource.pData;
        m_mappedSubresource.pitch = mappedSubresource.RowPitch;

        // For diagnosing [Bug 1428620:WatsonCrash: Microsoft.UI.Xaml.dll!DCompSurface::Ensure32BitSource -- INVALID_POINTER_WRITE c0000005].
        // Calling Map sometimes returned a bogus pitch, which caused us to AV when we loop over the mapped memory when writing to it.
        FAIL_FAST_ASSERT(m_mappedSubresource.pitch > 0);

        m_isMapped = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread immediately before a sys->vid copy
//      is executed wtih this surface.  This unmaps the surface to ensure
//      that the GPU can access the surface.
//
//-------------------------------------------------------------------------
void
SystemMemoryBitsDriver::EnsureUnmapped()
{
    if (m_isMapped)
    {
        m_deviceContext.GetLocked()->Unmap(m_pTexture, 0);

        m_isMapped = FALSE;
    }
}
