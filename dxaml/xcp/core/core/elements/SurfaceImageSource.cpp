// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Microsoft.UI.Xaml.media.dxinterop.h"
#include <WindowsGraphicsDeviceManager.h>
#include <OfferTracker.h>
#include <PixelFormat.h>

CSurfaceImageSource::CSurfaceImageSource(_In_ CCoreServices *pCore)
    : CImageSource(pCore)
    , m_isOpaque(FALSE)
    , m_isDrawing(FALSE)
    , m_onGlobalSISList(FALSE)
    , m_needReleaseHWSurfaceOnDraw(FALSE)
    , m_hasGPUWork(FALSE)
    , m_shouldPropagateDirtyFlag(FALSE)
{
    m_offerTracker = pCore->NWGetWindowRenderTarget()->GetDCompTreeHost()->GetOfferTrackerNoRef();
}

CSurfaceImageSource::~CSurfaceImageSource()
{
    if (IsDrawing())
    {
        // We cannot leave the SIS in the drawing state as this will prevent any other
        // surfaces on this SurfaceFactory from being drawn to.
        switch (m_interfaceUsed)
        {
            case InterfaceUsed::Native:
                VERIFYHR(EndDraw());
                break;
            case InterfaceUsed::NativeWithD2D:
                VERIFYHR(EndDrawWithD2D());
                break;
            default:
                ASSERT(FALSE);
                break;
        }
    }

    ReleaseInterface(m_pDevice);
    ReleaseInterface(m_pSurfaceFactory);

    // Remove the SIS from the global list if necessary
    if (m_onGlobalSISList)
    {
        VERIFYHR(GetContext()->RemoveSurfaceImageSource(this));
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new SurfaceImageSource by specifying Width and Height
//
//------------------------------------------------------------------------
static XUINT32 s_transparentPixel = 0;

_Check_return_ HRESULT
CSurfaceImageSource::Initialize(
    _In_ XINT32 width,
    _In_ XINT32 height,
    _In_ bool isOpaque
    )
{
    HRESULT hr = S_OK;

    auto core = GetContext();
    CMemorySurface *pMemorySurface = NULL;
    ImageSurfaceWrapper *pImageSurfaceWrapper = NULL;

    // Validate Parameters
    IFCEXPECT(width  >= 0);
    IFCEXPECT(height >= 0);

    m_width = width;
    m_height = height;
    m_isOpaque = isOpaque;

    IFC(ImageSurfaceWrapper::Create(
        core,
        FALSE /* mustKeepSoftwareSurface */,
        &pImageSurfaceWrapper
        ));

    // Initialize a 1x1 transparent surface that will be used to handle 2 cases:
    // 1. When rendering inside of a bitmap cache ({Virtual}SurfaceImageSource objects should not render)
    // 2. When rendering an opacity mask for a shape.  The Silverlight rasterizer locks
    //    the software surface associated with the fill/stroke brush, but then does nothing with it
    IFC(CMemorySurface::Create(
        pixelColor32bpp_A8R8G8B8,
        1, // width
        1, // height
        sizeof(s_transparentPixel), // stride
        sizeof(s_transparentPixel), // buffer size
        &s_transparentPixel,
        &pMemorySurface
        ));

    pImageSurfaceWrapper->SetSoftwareSurface(pMemorySurface);

    SetInterface(m_pImageSurfaceWrapper, pImageSurfaceWrapper);

    // The SIS should not already be on the global list
    ASSERT(!m_onGlobalSISList);

    // Add the SIS to the global list of VSIS objects
    IFC(core->AddSurfaceImageSource(this));

    m_onGlobalSISList = TRUE;

Cleanup:
    ReleaseInterface(pImageSurfaceWrapper);
    ReleaseInterface(pMemorySurface);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Changes the DX device that will be used to render updates into this surface image source
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::SetDevice(
    _In_ IUnknown *pDevice
    )
{
    HRESULT hr = S_OK;
    DCompSurfaceFactory *pSurfaceFactory = NULL;

    IFC(EnforceInterfaceUsage(InterfaceUsed::Native));

    // Changing devices in the middle of a BeginDraw/EndDraw is not supported
    if (IsDrawing())
    {
        IFC(E_FAIL);
    }

    if (pDevice != m_pDevice)
    {
        if (pDevice != NULL)
        {
            // TODO: see TFS Task 2369542 - Separate D3D Device creation from DComp device creation.
            // This call may result in a device loss and error. Decouple D3D and DComp device creations.
            IFC(ObtainSurfaceFactory(pDevice, &pSurfaceFactory));
        }

        // if there is already a DXGI device set to the SIS, we need to release the DComp surface.
        // We don't release the surface until next BeginDraw so that the old content continues to draw.
        if (m_pImageSurfaceWrapper->CheckForHardwareResources())
        {
            m_needReleaseHWSurfaceOnDraw = TRUE;
        }

        ReplaceInterface(m_pDevice, pDevice);
        ReplaceInterface(m_pSurfaceFactory, pSurfaceFactory);
    }

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactory);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Begins rendering an update into the surface image source
//      This retrieves a texture from the DComp surface
//      and returns that texture to the caller, the caller will render content into the texture
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::BeginDraw(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppSurface,
    _Out_ XPOINT *pSurfaceOffset
    )
{
    HRESULT hr = S_OK;
    HWTexture *pNewTexture = NULL;
    IUnknown *pUpdateSurface = NULL;
    IUnknown *pScratchSurface = NULL;

    IFC(EnforceInterfaceUsage(InterfaceUsed::Native));

    // SetDevice must have been called with non-NULL device
    if (m_pDevice == NULL)
    {
        IFC(E_FAIL);
    }

    IFC(ValidateUpdateRect(pUpdateRect));

    // BeginDraw cannot be called a second time without a preceding EndDraw.
    if (m_isDrawing)
    {
        IFC(E_FAIL);
    }

    // We might have lost the surface factory on device loss or after suspend.
    // Hence ensure that it is created.
    IFC(EnsureSurfaceFactory());

    if (m_needReleaseHWSurfaceOnDraw)
    {
        m_pImageSurfaceWrapper->ReleaseHardwareResources();
        m_needReleaseHWSurfaceOnDraw = FALSE;
    }

    if (!m_pImageSurfaceWrapper->CheckForHardwareResources())
    {
        // TODO: JCOMP: Access to the HWTextureManager is too convoluted from here.
        CWindowRenderTarget *pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
        HWTextureManager *pHWTextureMgrNoRef = pRenderTargetNoRef->GetHwWalk()->GetTextureManager();
        ASSERT(pHWTextureMgrNoRef != NULL);

        HWTextureFlags flags = HWTextureFlags_None;
        if (IsOpaque())
        {
            flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsOpaque);
        }

        // When we just draw a portion of the surface, we should make an IDCompositionVirtualSurface
        // because DComp doesn't allow partial update on a regular surface.
        if (IsVirtual()
            || pUpdateRect->Width < static_cast<XINT32>(GetWidth())
            || pUpdateRect->Height < static_cast<XINT32>(GetHeight()))
        {
            flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsVirtual);
        }
        // DComp doesn't allow creating a Texture that is bigger than the MaxTextureSize of the D3D device on a regular surface.
        // If CreateTexture function fails with invalid arguments on a regular sruface,
        // we try to call from IDCompositionVirtualSurface, just for once.
        while (true)
        {
            hr = pHWTextureMgrNoRef->CreateTexture(
                pixelColor32bpp_A8R8G8B8,
                GetWidth(),
                GetHeight(),
                flags,
                &pNewTexture,
                m_pSurfaceFactory
                );
            if (hr != E_INVALIDARG || flags == HWTextureFlags_IsVirtual)
            {
                IFC(hr);
                break;
            }
            else
            {
                ReleaseInterface(pNewTexture);
                flags = HWTextureFlags_IsVirtual;
            }
        }

        m_pImageSurfaceWrapper->SetHardwareSurface(pNewTexture);

        // If we set a new surface, then propagate a dirty flag up the tree to cause any elements using this V/SIS to redraw.
        // Otherwise, comp tracks dirty regions inside the surface, so we don't propagate the dirty flag to prevent unnecessary
        // overdraw in comp.
        m_shouldPropagateDirtyFlag = TRUE;
    }

    IFC(BeginDrawCommon(pUpdateRect, iid, &pUpdateSurface, pSurfaceOffset));

    // Upon success, transfer the appropriate pointer to the caller
    if (pScratchSurface != NULL)
    {
        *ppSurface = pScratchSurface;
        pScratchSurface = NULL;
    }
    else
    {
        *ppSurface = pUpdateSurface;
        pUpdateSurface = NULL;
    }

Cleanup:
    ReleaseInterfaceNoNULL(pNewTexture);
    ReleaseInterfaceNoNULL(pUpdateSurface);
    ReleaseInterfaceNoNULL(pScratchSurface);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Completes rendering an update into the surface image source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EndDraw()
{
    IFC_RETURN(EnforceInterfaceUsage(InterfaceUsed::Native));
    IFC_RETURN(EndDrawCommon());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Changes the rendering device that will be used to render updates
//      into this surface image source.  Can be either a D3D or D2D device.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::SetDeviceWithD2D(
    _In_ IUnknown *pDevice
    )
{
    HRESULT hr = S_OK;
    HWTexture *pNewTexture = NULL;
    DCompSurfaceFactory *pSurfaceFactory = NULL;

    auto guard = m_pLock.lock();

    IFC(EnforceInterfaceUsage(InterfaceUsed::NativeWithD2D));

    // Changing devices in the middle of a BeginDraw/EndDraw is not supported
    if (IsDrawing())
    {
        IFC(E_FAIL);
    }

    if (pDevice != m_pDevice ||                             // The device is now different
        m_pSurfaceFactory == NULL ||                        // Can happen after device lost
        !m_pImageSurfaceWrapper->CheckForHardwareResources())    // Can happen after resume fails to reclaim surface
    {
        if (pDevice != NULL)
        {
            // TODO: see TFS Task 2369542 - Separate D3D Device creation from DComp device creation.
            // This call may result in a device loss and error. Decouple D3D and DComp device creations.
            IFC(ObtainSurfaceFactory(pDevice, &pSurfaceFactory));
        }

        // If pDevice == NULL, we'll clean up the SurfaceFactory above, don't create a surface.
        if (pSurfaceFactory != NULL)
        {
            // TODO: JCOMP: Access to the HWTextureManager is too convoluted from here.
            CWindowRenderTarget *pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
            HWTextureManager *pHWTextureMgrNoRef = pRenderTargetNoRef->GetHwWalk()->GetTextureManager();
            ASSERT(pHWTextureMgrNoRef != NULL);

            HWTextureFlags flags = HWTextureFlags_None;
            if (IsOpaque())
            {
                flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsOpaque);
            }

            if (IsVirtual())
            {
                flags = static_cast<HWTextureFlags>(flags | HWTextureFlags_IsVirtual);
            }

            IFC(pHWTextureMgrNoRef->CreateTexture(
                pixelColor32bpp_A8R8G8B8,
                GetWidth(),
                GetHeight(),
                flags,
                &pNewTexture,
                pSurfaceFactory
                ));
        }

        if (m_pImageSurfaceWrapper->CheckForHardwareResources())
        {
            m_pImageSurfaceWrapper->ReleaseHardwareResources();
        }

        if (pNewTexture != NULL)
        {
            m_pImageSurfaceWrapper->SetHardwareSurface(pNewTexture);

            // If we set a new surface, then propagate a dirty flag up the tree to cause any elements using this V/SIS to redraw.
            // Otherwise, comp tracks dirty regions inside the surface, so we don't propagate the dirty flag to prevent unnecessary
            // overdraw in comp.
            m_shouldPropagateDirtyFlag = TRUE;
        }

        ReplaceInterface(m_pDevice, pDevice);
        ReplaceInterface(m_pSurfaceFactory, pSurfaceFactory);
    }

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactory);
    ReleaseInterfaceNoNULL(pNewTexture);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Begins rendering an update into the surface image source.
//      This retrieves either a texture or a D2D device context from the DComp surface
//      and returns to the caller, the caller will render content into it
//
//  Note:
//      This method can be called from a background thread.
//      Extreme care must be taken to ensure thread-safety if any
//      internal state changes are being made.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::BeginDrawWithD2D(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _In_ bool calledFromUIThread,
    _Outptr_ IUnknown **ppUpdateObject,
    _Out_ XPOINT *pSurfaceOffset
    )
{
    auto guard = m_pLock.lock();

    IFC_RETURN(EnforceInterfaceUsage(InterfaceUsed::NativeWithD2D));

    // SetDevice1 must have been called with non-NULL device
    if (m_pDevice == NULL)
    {
        IFC_RETURN(E_FAIL);
    }

    // If we're being called from a non-UI thread, make sure their device is multithreaded
    if (!calledFromUIThread)
    {
        IFC_RETURN(EnsureMultiThreadedDevice())
    }

    // Update rect must be within the bounds of the surface
    // And must have a positive area
    IFC_RETURN(ValidateUpdateRect(pUpdateRect));

    // RS2 bug # 10402989:  If we check for lost hardware resources while the device is in the offered state,
    // this will return true and cause us to return E_SURFACE_CONTENTS_LOST.
    // This is the wrong error code to return in this situation as the app cannot recover
    // until after the device has been reclaimed.
    // The tactical fix is to explicitly check if the device is in the offered state and return E_FAIL.
    // Note that SuspendDraw and ResumeDraw behavior are not being changed, to limit the scope of the behavior change.
    IFC_RETURN(EnsureSurfaceNotOffered());

    // If we lost the surface due to device lost or suspend tear-down,
    // return a custom HRESULT to let the app know they need to call SetDevice1 again on UI thread.
    if (m_pImageSurfaceWrapper->HasLostHardwareResources())
    {
        IFC_RETURN(static_cast<HRESULT>(E_SURFACE_CONTENTS_LOST));
    }

    IFC_RETURN(BeginDrawCommon(pUpdateRect, iid, ppUpdateObject, pSurfaceOffset));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Completes rendering an update into the surface image source.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EndDrawWithD2D()
{
    auto guard = m_pLock.lock();

    IFC_RETURN(EnforceInterfaceUsage(InterfaceUsed::NativeWithD2D));

    IFC_RETURN(EndDrawCommon());

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Puts this SurfaceImagesource in "Suspended-Drawing" state.
//     In this state, drawing cannot be performed.
//
//  Note:
//      This method can be called from a background thread.
//      Extreme care must be taken to ensure thread-safety if any
//      internal state changes are being made.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::SuspendDraw(_In_ bool calledFromUIThread)
{
    auto guard = m_pLock.lock();

    IFC_RETURN(EnforceInterfaceUsage(InterfaceUsed::NativeWithD2D));

    // BeginDraw must have been called
    if (!m_isDrawing)
    {
        IFC_RETURN(E_FAIL);
    }

    // If we're being called from a non-UI thread, make sure their device is multithreaded
    if (!calledFromUIThread)
    {
        IFC_RETURN(EnsureMultiThreadedDevice())
    }

    ASSERT(m_pImageSurfaceWrapper);

    // If we lost the surface due to device lost or suspend tear-down,
    // return a custom HRESULT to let the app know they need to call SetDevice1 again on UI thread.
    if (m_pImageSurfaceWrapper->HasLostHardwareResources())
    {
        IFC_RETURN(static_cast<HRESULT>(E_SURFACE_CONTENTS_LOST));
    }

    HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
    ASSERT(pHWSurfaceNoRef && pHWSurfaceNoRef->GetCompositionSurface());
    IFC_RETURN(pHWSurfaceNoRef->GetCompositionSurface()->SuspendDraw());

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Puts this SurfaceImagesource back in "Drawing" state.
//
//  Note:
//      This method can be called from a background thread.
//      Extreme care must be taken to ensure thread-safety if any
//      internal state changes are being made.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::ResumeDraw(_In_ bool calledFromUIThread)
{
    auto guard = m_pLock.lock();

    IFC_RETURN(EnforceInterfaceUsage(InterfaceUsed::NativeWithD2D));

    // BeginDraw must have been called
    if (!m_isDrawing)
    {
        IFC_RETURN(E_FAIL);
    }

    // If we're being called from a non-UI thread, make sure their device is multithreaded
    if (!calledFromUIThread)
    {
        IFC_RETURN(EnsureMultiThreadedDevice())
    }

    ASSERT(m_pImageSurfaceWrapper);

    // If we lost the surface due to device lost or suspend tear-down,
    // return a custom HRESULT to let the app know they need to call SetDevice1 again on UI thread.
    if (m_pImageSurfaceWrapper->HasLostHardwareResources())
    {
        IFC_RETURN(static_cast<HRESULT>(E_SURFACE_CONTENTS_LOST));
    }

    HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
    ASSERT(pHWSurfaceNoRef && pHWSurfaceNoRef->GetCompositionSurface());
    IFC_RETURN(pHWSurfaceNoRef->GetCompositionSurface()->ResumeDraw());

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Executes code common to both BeginDraw and BeginDraw1 methods.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::BeginDrawCommon(
    _In_ const XRECT *pUpdateRect,
    _In_ REFIID iid,
    _Outptr_ IUnknown **ppUpdateObject,
    _Out_ XPOINT *pSurfaceOffset
    )
{
    HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
    ASSERT(pHWSurfaceNoRef && pHWSurfaceNoRef->GetCompositionSurface());

    IFC_RETURN(pHWSurfaceNoRef->GetCompositionSurface()->BeginDraw(
        pUpdateRect,
        iid,
        ppUpdateObject,
        pSurfaceOffset
        ));

    m_isDrawing = TRUE;

    // If the app is using the V1 interface, m_updatedRects should be empty.
    ASSERT(m_interfaceUsed == InterfaceUsed::NativeWithD2D || m_updatedRects.size() == 0);

    IFC_RETURN(m_updatedRects.push_back(*pUpdateRect));

    TraceSurfaceImageSourceBeginDrawInfo((UINT64)this, pUpdateRect->X, pUpdateRect->Y, pUpdateRect->X + pUpdateRect->Width, pUpdateRect->Y + pUpdateRect->Height);

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Executes code common to both EndDraw and EndDraw1 methods.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EndDrawCommon()
{
    HRESULT hr = S_OK;

    // BeginDraw must have been called
    if (!m_isDrawing)
    {
        IFC(E_FAIL);
    }

    // If there is a failure after this point (most likely EndUpdate fails with DXGI_ERROR_DEVICE_REMOVED),
    // the SurfaceImageSource will be left in a state where BeginDraw can be called again (after the application has re-created it's D3D device)
    m_isDrawing = FALSE;

    ASSERT(m_pImageSurfaceWrapper);

    // The hardware surface could have been lost during
    // device lost cleanup. Hence check.
    if (m_pImageSurfaceWrapper->CheckForHardwareResources())
    {
        HWTexture *pHWSurfaceNoRef = m_pImageSurfaceWrapper->GetHardwareSurface();
        ASSERT(pHWSurfaceNoRef && pHWSurfaceNoRef->GetCompositionSurface());

        IFC(pHWSurfaceNoRef->GetCompositionSurface()->EndDraw());

        // Enable derived classes to manage tiles
        // This is done in EndDraw to ensure it only occurs when the update was successful.
        IFC(PreUpdateVirtual(&m_updatedRects));

        // If needed, mark the image source as dirty, which will force a re-render on the UI thread. If the only changes
        // were from BeginDraw/EndDraw, then we don't need to regenerate the sprite visual or primitive. DComp already has
        // enough information to redraw the updated regions. If we made a new sprite visual or primitive we would cause lots
        // of overdraw.
        if (m_shouldPropagateDirtyFlag)
        {
            IFC(SetDirty());
            m_shouldPropagateDirtyFlag = FALSE;
        }
        else
        {
            // A device commit is all that's needed. DComp has enough dirty region information to redraw the updated regions.
            IFC(GetContext()->RequestMainDCompDeviceCommit());
        }

        m_hasGPUWork = TRUE;
    }

    TraceSurfaceImageSourceEndDrawInfo((UINT64)this);

Cleanup:
    m_updatedRects.clear();
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Verifies that the device has all the appropriate multithreaded
//     features turned on.  Assumes the device is either a D2D or a D3D
//     device.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EnsureMultiThreadedDevice() const
{
    HRESULT hr = S_OK;

    // Review:  We only really need to re-check the D3DMultiThread object every call as this is the only one that
    // can have its multithreadedness toggled on/off.  The others could be checked just once.
    // Is it worth the trouble to optimize this?
    ID2D1Device *pD2DDevice = NULL;
    ID2D1Factory *pD2DFactory = NULL;
    ID2D1Multithread *pD2DMultiThread = NULL;
    ID3D10Multithread *pD3DMultithread = NULL;
    ID2D1Device2 *pD2DDevice2 = NULL;
    IDXGIDevice *pDXGIDevice = NULL;

    ASSERT(m_pDevice != NULL);

    // First see if this is a D2D device.
    hr = m_pDevice->QueryInterface(IID_ID2D1Device, reinterpret_cast<void**>(&pD2DDevice));
    if (SUCCEEDED(hr))
    {
        ASSERT(pD2DDevice != NULL);

        // Make sure the D2D factory that created this device was created as multithreaded
        pD2DDevice->GetFactory(&pD2DFactory);
        IFC(pD2DFactory->QueryInterface(IID_ID2D1Multithread, reinterpret_cast<void**>(&pD2DMultiThread)));
        IFCCHECK(pD2DMultiThread->GetMultithreadProtected());

        // Now extract the DXGI device from the D2D device so we can verify it as well below.
        IFC(pD2DDevice->QueryInterface(reinterpret_cast<REFIID>(__uuidof(ID2D1Device2)), reinterpret_cast<void**>(&pD2DDevice2)));
        IFC(pD2DDevice2->GetDxgiDevice(&pDXGIDevice));
    }

    // Now verify the DXGI device is multithreaded as well.
    if (pDXGIDevice == NULL)
    {
        IFC(m_pDevice->QueryInterface(IID_IDXGIDevice, reinterpret_cast<void**>(&pDXGIDevice)));
    }
    IFC(pDXGIDevice->QueryInterface(IID_ID3D10Multithread, reinterpret_cast<void**>(&pD3DMultithread)));
    IFCCHECK(pD3DMultithread->GetMultithreadProtected());

Cleanup:
    ReleaseInterfaceNoNULL(pD2DDevice);
    ReleaseInterfaceNoNULL(pD2DFactory);
    ReleaseInterfaceNoNULL(pD2DMultiThread);
    ReleaseInterfaceNoNULL(pD3DMultithread);
    ReleaseInterfaceNoNULL(pD2DDevice2);
    ReleaseInterfaceNoNULL(pDXGIDevice);

    RRETURN(hr);
}

_Check_return_ HRESULT CSurfaceImageSource::EnsureSurfaceNotOffered() const
{
    if (m_offerTracker->IsOffered())
    {
        ::RoOriginateError(E_FAIL, wrl_wrappers::HStringReference(L"ISurfaceImageSourceNativeWithD2D operation is not allowed while the app is suspended").Get());
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures the surface factory with the set device.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EnsureSurfaceFactory()
{
    if (m_pSurfaceFactory == NULL &&
        m_pDevice != NULL)
    {
        IFC_RETURN(ObtainSurfaceFactory(m_pDevice, &m_pSurfaceFactory))
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Obtains a surface factory for the specified device.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::ObtainSurfaceFactory(
    _In_ IUnknown *pDevice,
    _Outptr_ DCompSurfaceFactory **ppSurfaceFactory)
{
    HRESULT hr = S_OK;

    DCompSurfaceFactory *pSurfaceFactory = NULL;

    // Ensure device resources have been initialized.
    // SIS must do this because user code can cause us to call into DComp, which might not have been initialized yet
    // on either the start-up path.
    CWindowRenderTarget *pRenderTargetNoRef = GetContext()->NWGetWindowRenderTarget();
    IFC(pRenderTargetNoRef->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());

    IFC(pRenderTargetNoRef->GetDCompTreeHost()->ObtainSurfaceFactory(
        pDevice,
        &pSurfaceFactory
        ));
    *ppSurfaceFactory = pSurfaceFactory;
    pSurfaceFactory = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactory);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Currently two mutually exclusive interfaces are supported,
//      ISurfaceImageSourceNative and ISurfaceImageSourceNativeWithD2D.
//      The application can only use one of these interfaces for an instance
//      of this object.  This function verifies that the caller is not
//      trying to mix calls on both interfaces.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::EnforceInterfaceUsage(InterfaceUsed interfaceUsed)
{
    // See which interface the application has been using.
    // This is done in a thread-safe fashion due to possible races.
    InterfaceUsed lastInterfaceUsed = static_cast<InterfaceUsed>(::InterlockedCompareExchange(
        reinterpret_cast<LONG*>(&m_interfaceUsed),
        interfaceUsed,
        InterfaceUsed::None));

    // If we get back INTERFACE_USED_NONE it means this is the first time we're
    // being called, we'll just be storing the currently used interface away.
    // Otherwise we'll get back the last used interface.  It had better match
    // the one they are trying to use now or we will fail.
    if ((lastInterfaceUsed != InterfaceUsed::None) &&
        (interfaceUsed != lastInterfaceUsed))
    {
        IFC_RETURN(E_ILLEGAL_METHOD_CALL);
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function that performs sanity checks on the update rect.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::ValidateUpdateRect(_In_ const XRECT *pUpdateRect)
{
    // Update rect must be within the bounds of the surface
    // And must have a positive area
    if ((pUpdateRect->X < 0) ||
        (pUpdateRect->Y < 0) ||
        ((pUpdateRect->X + pUpdateRect->Width) > static_cast<XINT32>(GetWidth())) ||
        ((pUpdateRect->Y + pUpdateRect->Height) > static_cast<XINT32>(GetHeight())) ||
        (pUpdateRect->Width < 1) ||
        (pUpdateRect->Height < 1))
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Blocks the CPU on the GPU work submitted to the current device.
//      Used for animation synchronization.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::GetDeviceWithGPUWork(
    _Inout_ xvector<IUnknown *> *pDevices
    )
{
    HRESULT hr = S_OK;
    ID3D11Device *pD3DDevice = NULL;

    //
    // It's possible that this SIS submitted work to another device and switched devices with SetDevice.
    // This case is rare and we will not guarantee animation synchronization. We'll only return the
    // current device.
    //
    // The alternative is to add the device to a list in EndDraw as soon as work was submitted, but that
    // requires XAML to take another reference on the device. The application can switch devices, and we
    // will still keep the old device alive until we pick it up later. There would be no way of telling
    // XAML to release the old device.
    //
    if (m_hasGPUWork)
    {
        if (m_pDevice != NULL)
        {
            IFC(DCompHelpers::D3D11DeviceFromUnknownDevice(m_pDevice, &pD3DDevice));
            bool deviceAlreadyAdded = false;

            for (xvector<IUnknown *>::const_iterator it = pDevices->begin();
                 it != pDevices->end();
                 it++)
            {
                if (*it == pD3DDevice)
                {
                    deviceAlreadyAdded = TRUE;
                    break;
                }
            }

            if (!deviceAlreadyAdded)
            {
                IFC(pDevices->push_back(pD3DDevice));
            }
        }

        m_hasGPUWork = FALSE;
    }

Cleanup:
    ReleaseInterfaceNoNULL(pD3DDevice);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread during render thread error cleanup.
//
//------------------------------------------------------------------------
void
CSurfaceImageSource::CleanUpAfterRenderThreadFailure()
{
    m_hasGPUWork = FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread during during EndDraw
//      Allows the implementation to create/delete tiles
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CSurfaceImageSource::PreUpdateVirtual(
    _In_ const xvector<XRECT> *pUpdatedRects
    )
{
    HRESULT hr = S_OK;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the reference to surface factory
//
//------------------------------------------------------------------------
void CSurfaceImageSource::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    auto guard = m_pLock.lock();

    CImageSource::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    ReleaseInterface(m_pSurfaceFactory);
}

bool CSurfaceImageSource::CheckForLostHardwareResources()
{
    auto guard = m_pLock.lock();

    return __super::CheckForLostHardwareResources();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a newly-allocated SIS.
//
//------------------------------------------------------------------------

namespace CoreImports
{
    _Check_return_ HRESULT SurfaceImageSource_Initialize(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ XINT32 iWidth,
        _In_ XINT32 iHeight,
        _In_ bool isOpaque
        )
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->Initialize(iWidth, iHeight, isOpaque));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_SetDevice(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ IUnknown *pDevice
        )
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->SetDevice(pDevice));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_BeginDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _Out_ IUnknown **ppSurface,
        _Out_ XPOINT *pPoint
        )
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->BeginDraw(pUpdateRect, iid, ppSurface, pPoint));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_EndDraw(_In_ CSurfaceImageSource* pSurfaceImageSource)
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->EndDraw());

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_SetDeviceWithD2D(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ IUnknown *pDevice
        )
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->SetDeviceWithD2D(pDevice));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_BeginDrawWithD2D(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ const XRECT *pUpdateRect,
        _In_ REFIID iid,
        _In_ bool calledFromUIThread,
        _Outptr_ IUnknown **ppUpdateObject,
        _Out_ XPOINT *pPoint
        )
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->BeginDrawWithD2D(pUpdateRect, iid, calledFromUIThread, ppUpdateObject, pPoint));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_EndDrawWithD2D(_In_ CSurfaceImageSource* pSurfaceImageSource)
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->EndDrawWithD2D());

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_SuspendDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ bool calledFromUIThread)
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->SuspendDraw(calledFromUIThread));

        return S_OK;
    }

    _Check_return_ HRESULT SurfaceImageSource_ResumeDraw(
        _In_ CSurfaceImageSource* pSurfaceImageSource,
        _In_ bool calledFromUIThread)
    {
        IFCPTR_RETURN(pSurfaceImageSource);

        IFC_RETURN(pSurfaceImageSource->ResumeDraw(calledFromUIThread));

        return S_OK;
    }
}