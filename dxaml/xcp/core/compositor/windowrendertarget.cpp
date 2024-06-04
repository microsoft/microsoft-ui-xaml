// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A window render target is responsible for maintaining a rendering
// area that allows presentation of content to an OS window surface.

#include "precomp.h"
#include <HWWalk.h>
#include <CompositorTreeHost.h>
#include <InputServices.h>
#include <WindowsGraphicsDeviceManager.h>
#include <D3D11Device.h>
#include <WindowsPresentTarget.h>
#include <CompositorScheduler.h>

//------------------------------------------------------------------------------
//
//  Member:
//      CWindowRenderTarget ctor
//
//------------------------------------------------------------------------------
CWindowRenderTarget::CWindowRenderTarget(
    _In_ CCoreServices* pCore,
    bool isPrimaryWindowTarget,
    _In_ CompositorScheduler* compositorScheduler,
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _In_ WindowsPresentTarget* initialPresentTarget
    )
    : m_compositorScheduler(compositorScheduler)
    , m_graphicsDeviceManager(pGraphicsDeviceManager)
    , m_presentTarget(initialPresentTarget)
    , m_pHwWalk(NULL)
    , m_needsFullRedraw(FALSE)
    , m_hardwareResourcesHaveBeenReset(FALSE)
    , m_isPrimaryWindowTarget(isPrimaryWindowTarget)
    , m_pCoreNoRef(static_cast<CCoreServices *>(pCore))
{
    XCP_WEAK(&m_pCoreNoRef);

    m_graphicsDeviceChangeListenersNoRef.XcpMarkWeakPointers();
}

//------------------------------------------------------------------------------
//
//  Member:
//      CWindowRenderTarget dtor
//
//------------------------------------------------------------------------------
CWindowRenderTarget::~CWindowRenderTarget()
{
    IGNOREHR(m_pCoreNoRef->UnregisterPLMListener(this));

    m_presentTarget.reset();

    ReleaseInterface(m_pHwWalk);

    m_compositorScheduler.reset();

    ReleaseInterface(m_pCompositorTreeHost);
}


//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new instance of the CWindowRenderTarget class.
//
//------------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT
CWindowRenderTarget::Create(
    _In_ CCoreServices* pCore,
    bool isPrimaryWindowTarget,
    _In_ CompositorScheduler* compositorScheduler,
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _In_ WindowsPresentTarget* initialPresentTarget,
    _Outptr_ CWindowRenderTarget **ppWindowRenderTarget
    )
{
    ASSERT(*ppWindowRenderTarget == NULL);

    xref_ptr<CWindowRenderTarget> pWindowRenderTarget;
    pWindowRenderTarget.attach(new CWindowRenderTarget(
        pCore,
        isPrimaryWindowTarget,
        compositorScheduler,
        pGraphicsDeviceManager,
        initialPresentTarget));


    IFC_RETURN(pWindowRenderTarget->Initialize());

    *ppWindowRenderTarget = pWindowRenderTarget.detach();

    return S_OK;
}

// Called by the host whenever a (full or partial) redraw of the scene is necessary. Also allows the window render
// target to force a full redraw after recovering from device loss.
_Check_return_ HRESULT CWindowRenderTarget::Draw(
    _In_ CCoreServices* pCore,
    bool forceRedraw,
    _Out_ bool* pFrameDrawn)
{
    HRESULT hr = S_OK;
    CWindowRenderTarget* pRenderTargetNoRef = this;

    // First check to see if the compositor scheduler has had an error (which
    // will typically be either a device lost or a stale device).
    if (FAILED(hr = m_compositorScheduler->GetRenderThreadHR()))
    {
        // Note: we don't simply follow the normal error pattern to identify
        //       the device lost, because that would bring us back through
        //       this function where we would simply see the error again.
        //       We need to get into RecoverFromDeviceLost before the render
        //       thread Hr will be reset.  So we call HandleDeviceLost here
        //       to confirm we have a device lost error and set the appropriate
        //       state and then just continue.  If it wasn't a device lost error,
        //       the we fail out as normal.
        m_pCoreNoRef->HandleDeviceLost(&hr);
        IFC(hr);
    }

    ASSERT(m_isPrimaryWindowTarget);
    ASSERT(pCore == m_pCoreNoRef);

    // Handle device lost recovery, which may update m_hardwareResourcesHaveBeenReset, and then produce a frame.
    IFC(m_pCoreNoRef->RecoverFromDeviceLost());

    IFC(m_pCoreNoRef->NWDrawMainTree(
        pRenderTargetNoRef,
        !!(forceRedraw | m_needsFullRedraw | m_hardwareResourcesHaveBeenReset),
        pFrameDrawn
        ));

    if (*pFrameDrawn) 
    {
        m_needsFullRedraw = FALSE;
        m_hardwareResourcesHaveBeenReset = FALSE; 
    }
    
Cleanup:
    m_pCoreNoRef->HandleDeviceLost(&hr);

    RRETURN(hr);
}

// Ensures presence of an appropriate present method.
_Check_return_ HRESULT CWindowRenderTarget::Initialize()
{
    IFC_RETURN(HWWalk::Create(this, m_pCoreNoRef, m_pCoreNoRef->GetMaxTextureSizeProvider(), &m_pHwWalk));

    IFC_RETURN(CompositorTreeHost::Create(&m_pCompositorTreeHost));

    // Create the composition resources.
    // Any expensive hardware resources should be lazily created off-thread - only wrapper
    // objects for these resources will be initialized here.
    IFC_RETURN(InitializeResources());

    IFC_RETURN(m_pCoreNoRef->RegisterPLMListener(this));

    // Ensure the render thread has started.
    if (!m_compositorScheduler->IsStarted())
    {
        IFC_RETURN(m_compositorScheduler->Startup());
    }

    // Initialize the window visibility info on compositor scheduler.
    if (!m_pCoreNoRef->IsRenderEnabled())
    {
        IFC_RETURN(m_compositorScheduler->OnRenderStateChanged(FALSE));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up all compositors after losing the device. Called on each
//      render target to clean up its HwWalk and compositors.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowRenderTarget::CleanupResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost)
{
    m_pHwWalk->HandleDeviceLost(cleanupDComp);

    if (cleanupDComp)
    {
        m_pCompositorTreeHost->Cleanup();
    }

    //
    // Clean up DComp resources, this will also release all DComp visuals.
    // Do so under the draw lock, to avoid access violations on animation thread.
    //
    {
        auto lock = m_compositorScheduler->GetDrawListsLock();

        TraceCompositorLockBegin();

        if (m_isPrimaryWindowTarget)
        {
            // The device cached in the device manager has been lost and can no longer be reused.
            // This may also clean-up DComp resources.
            m_graphicsDeviceManager->CleanupCachedDeviceResources(cleanupDComp, isDeviceLost);
        }

        TraceCompositorLockEnd();
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Re-creates resources. Called on each render target after cleanup. Each
//      render target is responsible for recovering itself. The meta compositor
//      was marked by the primary window target during cleanup.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CWindowRenderTarget::RebuildResources()
{
    IFC_RETURN(InitializeResources());

    // Our hardware resources are being reset due to a lost device.
    m_hardwareResourcesHaveBeenReset = TRUE;

    // Since we now have a new device, ensure that the render thread
    // error is cleared.
    m_compositorScheduler->ClearRenderThreadHR();

    return S_OK;
}

// Ensures presence of a present method and of a matching compositor.
_Check_return_ HRESULT CWindowRenderTarget::InitializeResources()
{
    // Start creating hardware resources again.
    IFC_RETURN(m_graphicsDeviceManager->StartResourceCreation());

    // Notify the graphics device change listeners.
    {
        CXcpList<IPALGraphicsDeviceChangeListener>::XCPListNode* pCurrent =
            m_graphicsDeviceChangeListenersNoRef.GetHead();

        while (pCurrent != NULL)
        {
            IPALGraphicsDeviceChangeListener* pListener = pCurrent->m_pData;

            pListener->OnGraphicsDeviceChanged();

            pCurrent = pCurrent->m_pNext;
        }
    }

    m_needsFullRedraw = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Register a listener for graphics device changes. The listener will
//      get a callback with the current graphics device right away...
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT CWindowRenderTarget::RegisterGraphicsDeviceChangeListener(
    _In_ IPALGraphicsDeviceChangeListener *pListener
    )
{
    pListener->OnGraphicsDeviceChanged();

    IFC_RETURN(m_graphicsDeviceChangeListenersNoRef.Add(pListener));

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Unregister a listener for graphics device changes.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT CWindowRenderTarget::UnregisterGraphicsDeviceChangeListener(
    _In_ IPALGraphicsDeviceChangeListener *pListener
    )
{
    RRETURN(m_graphicsDeviceChangeListenersNoRef.Remove(pListener, FALSE /* bDoDelete */));
}

_Check_return_ HRESULT CWindowRenderTarget::OnSuspend(_In_ bool isTriggeredByResourceTimer)
{
    CD3D11Device *pGraphicsDeviceNoRef = m_graphicsDeviceManager->GetGraphicsDevice();

    if (pGraphicsDeviceNoRef != nullptr)
    {
        IFC_RETURN(pGraphicsDeviceNoRef->ReleaseScratchResources());
    }

    // isTriggeredByResourceTimer is for apps that want to have the audio play from a video if it is running
    // in the background.  Refer to MediaEngineHost::OnSuspend() for full description.  In this case, we do
    // not release scratch resources since DComp rendering is still in progress.
    CD3D11Device *pVideoGraphicsDeviceNoRef = m_graphicsDeviceManager->GetVideoGraphicsDeviceNoRef();

    if ((pVideoGraphicsDeviceNoRef != nullptr) &&
        !isTriggeredByResourceTimer)
    {
        IFC_RETURN(pVideoGraphicsDeviceNoRef->ReleaseScratchResources());
    }

    return S_OK;
}

_Check_return_ HRESULT CWindowRenderTarget::OnResume()
{
    // Resources may have been lost on suspend, so do a full render walk on resume.
    m_needsFullRedraw = TRUE;

    return S_OK;
}

DCompTreeHost* CWindowRenderTarget::GetDCompTreeHost()
{
    return m_graphicsDeviceManager->GetDCompTreeHost();
}

_Check_return_ HRESULT CWindowRenderTarget::RequestMainDCompDeviceCommit()
{
    if (m_pCoreNoRef != nullptr)
    {
        IFC_RETURN(m_pCoreNoRef->RequestMainDCompDeviceCommit());
    }
    return S_OK;
}

WindowsGraphicsDeviceManager* CWindowRenderTarget::GetGraphicsDeviceManager() const
{
    return m_graphicsDeviceManager.get();
}

CompositorTreeHost* CWindowRenderTarget::GetCompositorTreeHost() const
{
    return m_pCompositorTreeHost;
}

HWWalk* CWindowRenderTarget::GetHwWalk()
{
    return m_pHwWalk;
}

XUINT32 CWindowRenderTarget::GetWidth() const
{
    return m_presentTarget != nullptr ? m_presentTarget->GetWidth() : 0;
}

XUINT32 CWindowRenderTarget::GetHeight() const
{
    return m_presentTarget != nullptr ? m_presentTarget->GetHeight() : 0;
}

xref_ptr<WindowsPresentTarget> CWindowRenderTarget::GetPresentTarget()
{
    return m_presentTarget;
}

void CWindowRenderTarget::Retarget(_In_ WindowsPresentTarget* presentTarget)
{
    m_presentTarget = presentTarget;
}
