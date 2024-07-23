// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "D3D11DeviceInstance.h"

#include <d3d11_1.h>
#include <dxgi1_2.h>
#include <D3d11_4.h>

#include "D2DAccelerated.h"
#include "D2DAcceleratedBrushes.h"
#include "D2DAcceleratedPrimitives.h"
#include "D2DAcceleratedRT.h"

#ifndef ARRAYSIZE
#define ARRAYSIZE(a) (sizeof(a)/sizeof(a[0]))
#endif

XUINT32 g_uD3D11TextureMemoryUsage = 0;
XUINT32 g_uD3D11TextureMemoryUsageNPOT = 0;


typedef HRESULT (STDMETHODCALLTYPE *MFLOCKDXGIDEVICEMANAGER)(
    _Out_ UINT* resetToken,
    _Outptr_ IMFDXGIDeviceManager** ppDeviceManager
    );

typedef HRESULT (STDMETHODCALLTYPE *MFUNLOCKDXGIDEVICEMANAGER)( );

struct VertexShaderConstantBuffer
{
    float VertexTransform[4][4];
};

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Create a CD3D11Device object.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::Create(
    _Outptr_ CD3D11Device **ppD3D11Device,
    _In_ bool useUniqueDevice
    )
{
    HRESULT hr = S_OK;

    CD3D11Device *pD3DDevice = new CD3D11Device();

    IFC(CD3D11DeviceInstance::GetInstance(pD3DDevice->m_deviceInstance, useUniqueDevice));

    *ppD3D11Device = pD3DDevice;
    pD3DDevice = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pD3DDevice);

    return hr;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      ctor
//
//-------------------------------------------------------------------------
CD3D11Device::CD3D11Device()
    : m_uResetToken(0)
{
    m_UploadIntermediate.pTexture = NULL;
}


//-------------------------------------------------------------------------
//
//  Synopsis:
//      dtor
//
//-------------------------------------------------------------------------
CD3D11Device::~CD3D11Device()
{
    // Release intermediate upload texture
    ReleaseInterface(m_UploadIntermediate.pTexture);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure the D3D11 device and other state is initialized.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::EnsureResources()
{
    return m_deviceInstance->EnsureResources();
}

//------------------------------------------------------------------------
//
// Synopsis:
//     Does the work to create the D2D device context and related resources.
//
_Check_return_ HRESULT
CD3D11Device::EnsureD2DResources()
{
    // First make sure we have a d2d device
    IFC_RETURN(m_deviceInstance->EnsureD2DResources());

    // Create a unique context for this particular instance. Store it in the device instance so all contexts can be
    // cleared at the same time.
    IFC_RETURN(m_deviceInstance->EnsureDeviceSpecificD2DResources(this));
    return S_OK;
}

ID2D1DeviceContext* CD3D11Device::GetD2DDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return m_deviceInstance->GetD2DDeviceContext(guard, this);
}

ID2D1SolidColorBrush* CD3D11Device::GetD2DSolidColorBrush(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return m_deviceInstance->GetD2DSolidColorBrush(guard, this);
}

_Check_return_ HRESULT CD3D11Device::TakeLockAndCheckDeviceLost(_In_ CD3D11SharedDeviceGuard* guard)
{
    return m_deviceInstance->TakeLockAndCheckDeviceLost(guard);
}

_Ret_notnull_ ID3D11Device * CD3D11Device::GetDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return m_deviceInstance->GetDevice(guard);
}

ID3D11Device * CD3D11Device::TestHook_GetDevice() const
{
    return m_deviceInstance->TestHook_GetDevice();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Return a D3D device context while holding a lock
//
//------------------------------------------------------------------------------
CLockedGraphicsPointer<ID3D11DeviceContext>
CD3D11Device::GetLockedDeviceContext(_In_ const CD3D11SharedDeviceGuard* guard)
{
    return m_deviceInstance->GetLockedDeviceContext(guard);
}

ID2D1Device* CD3D11Device::GetD2DDevice(_In_ const CD3D11SharedDeviceGuard* guard) const
{
    return m_deviceInstance->GetD2DDevice(guard);
}

CD2DFactory *
CD3D11Device::GetD2DFactory() const
{
    return m_deviceInstance->GetD2DFactory();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Initialize ID3D11VideoDevice for video playback.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::InitializeVideoDevice()
{
    return m_deviceInstance->InitializeVideoDevice();
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Return texture memory usage
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::GetTextureMemoryUsage(
    _Out_ UINT32 *puTextureMemoryUsage,
    _Out_ UINT32 *puTextureMemoryUsageNPOT
    )
{
    *puTextureMemoryUsage = g_uD3D11TextureMemoryUsage / 1024;
    *puTextureMemoryUsageNPOT = g_uD3D11TextureMemoryUsageNPOT / 1024;

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Create an event and insert a marker into the command queue that signals
//      the event when the marker is executed.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::CreateEventAndEnqueueWait(
    _Outptr_ IPALWaitable **ppWaitHandle
    )
{
    HRESULT hr = S_OK;
    IPALEvent *pEvent = NULL;
    bool setEventEnqueued = false;

    IFC(GetPALThreadingServices()->EventCreate(&pEvent, FALSE /* bSignaled */, FALSE /* bManual */));

    {
        // Despite being on the Device, EnqueueSetEvent is not thread safe, so we have to lock
        // around it.  Take our standard lock by retrieving the locked context.
        CD3D11SharedDeviceGuard guard;
        IFC(m_deviceInstance->TakeLockAndCheckDeviceLost(&guard));
        auto lockedContext = GetLockedDeviceContext(&guard);
        IFC(EnqueueSetEvent(pEvent, &setEventEnqueued));
    }

    ASSERT(setEventEnqueued);

    *ppWaitHandle = pEvent;
    pEvent = NULL;

Cleanup:
    if (pEvent != NULL)
    {
        VERIFYHR(pEvent->Close());
    }

    return hr;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Insert a marker into the command queue that signals an event when it is
//      executed.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::EnqueueSetEvent(
    _In_ IPALEvent *pEvent,
    _Out_ bool *pSetEventEnqueued
    )
{
    HRESULT hr = S_OK;
    bool setEventEnqueued = false;

    //
    // In the case of suspend/resume, we may get called before device initialization is completed
    // on the render thread. Skip enqueueing the SetEvent.
    //

    CD3D11SharedDeviceGuard guard;
    IFC(m_deviceInstance->TakeLockAndCheckDeviceLost(&guard));
    IDXGIDevice2* pDXGIDevice = m_deviceInstance->GetDXGIDevice(&guard);
    if (pDXGIDevice != NULL)
    {
        // Despite being on the Device, EnqueueSetEvent is not thread safe, so we have to lock
        // around it.  Take our standard lock by retrieving the locked context.
        auto lockedContext = GetLockedDeviceContext(&guard);
        IFC(pDXGIDevice->EnqueueSetEvent(pEvent->GetHandle()));
        setEventEnqueued = TRUE;
    }

    *pSetEventEnqueued = setEventEnqueued;

Cleanup:
    return hr;
}

_Check_return_ HRESULT CD3D11Device::RegisterDeviceRemovedEvent(_In_ HANDLE event, _Out_ DWORD* cookie)
{
    CD3D11SharedDeviceGuard guard;
    IFC_RETURN(m_deviceInstance->TakeLockAndCheckDeviceLost(&guard));

    wrl::ComPtr<ID3D11Device4> device4;
    VERIFYHR(m_deviceInstance->GetDXGIDevice(&guard)->QueryInterface(IID_PPV_ARGS(&device4)));

    IFC_RETURN(device4->RegisterDeviceRemovedEvent(event, cookie));

    return S_OK;
}

void CD3D11Device::UnregisterDeviceRemoved(DWORD cookie)
{
    // Ignore device lost errors. This method is in the device lost recovery code path, when we release the old
    // D3D device, so it's expected that we'll get a device lost error. We don't always get device lost because
    // it's also on the code path that releases the D3D device instance after the app suspends.
    CD3D11SharedDeviceGuard guard;
    IGNOREHR(m_deviceInstance->TakeLockAndCheckDeviceLost(&guard));

    IDXGIDevice2* dxgiDevice = m_deviceInstance->GetDXGIDevice(&guard);
    if (dxgiDevice)
    {
        wrl::ComPtr<ID3D11Device4> device4;
        VERIFYHR(dxgiDevice->QueryInterface(IID_PPV_ARGS(&device4)));
        device4->UnregisterDeviceRemoved(cookie);
    }
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread after each frame.
//      Releases up to 1 system memory surface that has not been used in a long time.
//
//------------------------------------------------------------------------------
void
CD3D11Device::TrimMemory()
{
    const UINT64 releaseTime = gps->GetCPUMilliseconds() - sc_stagingTextureReleaseFrameDelayMilliseconds;

   m_deviceInstance->TrimMemory(releaseTime);

    if (NULL != m_UploadIntermediate.pTexture)
    {
        if ((m_UploadIntermediate.lastUse) < releaseTime)
        {
            // Intermediate upload surface has been unused for too long
            ReleaseInterface(m_UploadIntermediate.pTexture);
        }
    }
}
//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread.
//      Releases up all system memory surfaces in the pool.
//      Also notifies D2D and drivers to give up intermediate
//      scratch/staging allocations.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::ReleaseScratchResources()
{

    // Release intermediate upload texture
    ReleaseInterface(m_UploadIntermediate.pTexture);

    IFC_RETURN(m_deviceInstance->ReleaseScratchResources());

    return S_OK;

}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether A8 Textures should be used
//
//------------------------------------------------------------------------------
bool
CD3D11Device::ShouldAttemptToUseA8Textures() const
{
    return m_deviceInstance->ShouldAttemptToUseA8Textures();
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread after the render thread has finished reading from a
//      SystemMemoryBits object. This returns a SystemMemoryBits object to the pool.
//
//------------------------------------------------------------------------------
void
CD3D11Device::ReturnToSysMemBitsPool(
_In_ SystemMemoryBits *pSysMemBitsIn
)
{
    m_deviceInstance->ReturnToSysMemBitsPool(pSysMemBitsIn);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread to allocate a SystemMemoryBits from the pool.
//      This will create a new SystemMemoryBits object if no suitable one is found.
//
//      8bpp textures are allocated in the heap. They will be copied out and
//      expanded to 32bpp before they're uploaded to DComp. 32bpp textures are
//      created in driver-visible memory.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CD3D11Device::AllocateFromSysMemBitsPool(
XUINT32 width,
XUINT32 height,
DXGI_FORMAT dxgiFormat,
bool hasNative8BitSupport,
_Outptr_ SystemMemoryBits **ppOut
)
{
    return m_deviceInstance->AllocateFromSysMemBitsPool(width, height, dxgiFormat, hasNative8BitSupport, ppOut);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread as SW generated textures are uploaded to
//      the DComp surface backing them.
//
//------------------------------------------------------------------------------
ID3D11Texture2D *
CD3D11Device::GetIntermediateUploadSurfaceNoRef(
    DXGI_FORMAT format
    ) const
{
    ID3D11Texture2D *pTexture = NULL;

    if (DXGI_FORMAT_B8G8R8A8_UNORM == format)
    {
        pTexture = m_UploadIntermediate.pTexture;
    }

    return pTexture;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread by the HW texture manager so that we can
//      create an appropriately sized intermediate surface for texture updates
//      in the frame.  We only do this for DXGI_FORMAT_B8G8R8A8_UNORM textures
//      because A8 textures are unlikely to be supported by hardware to which
//      this optimization applies.
//
//------------------------------------------------------------------------------
void
CD3D11Device::NotifyLargestTextureInFrame(
    DXGI_FORMAT format,
    UINT32 uMaxWidth,
    UINT32 uMaxHeight
    )
{
    HRESULT hr = S_OK;

    if (m_deviceInstance->UseIntermediateUploadSurface())
    {
        if (DXGI_FORMAT_B8G8R8A8_UNORM == format)
        {
            if ((uMaxWidth > m_UploadIntermediate.width) ||
                (uMaxHeight > m_UploadIntermediate.height))
            {
                ReleaseInterface(m_UploadIntermediate.pTexture);
            }

            if (NULL == m_UploadIntermediate.pTexture)
            {
                D3D11_TEXTURE2D_DESC desc = { 0 };

                desc.Width      = uMaxWidth;
                desc.Height     = uMaxHeight;
                desc.MipLevels  = 1;
                desc.ArraySize  = 1;
                desc.Format     = format;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.Usage      = D3D11_USAGE_DEFAULT;
                desc.BindFlags  = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = 0;
                desc.MiscFlags  = 0;

                CD3D11SharedDeviceGuard guard;
                IFC(m_deviceInstance->TakeLockAndCheckDeviceLost(&guard));
                IFC(m_deviceInstance->GetDevice(&guard)->CreateTexture2D(&desc, NULL, &m_UploadIntermediate.pTexture));

                m_UploadIntermediate.width = uMaxWidth;
                m_UploadIntermediate.height = uMaxHeight;
            }

            m_UploadIntermediate.lastUse = gps->GetCPUMilliseconds();
        }
    }

Cleanup:
    return;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Records the current device as lost
//
//------------------------------------------------------------------------------
void
CD3D11Device::RecordDeviceAsLost()
{
    m_deviceInstance->RecordDeviceAsLost();
}

void
CD3D11Device::TestHook_LoseDevice()
{
    m_deviceInstance->TestHook_LoseDevice();
}

bool CD3D11Device::IsDeviceLost() const
{
    return m_deviceInstance->IsDeviceLost();
}

_Check_return_ HRESULT CD3D11Device::IsHdrOutput(_In_ HMONITOR monitor, _Out_ bool* isHDR) const
{
    IFC_RETURN(m_deviceInstance->IsHdrOutput(monitor, isHDR));
    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Determines the unique identifier for the adapter that pDevice runs on
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
GetDeviceAdapterLuid(
    _In_ ID3D11Device *pDevice,
    _Out_ LUID *pLuid
    )
{
    HRESULT hr = S_OK;
    IDXGIDevice *pDxgiDevice = NULL;
    IDXGIAdapter *pAdapter = NULL;
    DXGI_ADAPTER_DESC adapterDesc;

    IFC(pDevice->QueryInterface(&pDxgiDevice));

    IFC(pDxgiDevice->GetParent(
        __uuidof(IDXGIAdapter),
        reinterpret_cast<void **>(&pAdapter)
        ));

    IFC(pAdapter->GetDesc(&adapterDesc));

    *pLuid = adapterDesc.AdapterLuid;

Cleanup:
    ReleaseInterface(pAdapter);
    ReleaseInterface(pDxgiDevice);

    return hr;
}
