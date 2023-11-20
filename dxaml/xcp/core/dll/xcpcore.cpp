// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "xcptypes.h"
#include "DependencyObjectTraits.g.h"
#include "XStringUtils.h"
#include <string.h>
#include "ImageProviderInterfaces.h"
#include "AsyncImageFactory.h"
#include "ImageProvider.h"
#include "ImageTaskDispatcher.h"
#include "InputPaneHandler.h"
#include "hwsurfacecache.h"
#include "WinTextCore.h"
#include "hwtexturemgr.h"
#include "CompositorTreeHost.h"
#include "DCompSurfaceMonitor.h"
#include "DCompTreeHost.h"
#include "CustomClassInfo.h"
#include "MetadataAPI.h"
#include <DWrite_3.h>
#include "Timemgr.h"
#include "TimeSpan.h"
#include "NodeStreamCache.h"
#include <DependencyLocator.h>
#include "XamlTraceLogging.h"
#include "XamlTraceSession.h"
#include <TrimWhitespace.h>
#include <StringConversions.h>
#include <ParserAPI.h>
#include <CColor.h>
#include <FrameworkTheming.h>
#include <SystemThemingInterop.h>
#include <ThemeWalkResourceCache.h>
#include <GraphicsUtility.h>
#include <DXamlServices.h>
#include <AutoReentrantReferenceLock.h>
#include <RuntimeEnabledFeatures.h>
#include <DeferredMapping.h>
#include <ImageDecodeBoundsFinder.h>
#include <isapipresent.h>
#include <algorithm>
#include <wrlhelper.h>
#include <DeferredAnimationOperation.h>
#include <D3D11Device.h>
#include <WindowsGraphicsDeviceManager.h>
#include <WindowsPresentTarget.h>
#include "hal.h"
#include "AsyncDownloadRequestManager.h"
#include "FrameCounter.h"
#include "CommonBrowserHost.h"
#include "WinBrowserHost.h"
#include "UIAWindow.h"
#include <windows.applicationmodel.preview.holographic.h>
#include <XamlSchemaContext.h>
#include "ParserSettings.h"
#include "XamlParser.h"
#include "Theme.h"
#include "DeviceListener.h"
#include "DisplayListener.h"
#include "RenderStateListener.h"
#include "OfferableMemory.h"
#include "XamlPredicateService.h"
#include "XcpWindow.h"
#include <FeatureFlags.h>
#include "DesignMode.h"
#include <SimpleProperties.h>
#include <GCInstrumentationAggregator.h>
#include "HitTestParams.h"
#include "CoreWindowRootScale.h"
#include <XamlParserCallbacks.h>
#include <windows.system.h>
#include "DXamlServices.h"
#include "JupiterWindow.h"
#include <FrameworkUdk/Containment.h>

#if XCP_MONITOR
#include "XcpAllocationDebug.h"
#endif

#include <FrameworkUdk/Containment.h>

// Bug 45792810: Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
// Bug 46468883: [1.4 servicing] Explorer first frame - DesktopWindowXamlSource spends 30ms on RoGetActivationFactory
#define WINAPPSDK_CHANGEID_46468883 46468883

// Bug 46833401: [1.4 Servicing] AutoSuggestBox's dropdown flickers when focus moves back to the island
#define WINAPPSDK_CHANGEID_46833401 46833401

#undef max

using namespace RuntimeFeatureBehavior;

using namespace DirectUI;

// Bug 46076120: Explorer first frame - resource lookups don't use the lookup cache during layout
#define WINAPPSDK_CHANGEID_46076120 46076120

// The one and only global variable.  This is shared by all instances of core
// objects in the process.

#if !defined (STERLING)
EncodedPtr<IPlatformServices> gps;
#endif //STERLING

// We don't use any of the C runtime floating point routines but the compiler
// thinks we might so forces this reference.

extern XUINT32 g_csPreviousBatchCount;
extern XUINT32 g_csMaxBatchCount;
extern XUINT32 g_csPreviousVisiblePrimitiveCount;
extern XUINT32 g_csMaxVisiblePrimitiveCount;

// Allows suspension on timer to be disabled for local debugging.
XUINT32 g_backgroundResourceTimerEnabled = 1;

static const UINT64 MIN_POINTER_REPLAY_PERIOD_IN_MS = 500;

const UINT64 CCORESERVICES_TICKS_PER_MILLISECOND = 10000;

#if !defined (STERLING)

//------------------------------------------------------------------------
//
//  Function:   ObtainCoreServices
//
//  Synopsis:
//      Creates a new core services object and returns an interface to it.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
__stdcall
ObtainCoreServices(
                   _In_ IPlatformServices *pPlatform,
                   _Outptr_ CCoreServices **ppCore
                   )
{
    // Deal with the platform services interface

    if (!pPlatform)
        return E_UNEXPECTED;

    // Save the global instance of the interface
    gps.Set(pPlatform);

    // Now allocate the core services object

    IFC_RETURN(CCoreServices::Create(ppCore));

    return S_OK;
}
#endif

//------------------------------------------------------------------------
//
//  Method:   ReleaseResources
//
//  Synopsis:
//      Free all resources owned by the DREQUEST object
//
//------------------------------------------------------------------------
void DREQUEST::ReleaseResources()
{
    strRelativeUri.Reset();
    ReleaseInterface(pIAbortableDownload);
    ReleaseInterface(pPreferredBaseUri);
    ReleaseInterface(pDownloadRequest);

    pIAbortableDownload = NULL;
    pPreferredBaseUri = NULL;
    pDownloadRequest = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Small adapter to listen for font downloads and mark a dirty flag
//      checked by CCoreServices.
//------------------------------------------------------------------------

class CCoreServices::FontDownloadListener : public IDWriteFontDownloadListener
{
public:
    FontDownloadListener(_In_ CCoreServices* pCoreServices)
    :   m_pCoreServices(pCoreServices)
    {}

    HRESULT STDMETHODCALLTYPE QueryInterface(IID const& iid, _Out_ void** object) override
    {
        if (iid == __uuidof(IUnknown)
        ||  iid == __uuidof(IDWriteFontDownloadListener))
        {
            *object = static_cast<IDWriteFontDownloadListener*>(this);
        }
        else
        {
            *object = nullptr;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    // This singleton class owned by CCoreServices will not have any external
    // references to it, beyond the lifetime of CCoreServices. Thus there is no
    // need for ref-counting.
    unsigned long STDMETHODCALLTYPE AddRef() override
    {
        return 1;
    }

    unsigned long STDMETHODCALLTYPE Release() override
    {
        return 0;
    }

    // Check whether any new downloads have completed. Note it is okay to check the
    // variable without a lock or interlocked function because the tick loop checks
    // it frequently enough that if it misses an update, it will just realize it the
    // next frame. Only the update of the variable needs to be atomic.
    bool AreNewFontDownloadsCompleted()
    {
        return m_areFontDownloadsCompleted != false;
    }

    void ResetFontDownloadsCompleted()
    {
        static_assert(sizeof(uint32_t) == sizeof(m_areFontDownloadsCompleted), "InterlockedExchange expects a 32-bit quantity.");
        ::InterlockedExchange(reinterpret_cast<uint32_t*>(&m_areFontDownloadsCompleted), false);
    }

    //------------------------------------------------------------------------
    //
    //  Synopsis:
    //      Records the completion callback by setting a dirty flag.
    //
    //------------------------------------------------------------------------
    void STDMETHODCALLTYPE DownloadCompleted(
        _In_ IDWriteFontDownloadQueue* downloadQueue,
        _In_opt_ IUnknown* context,
        HRESULT downloadResult
        ) override
    {
        // If the download succeeded, set the flag so that the main tick loop
        // can invalidate text layout in controls. Any network failure or
        // cancellation does not set the flag. Although a failure might still
        // have downloaded some new data, we won't invalidate the tree for it,
        // and cancellation may actually be due to shutting down the core. So
        // we wouldn't want to request another frame for that.
        if (SUCCEEDED(downloadResult))
        {
            ::InterlockedExchange(reinterpret_cast<uint32_t*>(&m_areFontDownloadsCompleted), true);

            // Wake up CCoreServices so that Tick() will be called, lest the window is idle.
            IXcpBrowserHost *pBrowserHost = m_pCoreServices->GetBrowserHost();
            if (pBrowserHost != NULL)
            {
                ITickableFrameScheduler *pFrameScheduler = pBrowserHost->GetFrameScheduler();
                if (pFrameScheduler != NULL)
                {
                    pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::Download);
                }
            }
            // For a successful download, leave calling DecrementPendingFontDownloadCount
            // to the main loop instead inside ProcessDownloadRequests. That way we can
            // be sure it is signaled only after the controls have been invalidated.
        }

        // On failure, do not request another frame for laying out text controls, but do
        // still keep the pending count font downloads correct either way.
        m_pCoreServices->DecrementPendingFontDownloadCount();
    };

private:
    BOOL m_areFontDownloadsCompleted = false;   // Whether or not any new downloads have completed, checked by CCoreServices each Tick.
    CCoreServices* m_pCoreServices;             // Weak reference back to CCoreServices, which always outlives the FontDownloadListener it created.
};

/* static */ std::shared_ptr<ActivationFactoryCache> ActivationFactoryCache::GetActivationFactoryCache()
{
    static PROVIDE_DEPENDENCY(ActivationFactoryCache, DependencyLocator::StoragePolicyFlags::None);
    static DependencyLocator::Dependency<ActivationFactoryCache> s_activationFactoryCache;
    return s_activationFactoryCache.Get(true /* createIfNecessary */);
}

void ActivationFactoryCache::ResetCache()
{
    m_dispatcherQueueStatics.Reset();
    m_dragDropManagerStatics.Reset();
    m_desktopChildSiteBridgeStatics.Reset();
    m_compositionEasingFunctionStatics.Reset();
    m_interopCompositorFactoryPartner.Reset();
    m_compositionPathFactory.Reset();
    m_inputSystemCursorStatics.Reset();
    m_contentIslandStatics.Reset();
}

HRESULT ActivationFactoryCache::GetDispatcherQueueStatics(_Outptr_ msy::IDispatcherQueueStatics** statics)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_dispatcherQueueStatics)
    {
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Dispatching_DispatcherQueue).Get(),
            &m_dispatcherQueueStatics));
    }

    m_dispatcherQueueStatics.CopyTo(statics);
    return S_OK;
}

HRESULT ActivationFactoryCache::GetDesktopChildSiteBridgeStatics(_Outptr_ ixp::IDesktopChildSiteBridgeStatics** statics)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_desktopChildSiteBridgeStatics)
    {
        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Content_DesktopChildSiteBridge).Get(),
            &m_desktopChildSiteBridgeStatics));
    }

    m_desktopChildSiteBridgeStatics.CopyTo(statics);
    return S_OK;
}

HRESULT ActivationFactoryCache::GetDragDropManagerStatics(_Outptr_ mui::DragDrop::IDragDropManagerStatics** statics)
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_dragDropManagerStatics)
    {
        IFC_RETURN(wf::GetActivationFactory(wrl_wrappers::HStringReference(
            RuntimeClass_Microsoft_UI_Input_DragDrop_DragDropManager).Get(),
            &m_dragDropManagerStatics));
    }

    m_dragDropManagerStatics.CopyTo(statics);
    return S_OK;
}

ixp::ICompositionEasingFunctionStatics* ActivationFactoryCache::GetCompositionEasingFunctionStatics()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_compositionEasingFunctionStatics)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_CompositionEasingFunction).Get(),
            &m_compositionEasingFunctionStatics));
    }

    return m_compositionEasingFunctionStatics.Get();
}

ixp::IInteropCompositorFactoryPartner* ActivationFactoryCache::GetInteropCompositorFactoryPartner()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_interopCompositorFactoryPartner)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_Compositor).Get(),
            &m_interopCompositorFactoryPartner));
    }

    return m_interopCompositorFactoryPartner.Get();
}

ixp::ICompositionPathFactory* ActivationFactoryCache::GetPathFactory()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_compositionPathFactory)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Composition_CompositionPath).Get(),
            &m_compositionPathFactory));
    }

    return m_compositionPathFactory.Get();
}

ixp::IInputSystemCursorStatics* ActivationFactoryCache::GetInputSystemCursorStatics()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_inputSystemCursorStatics)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Input_InputSystemCursor).Get(),
            &m_inputSystemCursorStatics));
    }

    return m_inputSystemCursorStatics.Get();
}

ixp::IContentIslandStatics* ActivationFactoryCache::GetContentIslandStatics()
{
    wil::cs_leave_scope_exit guard = m_lock.lock();

    if (!m_contentIslandStatics)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Content_ContentIsland).Get(),
            &m_contentIslandStatics));
    }

    return m_contentIslandStatics.Get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Constructor for CCoreServices object
//
//------------------------------------------------------------------------
CCoreServices::CCoreServices()
    : m_testOverrideImageSurfaceWrapperReleaseDelay(false)
    , m_isTransparentBackground(false)
    , m_isFirstFrameAfterAppStart(false)
    , m_calledOleInitialize(false)
    , m_forceDisconnectRootOnSuspend(false)
    , m_requestedThemeForSubTree(Theming::Theme::None)
    , m_flyweightState(
        CDOSharedState(
            this,
            CDOSharedState::s_defaultRenderChangedHandler,
            CDOSharedState::s_defaultBaseUri,
            xref::weakref_ptr<VisualTree>()))
    , m_maxTextureSizeProvider(this)
    , m_atlasRequestProvider(this)
    , m_contentRootCoordinator(*this)
{
    // Please keep these in the same order as the
    // corresponding declarations in corep.h, as it makes it
    // easier to spot missing initializations.
    m_uFrameNumber                = 1;
    m_pdoInSetValueFromManaged    = NULL;
    m_allowTransitionTargetsToBeCreated = FALSE;
    m_cRef                        = 1;
    m_pAllSurfaceImageSources = NULL;
    m_pAllVirtualSurfaceImageSources = NULL;
    m_pMainVisualTree             = NULL;
    m_pDeploymentTree             = NULL;
    m_pTimeManager                = NULL;
    m_pEventManager               = NULL;
    m_inputServices               = NULL;
    m_pmqm                        = NULL;
    m_pErrorServiceForXbf         = NULL;
    XCP_WEAK(&m_pBrowserHost);
    m_pBrowserHost                = NULL;
    XCP_WEAK(&m_pHostSite);
    m_pHostSite                   = NULL;
    XCP_WEAK(&m_pSite);
    m_pSite                       = NULL;
    m_bBuilderReady[0]            = FALSE;
    m_bBuilderReady[1]            = FALSE;
    m_pBuilder[0]                 = NULL;
    m_pBuilder[1]                 = NULL;
    m_pGlyphPathBuilder           = NULL;
    m_pTextCore                   = NULL;
    m_isMainTreeLoading           = FALSE;
    m_preqQueue                   = NULL;

    m_fWatch                      = 0;

    m_fIsCoreReady  = FALSE;
    m_objIdentity   = 0;

    // marking isn't strictly necessary because points to this
    XCP_WEAK(&m_pDrawReentrancyCheck);
    m_pDrawReentrancyCheck = this;

    m_bProcessingDownloads        = FALSE;
    m_pNativeManagedPeerQ         = NULL;

    m_lPrvMemCount = MAX_ALLOWEDMEMORY_INCREASE;  // 50 * 1024 *1024;
    m_cTrackingInterval = TRACKING_INTERVAL;
    m_cMaxAllowedMemoryincrease = MAX_ALLOWEDMEMORY_INCREASE; //50 * 1024 *1024;
    m_State = GCTRACKING_STATE_WAITING;

    m_uRuntimeId = 1;

    m_cInheritedPropGenerationCounter = 0;
    m_cIsRightToLeftGenerationCounter = 0;

    m_fWantsRenderingEvent = FALSE;
    m_fSurfaceContentsLostEnqueued = FALSE;

    m_pNWWindowRenderTarget = NULL;

    m_bInResetVisualTree = false;
    m_isFrameAfterVisualTreeReset = false;

    m_fDirtyState = 1;
    m_fInRenderWalk = FALSE;
    m_fDbgEnableThreadingAssert = true;

    m_animationSlowFactor = 0;

    m_bVisibilityToggled = FALSE;

    m_bIsShuttingDown = FALSE;
    m_bInvisibleHitTestMode = FALSE;

    m_cPendingDecodes = 0;
    m_cPendingFontDownloads = 0;
    m_pendingImplicitShowHideCount = 0;

    m_pSystemColorsResources = NULL;
    m_pThemeResources = NULL;

    m_fPrinting = FALSE;
    m_isHolographicOverrideSet = false;

    m_frameworkContext = NULL;
    m_bIsDestroyingCoreServices = FALSE;
    m_pCustomResourceLoader = NULL;

    XCP_STRONG(&m_pWorkItemFactory);
    m_pWorkItemFactory = NULL;
    m_pResourceManager = NULL;
    m_pAppVerificationId = NULL;

    m_qpcDrawMainTreeStart = 0;

    m_pLastLayoutExceptionElement = NULL;

    m_plmListeners.XcpMarkWeakPointers();

    m_zoomScaleChanged_ForceRelayout = FALSE;
    m_zoomScaleChanged_ForceRerender = FALSE;
    m_debugSettingsChanged = TRUE;
    m_commitRequested = FALSE;

    m_fLayoutCompletedNeeded = FALSE;

    m_isWindowVisible = true;
    m_wasWindowEverMadeVisible = false;
    m_shouldUpdateRotationManagerAfterWindowMadeVisible = false;
    m_isRenderEnabled = TRUE;
    m_renderStateChanged = FALSE;
    m_isSuspended = FALSE;
    m_currentSuspendReason = SuspendReason::NotSuspended;

    m_pBackgroundResourceTimer = NULL;
    m_pRenderTargetBitmapManager = NULL;
    m_deviceLost = DeviceLostState::None;
    m_localeSettingChanged = FALSE;
    m_isGeneratingBinaryXaml = FALSE;
    m_bIsUsingGenericXamlFilePath = false;

    m_pHasAnimationsEvent = NULL;
    m_pAnimationsCompleteEvent = NULL;
    m_pHasDeferredAnimationOperationsEvent = nullptr;
    m_pDeferredAnimationOperationsCompleteEvent = nullptr;
    m_pRootVisualResetEvent = NULL;
    m_pImageDecodingIdleEvent = NULL;
    m_pFontDownloadsIdleEvent = nullptr;
    m_pPopupMenuCommandInvokedEvent = nullptr;
    m_pHasBuildTreeWorksEvent = NULL;
    m_pBuildTreeServiceDrainedEvent = nullptr;
    m_pKeyboardInputEvent = nullptr;
    m_pPointerInputEvent = nullptr;
    m_pImplicitShowHideCompleteEvent = nullptr;
    m_fireKeyboardInputEvent = false;
    m_firePointerInputEvent = false;
    m_fontScale = 1.0f;

    m_pendingFirstFrameTraceLoggingEvent = true;
}

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for CCoreServices object
//
//------------------------------------------------------------------------
CCoreServices::~CCoreServices() noexcept
{
    CCoreServices::SetIsCoreServicesReady(FALSE);

    m_bIsDestroyingCoreServices = TRUE;

    // TODO: Do this now or in ResetVisualTree?
    // Shutdown the work items early in the process of shutting down the core
    if (m_pWorkItemFactory != NULL)
    {
        m_pWorkItemFactory->CancelAndCleanupAllWork();
        ReleaseInterface(m_pWorkItemFactory);
    }

    if (m_pmqm)
    {
        // Shut down the media queue manager and prevent adding any more events to queues.
        // Mark the MediaEventQueues as Shutdown state before ResetCoreWindowVisualTree(), so that
        // ResetCoreWindowVisualTree() will not treat the media Queues as not-shutdown while in
        // destroying CoreService.
        //
        // Note: Once MediaEventQueue is set to Shutdown mode, it won't get back to non-shutdown mode.
        //
        IGNOREHR(m_pmqm->ProcessQueues(TRUE));
    }

    // Release the theme before we reset the visual tree
    m_strSystemColorsResourcesXaml.Reset();

    ReleaseInterface(m_pSystemColorsResources);
    if (m_pThemeResources)
    {
        m_pThemeResources->UnpegManagedPeer();
        ReleaseInterface(m_pThemeResources);
    }

    if (m_pBackgroundResourceTimer != NULL)
    {
        IGNOREHR(m_pBackgroundResourceTimer->Stop());

        CValue handler;
        handler.SetInternalHandler(&OnBackgroundResourceTimeout);
        IGNOREHR(m_pBackgroundResourceTimer->RemoveEventListener(EventHandle(KnownEventIndex::DispatcherTimer_Tick), &handler));
    }
    ReleaseInterface(m_pBackgroundResourceTimer);

    // Proactively release our D3D device lost listener to guarantee we synchronize with any pending callback that might be in-flight.
    ReleaseDeviceLostListener();

    // Release the primary window render target
    if (m_pNWWindowRenderTarget != nullptr)
    {
        if (m_pNWWindowRenderTarget->GetDCompTreeHost() != nullptr)
        {
            // Releasing the DComp resources in the registry can clear out DPs in any remaining DOs, which can call back into the core.
            // Since we're shutting down anyway, we don't care about cleaning up - just empty out the registry so that they won't call back.
            m_pNWWindowRenderTarget->GetDCompTreeHost()->AbandonDCompObjectRegistry();
        }

        ReleaseInterface(m_pNWWindowRenderTarget);
    }

    // Take down the visual tree.
    VERIFYHR(ResetCoreWindowVisualTree());
    m_inputServices = nullptr;

    // Clean this up before the rest of the core as its contents may have
    // the final reference to CDependencyObjects. These will not expect
    // the core to be fully shut down before them.

    // End of pre-cleanup
    delete m_pTextCore;
    m_pTextCore = NULL;

    ReleaseInterface(m_pDeploymentTree);
    if (m_pAllSurfaceImageSources != NULL)
    {
        m_pAllSurfaceImageSources->Clean(FALSE);
        SAFE_DELETE(m_pAllSurfaceImageSources);
    }
    if (m_pAllVirtualSurfaceImageSources != NULL)
    {
        m_pAllVirtualSurfaceImageSources->Clean(FALSE);
        SAFE_DELETE(m_pAllVirtualSurfaceImageSources);
    }
    ReleaseInterface(m_pRenderTargetBitmapManager);

    if (m_pEventManager)
    {
        IGNOREHR(m_pEventManager->Disable());
    }
    ReleaseInterface(m_pEventManager);
    ReleaseInterface(m_pTimeManager);

    m_nameScopeRoot.ReleaseAllTables(true);

    // Stop any potentially active downloads before shutting down so we do not get a rogue
    // unactionable callback. Upon canceling, we can expect one last ignorable callback to
    // DownloadCompleted with a cancellation result, and right after CancelDownload returns,
    // m_pFontDownloadListener may be deleted.
    if (m_pFontDownloadQueue != nullptr)
    {
        m_pFontDownloadQueue->CancelDownload();
        m_pFontDownloadQueue.reset();
    }

    //Walk the threadsafe download queue and free all returned items
    if (m_preqQueue)
    {
        DREQUEST *pReq = NULL;
        while (m_preqQueue->Get((void**)&pReq, 0) == S_OK)
        {
            if (pReq)
            {
                pReq->ReleaseResources();
                delete pReq;
                pReq = NULL;
            }
        }
        VERIFYHR(m_preqQueue->Close());
        //null out the pointer to the queue so we don't try to use it anymore
        m_preqQueue = NULL;
    }

    ClearContextObjects();

    // It's now safe to delete the media queue manager as all media queue clients should be cleaned up.
    // Note: XcpShutdown shuts down media queue clients, so this needs to happen after that.
    delete m_pmqm;
    m_pmqm = NULL;

    CLR_CleanupNativePeers();
    if (m_pNativeManagedPeerQ)
    {
        VERIFYHR(m_pNativeManagedPeerQ->Close());
    }

    // Clean up the queue of pending animation operations (for custom properties)
    VERIFYHR(FlushDeferredAnimationOperationQueue(false /*bDoDeferredOperation*/));

    m_spXamlNodeStreamCacheManager.reset();
    m_spXamlSchemaContext.reset();

#if DBG
    InheritedProperties::TraceDependencyObjectTextPropertyUsage();
#endif

    ReleaseInterface(m_pCustomResourceLoader);
    ReleaseInterface(m_pResourceManager);

    ReleaseInterface(m_pLastLayoutExceptionElement);

    // Release any custom error service register for Xbf Generation
    ReleaseInterface(m_pErrorServiceForXbf);

    if (m_pHasAnimationsEvent)
    {
        m_pHasAnimationsEvent->Close();
        m_pHasAnimationsEvent = nullptr;
    }

    if (m_pAnimationsCompleteEvent)
    {
        m_pAnimationsCompleteEvent->Close();
        m_pAnimationsCompleteEvent = nullptr;
    }

    if (m_pHasDeferredAnimationOperationsEvent)
    {
        m_pHasDeferredAnimationOperationsEvent->Close();
        m_pHasDeferredAnimationOperationsEvent = nullptr;
    }

    if (m_pDeferredAnimationOperationsCompleteEvent)
    {
        m_pDeferredAnimationOperationsCompleteEvent->Close();
        m_pDeferredAnimationOperationsCompleteEvent = nullptr;
    }

    if (m_pRootVisualResetEvent)
    {
        m_pRootVisualResetEvent->Close();
        m_pRootVisualResetEvent = nullptr;
    }

    if (m_pImageDecodingIdleEvent)
    {
        m_pImageDecodingIdleEvent->Close();
        m_pImageDecodingIdleEvent = nullptr;
    }

    if (m_pFontDownloadsIdleEvent)
    {
        m_pFontDownloadsIdleEvent->Close();
        m_pFontDownloadsIdleEvent = nullptr;
    }

    if (m_pPopupMenuCommandInvokedEvent != nullptr)
    {
        m_pPopupMenuCommandInvokedEvent->Close();
        m_pPopupMenuCommandInvokedEvent = nullptr;
    }

    if (m_pHasBuildTreeWorksEvent)
    {
        m_pHasBuildTreeWorksEvent->Close();
        m_pHasBuildTreeWorksEvent = nullptr;
    }

    if (m_pBuildTreeServiceDrainedEvent)
    {
        m_pBuildTreeServiceDrainedEvent->Close();
        m_pBuildTreeServiceDrainedEvent = nullptr;
    }

    if (m_pKeyboardInputEvent)
    {
        m_pKeyboardInputEvent->Close();
        m_pKeyboardInputEvent = nullptr;
    }

    if (m_pPointerInputEvent)
    {
        m_pPointerInputEvent->Close();
        m_pPointerInputEvent = nullptr;
    }

    if (m_pImplicitShowHideCompleteEvent)
    {
        m_pImplicitShowHideCompleteEvent->Close();
        m_pImplicitShowHideCompleteEvent = nullptr;
    }

    if (m_hasFacadeAnimationsEvent != nullptr)
    {
        m_hasFacadeAnimationsEvent->Close();
        m_hasFacadeAnimationsEvent = nullptr;
    }

    if (m_facadeAnimationsCompleteEvent != nullptr)
    {
        m_facadeAnimationsCompleteEvent->Close();
        m_facadeAnimationsCompleteEvent = nullptr;
    }

    if (m_hasBrushTransitionsEvent != nullptr)
    {
        m_hasBrushTransitionsEvent->Close();
        m_hasBrushTransitionsEvent = nullptr;
    }

    if (m_brushTransitionsCompleteEvent != nullptr)
    {
        m_brushTransitionsCompleteEvent->Close();
        m_brushTransitionsCompleteEvent = nullptr;
    }

    if (m_animatedFacadePropertyChangesCompleteEvent != nullptr)
    {
        m_animatedFacadePropertyChangesCompleteEvent->Close();
        m_animatedFacadePropertyChangesCompleteEvent = nullptr;
    }

    DetachMemoryManagerEvents();

    // Before terminating, we reset some pointers.  This is because the destruction of the
    // object they point to may reference core services (e.g. UnpegNoRefCoreObjectWithoutPeer)
    // and we want to be in a known state when they do.  If we leave it to the smart pointer,
    // we can't control the destruction order and may end up accessing some member that
    // has already been destructed itself.
    m_pDefaultContentPresenterTemplate.reset();
    m_pFontDownloadQueue.reset();
    m_parserNamescope.reset();

    if (m_calledOleInitialize)
    {
        ::OleUninitialize();
        m_calledOleInitialize = false;
    }

    ASSERT(m_deviceListeners.empty());
    ASSERT(m_renderStateListeners.empty());
    LEAVESECTION(Main);
}

// Creates an instance of the core services object and initializes its
// caches and state.
_Check_return_ HRESULT
CCoreServices::Create(_Out_ CCoreServices **ppCore)
{
    HRESULT hr;
    CCoreServices *pCore = nullptr;

    TraceCoreServicesCreateBegin();
    ENTERSECTION(Main);

    pCore = new CCoreServices;

    pCore->m_nThreadID = GetCurrentThreadId();
    pCore->m_objIdentity = gps->GenerateSecurityToken();

    // Initialize the other context objects

    pCore->InitializeContextObjects();

    pCore->m_spTheming.reset(new FrameworkTheming(
        std::shared_ptr<SystemThemingInterop>(new SystemThemingInterop()),
        [pCore](){ return pCore->NotifyThemeChange(); })
        );

    // This function gets called as a consequence of initializing the DXamlCore,
    // which will also later call OnThemeChanged on the appropriate FrameworkTheming
    // instance (see DXamlCore::InitializeInstance). As such, it is not necessary
    // to notify of a theme change in this call to SetRequestedTheme. Similarly,
    // another thing that might be useful to be aware of is that the first time this
    // function is called occurs as part of the UWP initialization of the FrameworkView.
    // At this point, the App object hasn't been created yet
    // (see FrameworkApplication::MainASTAInitialize), which means that
    // CFxCallBacks::FrameworkApplication_GetApplicationRequestedTheme
    // will return the default value of Theming::None. Once the App object gets
    // created, it might or might not set the RequestedTheme. If it does, the
    // associated call to SetRequestedTheme will override the default value and take
    // care of notifying of this change. Note that this does not apply to Desktop as
    // FrameworkView is not used to initialize DXamlCore in that environment
    // (see FrameworkApplication::StartDesktop).
    IFC(pCore->m_spTheming->SetRequestedTheme(
        FxCallbacks::FrameworkApplication_GetApplicationRequestedTheme(),
        false /* doNotifyThemeChange */));

    pCore->m_themeWalkResourceCache.reset(new ThemeWalkResourceCache());

    // Create the queue manager for deferring certain types of media event processing
    IFC( CMediaQueueManager::Create(&pCore->m_pmqm) );

    // Create the delayed download queue. Limit max download
    // queue length to 50 items
    IFC(gps->QueueCreate(&(pCore->m_preqQueue)));

    IFC(gps->QueueCreate(&(pCore->m_pNativeManagedPeerQ)));

    IFC(gps->CreateWorkItemFactory(&pCore->m_pWorkItemFactory));

    IFC(pCore->GetTextCore(NULL));

    IFC(CRenderTargetBitmapManager::Create(pCore, &pCore->m_pRenderTargetBitmapManager));

    // Now we can return the core services object to the caller

    *ppCore = pCore;
    pCore = NULL;
    hr = S_OK;

    CCoreServices::SetIsCoreServicesReady(TRUE);
Cleanup:

    TraceCoreServicesCreateEnd();

    delete pCore;

    RRETURN(hr);
}

void CCoreServices::InitializeContextObjects()
{
    m_bBuilderReady[0] = FALSE;
    m_bBuilderReady[1] = FALSE;

    m_pBuilder[0] = new CGeometryBuilder(this);
    m_pBuilder[1] = new CGeometryBuilder(this);

    m_pGlyphPathBuilder = new CGlyphPathBuilder;
}

void CCoreServices::ClearContextObjects()
{
    delete m_pBuilder[0];
    m_pBuilder[0] = nullptr;

    delete m_pBuilder[1];
    m_pBuilder[1] = nullptr;

    delete m_pGlyphPathBuilder;
    m_pGlyphPathBuilder = nullptr;
}

//------------------------------------------------------------------------
//
//  Method:   CCoreServices::CreateErrorService
//
//  Synopsis:
//      Creates an instance of an error services object
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCoreServices::CreateErrorService(_Outptr_ IErrorService **ppObject)
{
    RRETURN(CErrorService::Create(this, ppObject));
}

//------------------------------------------------------------------------
//
//  Method:   CCoreServices::CreateErrorServiceForXbfGenerator
//
//  Synopsis:
//      Creates an instance of an error services object that can
//      be used without initializing a browser host.
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCoreServices::CreateErrorServiceForXbfGenerator()
{
    ASSERT(m_pErrorServiceForXbf == NULL);
    IFC_RETURN(CErrorService::Create(this, &m_pErrorServiceForXbf));

    return S_OK;
}

void CCoreServices::RequestReplayPreviousPointerUpdate()
{
    // if we are already scheduled, don't do anything
    if (m_replayPointerUpdateAfterTick) return;

    // New code path reprocesses the pointer message during the tick
    m_replayPointerUpdateAfterTick = true;
    VERIFYHR(m_pBrowserHost->GetFrameScheduler()->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::ReplayPointerUpdate));
}

void CCoreServices::ReplayPreviousPointerUpdate()
{
    if (GetInitializationType() == InitializationType::IslandsOnly)
    {
        // Don't replay if we don't have an active render target
        if (m_pNWWindowRenderTarget)
        {
            // Once multiple islands are supported we should re-examine this approach. Work for this
            // is tracked by task 31608117.
            for (auto& islandData : GetDCompTreeHost()->GetXamlIslandRenderData())
            {
                CXamlIslandRoot* xamlIslandRoot = islandData.first;
                xamlIslandRoot->ReplayPointerUpdate();
            }
        }
    }
    else
    {
        DXamlServices::GetCurrentJupiterWindow()->ReplayPointerUpdate();
    }
}

_Check_return_ HRESULT
CCoreServices::WaitForCommitCompletion()
{
    return m_pNWWindowRenderTarget->GetDCompTreeHost()->WaitForCommitCompletion();
}

//------------------------------------------------------------------------
//
//  Method: ProcessInput
//
//  Synopsis:
//      If there exists an input manager then this will attempt to process input
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ProcessInput(
                            _In_ InputMessage *pMessage,
                            _In_opt_ CContentRoot* contentRoot,
                            _Out_ XINT32 *fHandled)
{
    IFCPTR_RETURN (pMessage);
    IFCPTR_RETURN (fHandled);
    // If there exists an input manager and we have a root visual to hit-test off
    if (m_inputServices && contentRoot && GetMainRootVisual())
    {
        IFC_RETURN(m_inputServices->ProcessInput(pMessage, contentRoot, fHandled));
    }
    return S_OK;

}

//------------------------------------------------------------------------
//
//  Method: ProcessTouchInteractionCallback
//
//  Synopsis:
//      Touch interaction engine notify to input manager for firing
//      touch interaction results(gesture or manipulation)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ProcessTouchInteractionCallback(
    _In_ const xref_ptr<CUIElement> &element,
    _In_ TouchInteractionMsg *message)
{
    IFCPTR_RETURN(element);
    IFCPTR_RETURN(message);

    if (m_inputServices && GetMainRootVisual())
    {
        IFC_RETURN(m_inputServices->ProcessTouchInteractionCallback(element, message));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the lists of all DM manipulations. Removes old manipulations.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetDirectManipulationChanges(
    _Inout_ xvector<CCompositorDirectManipulationViewport *>& directManipulationViewports,
    _Out_ bool *pViewportsChanged
    )
{
    HRESULT hr = S_OK;
    CDMCViewport* pCViewport = NULL;
    CCompositorDirectManipulationViewport* pCompositorDirectManipulationViewport = NULL;
    IObject* pOldCompositorViewport = NULL;
    xvector<CDMCViewport*> compositorViewports;
    xvector<CDMCViewport*>::const_iterator itCV;
    xvector<CDMCViewport*>::const_iterator endCV;

    *pViewportsChanged = FALSE;

    if (m_inputServices && GetMainRootVisual())
    {
        do
        {
            ReleaseInterface(pOldCompositorViewport);
            IFC(m_inputServices->GetOldDirectManipulationViewport(&pOldCompositorViewport));
            if (pOldCompositorViewport)
            {
                *pViewportsChanged = TRUE;
            }
        }
        while (pOldCompositorViewport);

        IFC(m_inputServices->GetDirectManipulationViewports(
            TRUE /*fReturnAllActiveViewports*/,
            compositorViewports
            ));

        endCV = compositorViewports.end();
        for (itCV = compositorViewports.begin(); itCV != endCV; ++itCV)
        {
            pCViewport = (*itCV);
            ASSERT(pCViewport);

            IFC(CCompositorDirectManipulationViewport::Create(
                &pCompositorDirectManipulationViewport,
                pCViewport->GetCompositorViewportNoRef(),
                pCViewport->GetCompositorPrimaryContentNoRef(),
                pCViewport->GetDMCompositorServiceNoRef()));

            IFC(directManipulationViewports.push_back(pCompositorDirectManipulationViewport));
            pCompositorDirectManipulationViewport = NULL;
            *pViewportsChanged = TRUE;
        }
    }

Cleanup:
    endCV = compositorViewports.end();
    for (itCV = compositorViewports.begin(); itCV != endCV; ++itCV)
    {
        pCViewport = (*itCV);
        ASSERT(pCViewport);
        ReleaseInterface(pCViewport);
    }

    ReleaseInterface(pCompositorDirectManipulationViewport);
    ReleaseInterface(pOldCompositorViewport);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   AddRef
//
//  Synopsis:
//      Increases the reference count on the object.
//
//------------------------------------------------------------------------

XUINT32
CCoreServices::AddRef()
{
    return  ++m_cRef;
}


//------------------------------------------------------------------------
//
//  Method:   Release
//
//  Synopsis:
//      Lowers the reference count on the object.  Will delete it when there
// are no remaining references.
//
//------------------------------------------------------------------------

XUINT32
CCoreServices::Release()
{
    XUINT32 cRef = --m_cRef;

    if (0 == cRef)
    {
        delete this;
    }

    return cRef;
}


// Recreates the parser's XamlSchemaContext.
//
// This method also releases the default content presenter's
// template. Any other cached static templates stored in the core
// must be cleared here.
//
//  Note:
//      This is used only by design surfaces (Blend).
// DEAD_CODE_REMOVAL
_Check_return_ HRESULT
CCoreServices::RefreshXamlSchemaContext()
{
    // Replace and initialize the schema context with a new instance
    IFC_RETURN(XamlSchemaContext::Create(this, m_spXamlSchemaContext));
    if (m_spXamlNodeStreamCacheManager)
    {
        IFC_RETURN(m_spXamlNodeStreamCacheManager->ResetCachedXbfV2Readers());
    }
    return S_OK;
}

_Check_return_ HRESULT CCoreServices::ClearDefaultLanguageString()
{
    CTextCore   *pTextCore = nullptr;
    IFC_RETURN(GetTextCore(&pTextCore));
    IFontAndScriptServices *pFontAndScriptServices = nullptr;
    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
    IFC_RETURN(pFontAndScriptServices->ClearDefaultLanguageString());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ResetState
//
//  Synopsis:
//      Reset all of the global state in the core.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCoreServices::ResetState()
{
    HRESULT recordHr = S_OK;

    // Shutdown and flush existing media queues through the manager.
    IGNOREHR(ProcessMediaEvents());

    if (m_pBackgroundResourceTimer != NULL)
    {
        IGNOREHR(m_pBackgroundResourceTimer->Stop());

        CValue handler;
        handler.SetInternalHandler(&OnBackgroundResourceTimeout);
        IGNOREHR(m_pBackgroundResourceTimer->RemoveEventListener(EventHandle(KnownEventIndex::DispatcherTimer_Tick), &handler));
    }
    ReleaseInterface(m_pBackgroundResourceTimer);

    if (CContentRoot* contentRoot = m_contentRootCoordinator.Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        RECORDFAILURE(contentRoot->Close());
    }

    m_pMainVisualTree = nullptr;
    m_inputServices = nullptr;

    // Release some stuff

    m_nameScopeRoot.ReleaseAllTables(false);

    ReleaseInterface(m_pTimeManager);


    if (m_pEventManager)
    {
        IGNOREHR(m_pEventManager->Disable());
    }
    ReleaseInterface(m_pEventManager);

    if (m_pResourceManager != nullptr)
    {
        m_pResourceManager->DetachEvents();
    }
    ReleaseInterface(m_pResourceManager);

    LogCoreServicesEvent(CoreServicesEvent::StateReset);
    m_uFrameNumber = 1;
    m_fIsCoreReady = FALSE;

    {
        auto lock = m_AppVerificationIdLock.lock();
        ReleaseInterface(m_pAppVerificationId);
    }

    m_elementsWithThemeChangedListener.clear();

    m_automationEventsHelper.CleanUpStructureChangedRequests();

    m_appliedStyleTables.clear();

    return recordHr; //RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the dirty state of registered elements
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCoreServices::UpdateDirtyState()
{
    UpdateStateChildrenVector::const_iterator end = m_rgpChildrenForUpdate.end();
    for (UpdateStateChildrenVector::const_iterator it = m_rgpChildrenForUpdate.begin(); it != end; ++it)
    {
        IFC_RETURN((*it)->UpdateState());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a redirection element to the registration list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RegisterRedirectionElement(_In_ CUIElement *pRedirectionElement)
{
    RRETURN(m_redirectionElementsNoRef.push_back(pRedirectionElement));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a redirection element from the registration list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UnregisterRedirectionElement(_In_ CUIElement *pRedirectionElement)
{
    xvector<CUIElement*>::const_reverse_iterator rend = m_redirectionElementsNoRef.rend();
    for (xvector<CUIElement*>::const_reverse_iterator it = m_redirectionElementsNoRef.rbegin(); it != rend; ++it)
    {
        if ((*it) == pRedirectionElement)
        {
            IFC_RETURN(m_redirectionElementsNoRef.erase(it));
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidates redirected elements, if necessary based on other changes that have
//      occurred in the tree.
//
//------------------------------------------------------------------------
void
CCoreServices::InvalidateRedirectionElements()
{
    // Redirection elements use context from parts of the UIElement tree that are neither in their
    // subgraph nor ancestor chain, where normal property change invalidation is handled.
    // Property changes anywhere in the tree might be used as context for a redirection element
    // during the render walk. The render walk may also modify the composition tree, which could change
    // the target comp nodes for redirection. As a result, all redirection elements are marked dirty prior
    // to each render walk to ensure the walk visits them.
    xvector<CUIElement*>::const_iterator end = m_redirectionElementsNoRef.end();
    for (xvector<CUIElement*>::const_iterator it = m_redirectionElementsNoRef.begin(); it != end; ++it)
    {
        CUIElement *pRedirectionElement = *it;

        CUIElement::NWSetRedirectionDataDirty(pRedirectionElement, DirtyFlags::Render);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Adds a gripper element to the registration list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RegisterGripper(_In_ IGripper *pGripper)
{
    IFC_RETURN(UnregisterGripper(pGripper));
    IFC_RETURN(m_grippersNoRef.push_back(pGripper));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Removes a gripper element from the registration list.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UnregisterGripper(_In_ IGripper *pGripper)
{
    xvector<IGripper*>::const_reverse_iterator rend = m_grippersNoRef.rend();
    for (xvector<IGripper*>::const_reverse_iterator it = m_grippersNoRef.rbegin(); it != rend; ++it)
    {
        if ((*it) == pGripper)
        {
            IFC_RETURN(m_grippersNoRef.erase(it));
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates grippers. The visibility of a gripper changes depending on whether its
//      overlap with ancestors, and whether there are active animations or manipulations
//      among its ancestors.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UpdateGripperVisibility()
{
    xvector<IGripper*>::const_iterator end = m_grippersNoRef.end();
    for (xvector<IGripper*>::const_iterator it = m_grippersNoRef.begin(); it != end; ++it)
    {
        IGripper *pGripper = *it;

        IFC_RETURN(pGripper->UpdateVisibility());
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   GetDefaultInheritedPropertyValue
//
//  Synopsis:
//      Provides the ultimate fallback value for an inherited property
//      when for GetValue when no ancestors in the tree have set the property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::GetDefaultInheritedPropertyValue(_In_ KnownPropertyIndex nUserIndex, _Out_ CValue* pValue)
{
    switch (nUserIndex)
    {
        case KnownPropertyIndex::FontIcon_FontFamily:
        {
            xref_ptr<CDependencyObject> fontFamily;
            IFC_RETURN(CDependencyProperty::GetDefaultFontIconFontFamily(this, fontFamily.ReleaseAndGetAddressOf()));
            pValue->SetObjectAddRef(fontFamily.get());
            break;
        }

        case KnownPropertyIndex::TextElement_FontFamily:
        case KnownPropertyIndex::TextBlock_FontFamily:
        case KnownPropertyIndex::Control_FontFamily:
        case KnownPropertyIndex::RichTextBlock_FontFamily:
        case KnownPropertyIndex::ContentPresenter_FontFamily:
        {
            CTextCore   *pTextCore = NULL;
            CFontFamily *pUltimateFont = NULL;
            IFC_RETURN(GetTextCore(&pTextCore));
            IFontAndScriptServices *pFontAndScriptServices = NULL;
            IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
            IFC_RETURN(pFontAndScriptServices->GetUltimateFont(&pUltimateFont));
            pValue->SetObjectNoRef(pUltimateFont);
            break;
        }

        case KnownPropertyIndex::TextElement_Foreground:
        case KnownPropertyIndex::TextBlock_Foreground:
        case KnownPropertyIndex::Control_Foreground:
        case KnownPropertyIndex::RichTextBlock_Foreground:
        case KnownPropertyIndex::ContentPresenter_Foreground:
        case KnownPropertyIndex::IconElement_Foreground:
        {
            CBrush *pDefaultTextBrush = NULL;
            IFC_RETURN(GetDefaultTextBrush(&pDefaultTextBrush));
            pValue->SetObjectNoRef(pDefaultTextBrush);
            break;
        }

        case KnownPropertyIndex::FontIcon_FontSize:
            pValue->SetFloat(20.f);
            break;

        case KnownPropertyIndex::TextElement_FontSize:
        case KnownPropertyIndex::TextBlock_FontSize:
        case KnownPropertyIndex::Control_FontSize:
        case KnownPropertyIndex::RichTextBlock_FontSize:
        case KnownPropertyIndex::ContentPresenter_FontSize:
            pValue->SetFloat(11.0f); // 11 pixels
            break;

        case KnownPropertyIndex::TextElement_CharacterSpacing:
        case KnownPropertyIndex::TextBlock_CharacterSpacing:
        case KnownPropertyIndex::Control_CharacterSpacing:
        case KnownPropertyIndex::RichTextBlock_CharacterSpacing:
        case KnownPropertyIndex::ContentPresenter_CharacterSpacing:
            pValue->SetSigned(0);
            break;

        case KnownPropertyIndex::TextElement_FontWeight:
        case KnownPropertyIndex::TextBlock_FontWeight:
        case KnownPropertyIndex::Control_FontWeight:
        case KnownPropertyIndex::RichTextBlock_FontWeight:
        case KnownPropertyIndex::ContentPresenter_FontWeight:
        case KnownPropertyIndex::FontIcon_FontWeight:
            pValue->Set(DirectUI::CoreFontWeight::Normal);
            break;

        case KnownPropertyIndex::TextElement_FontStyle:
        case KnownPropertyIndex::TextBlock_FontStyle:
        case KnownPropertyIndex::Control_FontStyle:
        case KnownPropertyIndex::RichTextBlock_FontStyle:
        case KnownPropertyIndex::ContentPresenter_FontStyle:
        case KnownPropertyIndex::FontIcon_FontStyle:
            pValue->Set(DirectUI::FontStyle::Normal);
            break;

        case KnownPropertyIndex::TextElement_FontStretch:
        case KnownPropertyIndex::TextBlock_FontStretch:
        case KnownPropertyIndex::Control_FontStretch:
        case KnownPropertyIndex::RichTextBlock_FontStretch:
        case KnownPropertyIndex::ContentPresenter_FontStretch:
            pValue->Set(DirectUI::FontStretch::Normal);
            break;

        case KnownPropertyIndex::TextElement_TextDecorations:
        case KnownPropertyIndex::TextBlock_TextDecorations:
        case KnownPropertyIndex::RichTextBlock_TextDecorations:
            pValue->Set(DirectUI::TextDecorations::None);
            break;

        case KnownPropertyIndex::TextOptions_TextHintingMode:
            pValue->Set(DirectUI::TextHintingMode::Fixed);
            break;

        case KnownPropertyIndex::TextOptions_TextFormattingMode:
            ASSERT(FALSE, L"Unexpected call to GetDefaultInheritedPropertyValue(KnownPropertyIndex::TextOptions_TextFormattingMode)");
            pValue->Set(DirectUI::TextFormattingMode::Ideal);
            break;

        case KnownPropertyIndex::TextOptions_TextRenderingMode:
            ASSERT(FALSE, L"Unexpected call to GetDefaultInheritedPropertyValue(KnownPropertyIndex::TextOptions_TextRenderingMode)");
            pValue->Set(DirectUI::TextRenderingMode::Auto);
            break;

        case KnownPropertyIndex::TextElement_Language:
        case KnownPropertyIndex::FrameworkElement_Language:
        {
            CTextCore *pTextCore = NULL;
            xstring_ptr strLanguage;
            IFC_RETURN(GetTextCore(&pTextCore));
            IFontAndScriptServices *pFontAndScriptServices = NULL;
            IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
            IFC_RETURN(pFontAndScriptServices->GetDefaultLanguageString(&strLanguage));
            pValue->SetString(strLanguage);
            break;
        }

        case KnownPropertyIndex::FrameworkElement_DataContext:
            pValue->SetObjectNoRef(nullptr);
            break;

        case KnownPropertyIndex::UIElement_AllowDrop:
            pValue->SetBool(FALSE);
            break;

        case KnownPropertyIndex::FrameworkElement_FlowDirection:
            pValue->Set(DirectUI::FlowDirection::LeftToRight);
            break;

        case KnownPropertyIndex::Run_FlowDirection:
            pValue->Set(DirectUI::FlowDirection::LeftToRight);
            break;

        case KnownPropertyIndex::TextElement_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::TextBlock_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::Control_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::RichTextBlock_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::ContentPresenter_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::FontIcon_IsTextScaleFactorEnabled:
        case KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal:
            pValue->SetBool(TRUE);
            break;

        case KnownPropertyIndex::FrameworkElement_AllowFocusOnInteraction:
        case KnownPropertyIndex::TextElement_AllowFocusOnInteraction:
        case KnownPropertyIndex::FlyoutBase_AllowFocusOnInteraction:
            pValue->SetBool(TRUE);
            break;

        case KnownPropertyIndex::FrameworkElement_AllowFocusWhenDisabled:
        case KnownPropertyIndex::FlyoutBase_AllowFocusWhenDisabled:
            pValue->SetBool(FALSE);
            break;

        case KnownPropertyIndex::UIElement_HighContrastAdjustment:
            pValue->Set(DirectUI::ElementHighContrastAdjustment::Application);
            break;

        case KnownPropertyIndex::UIElement_KeyTipPlacementMode:
        case KnownPropertyIndex::TextElement_KeyTipPlacementMode:
            pValue->Set(DirectUI::KeyTipPlacementMode::Auto);
            break;

        case KnownPropertyIndex::UIElement_KeyboardAcceleratorPlacementMode:
            pValue->Set(DirectUI::KeyboardAcceleratorPlacementMode::Auto);
            break;

        case KnownPropertyIndex::UIElement_KeyTipHorizontalOffset:
        case KnownPropertyIndex::TextElement_KeyTipHorizontalOffset:
        case KnownPropertyIndex::UIElement_KeyTipVerticalOffset:
        case KnownPropertyIndex::TextElement_KeyTipVerticalOffset:
            pValue->SetFloat(0.0f);
            break;

        case KnownPropertyIndex::UIElement_XYFocusLeftNavigationStrategy:
        case KnownPropertyIndex::UIElement_XYFocusRightNavigationStrategy:
        case KnownPropertyIndex::UIElement_XYFocusUpNavigationStrategy:
        case KnownPropertyIndex::UIElement_XYFocusDownNavigationStrategy:
        case KnownPropertyIndex::Hyperlink_XYFocusLeftNavigationStrategy:
        case KnownPropertyIndex::Hyperlink_XYFocusRightNavigationStrategy:
        case KnownPropertyIndex::Hyperlink_XYFocusUpNavigationStrategy:
        case KnownPropertyIndex::Hyperlink_XYFocusDownNavigationStrategy:
            pValue->Set(DirectUI::XYFocusNavigationStrategy::Auto);
            break;

        default:
            IFC_RETURN(E_INVALIDARG);
            break;
    }

    return S_OK;
}

// Retrieves a solid color brush to be used as the default FrameworkElement::FocusVisualSecondaryBrush
// or FrameworkElement::FocusVisualPrimaryBrush depending on the forFocusVisualSecondaryBrush flag.
// A solid color brush with the provided color is returned. The brush is cached for quicker
// subsequent retrievals. This method is only invoked in the rare cases where the
// SystemControlFocusVisualPrimaryBrush or SystemControlFocusVisualSecondaryBrush dictionary resource
// could not be found.
_Check_return_ HRESULT
CCoreServices::GetDefaultFocusVisualSolidColorBrush(
    _In_ bool forFocusVisualSecondaryBrush,
    _In_ XUINT32 color,
    _Outptr_ CSolidColorBrush** ppSolidColorBrush)
{
    *ppSolidColorBrush = nullptr;

    auto spNewSolidColorBrush = forFocusVisualSecondaryBrush ? m_defaultFocusVisualSecondarySolidColorBrush : m_defaultFocusVisualPrimarySolidColorBrush;
    if (spNewSolidColorBrush == nullptr || spNewSolidColorBrush->m_rgb != color)
    {
        // No solid color brush has been cached, or the cache's color does not match the requested color.
        // Create a new solid color brush with the requested color.
        CValue value;
        value.SetColor(color);
        CREATEPARAMETERS createParameters(this, value);
        IFC_RETURN(CSolidColorBrush::Create(reinterpret_cast<CDependencyObject**>(spNewSolidColorBrush.ReleaseAndGetAddressOf()), &createParameters));

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to nullptr.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay nullptr.
        spNewSolidColorBrush->SimulateFreeze();

        if (forFocusVisualSecondaryBrush)
        {
            m_defaultFocusVisualSecondarySolidColorBrush = spNewSolidColorBrush;
        }
        else
        {
            m_defaultFocusVisualPrimarySolidColorBrush = spNewSolidColorBrush;
        }
    }

    *ppSolidColorBrush = spNewSolidColorBrush.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetDefaultTextBrush
//
//  Synopsis:
//      Creates, caches and returns the default text brush.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::GetDefaultTextBrush(
                             _Outptr_ CBrush **ppBrush
                             )
{
    *ppBrush = nullptr;
    if (!m_defaultTextBrush)
    {
        // Query the default theme brush from the theme dictionary that matches the app's theme.
        // If we don't explicitly specify the theme to use during the lookup, we could end up
        // querying the default brush from a theme dictionary that doesn't match the app's theme.
        // This can happen if the first element that causes us to initialize our default text-formatting
        // object has a FrameworkElement.RequestedTheme that is different from the app's theme.
        // The default text brush should always match the app's theme.  Elements that override
        // theme via FrameworkElement.RequestedTheme will not use the default brush but will instead
        // override it in CFrameworkElement::NotifyThemeChangedForInheritedProperties().
        xref_ptr<CDependencyObject> defaultBrush;
        IFC_RETURN(LookupThemeResource(m_spTheming->GetBaseTheme(), XSTRING_PTR_EPHEMERAL(L"DefaultTextForegroundThemeBrush"), defaultBrush.ReleaseAndGetAddressOf()));

        // DefaultTextBrush was not found in Theme Resources. Use brush with hard-coded color
        if (!defaultBrush)
        {
            DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strWhite, L"White");
            CREATEPARAMETERS createParameters(this, c_strWhite);
            IFC_RETURN(CSolidColorBrush::Create(defaultBrush.ReleaseAndGetAddressOf(), &createParameters));
        }

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to NULL.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay NULL.
        defaultBrush->SimulateFreeze();
        m_defaultTextBrush = static_sp_cast<CBrush>(std::move(defaultBrush));
    }

    m_defaultTextBrush.CopyTo(ppBrush);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetTextSelectionGripperBrush
//
//  Synopsis:
//      Creates, caches and returns the text selection gripper brush.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::GetTextSelectionGripperBrush(
                             _Outptr_ CBrush **ppFillBrush
                             )
{
    *ppFillBrush = nullptr;

    if (m_textSelectionGripperFillBrush == nullptr)
    {
        xref_ptr<CDependencyObject> defaultBrush;
        if (m_spTheming->HasHighContrastTheme())
        {
            IFC_RETURN(LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"SystemColorWindowBrush"), defaultBrush.ReleaseAndGetAddressOf()));
        }
        else
        {
            CValue valueGripperFillColor;
            valueGripperFillColor.SetColor(0xFFFFFFFF);
            CREATEPARAMETERS cpGripperFillColor(this, valueGripperFillColor);
            IFC_RETURN(CSolidColorBrush::Create(defaultBrush.ReleaseAndGetAddressOf(), &cpGripperFillColor));
        }

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to NULL.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay NULL.
        defaultBrush->SimulateFreeze();
        m_textSelectionGripperFillBrush = static_sp_cast<CBrush>(std::move(defaultBrush));
    }

    m_textSelectionGripperFillBrush.CopyTo(ppFillBrush);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemTextSelectionBackgroundBrush
//
//  Synopsis:
//      Creates, caches and returns the selection background brush
//      for current theme.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetSystemTextSelectionBackgroundBrush(
    _Outptr_ CSolidColorBrush **ppBrush)
{
    *ppBrush = nullptr;
    if (m_textSelectionBackgroundBrush == nullptr)
    {
        xref_ptr<CDependencyObject> defaultBrush;
        IFC_RETURN(LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"SystemColorHighlightBrush"), defaultBrush.ReleaseAndGetAddressOf()));

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to NULL.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay NULL.
        defaultBrush->SimulateFreeze();
        m_textSelectionBackgroundBrush = static_sp_cast<CSolidColorBrush>(std::move(defaultBrush));
    }

    m_textSelectionBackgroundBrush.CopyTo(ppBrush);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemColorWindowBrush
//
//  Synopsis:
//      Creates, caches and returns the SystemColorWindow brush
//      for current theme.
//
//------------------------------------------------------------------------
CSolidColorBrush* CCoreServices::GetSystemColorWindowBrush()
{
    CSolidColorBrush *pBrush;

    EnsureBrush(m_systemColorWindowBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorWindowBrush"));

    m_systemColorWindowBrush.CopyTo(&pBrush);

    return pBrush;
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemColorWindowTextBrush
//
//  Synopsis:
//      Creates, caches and returns the SystemColorWindowText brush
//      for current theme.
//
//------------------------------------------------------------------------
CSolidColorBrush* CCoreServices::GetSystemColorWindowTextBrush()
{
    CSolidColorBrush *pBrush;

    EnsureBrush(m_systemColorWindowTextBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorWindowTextBrush"));

    m_systemColorWindowTextBrush.CopyTo(&pBrush);

    return pBrush;
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemColorDisabledTextBrush
//
//  Synopsis:
//      Creates, caches and returns the SystemColorDisabledText brush
//      for current theme.
//
//------------------------------------------------------------------------
CSolidColorBrush* CCoreServices::GetSystemColorDisabledTextBrush()
{
    CSolidColorBrush *pBrush;

    EnsureBrush(m_systemColorDisabledTextBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorDisabledTextBrush"));

    m_systemColorDisabledTextBrush.CopyTo(&pBrush);

    return pBrush;
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemColorHotlightBrush
//
//  Synopsis:
//      Creates, caches and returns the SystemColorHotlight brush
//      for current theme.
//
//------------------------------------------------------------------------
CSolidColorBrush* CCoreServices::GetSystemColorHotlightBrush()
{
    CSolidColorBrush *pBrush;

    EnsureBrush(m_systemColorHotlightBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorHotlightBrush"));

    m_systemColorHotlightBrush.CopyTo(&pBrush);

    return pBrush;
}

CSolidColorBrush* CCoreServices::GetSystemColorWindowBrushNoRef()
{
    EnsureBrush(m_systemColorWindowBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorWindowBrush"));

    return m_systemColorWindowBrush;
}

CSolidColorBrush* CCoreServices::GetSystemColorWindowTextBrushNoRef()
{
    EnsureBrush(m_systemColorWindowTextBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorWindowTextBrush"));

    return m_systemColorWindowTextBrush;
}

CSolidColorBrush* CCoreServices::GetSystemColorDisabledTextBrushNoRef()
{
    EnsureBrush(m_systemColorDisabledTextBrush, XSTRING_PTR_EPHEMERAL(L"SystemColorDisabledTextBrush"));

    return m_systemColorDisabledTextBrush;
}

void CCoreServices::EnsureBrush(xref_ptr<CSolidColorBrush>& brush, _In_ const xstring_ptr_view& key)
{
    if (brush == nullptr)
    {
        xref_ptr<CDependencyObject> defaultBrush;
        IFCFAILFAST(LookupThemeResource(key, defaultBrush.ReleaseAndGetAddressOf()));

        if (defaultBrush != nullptr)
        {
            defaultBrush->SimulateFreeze();
            brush = static_sp_cast<CSolidColorBrush>(std::move(defaultBrush));
        }
    }
}

// Reset the brushes that are lazily instantiated. We do this on theme changes and when changing theme resources.
// It is also done in the ShutdownToIdle method for leak detection.
void CCoreServices::ResetThemedBrushes()
{
    m_defaultTextBrush.reset();
    m_textSelectionGripperFillBrush.reset();
    m_textSelectionBackgroundBrush.reset();
    m_textSelectionForegroundBrush.reset();
    m_systemColorWindowBrush.reset();
    m_systemColorWindowTextBrush.reset();
    m_systemColorDisabledTextBrush.reset();
    m_systemColorHotlightBrush.reset();
    m_defaultFocusVisualSecondarySolidColorBrush.reset();
    m_defaultFocusVisualPrimarySolidColorBrush.reset();
}
//------------------------------------------------------------------------
//
//  Method:   GetSystemTextSelectionForegroundBrush
//
//  Synopsis:
//
//      Creates, caches and returns the selection foreground brush
//      for current theme.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetSystemTextSelectionForegroundBrush(
    _Outptr_ CSolidColorBrush **ppBrush)
{
    *ppBrush = nullptr;

    if (m_textSelectionForegroundBrush == nullptr)
    {
        xref_ptr<CDependencyObject> defaultBrush;
        IFC_RETURN(LookupThemeResource(XSTRING_PTR_EPHEMERAL(L"SystemColorHighlightTextBrush"), defaultBrush.ReleaseAndGetAddressOf()));

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to NULL.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay NULL.
        defaultBrush->SimulateFreeze();
        m_textSelectionForegroundBrush = static_sp_cast<CSolidColorBrush>(std::move(defaultBrush));
    }

    m_textSelectionForegroundBrush.CopyTo(ppBrush);
    return S_OK;
}

// Ensures theme resources and looks up resource from key.
// Uses the app theme if SetRequestedThemeForSubTree has not been called.
_Check_return_ HRESULT
CCoreServices::LookupThemeResource(
    _In_ const xstring_ptr_view& strKey,
    _Outptr_ CDependencyObject **ppValue)
{
    HRESULT hr = S_OK;
    CDependencyObject *pValue = NULL;
    ASSERT(!strKey.IsNull());

    // Load Theme Resources if needed
    if (!m_pThemeResources)
    {
        IFC(FxCallbacks::FrameworkCallbacks_LoadThemeResources());
    }

    // Get Resource from theme resources.
    if (m_pThemeResources)
    {
        IFC(m_pThemeResources->GetKeyNoRef(strKey, &pValue));

        // If this is a theme resource key, allow Application.Resources to override it.
        if (pValue)
        {
            auto appResources = GetApplicationResourceDictionary();
            if (appResources)
            {
                CDependencyObject *pApplicationResource = NULL;
                IFC(appResources->GetKeyNoRef(strKey, Resources::LookupScope::LocalOnly, &pApplicationResource));
                if (pApplicationResource)
                {
                    pValue = pApplicationResource;
                }
            }
        }

        // Resource Dictionary doesn't AddRef returned value
        AddRefInterface(pValue);
    }

    *ppValue = pValue;
    pValue = NULL;

Cleanup:
    ReleaseInterface(pValue);
    RRETURN(hr);
}

// Looks up a resource with the given theme.
_Check_return_ HRESULT CCoreServices::LookupThemeResource(_In_ Theming::Theme theme, _In_ const xstring_ptr_view& key, _Outptr_ CDependencyObject **ppValue)
{
    // Push theme to use for resource lookup.
    const auto oldTheme = GetRequestedThemeForSubTree();
    const bool pushPopTheme = Theming::GetBaseValue(theme) != oldTheme;
    if (pushPopTheme)
    {
        SetRequestedThemeForSubTree(theme);
    }

    // Pop theme if necessary when we return.
    auto popThemeScopeGuard = wil::scope_exit([&]()
    {
        if (pushPopTheme)
        {
            SetRequestedThemeForSubTree(oldTheme);
        }
    });

    // Look up resource.
    IFC_RETURN(LookupThemeResource(key, ppValue));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetTransparentBrush
//
//  Synopsis:
//      Creates, caches and returns a transparent brush
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::GetTransparentBrush(
                                   _Outptr_ CBrush **ppBrush
                                   )
{
    if (m_transparentBrush == nullptr)
    {
        CREATEPARAMETERS createParameters(this);
        xref_ptr<CDependencyObject> defaultBrush;
        IFC_RETURN(CTransparentUnoptimizedBrush::Create(defaultBrush.ReleaseAndGetAddressOf(), &createParameters));

        // This brush is a global default and should not be modified.  Setting
        //  this "simulate freeze" flag blocks calls to SetValue().  This is
        //  enough to make sure a SolidColorBrush won't change.  The local
        //  properties are: Color, Opacity, Transform, and RelativeTransform.
        // * Color & Opacity are local values, and blocking SetValue() prevents
        //  their change.
        // * Transform & RelativeTransform are references to other objects, but
        //  they default to NULL.  So for this brush there's nothing to change
        //  under us, and blocking SetValue() means they will stay NULL.
        defaultBrush->SimulateFreeze();

        m_transparentBrush = static_sp_cast<CSolidColorBrush>(std::move(defaultBrush));
    }

    m_transparentBrush.CopyTo(ppBrush);
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Function:   GetSystemGlyphTypefaces
//
//  Synopsis:
//
//------------------------------------------------------------------------
HRESULT _Check_return_
CCoreServices::GetSystemGlyphTypefaces(_Outptr_ CDependencyObject **ppDo)
{
    RRETURN(E_NOTIMPL);
}

//------------------------------------------------------------------------
//
//  Method:   RegisterScriptCallback
//
//  Synopsis:
//      Registers the global script call back
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::RegisterScriptCallback(
                                      _In_ void *pControl,
                                      _In_ EVENTPFN pfnAsync,
                                      _In_ EVENTPFN pfnSync)
{
    // store the pointer to the control and the call back
    m_pEventManager->m_pfnScriptCallbackAsync = pfnAsync;
    m_pEventManager->m_pfnScriptCallbackSync = pfnSync;
    m_pEventManager->m_pControl = pControl;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ParsePropertyPath
//
//  Synopsis:
//      Parses a property path and returns the object and offset to the data
//  for that property in the object.
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)
_Check_return_ HRESULT
CCoreServices::ParsePropertyPath(
    _Inout_ CDependencyObject **ppTarget,
    _Outptr_ const CDependencyProperty** ppDP,
    _In_ const xstring_ptr_view& strPath,
    _In_opt_ const std::map<xstring_ptr, const CDependencyProperty*>& resolvedPathTypes)
{
    HRESULT hr = E_FAIL;
    XNAME nameClass = {0};
    XNAME nameProperty;
    XUINT32 cString;
    const WCHAR* pString = strPath.GetBufferAndCount(&cString);
    XUINT32 nIndex;
    XINT32 bParen;
    XINT32 bResolve;
    CDependencyObject *pTargetNoRef = *ppTarget;
    CDependencyObject *pTempDO = NULL;
    CDependencyObject* pShareableCandidateNoRef = nullptr;
    CNoParentShareableDependencyObject *pClone = NULL;
    const CClassInfo *pClass = NULL;
    KnownPropertyIndex nPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    CValue value;

    value.Unset();

    // Get the metadata for the current object.  It is error to not find it.

    if (NULL == (pClass = pTargetNoRef->GetClassInformation()))
    {
        if (CTimeManager::ShouldFailFast())
        {
            IFCFAILFAST(E_UNEXPECTED);
        }
        else
        {
            IFC(E_UNEXPECTED);
        }
    }

    TrimWhitespace(cString, pString, &cString, &pString);

    while (cString)
    {
        // Check for initial parenthesis

        if (cString && (L'(' == *pString))
        {
            bParen = TRUE;
            pString++;
            cString--;
        }
        else
        {
            bParen = FALSE;
        }

        // This could either be a class or a property, we won't know until we find
        // a period in the property path.  Initially we'll assume it is a property.

        IFC(NameFromString(cString, pString, &cString, &pString, &nameProperty));

        TrimWhitespace(cString, pString, &cString, &pString);

        // If there is a period here then the previous name was the class.

        if (cString && (L'.' == *pString))
        {
            pString++;
            cString--;

            // Make the previous name the type

            nameClass = nameProperty;

            // Read the property name

            IFC(NameFromString(cString, pString, &cString, &pString, &nameProperty));
        }
        else
        {
            nameClass.cName = 0;
        }

        TrimWhitespace(cString, pString, &cString, &pString);

        // If there was an open parenthesis look for the closing one.

        if (bParen)
        {
            if (!cString || (L')' != *pString))
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_FAIL);
                }
                else
                {
                    IFC(E_FAIL);
                }
            }

            pString++;
            cString--;
        }

        TrimWhitespace(cString, pString, &cString, &pString);

        // Get the DP for this part of the path
        IFC(GetPropertyIndexForPropertyPath(
            pTargetNoRef,
            bParen,
            &nameClass,
            &nameProperty,
            pClass,
            &nPropertyIndex,
            resolvedPathTypes));

        bResolve = TRUE;

        // If we are at the first step of a multi-step property path, check to
        //  see if we need to substitute a cloned copy for the actual
        //  animation.  This is so animations targeting pTargetNoRef. pdp would not
        //  affect other objects sharing the same value.
        if (pTargetNoRef == *ppTarget &&
            cString && (L'[' == *pString || L'.' == *pString))
        {
            CNoParentShareableDependencyObject *pShareable = NULL;

            // Retrieve that property value.
            IFC(pTargetNoRef->GetValueByIndex(nPropertyIndex, &value));

            // Make sure we get an non-NULL object
            if (value.AsObject() == nullptr)
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_UNEXPECTED);
                }
                else
                {
                    IFC(E_UNEXPECTED);
                }
            }

            pShareableCandidateNoRef = value.AsObject();

            // Can't use DoPointerCast because CNoParentShareableDependencyObject is hidden from public view
            //  and is hence absent in the class hierarchy metadata.
            if (pShareableCandidateNoRef->OfTypeByIndex<KnownTypeIndex::Brush>() ||
                pShareableCandidateNoRef->OfTypeByIndex<KnownTypeIndex::GeneralTransform>())
            {
                pShareable = reinterpret_cast<CNoParentShareableDependencyObject*>(pShareableCandidateNoRef);

                // If the value was already clone of a shared value, we would not need
                //  to clone it again.
                if (!pShareable->IsClone())
                {
                    // See if this Sharable object implements cloning.
                    hr = pShareable->Clone(&pClone);

                    if (hr != E_NOTIMPL)
                    {
                        // If an actual error, pass it along.
                        IFC(hr);

                        // If this ASSERT fails, the Clone() implementation of pShareable failed to
                        //  call the correct constructor, or the constructor didn't properly call
                        //  its base type's constructor.  This flag is set all the way at the end
                        //  of the chain in CNoParentShareableDependencyObject(CNoParentShareableDependencyObject&,HRESULT&)
                        ASSERT(pClone->IsClone());

                        value.WrapObjectNoRef(pClone);
                        IFC(pTargetNoRef->SetValueByIndex(nPropertyIndex, value));
                    }
                    else
                    {
                        // This type does not support cloning, so we leave things
                        //  untouched.
                    }
                }
            }

            // Clean up
            ReleaseInterface(pClone);
            value.SetNull();
        }

        // Deal with indexed properties such as:
        // <... TargetProperty='RenderTransform.Children[3].X' ...>
        // Here we need to get the third object from the collection specified by
        // the property named 'Children'

        if (cString && (L'[' == *pString))
        {
            bResolve = FALSE;
            CDOCollection* pCollectionNoRef = nullptr;

            // Move to the new object, release the old one if it's not the original one we started with
            if (pTargetNoRef != *ppTarget)
            {
                ASSERT(pTargetNoRef == pTempDO);
                // Since pTempDO is the same as pTargetNoRef - release it so we don't double delete later.
                ReleaseInterface(pTempDO);
            }

            IFC(pTargetNoRef->GetValueByIndex(nPropertyIndex, &value));

            // Make sure we get an non-NULL object
            if (value.AsObject() == nullptr)
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_UNEXPECTED);
                }
                else
                {
                    IFC(E_UNEXPECTED);
                }
            }

            pTargetNoRef = value.AsObject();
            SetInterface(pTempDO, pTargetNoRef);

            // Get its metadata
            if (NULL == (pClass = pTargetNoRef->GetClassInformation()))
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_UNEXPECTED);
                }
                else
                {
                    IFC(E_UNEXPECTED);
                }
            }

            pString++;
            cString--;

            const XUINT32 cPrevious = cString;

            IFC(SignedFromDecimalString(cString, pString, &cString, &pString, (XINT32 *) &nIndex));

            const bool hasValidIndexValue = (cString != cPrevious);

            TrimWhitespace(cString, pString, &cString, &pString);

            // Fail on invalid index value or not finding a closing bracket.

            if (!hasValidIndexValue || (nIndex & 0x80000000) || !cString || (L']' != *pString))
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_FAIL);
                }
                else
                {
                    IFC(E_FAIL);
                }
            }

            pString++;
            cString--;

            // The target object must be a collection
            if (!pTargetNoRef->OfTypeByIndex<KnownTypeIndex::PresentationFrameworkCollection>())
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_FAIL);
                }
                else
                {
                    IFC(E_FAIL);
                }
            }
            pCollectionNoRef = static_cast<CDOCollection*>(pTargetNoRef);

            // Verify that the desired index is in bounds
            IFCCHECK(nIndex < pCollectionNoRef->GetCount());

            // Verify that the collection holds DependencyObject since
            // we can't target value types.
            IFCCATASTROPHIC(pCollectionNoRef->IsDOCollection());

            // Get the collection
            IFCCATASTROPHIC(!pCollectionNoRef->empty());

            // Make the desired object in the collection the new target
            ReleaseInterface(pTempDO);

            pTargetNoRef = pCollectionNoRef->GetCollection()[nIndex];
            IFCCATASTROPHIC(pTargetNoRef != nullptr);

            // Every time we get a new DO, we take a reference to it.
            // This is so we are consistent with public APIs such as GetValue
            // and collection methods.
            pTempDO = pTargetNoRef;
            pTempDO->AddRef();

            // Get its metadata
            if (NULL == (pClass = pTargetNoRef->GetClassInformation()))
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_UNEXPECTED);
                }
                else
                {
                    IFC(E_UNEXPECTED);
                }
            }
        }

        TrimWhitespace(cString, pString, &cString, &pString);

        // We must either be done or have a period here to indicate a sub-property

        if (cString)
        {
            if (L'.' != *pString)
            {
                if (CTimeManager::ShouldFailFast())
                {
                    IFCFAILFAST(E_FAIL);
                }
                else
                {
                    IFC(E_FAIL);
                }
            }

            pString++;
            cString--;

            TrimWhitespace(cString, pString, &cString, &pString);

            if (bResolve)
            {
                // Move to the new object, release the old one if it's not the original one we started with
                if (pTargetNoRef != *ppTarget)
                {
                    ASSERT(pTargetNoRef == pTempDO);
                    // Since pTempDO is the same as pTargetNoRef - release it so we don't double delete later.
                    ReleaseInterface(pTempDO);
                }
                IFC(pTargetNoRef->GetValueByIndex(nPropertyIndex, &value));

                // Make sure we get an non-NULL object
                if (value.AsObject() == nullptr)
                {
                    if (CTimeManager::ShouldFailFast())
                    {
                        IFCFAILFAST(E_UNEXPECTED);
                    }
                    else
                    {
                        IFC(E_UNEXPECTED);
                    }
                }

                pTargetNoRef = value.AsObject();
                SetInterface(pTempDO, pTargetNoRef);

                // Get its metadata
                if (NULL == (pClass = pTargetNoRef->GetClassInformation()))
                {
                    if (CTimeManager::ShouldFailFast())
                    {
                        IFCFAILFAST(E_UNEXPECTED);
                    }
                    else
                    {
                        IFC(E_UNEXPECTED);
                    }
                }
            }
        }
    }

    *ppTarget = pTargetNoRef;
    *ppDP = MetadataAPI::GetDependencyPropertyByIndex(nPropertyIndex);

Cleanup:
    ReleaseInterface(pTempDO);
    ReleaseInterface(pClone);

    RRETURN(hr);
}
#pragma warning(pop)

//--------------------------------------------------------------------------------
//
//  GetPropertyIndexForPropertyPath
//
//  Given a portion of a property path, attempt to find the DP.  If it can't
//  be found, return an error.
//
//--------------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::GetPropertyIndexForPropertyPath(
    _In_ CDependencyObject *pTarget,
    _In_ XUINT32 bParen,
    _In_ XNAME *pNameClass,
    _In_ XNAME *pNameProperty,
    _In_ const CClassInfo* pClass,
    _Out_ KnownPropertyIndex* pnPropertyIndex,
    _In_ const std::map<xstring_ptr, const CDependencyProperty*>& resolvedPathTypes)
{
    XUINT32 attempts = 0;
    KnownTypeIndex nClassID = KnownTypeIndex::UnknownType;
    KnownPropertyIndex nCustomPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    *pnPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
    const CDependencyProperty* pDP = nullptr;
    XStringBuilder nameBuilder;

    // Create a name builder
    IFC_RETURN(nameBuilder.Initialize());

    while (attempts < 2 && (*pnPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty) && (nCustomPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty))
    {
        // If we didn't find the property on the first try, try again but look for custom
        // properties.

        if (attempts == 1)
        {
            IFC_RETURN(FxCallbacks::FrameworkCallbacks_GetCustomTypeIDFromObject(pTarget, &nClassID));

            if (nClassID == KnownTypeIndex::UnknownType)
            {
                nClassID = pTarget->GetTypeIndex();
            }
        }

        // If this clause of the property path was parenthetical then the class
        // and property might represent an attached property. Check for that now.

        if (bParen)
        {
            if (pNameClass->cName > 0)
            {
                if (pNameClass->cNamespace > 0)
                {
                    IFC_RETURN(nameBuilder.Append(pNameClass->pNamespace, pNameClass->cNamespace));
                    IFC_RETURN(nameBuilder.AppendChar(L':'));
                }

                IFC_RETURN(nameBuilder.Append(pNameClass->pName, pNameClass->cName));
                IFC_RETURN(nameBuilder.AppendChar(L'.'));
            }

            if (pNameProperty->cNamespace > 0)
            {
                IFC_RETURN(nameBuilder.Append(pNameProperty->pNamespace, pNameProperty->cNamespace));
                IFC_RETURN(nameBuilder.AppendChar(L':'));
            }

            IFC_RETURN(nameBuilder.Append(pNameProperty->pName, pNameProperty->cName));

            if (attempts == 0)
            {
                xstring_ptr searchDP;

                nameBuilder.DetachString(&searchDP);

                auto it = resolvedPathTypes.find(searchDP);

                if (it != resolvedPathTypes.end())
                {
                    pDP = it->second;
                }

                if(pDP != nullptr)
                {
                    *pnPropertyIndex = pDP->GetIndex();
                }
                else
                {
                    IFC_RETURN(DirectUI::MetadataAPI::TryGetDependencyPropertyByFullyQualifiedName(
                        searchDP,
                        nullptr, // XamlServiceProviderContext
                        &pDP));
                    if (pDP != nullptr && DirectUI::MetadataAPI::IsAssignableFrom(pDP->GetTargetType(), pClass))
                    {
                        *pnPropertyIndex = pDP->GetIndex();
                    }
                }
            }
            else
            {
                IFC_RETURN(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(
                    DirectUI::MetadataAPI::GetClassInfoByIndex(nClassID),
                    XSTRING_PTR_EPHEMERAL_FROM_BUILDER(nameBuilder),
                    &pDP));
                if (pDP != nullptr)
                {
                    nCustomPropertyIndex = pDP->GetIndex();
                }
            }
        }

        // If it wasn't an attached property then just try to find the property in
        // the current target object.

        if ((*pnPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty) && (nCustomPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty))
        {
            IFC_RETURN(nameBuilder.Reset());

            if (pNameProperty->cNamespace > 0)
            {
                IFC_RETURN(nameBuilder.Append(pNameProperty->pNamespace, pNameProperty->cNamespace));
                IFC_RETURN(nameBuilder.AppendChar(L':'));
            }

            IFC_RETURN(nameBuilder.Append(pNameProperty->pName, pNameProperty->cName));

            if (attempts == 0)
            {
                IFC_RETURN(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(
                    pClass,
                    XSTRING_PTR_EPHEMERAL_FROM_BUILDER(nameBuilder),
                    &pDP));
                if (pDP != nullptr)
                {
                    *pnPropertyIndex = pDP->GetIndex();
                }
            }
            else
            {
                IFC_RETURN(DirectUI::MetadataAPI::TryGetDependencyPropertyByName(
                    DirectUI::MetadataAPI::GetClassInfoByIndex(nClassID),
                    XSTRING_PTR_EPHEMERAL_FROM_BUILDER(nameBuilder),
                    &pDP));
                if (pDP != nullptr)
                {
                    nCustomPropertyIndex = pDP->GetIndex();
                }
            }
        }

        IFC_RETURN(nameBuilder.Reset());
        attempts++;
    }

    if (*pnPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
    {
        // Failure to find the specified property here is an error
        if (nCustomPropertyIndex == KnownPropertyIndex::UnknownType_UnknownProperty)
        {
            IFC_RETURN(E_FAIL);
        }
        else
        {
            *pnPropertyIndex = static_cast<KnownPropertyIndex>(nCustomPropertyIndex);
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CreateObject
//
//  Synopsis:
//      Given Metadata and a string creates a DO from it
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::CreateObject(
    _In_ const CClassInfo* pClass,
    _In_ const xstring_ptr& strString,
    _Outptr_result_maybenull_ CDependencyObject** ppDO)
{
    CREATEPARAMETERS cp(this);
    CDependencyObject *pDO = NULL;

    if (strString.GetCount() > 0)
    {
        cp.m_value.SetString(strString);
    }

    const CREATEPFN pfnCreate = pClass->GetCoreConstructor();
    IFC_RETURN(pfnCreate(&pDO, &cp));
    *ppDO = static_cast<CDependencyObject*>(pDO);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateObject
//
//  Synopsis:
//      Given Metadata and a string creates a DO from it
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::CreateObject(
    _In_ const CClassInfo* pClass,
    _In_ CDependencyObject *pInDO,
    _Outptr_result_maybenull_ CDependencyObject ** ppDO
)
{
    CREATEPARAMETERS cp(this);
    CDependencyObject *pDO = NULL;

    if (pInDO)
    {
        cp.m_value.WrapObjectNoRef(static_cast<CDependencyObject*>(pInDO));
    }

    const CREATEPFN pfnCreate = pClass->GetCoreConstructor();
    IFC_RETURN(pfnCreate(&pDO, &cp));
    *ppDO = static_cast<CDependencyObject *>(pDO);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetBaseUriNoRef
//
//  Synopsis: Returns the base URI (does not addref the IPALUri).
//
//------------------------------------------------------------------------
IPALUri* CCoreServices::GetBaseUriNoRef()
{
    IPALUri* pBaseUri = NULL;

    if (m_pBrowserHost)
    {
        IGNOREHR(m_pBrowserHost->get_BaseUri(&pBaseUri));
    }

    return pBaseUri;
}

//------------------------------------------------------------------------
//
//  Method:   GetFullClassName
//
//  Synopsis:
//      Get the full CLR class name from the core class name
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CCoreServices::GetFullClassName(_In_ const xstring_ptr_view& strClass, _Out_ xstring_ptr* pstrFullName)
{
    const CClassInfo *pClassInfo = DirectUI::MetadataAPI::GetBuiltinClassInfoByName(strClass);
    IFCEXPECT_RETURN(pClassInfo);

    *pstrFullName = pClassInfo->GetFullName();

    return S_OK;
}



std::shared_ptr<XamlSchemaContext> CCoreServices::GetSchemaContext()
{
    if (!m_spXamlSchemaContext)
    {
        VERIFYHR(XamlSchemaContext::Create(this, m_spXamlSchemaContext));
    }

    return m_spXamlSchemaContext;
}

//------------------------------------------------------------------------
//
//  Returns:
//      The single XamlNodeStreamCacheManager associated with this core.
//
//  Note:
//      The XamlNodeStreamCacheManager will be created if it doesn't yet
//      exist.
//
//------------------------------------------------------------------------
//
_Check_return_
HRESULT CCoreServices::GetXamlNodeStreamCacheManager(_Out_ std::shared_ptr<XamlNodeStreamCacheManager>& spXamlNodeStreamCacheManager)
{
    if (!m_spXamlNodeStreamCacheManager)
    {
        IFC_RETURN(XamlNodeStreamCacheManager::Create(this, m_spXamlNodeStreamCacheManager));
    }

    spXamlNodeStreamCacheManager = m_spXamlNodeStreamCacheManager;
    return S_OK;
}

_Check_return_
HRESULT CCoreServices::LoadXamlResource(
    _In_            IPALUri       *pUri,
    _Out_           bool          *pfHasBinaryFile,
    _Outptr_        IPALMemory    **ppMemory,
    _Out_opt_       Parser::XamlBuffer* pBuffer,
    _Outptr_opt_    IPALUri       **ppPhysicalUri
)
{
    if (FAILED(TryLoadXamlResourceHelper(pUri, pfHasBinaryFile, ppMemory, pBuffer, ppPhysicalUri)))
    {
        IErrorService *pErrorService = nullptr;
        IFC_RETURN(getErrorService(&pErrorService));

        if (pErrorService)
        {
            xstring_ptr strCanonicalUri;
            IFC_RETURN(pUri->GetCanonical(&strCanonicalUri));

            IFC_RETURN(CErrorService::OriginateInvalidOperationError(this, AG_E_RUNTIME_INVALID_RESOURCE, strCanonicalUri));
        }
        return E_FAIL;
    }

    return S_OK;
}

_Check_return_
HRESULT CCoreServices::TryLoadXamlResource(
    _In_            IPALUri       *pUri,
    _Out_           bool          *pfHasBinaryFile,
    _Outptr_        IPALMemory    **ppMemory,
    _Out_opt_       Parser::XamlBuffer* pBuffer,
    _Outptr_opt_    IPALUri       **ppPhysicalUri
)
{
    return TryLoadXamlResourceHelper(pUri, pfHasBinaryFile, ppMemory, pBuffer, ppPhysicalUri);
}

//------------------------------------------------------------------------
//
//  Returns:
//      Attempts to load a Xaml File Resource and return
//      the memory stream (or) indicates that the resource has
//      a binary file backing it through the pfHasBinaryFile parameter.
//
//  Runtime Behavior:
//      1. Resolve the URI ending with .xbf to probe for binary content.
//         i. On Success: Set the isBinaryXaml flag (don't retrieve contents),
//                        ppMemory set to dummy contents.
//         ii. On Failure: Resolve the URI ending .xaml to probe for text content.
//             --> On Success: Retrieve the text buffer.
//             --> On Failure: ppMemory == NULL, isBinaryXaml = FALSE
//
//      If a binary file is to be used for the parsing,
//      the returned stream is empty as the binary file will
//      be picked up through the NodeStreamCacheManager based on the
//      Uri requested for the Xaml Resource.
//
//------------------------------------------------------------------------
//
_Check_return_
HRESULT CCoreServices::TryLoadXamlResourceHelper(
    _In_            IPALUri       *pUri,
    _Out_           bool          *pfHasBinaryFile,
    _Outptr_        IPALMemory    **ppMemory,
    _Out_opt_       Parser::XamlBuffer* pBuffer,
    _Outptr_opt_    IPALUri       **ppPhysicalUri
)
{
    std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
    xstring_ptr          strCanonicalUri;
    xref_ptr<IPALMemory> spResourceMem;
    xref_ptr<IPALResource> spResource;
    bool foundResource = true;

    *pfHasBinaryFile = false;
    *ppMemory = nullptr;

    // initialize to dummy data
    const XUINT8* pXAMLstr = reinterpret_cast<const XUINT8*>(L" ");
    XUINT32 cXAMLstrCount = 2;

    xref_ptr<IPALResourceManager> spResourceManager;
    IFC_RETURN(GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
    IFC_RETURN(GetXamlNodeStreamCacheManager(spXamlNodeStreamCacheManager));
    IFC_RETURN(pUri->GetCanonical(&strCanonicalUri));

    // lookup .xbf
    IFC_RETURN(spXamlNodeStreamCacheManager->GetBinaryResourceForXamlUri(strCanonicalUri, spResource));

    if (!spResource)
    {
        // lookup .xaml
        IFC_RETURN(spResourceManager->TryGetLocalResource(pUri, spResource.ReleaseAndGetAddressOf()));
        if (spResource)
        {
            IFC_RETURN(spResource->Load(spResourceMem.ReleaseAndGetAddressOf()));
            cXAMLstrCount = spResourceMem->GetSize();
            pXAMLstr = static_cast<const XUINT8*>(spResourceMem->GetAddress());
        }
        else
        {
            // failure to retrieve .xaml and .xbf
            foundResource = false;
        }
    }
    else
    {
        *pfHasBinaryFile = true;
    }

    if (ppPhysicalUri && spResource)
    {
        *ppPhysicalUri = spResource->GetPhysicalResourceUriNoRef();
        AddRefInterface(*ppPhysicalUri);
    }

    if (foundResource)
    {
        *ppMemory = spResourceMem.detach();

        if (pBuffer)
        {
            pBuffer->m_count = cXAMLstrCount;
            pBuffer->m_buffer = pXAMLstr;
            pBuffer->m_bufferType = Parser::XamlBufferType::Text;
        }
    }
    else
    {
        return E_FAIL;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CleanupNativePeers
//
//  Synopsis:
//      When a managed peer releases its ref on a native object
//      from the finalizer thread, the release isn't actually performed,
//      since we don't want to do work off of the UI thread.  So the
//      object is added to the m_pNativeManagedPeerQ instead.  When this
//      method is called, we call Release on all objects in that Q.
//
//------------------------------------------------------------------------
void
CCoreServices::CLR_CleanupNativePeers()
{
    CDependencyObject *pNativeManagedPeer = NULL;
    HRESULT hr = S_OK;

    if (m_pNativeManagedPeerQ)
    {
        // Initial wait on queue with 0 time
        hr = m_pNativeManagedPeerQ->Get((void**)&pNativeManagedPeer, 0);

        while (hr == S_OK)
        {
            // Release the object.
            // Note: since this is often the final release on an object, if we've
            // hit a ref-counting bug anywhere during the lifetime of this object
            // (a missing AddRef or an extra Release), this is where the
            // bug will show up.
            pNativeManagedPeer->Release();

            hr = m_pNativeManagedPeerQ->Get((void**)&pNativeManagedPeer, 0);
        }
    }
}

// Perform deferred operation for each entry in  m_deferredAnimationOperationQueue
_Check_return_ HRESULT
CCoreServices::FlushDeferredAnimationOperationQueue(bool doDeferredOperation)
{
    bool hadQueuedOperation = !m_deferredAnimationOperationQueue.empty();

    while (m_deferredAnimationOperationQueue.size() > 0)
    {
        auto deferredAnimationOperation = m_deferredAnimationOperationQueue.front();
        m_deferredAnimationOperationQueue.pop_front();

        if (doDeferredOperation)
        {
            // This will result in a call out to user code, which might fail (either
            // within the framework, or in the user code). If that's the case, and
            // assuming it didn't result in a failfast, we'll abort flushing the queue
            // for now, but leave it untouched and pick it up again at some point in
            // the future.
            IFC_RETURN(deferredAnimationOperation->Execute());
        }
    }

    if (hadQueuedOperation)
    {
        IFCFAILFAST(SetHasDeferredAnimationOperationsEventSignaledStatus(FALSE /* bSignaled */));
        IFCFAILFAST(SetDeferredAnimationOperationsCompleteEvent());
    }

    return S_OK;
}

// Queue an animation operation to be performed at a later time
void
CCoreServices::EnqueueDeferredAnimationOperation(
    _In_ std::shared_ptr<CDeferredAnimationOperation> deferredAnimationOperation)
{
    m_deferredAnimationOperationQueue.emplace_back(std::move(deferredAnimationOperation));

    IFCFAILFAST(SetHasDeferredAnimationOperationsEventSignaledStatus(TRUE /* bSignaled */));
}

// Remove from the queue any animation operations that match the set of DO/DP pairs
void
CCoreServices::RemoveMatchingDeferredAnimationOperations(_In_ CDependencyObject* targetObject, KnownPropertyIndex targetPropertyIndex)
{
    m_deferredAnimationOperationQueue.erase(
        std::remove_if(m_deferredAnimationOperationQueue.begin(), m_deferredAnimationOperationQueue.end(),
            [targetObject, targetPropertyIndex](const auto& operation)
            {
                return (operation->Target().first == targetObject && operation->Target().second == targetPropertyIndex);
            }),
        m_deferredAnimationOperationQueue.end());

    if (m_deferredAnimationOperationQueue.empty())
    {
        IFCFAILFAST(SetHasDeferredAnimationOperationsEventSignaledStatus(FALSE /* bSignaled */));
    }
}

//------------------------------------------------------------------------
//
//  Method:   CLR_AsyncReleaseNativeObject
//
//  Synopsis:  Asynchronously releases this native object in a thread safe
//             way.
//
//------------------------------------------------------------------------
void
CCoreServices::CLR_AsyncReleaseNativeObject(_In_ CDependencyObject* const pObj)
{
    if (m_pNativeManagedPeerQ)
    {
        m_pNativeManagedPeerQ->Post((void*)pObj);
    }
}

//------------------------------------------------------------------------
//
//  Method:   CLR_FireEvent
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CLR_FireEvent(
    _In_ CDependencyObject *pListener,
    _In_ EventHandle hEvent,
    _In_ CDependencyObject* pSender,
    _In_ CEventArgs* pArgs,
    _In_ XUINT32 flags
    )
{
    HRESULT hr = S_OK;
    CDependencyObject* pSenderLocal = NULL;
    CExceptionRoutedEventArgs* pExceptionRoutedEventArgs = NULL;

    // CLR Event.
    if (pSender && !pSender->GetClassName().IsNullOrEmpty())
    {
        pSenderLocal = pSender;
        AddRefInterface(pSenderLocal);

        if (pArgs)
        {
            if (pArgs->GetTypeIndex() == KnownTypeIndex::ErrorEventArgs)
            {
                // for ImageError and MediaError we need to expose them in the managed side as ExceptionRoutedEventArgs
                CErrorEventArgs* pErrEventArgs = static_cast<CErrorEventArgs*>(pArgs);

                if ((pErrEventArgs->m_eType == ImageError) || (pErrEventArgs->m_eType == MediaError))
                {
                    pExceptionRoutedEventArgs = new CExceptionRoutedEventArgs();

                    // Update the error message from error code if the error message is not yet set.
                    IGNOREHR(pErrEventArgs->UpdateErrorMessage());

                    if (!pErrEventArgs->m_strErrorMessage.IsNullOrEmpty())
                    {
                        pExceptionRoutedEventArgs->m_strErrorMessage = pErrEventArgs->m_strErrorMessage;
                    }

                    // replace the CErrorEventArgs by the CExceptionRoutedEventArgs for fileEvent below
                    pArgs = pExceptionRoutedEventArgs;
                }

            }
        }

        hr = FxCallbacks::JoltHelper_FireEvent(
            static_cast<CDependencyObject*>(pListener),
            hEvent.index,
            static_cast<CDependencyObject*>(pSender),
            pArgs,
            flags);
        IFC(hr);

        if (static_cast<CDependencyObject*>(pSender)->GetTypeIndex() == KnownTypeIndex::DispatcherTimer)
        {
            // Since this is a timer we need to restart the timer now that
            // we have allowed the user code to do its work
            CDispatcherTimer *pdt = static_cast<CDispatcherTimer*>(pSender);
            if (pdt)
            {
                IGNOREHR_UNTRIAGED(pdt->WorkComplete());
            }
        }
    }

    //Invokes handler for FocusManagerGotFocus/LostFocus asynchronous event on focus manager
    if (hEvent.index == KnownEventIndex::FocusManager_LostFocus || hEvent.index == KnownEventIndex::FocusManager_GotFocus)
    {
        VERIFYHR(FxCallbacks::JoltHelper_RaiseEvent(
            nullptr,
            hEvent.index == KnownEventIndex::FocusManager_GotFocus ? DirectUI::ManagedEvent::ManagedEventGotFocus : DirectUI::ManagedEvent::ManagedEventLostFocus,
            pArgs));
    }

Cleanup:
    ReleaseInterface(pSenderLocal);
    ReleaseInterface(pExceptionRoutedEventArgs);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ShouldFireEvent
//
//  Synopsis:
//      If EVENT_HANDLEDEVENTSTOO set, pfShouldFire value set as true
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ShouldFireEvent(
    _In_ CDependencyObject *pListener,
    _In_ EventHandle hEvent,
    _In_ CDependencyObject* pSender,
    _In_ CEventArgs* pArgs,
    _In_ XINT32 flags,
    _Out_ XINT32* pfShouldFire)
{
    *pfShouldFire = TRUE;

    if (!pArgs)
    {
        return S_OK;
    }

    if (!(flags & EVENT_HANDLEDEVENTSTOO))
    {
        CRoutedEventArgs *pRoutedEventArgs = pArgs->AsRoutedEventArgs();
        if (pRoutedEventArgs && pRoutedEventArgs->m_bHandled)
        {
            *pfShouldFire = FALSE;
        }
    }

    if (!(*pfShouldFire))
    {
        if (flags & EVENT_SENDER_PEGGED)
        {
            ((CDependencyObject *)pSender)->UnpegManagedPeer(/*isShutdownException*/TRUE);
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ApplicationStartupEventComplete
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ApplicationStartupEventComplete(
    )
{
    if (m_pDeploymentTree)
    {
        IFC_RETURN(m_pDeploymentTree->StartupEventComplete());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   SetCurrentApplication
//
//  Synopsis:
//
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::SetCurrentApplication(_In_ CApplication *pApplication)
{
    CREATEPARAMETERS cp(this);

    // Jupiter uses a NULL Application pointer to create m_pDeploymentTree

    if (!m_pDeploymentTree)
    {
        IFC_RETURN(CDeployment::Create( (CDependencyObject**)(&m_pDeploymentTree), &cp));
    }

    IFC_RETURN(m_pDeploymentTree->SetCurrentApplication(pApplication));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   HasActiveAnimations
//
//  Synopsis:
//      Checks if any animations are currently running.
//
//
//------------------------------------------------------------------------
bool
CCoreServices::HasActiveAnimations()
{
    if (GetTimeManager())
    {
        return GetTimeManager()->HasActiveAnimations();
    }
    else
    {
        return false;
    }
}

//------------------------------------------------------------------------
//
//  Method:   Initialize
//
//  Synopsis:
//      Initializes the TimeManager, EventManager, and InputManagers
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::Initialize()
{
    // Recreate the time manager.  Currently it gets told about all the objects
    // that affect timelines, etc. while we're parsing.
    IFC_RETURN(CTimeManager::Create(
        this,
        GetBrowserHost()->GetFrameScheduler()->GetClock(),
        &m_pTimeManager
    ));

    // Recreate the event manager.  Currently it gets told about all the objects
    // that have events while we're parsing.
    IFC_RETURN(CEventManager::Create(this, &m_pEventManager));

    m_fIsCoreReady = TRUE;

    return S_OK;
}
wf::IPropertyValueStatics* CCoreServices::GetPropertyValueStatics()
{
    //Get the factory instance to create the string IInspectable object for tooltips
    if (m_spPropertyValueFactory == nullptr)
    {
        IFCFAILFAST(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Foundation_PropertyValue).Get(),
            m_spPropertyValueFactory.ReleaseAndGetAddressOf()));
    }

    return m_spPropertyValueFactory.Get();
}

//------------------------------------------------------------------------
//
//  Method:   LoadXaml
//
//  Synopsis:
//      Parses a xaml represented by a string and instantiates a xaml object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::LoadXaml(
                        _In_ XUINT32 cBuffer,
                        _In_reads_opt_(cBuffer) const XUINT8 *pBuffer,
                        _Outptr_ CDependencyObject **ppDependencyObject
                        )
{
    HRESULT hr = S_OK;
    CDependencyObject *pTempDependencyObject = NULL;

    // This needs to be resolved.  We don't really want to reset the state but as
    // of now we need to or we can't parse correctly.

    IFC(ResetState());

    IFC(Initialize());

    // The passing empty XAML(cBuffer=0, pBuffer=NULL) argument is valid.
    // The browser's ResetCore() calls this with the empty xaml to ensure
    // the core services is initialized.

    // Create an object from xaml text

    if (cBuffer > 0)
    {
        CParserSettings parserSettings;
        parserSettings.set_RequireDefaultNamespace(true);

        Parser::XamlBuffer buffer;
        buffer.m_count = cBuffer;
        buffer.m_buffer = pBuffer;
        buffer.m_bufferType = Parser::XamlBufferType::Text;

        xstring_ptr sourceAssemblyName;
        // Performance marker
        XCP_PERF_LOG(XCP_PMK_PARSE_BEGIN);

        TraceParseXamlBegin(L"Uri unavailable - Parse from buffer");

        IFC(CParser::LoadXaml(this,
                              parserSettings,
                              buffer,
                              &pTempDependencyObject,
                              sourceAssemblyName));

        // Performance marker
        XCP_PERF_LOG(XCP_PMK_PARSE_END);

        TraceParseXamlEnd(L"Uri unavailable - Parse from buffer");
    }
    else
    {
        // Empty text is also allowed returning a NULL as an object.
        pTempDependencyObject = NULL;
    }

    // Only set the out parameter on success
    *ppDependencyObject = pTempDependencyObject;
    pTempDependencyObject = NULL;

Cleanup:
    ReleaseInterface(pTempDependencyObject);

    if (FAILED(hr))
    {
        m_fIsCoreReady = FALSE;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ParseXaml
//
//  Synopsis:
//      Parses a xaml represented by a string and instantiates a xaml object.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CCoreServices::ParseXaml(
    _In_ const Parser::XamlBuffer& buffer,
    _In_ bool bForceUtf16,
    _In_ bool bCreatePermanentNamescope,
    _In_ bool bRequiresDefaultNamespace,
    _Outptr_ CDependencyObject **ppDependencyObject,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ bool bExpandTemplatesDuringParse,
    _In_ const xstring_ptr_view& strXamlResourceUri)
{
    HRESULT hr = S_OK;
    IErrorService *pErrorService = nullptr;
    IError *pError = nullptr;
    xref_ptr<IPALUri> spXamlResourceUri;

    if (m_pDeploymentTree && m_pDeploymentTree->m_pApplication && !strXamlResourceUri.IsNull())
    {
        // IGNOREHR because the baseUri is only used for Binding Debugging in this case.  We do not
        // want to change runtime behavior if a conversion fails for a Debugging feature
        IGNOREHR(m_pDeploymentTree->m_pApplication->CreateBaseUri(strXamlResourceUri.GetCount(), strXamlResourceUri.GetBuffer(), spXamlResourceUri.ReleaseAndGetAddressOf()));
    }

    hr = ParseXamlWithEventRoot(
        buffer,
        bForceUtf16,
        bCreatePermanentNamescope,
        bRequiresDefaultNamespace,
        ppDependencyObject,
        spXamlResourceUri,
        strSourceAssemblyName,
        bExpandTemplatesDuringParse);

    if (FAILED(hr))
    {
        VERIFYHR(getErrorService(&pErrorService));
        if (pErrorService)
        {
            VERIFYHR(pErrorService->GetFirstError(&pError));
            if (pError)
            {
                pError->SetIsRecoverable(1);
            }
        }
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   ParseXamlWithEventRoot
//
//  Synopsis:
//      Parses a xaml represented by a string.  The root element in the XAML
//  will be returned in ppDependencyObject.  If pEventRoot is non-null, event
//  handlers described in the XAML will be hooked up to pEventRoot instead of
//  ppDependencyObject.  Otherwise ppDependencyObject gets the events.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CCoreServices::ParseXamlWithEventRoot(
    _In_ const Parser::XamlBuffer& buffer,
    _In_ bool bForceUtf16,
    _In_ bool bCreatePermanentNamescope,
    _In_ bool bRequireDefaultNamespace,
    _Outptr_ CDependencyObject **ppDependencyObject,
    _In_opt_ IPALUri *pXamlResourceUri,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ bool bExpandTemplatesDuringParse)
{
    *ppDependencyObject = nullptr;
    xstring_ptr strXamlResourceUri;

    if (EventEnabledParseXamlBegin())
    {
        if (pXamlResourceUri)
        {
            IGNOREHR(pXamlResourceUri->GetCanonical(&strXamlResourceUri));
        }

        if (strXamlResourceUri.IsNullOrEmpty())
        {
            DECLARE_STATIC_CONST_STRING_IN_FUNCTION_SCOPE(c_strUriUnavailable, L"Uri unavailable - ParseXamlWithEventRoot");
            strXamlResourceUri = c_strUriUnavailable;
        }

        TraceParseXamlBegin(strXamlResourceUri.GetBuffer());
    }
    auto traceGuard = wil::scope_exit([&]
    {
        TraceParseXamlEnd(strXamlResourceUri.GetBuffer());
    });

    if (buffer.m_count > 0)
    {
        xref_ptr<CDependencyObject> spTempDependencyObject;
        CParserSettings parserSettings;
        parserSettings.set_IsUtf16Encoded(bForceUtf16);
        parserSettings.set_RequireDefaultNamespace(bRequireDefaultNamespace);
        parserSettings.set_XamlResourceUri(pXamlResourceUri);
        parserSettings.set_ExpandTemplatesDuringParse(bExpandTemplatesDuringParse);

        IFC_RETURN(CParser::LoadXaml(
            this,
            parserSettings,
            buffer,
            spTempDependencyObject.ReleaseAndGetAddressOf(),
            strSourceAssemblyName));

        // Addref the core that the DO points to, since this fragment is not in the tree..
        spTempDependencyObject->ContextAddRef();
        *ppDependencyObject = spTempDependencyObject.detach();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ParseXamlWithExistingFrameworkRoot
//
//  Synopsis:
//      Parses a xaml represented by a string.  The root element in the XAML
//  will be returned in ppDependencyObject.
//
//  This takes in a existing root to use as the root of the newly parsed xaml, the
//  passed in root and the root of the xaml must match.
//------------------------------------------------------------------------

_Check_return_
HRESULT
CCoreServices::ParseXamlWithExistingFrameworkRoot(
    _In_ const Parser::XamlBuffer& buffer,
    _In_ CDependencyObject *pExistingFrameworkRoot,
    _In_ const xstring_ptr_view& strSourceAssemblyName,
    _In_ const xstring_ptr_view& strXamlResourceUri,
    _In_ const bool expandTemplatesDuringParse,
    _Outptr_ CDependencyObject **ppDependencyObject
)
{
    *ppDependencyObject = nullptr;
    TraceParseXamlBegin(strXamlResourceUri.GetBuffer());
    auto traceGuard = wil::scope_exit([&]()
    {
        TraceParseXamlEnd(strXamlResourceUri.GetBuffer());
    });

    // Create an object from xaml text
    if (buffer.m_count > 0)
    {
        xref_ptr<CDependencyObject> spTempDependencyObject;
        xref_ptr<IPALUri> spXamlResourceUri;

        CParserSettings parserSettings;
        parserSettings.set_ExistingFrameworkRoot(pExistingFrameworkRoot);
        parserSettings.set_ExpandTemplatesDuringParse(expandTemplatesDuringParse);
        if (!strXamlResourceUri.IsNull())
        {
            IFC_RETURN(gps->UriCreate(strXamlResourceUri.GetCount(), strXamlResourceUri.GetBuffer(), spXamlResourceUri.ReleaseAndGetAddressOf()));
            parserSettings.set_XamlResourceUri(spXamlResourceUri);
            parserSettings.set_UseXamlResourceUri(true);
        }

        // Tell LoadXaml not to create a name scope if the existing root is already in one.
        bool alreadyInNamescope = nullptr != pExistingFrameworkRoot->GetStandardNameScopeOwner();
        if (alreadyInNamescope)
        {
            parserSettings.set_CreateNamescope(false);
        }

        IFC_RETURN(CParser::LoadXaml(
                this,
                parserSettings,
                buffer,
                spTempDependencyObject.ReleaseAndGetAddressOf(),
                strSourceAssemblyName));

        // Set the appropriate flags on this DO as to indicate that it could have a
        // definition name set that should not be registered in the parent's namescope table.
        spTempDependencyObject->MarkAsPossiblyHavingDefinitionName();

        // If the existing root was already in a name scope, we didn't register names during the parse, and instead
        // we have to rely on the tree walk.
        if (alreadyInNamescope)
        {
            IFC_RETURN(PostParseRegisterNames(spTempDependencyObject));

            // Register definition name in namescope owner's namescope (definition
            // namescope), so animation, binding etc. in this XAML can bind to
            // namescope owner using the definition name
            //
            // Definition name will not be registered in parent namescope because
            // HasUsageName is FALSE until a usage name is given.
            //
            // Given the following XAML snippet:
            //
            //    <UserControl x:Class="UC1" x:Name="foo">
            //      ... </UserControl>
            //
            // This ensures that children of the UserControl can bind and use 'foo' to refer to
            // the control.
            //
            // The usage name, bar and baz, as in the following snippet:
            //
            //    <Page>
            //       <UC1 x:Name="bar"/>
            //       <UC1 x:Name="baz"/>
            //    </Page>
            //
            // Are set during the parse of the containing root, registered in the parent namescope,
            // and overridden once the control is fully parsed, as the UserControl would otherwise
            // have two m_strName values.
            if (!spTempDependencyObject->m_strName.IsNullOrEmpty())
            {
                // If there already exists an entry in the namescope with the same name,
                // we do not want to overwrite it. The name was either already registered
                // during parse or it is a child with the same name, so we skip registration.
                if (!m_nameScopeRoot.GetNamedObjectIfExists(spTempDependencyObject->m_strName, spTempDependencyObject.get(), Jupiter::NameScoping::NameScopeType::StandardNameScope))
                {
                    // Previously we had a lot of hoops to jump through to make this work. Now we simply register it
                    // in the namescope root and call it a day. There is a very minor backcompat risk here: if AFTER
                    // all the other elements are added the tree is modified after parse in such a way that an element
                    // with a duplicate name is added, and then subsequently removed, the original namescope owner definition
                    // name will fail to resolve to the namescope owner. This is an extreme edge case, and the behavior
                    // here is more logical, as now the definition name follows the name precedence rules as all the other
                    // names do.
                    //
                    // We register this name as a weakref to avoid circular references. Alternatively we could add support
                    // for a ownership-free reference, as existence is guaranteed, but the overhead here is likely to be minimal.
                    m_nameScopeRoot.SetNamedObject(spTempDependencyObject->m_strName, spTempDependencyObject.get(), Jupiter::NameScoping::NameScopeType::StandardNameScope,
                        xref::get_weakref(spTempDependencyObject.get()));
                }
            }
        }

        *ppDependencyObject = spTempDependencyObject.detach();
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   ResetVisualTree
//
//  Synopsis:
//      Resets the entire current visual tree.
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CCoreServices::ResetCoreWindowVisualTree()
{
    HRESULT hr = S_OK;
    HRESULT recordHr = S_OK;
    m_bInResetVisualTree = true;
    m_isTearingDownIsland = true;

    // Clear any pending download requests
    RECORDFAILURE(ProcessDownloadRequests(TRUE));

    // Fire ApplicationExit event if needed
    if (m_pDeploymentTree)
    {
        m_pDeploymentTree->Exit();
    }

    //
    // Abort all decodes in progress as all images are being unloaded
    // and will raise abort events.
    //
    if (m_imageFactory)
    {
        VERIFYHR(m_imageFactory->Shutdown());

        m_imageFactory.reset();
    }

    //
    // Release image provider and clean up image caches.
    // TODO: Why is this done here? This will make merging the surface cache difficult.
    //
    m_imageProvider.reset();

    //
    // Flush the image task dispatcher after the networking
    // stack is shut down to ensure all download events
    // have been aborted.
    //
    if (m_imageTaskDispatcher)
    {
        VERIFYHR(m_imageTaskDispatcher->Execute());

        m_imageTaskDispatcher->Shutdown();

        m_imageTaskDispatcher.reset();
    }

    // Clean up the queue of pending animation operations (for custom properties)
    // We need to do this before shutting down the peers
    VERIFYHR(FlushDeferredAnimationOperationQueue(false /*bDoDeferredOperation*/));

    ReleaseInterface(m_pLastLayoutExceptionElement);

    if (CContentRoot* contentRoot = m_contentRootCoordinator.Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        contentRoot->Close();
    }

    m_pMainVisualTree = nullptr;

    // Reset the input manager - this removes circular references to the tree
    if (m_inputServices)
    {
        m_inputServices->DestroyPointerObjects();
    }

    // Reset the time manager - this destroys the timing tree
    if (m_pTimeManager)
    {
        m_pTimeManager->Reset();
    }

    // Flush event manager's queue - this removes circular references to the tree
    if (m_pEventManager)
    {
        if (IsShuttingDown())
        {
            VERIFYHR(m_pEventManager->ClearRequests());
        }
        else
        {
            IGNOREHR(m_pEventManager->FlushQueue());
        }
    }

    ReleaseInterface(m_pDeploymentTree);

    // Flush out the media UI-thread synchronization queue
    //NOTE: Don't shut down here, just flush.
    IGNOREHR(ProcessMediaEvents());

    {
        auto lock = m_AppVerificationIdLock.lock();
        ReleaseInterface(m_pAppVerificationId);
    }

    IFC(recordHr);

    m_isFrameAfterVisualTreeReset = true;

Cleanup:
    m_isTearingDownIsland = false;
    m_bInResetVisualTree = false;

    RRETURN(hr);
}

// Called by CommonBrowserHost immediately before it releases the final
// reference on CCoreServices
void CCoreServices::ClearCompositorTreeHostQueues()
{
    if (m_pNWWindowRenderTarget)
    {
        auto treeHost = m_pNWWindowRenderTarget->GetCompositorTreeHost();
        if (treeHost)
        {
            // Release all other commands, we don't actually need to process these commands
            // as we are tearing things down, but we do need to release any remaining references
            // on CompNodes.
            treeHost->Cleanup();
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the name of the current XML source file.  If there is an existing
//  file loaded it will release it.  This method forces a redraw.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::putVisualRoot(_In_opt_ CDependencyObject *pDependencyObject)
{
    IFC_RETURN(ResetCoreWindowVisualTree());

    InitCoreWindowContentRoot();

    // Set the root visual from the parser result.  If we're passed NULL it means
    // we're supposed to just clear the tree.
    if (pDependencyObject)
    {
        ASSERT(!pDependencyObject->OfTypeByIndex<KnownTypeIndex::DependencyObject>()); // this should never happen in Jupiter

        CUIElement *pRoot = NULL;
        IFC_RETURN(DoPointerCast(pRoot, pDependencyObject));
        IFC_RETURN(m_pMainVisualTree->SetPublicRootVisual(pRoot, NULL /*pRootScrollViewer*/, NULL /*pRootContentPresenter*/));
    }

    return S_OK;
}

void CCoreServices::InitCoreWindowContentRoot()
{
    xref_ptr<CContentRoot> contentRoot = m_contentRootCoordinator.CreateContentRoot(CContentRoot::Type::CoreWindow, m_spTheming->GetRootVisualBackground(), nullptr);
    m_pMainVisualTree = contentRoot->GetVisualTreeNoRef();

    m_inputServices.attach(new CInputServices(this));

    // While the tree is loading, delay async processing (such as downloads and drawing)
    // until we're ready to raise the Loaded event and render the first frame.
    if (m_pBrowserHost)
    {
        m_isMainTreeLoading = TRUE;
    }
}

//------------------------------------------------------------------------
//
//  Method:   getVisualRoot
//
//  Synopsis:
//      Gets the current root of the visual tree
//
//------------------------------------------------------------------------
_Check_return_
CDependencyObject* CCoreServices::getVisualRoot()
{
    if (m_pMainVisualTree)
    {
        return m_pMainVisualTree->GetPublicRootVisual();
    }
    else
    {
        return NULL;
    }
}

_Check_return_
CDependencyObject* CCoreServices::GetPublicOrFullScreenRootVisual()
{
    CDependencyObject* pFSMR = GetMainFullWindowMediaRoot();
    if (pFSMR!=nullptr)
    {
        CValue value;
        if (SUCCEEDED(pFSMR->GetValueByIndex(KnownPropertyIndex::UIElement_Visibility, &value))
            && value.AsEnum() == static_cast<uint32_t>(DirectUI::Visibility::Visible))
        {
            return pFSMR;
        }
    }
    return getVisualRoot();
}

//------------------------------------------------------------------------
//
//  Method:   getRootScrollViewer
//
//  Synopsis:
//      Gets the current content root ScrollViewer
//
//------------------------------------------------------------------------
_Check_return_
CDependencyObject* CCoreServices::getRootScrollViewer()
{
    CDependencyObject* pRootScrollViewer = NULL;
    CDependencyObject* pRootScrollViewerFromWindow = NULL;
    CDeployment* pDeploymentNoRef = NULL;

    pDeploymentNoRef = GetDeployment();
    if (pDeploymentNoRef)
    {
        if (pDeploymentNoRef->m_pApplication)
        {
            pRootScrollViewer = pDeploymentNoRef->m_pApplication->m_pRootScrollViewer;
        }
        else
        {
            pRootScrollViewer = pDeploymentNoRef->m_pTempRootScrollViewer;
        }
    }
    else
    {
        IGNOREHR(FxCallbacks::Window_GetRootScrollViewer(&pRootScrollViewerFromWindow));
        if (pRootScrollViewerFromWindow)
        {
            pRootScrollViewer = static_cast<CDependencyObject*>(pRootScrollViewerFromWindow);
            ReleaseInterface(pRootScrollViewerFromWindow);
        }
    }

    return pRootScrollViewer;
}

//------------------------------------------------------------------------
//
//  Method:   getErrorService
//
//  Synopsis:
//      Returns the ErrorService instance for this core service.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CCoreServices::getErrorService(_Out_ IErrorService **ppErrorService)
{
    if (ppErrorService == NULL)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (m_pBrowserHost != NULL)
    {
        IFC_RETURN(m_pBrowserHost->GetErrorService(ppErrorService));
    }
    else if (m_pErrorServiceForXbf != NULL)
    {
        *ppErrorService = m_pErrorServiceForXbf;
    }
    else
    {
        *ppErrorService = NULL;
    }

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::GetParserErrorService(_Out_ IErrorService **ppErrorService)
{
    RRETURN(getErrorService(ppErrorService));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Report an unhandled error to the user.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ReportUnhandledError(HRESULT errorXR)
{
    IGNORERESULT(FxCallbacks::Error_ReportUnhandledError(errorXR));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates internal state (animations,events,etc) without drawing
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::Tick(
    bool tickForDrawing,
    _Out_opt_ bool *checkForAnimationComplete,
    _Out_opt_ bool *hasActiveFiniteAnimations
    )
{
    ASSERT(tickForDrawing);

    // Tick the timing manager
    if (m_pTimeManager)
    {
        IXcpBrowserHost *pBH = GetBrowserHost();
        ASSERT(pBH != NULL);

        FAIL_FAST_ASSERT(m_pNWWindowRenderTarget->GetDCompTreeHost() != nullptr);

        // Wait for resource creation to complete and register the callback thread. Otherwise Xaml timelines can't add
        // completed time events.
        IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());
        m_pNWWindowRenderTarget->GetDCompTreeHost()->RegisterDCompAnimationCompletedCallbackThread();

        HRESULT hrTick = m_pTimeManager->Tick(
            FALSE /* newTimelinesOnly */,
            tickForDrawing /* processIATargets */,
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetMainDevice(),
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetEasingFunctionStatics(),
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor(),
            pBH->GetFrameScheduler(),
            false /* tickOnlyTimers */,
            checkForAnimationComplete,
            hasActiveFiniteAnimations
            );

        // If there are any pending SetValue calls (queued during Tick),
        // invoke them or remove them now, depending on whether the Tick worked
        // or not.
        if (SUCCEEDED(hrTick))
        {
            IFC_RETURN(FlushDeferredAnimationOperationQueue());
        }
        else
        {
            IGNOREHR(FlushDeferredAnimationOperationQueue(false /*fDoDeferredOperation*/));
            IFC_RETURN(hrTick);
        }
    }

    if (m_pEventManager)
    {
        IFC_RETURN(m_pEventManager->ProcessQueue());
    }

    CLR_CleanupNativePeers();

    // This is a place code that needs to run on the UI thread once per frame but lives up in DXaml can be called.
    // It was designed for XAML to clean up COM objects that need to have their final Release happen on the UI thread,
    // but is also more general.  Not to be confused with the CompositionTarget callback event.
    FxCallbacks::XcpImports_PerFrameCallback();

    IGNOREHR(ProcessMediaEvents());

    // Process any queued download requests.
    // On the first frame, these requests will be delayed until the very end of the frame to minimize contention.
    // TODO: TICK: This case is only here for Blend hosting mode, since StartApplication isn't called there.
    if (!m_isFirstFrameAfterAppStart)
    {
        IFC_RETURN(ProcessDownloadRequests(FALSE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ForceGCCollect
////  Synopsis: Function is called as repose to the message posted from managed code.
//            This function makes sure that collection state is changed to
//            aggressive collect mode. During next collect iteration, collector will call back to
//            the managed side to subsequent collection
//
//------------------------------------------------------------------------
void CCoreServices::ForceGCCollect()
{
    m_State = GCTRACKING_STATE_COLLECTING;
    m_cTrackingInterval = m_cTrackingInterval / AGGRESSIVE_COLLECT_FACTOR;
    m_cMaxAllowedMemoryincrease = (MAX_ALLOWEDMEMORY_INCREASE / AGGRESSIVE_COLLECT_FACTOR);
}

//------------------------------------------------------------------------
//
//  Method:   StartMemoryTracking
////  Synopsis: Function triggers the start of memory tracking/collection.
//
//------------------------------------------------------------------------
void CCoreServices::StartMemoryTracking()
{
    m_cTrackingInterval = TRACKING_INTERVAL;
    m_cMaxAllowedMemoryincrease = MAX_ALLOWEDMEMORY_INCREASE;
    m_State = GCTRACKING_STATE_LISTENING; //Tracking
}

//------------------------------------------------------------------------
//
//  Method:   TrackMemory
//  Synopsis: Function gets called on every frame. Function scales itself based on
//            what mode is in.
//
//
//
//------------------------------------------------------------------------
void CCoreServices::TrackMemory()
{
    XUINT64 cMemCount = 0;

    if (m_State == GCTRACKING_STATE_WAITING)
        return;

    if (m_cTrackingInterval > 0)
    {
        m_cTrackingInterval --;
        return;
    }

    if (m_State == GCTRACKING_STATE_COLLECTING)
    {
        StartMemoryTracking();

        IGNORERESULT(FxCallbacks::JoltHelper_TriggerGCCollect());
        return;
    }

    if (m_State == GCTRACKING_STATE_LISTENING)
    {
        if (gps.IsValid())
        {
            cMemCount = gps->GetMemoryCount();
        }

        if (m_lPrvMemCount < cMemCount)
        {
            if ((cMemCount - m_lPrvMemCount) > m_cMaxAllowedMemoryincrease)
            {
                IGNORERESULT(FxCallbacks::JoltHelper_TriggerGCCollect());
                m_lPrvMemCount = cMemCount;
            }
        }
        else
        {
            m_lPrvMemCount = cMemCount;
        }

        StartMemoryTracking();
    }

}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Call into user code before every frame drawing.
//      The call is synchronous, but for perf it never calls into managed
//      unless a managed listener exists.
//
//      The time parameter is in seconds.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CallPerFrameCallback(XFLOAT time)
{
    wf::TimeSpan ts = {};

    if (m_fWantsRenderingEvent)
    {
        TracePerFrameCallbackBegin();

        xref_ptr<CRenderingEventArgs> pArgs;
        pArgs.attach(new CRenderingEventArgs());

        // Convert seconds to ticks.
        ts.Duration = static_cast<INT64>(time * /* MillisecondsPerSecond */ 1000 * /* TicksPerMillisecond */ 10000);
        IFCFAILFAST(pArgs->put_RenderingTime(ts));

        // AddRef for pArgs is called on the managed side
        IGNOREHR(FxCallbacks::JoltHelper_RaiseEvent(
            /* target */ NULL,
            DirectUI::ManagedEvent::ManagedEventRendering,
            pArgs));

        TracePerFrameCallbackEnd();

        // If we're still registered for CT.R after the callback, schedule a subsequent frame.
        if (FxCallbacks::CompositionTarget_HasHandlers())
        {
            ITickableFrameScheduler *pScheduler = GetBrowserHost()->GetFrameScheduler();
            ASSERT(pScheduler != NULL && pScheduler->IsInTick());
            IFC_RETURN(pScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::PerFrameCallback));
        }
        else
        {
            m_fWantsRenderingEvent = FALSE;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Called from the UI thread to signal that we have had a TDR, adapter
//      change or other DX device change and need to fire an event to the
//      application.  Can be called multiple times and will result in one
//      event being fired (on the UI thread) to the application (via
//      CompositionTarget.SurfaceContentsLost). This method works by simply
//      setting a flag, which RaiseQueuedSurfaceContentsLostEvent will
//      check at the appropriate time.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::EnqueueSurfaceContentsLostEvent()
{
    // Set the flag in a thread-safe manner
    if (gps.IsValid())
    {
        InterlockedExchange(&m_fSurfaceContentsLostEnqueued, TRUE);
    }

    ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
    if (pFrameScheduler != NULL)
    {
        IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::SurfaceContentsLost));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Call this on the UI thread to actually raise the event to the application.
//      Only raised if one has been enqueued.  Clears the "queue" (which is
//      actually just a flag).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RaiseQueuedSurfaceContentsLostEvent()
{
    if (gps.IsValid())
    {
        // If the flag is TRUE, set it to FALSE.  If it was TRUE, we raise the event
        if (InterlockedCompareExchange(&m_fSurfaceContentsLostEnqueued, FALSE, TRUE) == TRUE)
        {
            IGNOREHR(FxCallbacks::JoltHelper_RaiseEvent(
                /* target */ NULL,
                DirectUI::ManagedEvent::ManagedEventSurfaceContentsLost,
                /* pArgs */ NULL));
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Add to the list of SurfaceImageSources on this Core.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::AddSurfaceImageSource(CSurfaceImageSource *pItem)
{
    IFCPTR_RETURN(pItem);

    if (m_pAllSurfaceImageSources == NULL)
    {
        m_pAllSurfaceImageSources = new CXcpList<CSurfaceImageSource>();
    }
    IFC_RETURN(m_pAllSurfaceImageSources->Add(pItem));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Remove from the list of SurfaceImageSources on this Core.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RemoveSurfaceImageSource(CSurfaceImageSource *pItem)
{
    IFCPTR_RETURN(pItem);
    IFCPTR_RETURN(m_pAllSurfaceImageSources);

    IFC_RETURN(m_pAllSurfaceImageSources->Remove(pItem, FALSE));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Add to the list of VirtualSurfaceImageSources on this Core.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::AddVirtualSurfaceImageSource(CVirtualSurfaceImageSource *pItem)
{
    IFCPTR_RETURN(pItem);

    if (m_pAllVirtualSurfaceImageSources == NULL)
    {
        m_pAllVirtualSurfaceImageSources = new CXcpList<CVirtualSurfaceImageSource>();
    }
    IFC_RETURN(m_pAllVirtualSurfaceImageSources->Add(pItem));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Remove from the list of VirtualSurfaceImageSources on this Core.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RemoveVirtualSurfaceImageSource(CVirtualSurfaceImageSource *pItem)
{
    IFCPTR_RETURN(pItem);
    IFCPTR_RETURN(m_pAllVirtualSurfaceImageSources);

    IFC_RETURN(m_pAllVirtualSurfaceImageSources->Remove(pItem, FALSE));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis: Call all VirtualSurfaceImageSources and have them do per-frame processing
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::VirtualSurfaceImageSourcePerFrameWork(
    _In_ const XRECTF_RB *pWindowBounds
    )
{
    CXcpList<CVirtualSurfaceImageSource>::XCPListNode *pPrevious = nullptr;
    CXcpList<CVirtualSurfaceImageSource>::XCPListNode *pCurrent = nullptr;
    const XUINT64 maxCachedTileMemory = 1024 * 1024 * 16; // 16MB
    VirtualSurfaceImageSourceStandbyLists standbyLists;
    bool wantAdditionalFrame = false;
    VisibleBoundsMap visibleBoundsMap;
    m_pVisibleBoundsMap = &visibleBoundsMap;
    auto mapCleanup = wil::scope_exit([&]()
    {
        m_pVisibleBoundsMap = nullptr;
    });

    if (m_pAllVirtualSurfaceImageSources != NULL)
    {
        // Do per-frame processing
        pCurrent = m_pAllVirtualSurfaceImageSources->GetHead();

        while (pCurrent)
        {
            bool thisSurfaceWantsAdditionalFrame = false;

            // Perf optimization:
            // The pattern used by Office, particularly Word, is for many sibling VSIS's to share a common ancestor.
            // In this case we can optimize by caching the visible bounds of the common ancestor, allowing
            // us to share the result of a large part of the work between these sibling VSIS's.
            // Note that this optimization relies on the m_pAllVirtualSurfaceImageSources list to be ordered in
            // roughly sibling order, otherwise the simple common ancestor search may not find the common ancestor.
            if (pPrevious != nullptr)
            {
                IFC_RETURN(CVirtualSurfaceImageSource::CacheCommonAncestorVisibleRect(pPrevious->m_pData, pCurrent->m_pData, pWindowBounds));
            }
            IFC_RETURN(pCurrent->m_pData->PreRender(pWindowBounds, &thisSurfaceWantsAdditionalFrame));
            pPrevious = pCurrent;
            pCurrent = pCurrent->m_pNext;

            // If any VSIS requests an additional tick, then another one will be requested
            wantAdditionalFrame |= thisSurfaceWantsAdditionalFrame;
        }

        // Now that all callbacks have completed - put cached tiles onto the standby lists
        standbyLists.totalSize = 0;

        pCurrent = m_pAllVirtualSurfaceImageSources->GetHead();

        while (pCurrent)
        {
            IFC_RETURN(pCurrent->m_pData->UpdateStandbyLists(&standbyLists));
            pCurrent = pCurrent->m_pNext;
        }

        // Free tiles until the total amount of memory consumed by all non-desired tiles is under the maximum limit
        for (XUINT32 i = 0; i < ARRAY_SIZE(standbyLists.list); i++)
        {
            if (standbyLists.totalSize <= maxCachedTileMemory)
            {
                // The limit has been reached, no need to free more tiles
                break;
            }

            // The oldest tiles are in the last standby list - they are freed first
            XUINT32 index = ARRAY_SIZE(standbyLists.list) - i - 1;
            ASSERT(index < ARRAY_SIZE(standbyLists.list));

            xvector<VirtualSurfaceImageSourceStandbyListRecord> &standbyList = standbyLists.list[index];

            for (XUINT32 tileIndex = 0; tileIndex < standbyList.size(); tileIndex++)
            {
                if (standbyLists.totalSize <= maxCachedTileMemory)
                {
                    // The limit has been reached, no need to free more tiles
                    break;
                }

                const VirtualSurfaceImageSourceStandbyListRecord &record = standbyList[tileIndex];

                XUINT64 tileSize = record.pTile->GetAllocationSize();
                ASSERT(tileSize <= standbyLists.totalSize);

                standbyLists.totalSize -= tileSize;

                IFC_RETURN(record.pVSIS->FreeTile(record.pTile));
            }

            standbyList.clear();
        }

        // Walk through each VSIS one last time and attempt to trim memory for any freed tiles.
        // When the app is suspended, the DComp device and its corresponding resources have been offered.
        // It's not safe to trim DComp surface memory at this time - it will be deferred until the tick after Resume().
        if (!m_isSuspended)
        {
            pCurrent = m_pAllVirtualSurfaceImageSources->GetHead();
            while (pCurrent)
            {
                IFC_RETURN(pCurrent->m_pData->TrimTilesIfPossible());
                pCurrent = pCurrent->m_pNext;
            }
            }
    }

    // Each VSIS can request an additional frame to render low-priority updates
    if (wantAdditionalFrame)
    {
        ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
        ASSERT(pFrameScheduler != NULL && pFrameScheduler->IsInTick());
        IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::VSISUpdate))
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      HitTest content to our XAML file
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::HitTest(
    XPOINTF ptHit,
    _In_opt_ CDependencyObject *pHitTestRoot,
    _Outptr_result_maybenull_ CDependencyObject **ppVisualHit,
    bool hitDisabledElement
    )
{
    ASSERT(ppVisualHit);

    CUIElement *pRoot = NULL;

    // Since this entry point for hit testing never originates from XamlDiagnostics, XamlDiagHitTestMode
    // should always be false.
    const bool c_xamlDiagHitTestMode = false;

    // Assume nothing is hit
    *ppVisualHit = NULL;

    if (pHitTestRoot)
    {
        IFC_RETURN(DoPointerCast(pRoot, pHitTestRoot));
    }
    else
    {
        pRoot = static_cast< CUIElement * >(GetMainRootVisual());
    }

    if (pRoot != NULL)
    {
        CUIElement * pResult = NULL;

        // The callback is not exposed to the calling interfaces because it is
        // synchronous and not safe for public interfaces.  The callback is
        // only used internally to queue an async event.

        HitTestPerfData hitTestPerfData;
        HitTestParams hitTestParams(&hitTestPerfData);

        hitTestParams.SaveWorldSpaceHitTarget(ptHit);

        IFC_RETURN(pRoot->HitTestEntry(hitTestParams, ptHit, hitDisabledElement, c_xamlDiagHitTestMode, &pResult));

        if (pResult != NULL)
        {
            *ppVisualHit = static_cast<CDependencyObject *>( pResult );
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetTextElementBoundingRect
//
//  Synopsis:
//      Calculate the text element bounds from the control it contains
//      There is no bounding box API for TextElement so we need to implement it here.
//      Hence we ask the the container to calculate the bounds
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetTextElementBoundingRect(
    _In_ CDependencyObject *pElement,
    _Out_ XRECTF *pRectBound,
    _In_ bool ignoreClip)
{
    CFrameworkElement *pContentStartVisualParent = NULL;

    CTextElement *pTextElement = do_pointer_cast<CTextElement>(pElement);
    xref_ptr<CTextPointerWrapper> pContentStart;
    IFC_RETURN(pTextElement->GetContentStart(pContentStart.ReleaseAndGetAddressOf()));
    IFC_RETURN(pContentStart->GetVisualParent(&pContentStartVisualParent));
    if (pContentStartVisualParent != NULL)
    {
        if (CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(pContentStartVisualParent))
        {
            IFC_RETURN(pTextBlock->GetTextElementBoundRect(pTextElement, pRectBound, ignoreClip));
        }
        else if (CRichTextBlock *pRichTextBlock = do_pointer_cast<CRichTextBlock>(pContentStartVisualParent))
        {
            IFC_RETURN(pRichTextBlock->GetTextElementBoundRect(pTextElement, pRectBound, ignoreClip));
        }
        else if (CRichTextBlockOverflow *pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pContentStartVisualParent))
        {
            IFC_RETURN(pRichTextBlockOverflow->GetTextElementBoundRect(pTextElement, pRectBound, ignoreClip));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::HitTestLinkFromTextControl(
                       _In_ XPOINTF ptHit,
                       _In_ CDependencyObject *pLinkContainer,
                       _Outptr_result_maybenull_ CDependencyObject **ppLink)
{
    *ppLink = nullptr;

    CHyperlink *pLink = nullptr;

    if (CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(pLinkContainer))
    {
        IFC_RETURN(pTextBlock->HitTestLink(&ptHit, &pLink));
    }
    else if (CRichTextBlock *pRichTextBlock = do_pointer_cast<CRichTextBlock>(pLinkContainer))
    {
        IFC_RETURN(pRichTextBlock->HitTestLink(&ptHit, &pLink));
    }
    else if (CRichTextBlockOverflow *pRichTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(pLinkContainer))
    {
        IFC_RETURN(pRichTextBlockOverflow->HitTestLink(&ptHit, &pLink));
    }

    if (pLink)
    {
        AddRefInterface(pLink);
        *ppLink = pLink;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetGeometryBuilder
//
//  Synopsis:
//      Gets the cached geometry builder.  If it needs to be initialized this
//  call could fail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetGeometryBuilder(
    XUINT32 nIndex,
    _Outptr_ CGeometryBuilder **ppBuilder,
    bool fGetPathGeometryBuilder
    )
{
    if (nIndex >= 2)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    if (!m_bBuilderReady[nIndex])
    {
        if (fGetPathGeometryBuilder)
        {
            IFC_RETURN(m_pBuilder[nIndex]->OpenPathGeometryBuilder());
        }
        else
        {
            IFC_RETURN(m_pBuilder[nIndex]->OpenGeometry());
        }
        m_bBuilderReady[nIndex] = TRUE;
    }

    *ppBuilder = m_pBuilder[nIndex];

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Register the IXcpHostSite implementation of the control that is hosting
//      Jolt.
//
//------------------------------------------------------------------------
void
CCoreServices::RegisterHostSite(_In_ IXcpHostSite *pHostSite)
{
    m_pHostSite = pHostSite;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Unregister the IXcpHostSite implementation of the control that is hosting
//      Jolt.
//
//------------------------------------------------------------------------
void
CCoreServices::UnregisterHostSite()
{
    m_pHostSite = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Register the IXcpBrowserHost implementation of the control that is hosting
//      Jolt.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RegisterBrowserHost(_In_ IXcpBrowserHost *pBrowserHost)
{
    m_pBrowserHost = pBrowserHost;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Null out the m_pBrowserHost pointer to prevent access to the
//      m_pBrowserHost upon releasing the core.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UnregisterBrowserHost()
{
    m_pBrowserHost = NULL;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Saves the information needed for a download request so it can be processed later.
//
//------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable: 6387)
_Check_return_
HRESULT
CCoreServices::AddDownloadRequest(
    _In_ IPALDownloadRequest* pDownloadRequest,
    _Outptr_result_maybenull_ IPALAbortableOperation **ppIAbortableDownload,
    _In_opt_ IPALUri *pPreferredBaseUri
    )
{
    HRESULT hr = S_OK;
    DREQUEST *temp = NULL;
    CAbortableDownloadWrapper* pTempAbortable = NULL;

    if (ppIAbortableDownload)
    {
        *ppIAbortableDownload = NULL;
    }

    temp = new DREQUEST;

    AddRefInterface(pPreferredBaseUri);

    AddRefInterface(pDownloadRequest);

    temp->pIAbortableDownload = NULL;
    temp->pPreferredBaseUri = pPreferredBaseUri;
    temp->strRelativeUri.Reset();
    temp->pDownloadRequest = pDownloadRequest;

    // Resources owned by temp now.
    pPreferredBaseUri = NULL;

    if (ppIAbortableDownload != NULL)
    {
        IFC(CAbortableDownloadWrapper::Create(temp, this, &pTempAbortable));

        // AddRef here, so that we guarantee it's still live to return
        // a copy to the caller.
        //
        // The 1 ref (from create) is now owned by temp. And one we hold
        // and either give to caller, or release in cleanup.
        AddRefInterface(pTempAbortable);
        temp->pIAbortableDownload = pTempAbortable;
    }

    // Add the new request to the queue
    if (m_preqQueue)
    {
        m_preqQueue->Post((void*) temp);
        temp = NULL;
    }

    if (ppIAbortableDownload != NULL)
    {
        // Given to caller, prevent release in cleanup.
        *ppIAbortableDownload = pTempAbortable;
        pTempAbortable = NULL;
    }

Cleanup:
    ReleaseInterface(pTempAbortable);

    if (temp)
    {
        temp->ReleaseResources();
        delete temp;
    }

    RRETURN(hr);
}
#pragma warning(pop)

//------------------------------------------------------------------------
//
//  Synopsis:
//      Process the queued download requests.  If the flag is TRUE it simply
//  clears the requests, this is needed to ensure proper reference counting
//  and shutdown semantics.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ProcessDownloadRequests(_In_ XINT32 bClear)
{
    HRESULT hr = S_OK;
    HRESULT waithr = S_OK;
    DREQUEST *curr = NULL;
    IPALAbortableOperation* pAbortable = NULL;

    // Assert because this function should never be called when holding
    // unless it is just going to clean up...
    ASSERT(bClear || !m_isMainTreeLoading);

    if (!bClear) // Only if not shutting down.
    {
        // Get the font download queue from the DirectWrite factory.
        if (m_pFontDownloadListener == nullptr)
        {
            m_pFontDownloadListener.reset(new FontDownloadListener(this));
        }
        if (m_pFontDownloadQueue == nullptr)
        {
            CTextCore *pTextCore = nullptr;
            IFC(GetTextCore(&pTextCore));
            Microsoft::WRL::ComPtr<IDWriteFactory3> dwriteFactory;
            IFC(pTextCore->GetWinTextCore()->GetDWriteFactory(&dwriteFactory));
            IFC(dwriteFactory->GetFontDownloadQueue(m_pFontDownloadQueue.ReleaseAndGetAddressOf()));
        }

        // Start any font downloads if the queue has accumulated download requests during layout.
        // DownloadsCompleted will be called on the listener, which will reawaken the main loop.
        if (!m_pFontDownloadQueue->IsEmpty())
        {
            IncrementPendingFontDownloadCount(); // Halt any test code waiting on idleness.
            if (FAILED(m_pFontDownloadQueue->BeginDownload(m_pFontDownloadListener.get())))
            {
                // Restore the count on error, since the DownloadCompleted callback will never enter.
                // This resumes any test code waiting on idleness.
                DecrementPendingFontDownloadCount();
            }
        }

        // Invalidate the layout tree if more font data is available due to newly completed downloads
        // since differing font selection could dramatically change the size of controls.
        if (m_pFontDownloadListener->AreNewFontDownloadsCompleted())
        {
            m_pFontDownloadListener->ResetFontDownloadsCompleted();
            CUIElement *pVisualRoot = m_pMainVisualTree->GetRootVisual();
            TraceRecursiveInvalidateMeasureInfo(L"New Font Download Completed");
            pVisualRoot->RecursiveInvalidateMeasure();
            IXcpBrowserHost *pBrowserHost = GetBrowserHost();
            if (pBrowserHost != nullptr)
            {
                ITickableFrameScheduler *pFrameScheduler = pBrowserHost->GetFrameScheduler();
                if (pFrameScheduler != nullptr)
                {
                    pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::Download);
                }
            }
        }
    }

    // if we are reentering, we skip the processing
    if (m_bProcessingDownloads)
    {
        hr = S_OK;
        goto Cleanup;
    }

    m_bProcessingDownloads = TRUE;
    // If we're not trying to shutdown the core it is an error to not have a
    // valid host site.

    if (!bClear && !m_pSite)
    {
        IFC(E_FAIL);
    }

    // Walk the queue and process the requests
    if (m_preqQueue)
    {
        // Initial wait on queue with 0 time
        waithr = m_preqQueue->Get((void**)&curr, 0);

        // If queue has objects, process them
        // If the queue timeouts, it will not
        // return S_OK and the loop will break
        while (waithr == S_OK)
        {
            if (!bClear)
            {
                IFCPTR(curr);

                if (curr->pIAbortableDownload)
                {
                    IFC(m_pSite->UnsecureDownload(curr->pDownloadRequest,
                        &pAbortable,
                        curr->pPreferredBaseUri));

                    if (pAbortable)
                    {
                        IFC((curr->pIAbortableDownload)->SetAbortable(pAbortable));
                    }

                    ReleaseInterface(pAbortable);
                }
                else
                {
                    IFC(m_pSite->UnsecureDownload(curr->pDownloadRequest,
                        NULL,
                        curr->pPreferredBaseUri));
                }
            }

            curr->ReleaseResources();
            delete curr;
            curr = NULL;

            waithr = m_preqQueue->Get((void**)&curr, 0);
        }
    }

Cleanup:
    ReleaseInterface(pAbortable);
    if (curr)
    {
        if (curr->pDownloadRequest)
        {
            IPALDownloadResponseCallback* pCallback = NULL;
            if (SUCCEEDED(curr->pDownloadRequest->GetCallback(&pCallback)))
            {
                IGNOREHR(pCallback->GotResponse(NULL, E_INVALIDARG));
            }

            //
            // GetCallback() would addref on pCallback,
            // the ref-count should be released here to avoid object leaking.
            //
            ReleaseInterface(pCallback);
        }
        curr->ReleaseResources();
        delete curr;
        curr = NULL;
    }

    if (FAILED(hr))
    {
        IErrorService *pErrorService = NULL;
        VERIFYHR(getErrorService(&pErrorService));

        if (pErrorService)
        {
            IGNOREHR(pErrorService->ReportGenericError(hr, DownloadError, hr, 1, 0, 0, NULL, 0));
            ReportAsyncErrorToBrowserHost( );
        }
    }

    m_bProcessingDownloads = FALSE;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Remove a given request from the request queue in the case that it
//      is aborted before the download is actually started
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RemoveDownloadRequest(_In_ DREQUEST* pRequest)
{
    HRESULT hr = E_FAIL;
    HRESULT waithr = S_OK;
    DREQUEST *curr = NULL;
    IPALQueue *pTemp = NULL;

    // Create the temporary download queue.
    IFC(gps->QueueCreate(&pTemp));

    if (m_preqQueue && pTemp)
    {
        // Initial wait on queue with 0 time
        waithr = m_preqQueue->Get((void**)&curr, 0);

        // If queue has objects, process them
        // If the queue timeouts, it will not
        // return S_OK and the loop will break

        //We queue all requests to a temporary
        //queue and then restore them. It's the
        //safest option that maintains queue
        //order.
        //BUGBUG: Long term we need the IPALQueue
        //to support a Remove method or use
        //Critical sections in CCoreServices

        while (waithr == S_OK)
        {
            IFCPTR(curr);
            if (curr == pRequest)
            {
                //Free resources
                curr->ReleaseResources();
                delete curr;
                curr = NULL;
                hr = S_OK;
            }
            else
            {
                //Add the processed request to the temporary queue
                pTemp->Post((void*) curr);
            }

            waithr = m_preqQueue->Get((void**)&curr, 0);
        }
        //Add the nodes in the temporary queue back to the delayed
        //download queue. Any cross-thread incoming requests may be queued before
        //all requests are in.
        waithr = pTemp->Get((void**)&curr, 0);
        while (waithr == S_OK)
        {
            m_preqQueue->Post((void*) curr);
            waithr = pTemp->Get((void**)&curr, 0);
        }
    }

Cleanup:
    if (pTemp) IGNOREHR(pTemp->Close());
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Register the callback provider in the host for downloading data.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RegisterDownloadSite(_In_ ICoreServicesSite *pSite)
{
    m_pSite = pSite;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Unregister the callback provider in the host for downloading data.
//
//------------------------------------------------------------------------
void
CCoreServices::UnregisterDownloadSite()
{
    m_pSite = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the site has been provided call back to it to perform the download.
//
// This member is used for cases where there are legitimate exceptions
// to site-of-origin download restriction.  If you make a call to this
// member, comment the call as to why the exception is valid and secure.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UnsecureDownloadFromSite(
    _In_ const xstring_ptr& strRelativeUri,
    _In_ IPALUri *pAbsoluteUri,
    _In_ IPALDownloadResponseCallback* pICallback,
    _In_ XUINT32 eUnsecureDownloadAction,
    _Outptr_opt_ IPALAbortableOperation **ppIAbortableDownload,
    _In_opt_ IPALUri *pPreferredBaseUri
    )
{
    // We don't want to try to download a null string


    if ((strRelativeUri.GetCount() == 0) || (!m_pSite))
    {
        IFC_RETURN(E_FAIL);
    }

    // If we're holding for the control to process the loaded event then add the request
    // to the download queue.

    // Construct IPALDownloadRequest
    xref_ptr<CDownloadRequestInfo> pRequestInfo;
    IFC_RETURN(CDownloadRequestInfo::Create(strRelativeUri, pAbsoluteUri, pICallback, pRequestInfo.ReleaseAndGetAddressOf()));
    IFC_RETURN(pRequestInfo->SetOptionFlags(eUnsecureDownloadAction));

    if (m_isMainTreeLoading)
    {
        IFC_RETURN(AddDownloadRequest(pRequestInfo, ppIAbortableDownload, pPreferredBaseUri));
    }
    else
    {
        IFC_RETURN(m_pSite->UnsecureDownload(pRequestInfo, ppIAbortableDownload, pPreferredBaseUri));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Validate the Uri for security.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CheckUri(
                        _In_ const xstring_ptr& strRelativeUri,
                        _In_ XUINT32 eUnsecureDownloadAction)
{
    if (strRelativeUri.GetCount() == 0)
    {
        return E_FAIL;
    }
    return m_pSite ? m_pSite->CheckUri(strRelativeUri, eUnsecureDownloadAction) : E_FAIL;
}

//------------------------------------------------------------------------
//
// Method: EnsureUriCache
//
// Returns Uri cache, creating it if necessary
//
//------------------------------------------------------------------------
xstringmap<bool>&
CCoreServices::GetResourceDictionaryUriCache()
{
    return m_resourceDictionaryUriCache;
}

//------------------------------------------------------------------------
//
//  Method:   GetGlyphPathBuilder
//
//  Synopsis:
//      Gets the cached glyph path builder.  If it needs to be initialized
//  this call could fail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetGlyphPathBuilder(_Outptr_ CGlyphPathBuilder **ppBuilder)
{
    HRESULT hr = S_OK;

    *ppBuilder = m_pGlyphPathBuilder;

    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   GetTextCore
//
//  Synopsis:
//      Gets the cached glyph path builder.  If it needs to be initialized
//  this call could fail.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetTextCore(_Outptr_result_maybenull_ CTextCore **ppTextCore)
{
    if (m_pTextCore == NULL)
    {
        // Initialize the text core services.
        m_pTextCore = new CTextCore(this);
        IFC_RETURN(m_pTextCore->Initialize());
    }

    if (ppTextCore != NULL)
    {
        *ppTextCore = m_pTextCore;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for a debug-settings change from the host site.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::OnDebugSettingsChanged()
{
    if (!m_debugSettingsChanged)
    {
        m_debugSettingsChanged = TRUE;

        // Request a tick to pick up the debug settings change.
        IXcpBrowserHost *pBH = GetBrowserHost();
        if (pBH != NULL)
        {
            ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::SettingsChanged));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called by CJupiterWindow to notify VisualState adaptive trigger
//      engine that the window size has changed
//
//------------------------------------------------------------------------
void
CCoreServices::OnWindowSizeChanged(
    _In_ XUINT32 windowWidth,
    _In_ XUINT32 windowHeight,
    _In_ XUINT32 layoutBoundsWidth,
    _In_ XUINT32 layoutBoundsHeight
    )
{
    // If the content root for the main visual tree is a XAML island, then its window size
    // will always be a meaningless (1, 1), which we want to ignore.
    if (layoutBoundsWidth <= 1 && layoutBoundsHeight <= 1)
    {
        return;
    }

    const auto& roots = m_contentRootCoordinator.GetContentRoots();

    for(const auto& root : roots)
    {
        const auto type = root->GetType();
        const bool isCoreRoot = (type == CContentRoot::Type::CoreWindow);
        if (isCoreRoot)
        {
            root->RaiseXamlRootChanged(CContentRoot::ChangeType::Size);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   ConfigureNumberSubstitution
//
//  Synopsis:
//      Update the shared IDWriteNumberSubstitution object.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ConfigureNumberSubstitution()
{
    IFC_RETURN(m_pTextCore->ConfigureNumberSubstitution());
    m_localeSettingChanged = TRUE;

    // Request additional frame
    IXcpBrowserHost *pBH = GetBrowserHost();
    if (pBH != nullptr)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != nullptr)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::SettingsChanged));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetNextRuntimeId
//
//  Synopsis:
//      Returns the next available RuntimeID for UIAutomation
//
//------------------------------------------------------------------------
_Check_return_ XUINT32
CCoreServices::GetNextRuntimeId()
{
    return m_uRuntimeId++;
}

const xref_ptr<CDisplayMemberTemplate>& CCoreServices::GetDefaultContentPresenterTemplate()
{
    // If this is the first content presenter to be created
    if (!m_pDefaultContentPresenterTemplate)
    {
        CREATEPARAMETERS cp(this);
        VERIFYHR(CDisplayMemberTemplate::Create(reinterpret_cast<CDependencyObject**>(m_pDefaultContentPresenterTemplate.ReleaseAndGetAddressOf()), &cp));
        static_cast<CDisplayMemberTemplate*>(m_pDefaultContentPresenterTemplate)->m_isDefaultTemplate = true;

        // We're a member of CCoreServices, no need to hold a ref
        // This was causing a leak for some apps (DRT 432) only in FF!
        m_pDefaultContentPresenterTemplate->ContextRelease();
    }

    return m_pDefaultContentPresenterTemplate;
}

//------------------------------------------------------------------------
//
//  Method:   GetAdjustedPopupRootForElement
//
//  Synopsis:
//      Returns a popup root associated with the given object. If the
//      splash screen is showing or if we don't have a visual tree yet,
//      the secondary popup root is used. If the given DO is NULL, the main
//      popup root is used. Otherwise, this method delegates to
//      VisualTree::GetPopupRootForElement().
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::GetAdjustedPopupRootForElement(_In_opt_ CDependencyObject *pObject, _Outptr_result_maybenull_ CPopupRoot **ppPopupRoot)
{
    CPopupRoot *pPopupRoot = NULL;

    IFCPTR_RETURN(ppPopupRoot);

    if (m_pMainVisualTree)
    {
        if (NULL == pObject)
        {
            pPopupRoot = GetMainPopupRoot();
        }
        else
        {
            IFC_RETURN(VisualTree::GetPopupRootForElementNoRef(pObject, &pPopupRoot));
        }
    }

    *ppPopupRoot = pPopupRoot;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetVisualDiagnosticsRoot
//
//  Synopsis:
//      Returns the visual diagnostic root associated with the current tree
//      The returned Grid object is the top most root hosted by the tree,
//      any elements added to it will be drawn on top of any existing UI
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::GetVisualDiagnosticsRoot(_Outptr_result_maybenull_ CGrid **ppGrid)
{
    CGrid *pGrid = nullptr;

    IFCPTR_RETURN(ppGrid);

    if (m_pMainVisualTree)
    {
        pGrid = m_pMainVisualTree->GetVisualDiagnosticsRoot();
    }

    *ppGrid = pGrid;

    return S_OK;
}

//
// Report error information to Browser host for further handling.
// The error information should be created before reporting.
//
void CCoreServices::ReportAsyncErrorToBrowserHost( )
{
    if (m_pBrowserHost != NULL)
    {
        m_pBrowserHost->ReportError( );
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a media queue that can be used for processing events on
//      the UI thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CreateMediaQueue(_In_ IMediaQueueClient *pClient, IMediaQueue** ppQueue)
{
    IFCPTR_RETURN(pClient);

    xref_ptr<CMediaQueue> pMediaQueue;
    IFC_RETURN(m_pmqm->CreateMediaQueue(pClient, this, pMediaQueue.ReleaseAndGetAddressOf()));

    *ppQueue = pMediaQueue.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calls ProcessQueue on the MediaQueue, from the UI thread
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ProcessMediaEvents(bool fShuttingDown)
{
    if (m_pmqm)
    {
        IFC_RETURN(m_pmqm->ProcessQueues(fShuttingDown));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CCoreServices::CallbackEventListener
//
//  Synopsis:
//      Fires an async callback to a delegate based on the eventarg type
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CallbackEventListener(_In_ CDependencyObject *pSender, _In_ CEventArgs* pEventArgs)
{
    // This is a strange function that seems to exist to serve as a catch-all for the callbacks
    // that come from events of the type KnownEventIndex::DependencyObject_RaiseAsyncCallback.
    // Presently, the only function that makes use of this is CAutomationPeer::InvalidatePeer,
    // which triggers the recalculation of the main properties of the AutomationPeer, and raises
    // the PropertyChanged notification to the Automation Client if the properties changed. These
    // notifications would normally fire "automatically" at the end of
    // CLayoutManager::UpdateLayout. The mechanism to fire them on command exists because,
    // according to the documentation, sometimes properties change without triggering a layout
    // pass. For example, when the whole Window moves, layout is not invalidated. In this example,
    // the set of BoundingRectangle objects change because the objects are expressed in screen
    // coordinates, and thus the Window must call InvalidatePeer on its WindowAutomationPeer to
    // force the recalculation. Now, we must be careful when trying to apply this same logic to
    // other automation events that would normally fire "automatically". Consider the
    // StructureChanged event. By definition, this event will only provide correct information if
    // it is fired after CLayoutManager::UpdateLayout has completed, because it requires the tree
    // structure to be stable.
    IFCEXPECT_RETURN(pEventArgs);

    CAutomationPeerEventArgs* pAPEventArgs = pEventArgs->AsAutomationPeerEventArgs();
    if (pAPEventArgs != nullptr)
    {
        if (pAPEventArgs->m_pAP != nullptr)
        {
            pAPEventArgs->m_pAP->RaiseAutomaticPropertyChanges(/* firePropertyChangedEvents */ TRUE);
            pAPEventArgs->m_pAP->UnpegManagedPeerNoRef();
        }
    }
    else
    {
        IFC_RETURN(E_INVALIDARG);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
// Method:  CCoreServices::UIAClientsAreListening
//
// Synopsis:
//     Return S_OK if UIA client listen the specified event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UIAClientsAreListening(_In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    HRESULT hr = S_FALSE;

    if (m_pBrowserHost != nullptr)
    {
        hr = m_pBrowserHost->UIAClientsAreListening(eAutomationEvent);
        if (hr == S_OK)
        {
            return S_OK;
        }
        else if (hr == E_NOTIMPL)
        {
            hr = S_FALSE;
        }

        if (HasXamlIslands())
        {
            // Check to see if we have a XamlIsland that has active UIA
            CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();
            if (xamlIslandRootCollection)
            {
                CDOCollection* collection = xamlIslandRootCollection->GetChildren();
                if (collection)
                {
                    for (CDependencyObject* child : *collection)
                    {
                        CXamlIslandRoot* island = do_pointer_cast<CXamlIslandRoot>(child);
                        if (S_OK == island->UIAClientsAreListening(eAutomationEvent))
                        {
                            return S_OK;
                        }
                    }
                }
            }
        }
    }

    return hr;
}

//------------------------------------------------------------------------
//
// Method:  CCoreServices::UIARaiseAutomationEvent
//
// Synopsis:
//     Raises a UIAutomation Event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UIARaiseAutomationEvent(_In_ CAutomationPeer *pAP,
                                       _In_ UIAXcp::APAutomationEvents eAutomationEvent)
{
    CUIAWindow* uiaWindow = GetUIAWindowForElementRootNoRef(pAP->GetRootNoRef());
    if (uiaWindow)
    {
        IFC_RETURN_ALLOW(uiaWindow->UIARaiseAutomationEvent(pAP, eAutomationEvent), E_NOTIMPL);
    }
    return S_OK;
}

void CCoreServices::RegisterForStructureChangedEvent(
    _In_ CAutomationPeer* automationPeer,
    _In_ CAutomationPeer* parent,
    AutomationEventsHelper::StructureChangedType type)
{
    m_automationEventsHelper.RegisterForStructureChangedEvent(automationPeer, parent, type);
}

//------------------------------------------------------------------------
//
// Method:  CCoreServices::RaisePropertyChangedEvent
//
// Synopsis:
//     Raises a UIAutomation Event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UIARaiseAutomationPropertyChangedEvent(_In_ CAutomationPeer *pAP,
                                                      _In_ UIAXcp::APAutomationProperties eAutomationProperty,
                                                      _In_ const CValue& oldValue,
                                                      _In_ const CValue& newValue)
{
    CUIAWindow* uiaWindow = GetUIAWindowForElementRootNoRef(pAP->GetRootNoRef());
    if (uiaWindow)
    {
        IFC_RETURN_ALLOW(uiaWindow->UIARaiseAutomationPropertyChangedEvent(pAP, eAutomationProperty, oldValue, newValue), E_NOTIMPL);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// Method:  CCoreServices::UIARaiseFocusChangedEventOnUIAWindow
//
// Synopsis:
//     Raises a UIAutomation Event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UIARaiseFocusChangedEventOnUIAWindow(_In_ CDependencyObject* sender)
{
    CUIAWindow* uiaWindow = GetUIAWindowForElementRootNoRef(GetRootForElement(sender));

    if (uiaWindow)
    {
        IFC_RETURN_ALLOW(uiaWindow->UIARaiseFocusChangedEventOnUIAWindow(), E_NOTIMPL);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
// Method:  CCoreServices::UIARaiseTextEditTextChangedEvent
//
// Synopsis:
//     Raises a TextEditTextChanged Event
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UIARaiseTextEditTextChangedEvent(_In_ CAutomationPeer *pAP, _In_ UIAXcp::AutomationTextEditChangeType automationTextEditType, _In_ CValue *pChangedData)
{
    CUIAWindow* uiaWindow = GetUIAWindowForElementRootNoRef(pAP->GetRootNoRef());
    if (uiaWindow)
    {
        IFC_RETURN_ALLOW(uiaWindow->UIARaiseTextEditTextChangedEvent(pAP, automationTextEditType, pChangedData), E_NOTIMPL);
    }
    return S_OK;
}

_Check_return_ HRESULT CCoreServices::UIARaiseNotificationEvent(
    _In_ CAutomationPeer* ap,
    UIAXcp::AutomationNotificationKind notificationKind,
    UIAXcp::AutomationNotificationProcessing notificationProcessing,
    _In_opt_ xstring_ptr displayString,
    _In_ xstring_ptr activityId)
{
    CUIAWindow* uiaWindow = GetUIAWindowForElementRootNoRef(ap->GetRootNoRef());
    if (uiaWindow)
    {
        IFC_RETURN_ALLOW(uiaWindow->UIARaiseNotificationEvent(ap, notificationKind, notificationProcessing, displayString, activityId), E_NOTIMPL);
    }
    return S_OK;
}

CUIAWindow* CCoreServices::GetUIAWindowForElementRootNoRef(_In_ CDependencyObject* elementRoot)
{
    CXamlIslandRoot* xamlIsland = do_pointer_cast<CXamlIslandRoot>(elementRoot);
    if (xamlIsland)
    {
        return xamlIsland->GetUIAWindowNoRef();
    }

    if (UseUiaOnMainWindow() && m_pBrowserHost)
    {
        CUIAWindow* uiaWindow = nullptr;
        IFCFAILFAST(m_pHostSite->GetUIAWindow(elementRoot, GetHostSite()->GetXcpControlWindow(), true /* onlyGet */, &uiaWindow));
        return uiaWindow;
    }

    return nullptr;
}


//------------------------------------------------------------------------
//
//  CAbortableDownloadWrapper::Create
//
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CAbortableDownloadWrapper::Create(
                                  _In_ DREQUEST* pDownloadRequest,
                                  _In_ CCoreServices* pcs,
                                  _Outptr_ CAbortableDownloadWrapper** ppAbortableDownload)
{

    IFCPTR_RETURN(pDownloadRequest);
    IFCPTR_RETURN(pcs);
    IFCPTR_RETURN(ppAbortableDownload);

    xref_ptr<CAbortableDownloadWrapper> pAbortableDownload;
    pAbortableDownload.attach(new CAbortableDownloadWrapper);

    pAbortableDownload->m_pcs = pcs;
    pAbortableDownload->m_pRequest = pDownloadRequest;

    *ppAbortableDownload = pAbortableDownload.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  CAbortableDownloadWrapper::Abort
//
//  Synopsis:
//      Tell the core to remove the download request from the queue
//
//------------------------------------------------------------------------
void
CAbortableDownloadWrapper::Abort()
{
    if (m_pcs)
    {
        // we still want to NULL our the pcs and the request in the
        // failure case.  This will only fail if the request is not
        // not found.
        IGNOREHR(m_pcs->RemoveDownloadRequest(m_pRequest));
        m_pRequest = NULL;
        m_pcs = NULL;
    }
    else if (m_pAbortable)
    {
        m_pAbortable->Abort();
        ReleaseInterface(m_pAbortable);
    }
}

//------------------------------------------------------------------------
//
//  CAbortableDownloadWrapper::AddRef
//
//------------------------------------------------------------------------
XUINT32
CAbortableDownloadWrapper::AddRef()
{
    return ++m_cRef;
}

//------------------------------------------------------------------------
//
//  CAbortableDownloadWrapper::Release
//
//------------------------------------------------------------------------
XUINT32
CAbortableDownloadWrapper::Release()
{
    XUINT32 cRef = --m_cRef;
    if (cRef == 0)
    {
        delete this;
    }
    return cRef;
}

//------------------------------------------------------------------------
//
//  CAbortableDownloadWrapper::SetAbortable
//
//  Synopsis:
//      Provide abortable wrapper with an abortable and switch it to
//      use that when aborting rather than the core's queue
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CAbortableDownloadWrapper::SetAbortable(_In_ IPALAbortableOperation* pAbortable)
{
    IFCPTR_RETURN(pAbortable);

    if (m_pcs)
    {
        // No longer need to know temporary information
        m_pRequest = NULL;
        m_pcs = NULL;
    }

    IFCEXPECTRC_RETURN(m_pAbortable == NULL, E_FAIL);
    m_pAbortable = pAbortable;
    m_pAbortable->AddRef();

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the window render target attached to this object.
//
//------------------------------------------------------------------------------
_Ret_maybenull_ CWindowRenderTarget* CCoreServices::NWGetWindowRenderTarget()
{
    return m_pNWWindowRenderTarget;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the graphics device attached to this object.
//
//------------------------------------------------------------------------------
_Ret_maybenull_ CD3D11Device* CCoreServices::GetGraphicsDevice()
{
    return m_pNWWindowRenderTarget ? m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice() : nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a flag to indicate we are in middle of a print walk.
//
//------------------------------------------------------------------------
bool CCoreServices::IsPrinting()
{
    return m_fPrinting;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a flag to indicate we are in middle of a print walk.
//
//------------------------------------------------------------------------
void CCoreServices::SetIsPrinting(bool val)
{
    m_fPrinting = val;
}

// Determine if the current window is presenting to Head-Mounted-Display or simulating HMD for test purposes
bool CCoreServices::IsHolographic() const
{
    return m_isHolographicOverrideSet || IsPresentationModeHolographic();
}

// Determine if the current window is presenting to a Head-Mounted-Display
bool CCoreServices::IsPresentationModeHolographic()
{
    // Default to false. We'll also return false (which implies we're presenting to desktop) if there is ever any problem querying HolographicApplicationPreviewS.
    bool isHolographic = false;
    ctl::ComPtr<wa::Preview::Holographic::IHolographicApplicationPreviewStatics> holographicApplicationPreviewStatics;

    if (SUCCEEDED(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_ApplicationModel_Preview_Holographic_HolographicApplicationPreview).Get(),
            holographicApplicationPreviewStatics.ReleaseAndGetAddressOf())))
    {
        boolean result = false;
        VERIFYHR(holographicApplicationPreviewStatics->IsCurrentViewPresentedOnHolographicDisplay(&result));
        isHolographic = !!result;
    }

    return isHolographic;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a flag to indicate the application is listening for
//      CompositionTarget.Rendering.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetWantsRendering(bool value)
{
    if (m_fWantsRenderingEvent != value)
    {
        m_fWantsRenderingEvent = value;

        if (m_fWantsRenderingEvent)
        {
            // The event will be raised during Tick, so make sure a Tick is scheduled.
            // The browser host and/or frame scheduler can be NULL during shutdown.
            IXcpBrowserHost *pBH = GetBrowserHost();
            if (pBH != NULL)
            {
                ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
                if (pFrameScheduler != NULL)
                {
                    IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::PerFrameCallback));
                }
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Lazily create Component Host manager, because component hosts are
//  not common
//
//------------------------------------------------------------------------

const wchar_t* CCoreServices::GetAppSessionId()
{
    const auto traceSession = Instrumentation::GetXamlTraceSession();
    return traceSession->GetSessionId();
}

const wchar_t* CCoreServices::GetAppId()
{
    const auto traceSession = Instrumentation::GetXamlTraceSession();
    return traceSession->GetAppId();
}

DWORD CCoreServices::GetProcessId()
{
    const auto traceSession = Instrumentation::GetXamlTraceSession();
    return traceSession->GetProcessId();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The main visual tree update method.  Ticks the time manager, fires events,
//      runs layout, and renders the element tree into the given surface.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::NWDrawMainTree(
    _In_ CWindowRenderTarget* pIRenderTarget,
    bool forceRedraw,
    _Out_ bool* pFrameDrawn
    )
{
    HRESULT hr = S_OK;
    *pFrameDrawn = false;

    // No-op tick processing while the main tree is loading.  Once it's loaded, another tick will
    // be requested which will run layout, raise the loaded event, render the first frame, etc.
    if (!m_isMainTreeLoading)
    {
        // Capture the start time.
        if (GetPALCoreServices())
        {
            XINT64_LARGE_INTEGER liQpc = {0};
            GetPALCoreServices()->PerformanceCounter(&liQpc);
            m_qpcDrawMainTreeStart = liQpc.QuadPart;
        }

        // Ensure that HWnd-specific resources are created on the DCompTreeHost.
        // This needs to happen early in the lifted world. We need to create the ICoreWindowSiteBridge and the InputSite with
        // the CoreWindow before we can call into InputServices to initialize DManip. Otherwise attempting to call Activate on the
        // DirectManipulationManager will fail.
        switch (GetInitializationType())
        {
            case InitializationType::BackgroundTask:
            case InitializationType::IslandsOnly:
                break;

            default:
            {
                const auto& presentTarget = pIRenderTarget->GetPresentTarget();
                XHANDLE targetWindow = presentTarget != nullptr ? presentTarget->GetTargetWindowHandle() : nullptr;

                const auto& dcompTreeHost = pIRenderTarget->GetDCompTreeHost();
                IFCFAILFAST(dcompTreeHost->EnsureDCompDevice());

                auto previousIsland = dcompTreeHost->GetCoreWindowContentIsland();
                IFC(dcompTreeHost->SetTargetWindowUWP(static_cast<HWND>(targetWindow)));
                if (previousIsland == nullptr)
                {
                    auto currentIsland = dcompTreeHost->GetCoreWindowContentIsland();
                    if (currentIsland != nullptr)
                    {
                        // ContentIsland is now initialized, we can notify JupiterWindow
                        DXamlServices::GetCurrentJupiterWindow()->EnsureInputSiteAdapterForCoreWindow(currentIsland);
                    }
                }
            }
        }

        // Update input manager state.
        // This only happens when ticking and drawing the main tree - there's no DM support for hosted visual trees.
        if (m_inputServices)
        {
            IFC(m_inputServices->ProcessUIThreadTick());
        }

        IFC(NWDrawTree(
            GetHWWalk(),
            pIRenderTarget,
            m_pMainVisualTree,
            forceRedraw,
            pFrameDrawn
            ));

        if (m_inputServices)
        {
            m_inputServices->OnPostUIThreadTick();
        }

        // This callback is used only by test code
        if (m_postTickCallback)
        {
            m_postTickCallback();
        }
    }
    // For tests that need the UI thread to tick and clean up after device lost and pick up MockDComp. Xaml islands
    // need to do this to make sure the compositor doesn't get released after the island itself was created.
    else if (m_canTickWithNoContent && m_fWantsRenderingEvent)
    {
        IFCFAILFAST(CallPerFrameCallback(0));
    }

Cleanup:
    m_qpcDrawMainTreeStart = 0;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The main visual tree update method.  Ticks the time manager, fires events,
//      runs layout, and renders the element tree into the given surface.
//
//      IMPORTANT: This function makes multiple synchronous callouts to app
//      code. It needs to be hardened against reentrancy, such as keeping
//      objects alive during callouts and re-validating state when callouts return.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::NWDrawTree(
    _In_ HWWalk *pHWWalk,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_opt_ VisualTree *pVisualTree,
    bool forceRedraw,
    _Out_ bool * pFrameDrawn
    )
{
    HRESULT hr = S_OK;

    CLayoutManager* pLayoutManager = NULL;

    XFLOAT rFrameStartTime = 0.0f;
    xvector<CCompositorDirectManipulationViewport *> directManipulationViewports;
    bool directManipulationViewportsChanged = false;
    CUIElement *pVisualRoot = pVisualTree != nullptr ? pVisualTree->GetRootVisual() : nullptr;

    XSIZE_LOGICAL layoutSize;
    XUINT32 uLayoutWidth = 0;
    XUINT32 uLayoutHeight = 0;
    *pFrameDrawn = false;

    // TODO: This is redundant. It should be replaced with the value from GetWindowSize.
    XRECTF_RB windowBounds =
    {
        0.0f,
        0.0f,
        static_cast<XFLOAT>(pRenderTarget->GetWidth()),
        static_cast<XFLOAT>(pRenderTarget->GetHeight())
    };

    CTimeManager *pTimeMgrNoRef = GetTimeManager();

    bool checkForAnimationComplete = false;
    bool hasActiveFiniteAnimations = false;

    const bool isRenderEnabled = IsRenderingFrames();

    const auto& dcompTreeHost = pRenderTarget->GetDCompTreeHost();

    //
    // Trace the beginning of the frame
    //
    TraceFrameBegin();

    // tell framework about this time
    IFC(FxCallbacks::FrameworkCallbacks_BudgetService_StoreFrameTime(TRUE /* beginning of tick */));

    // Check for re-entrancy by forcing a XAML_FAIL_FAST() right here if we re-enter.
    // This is to prevent us from corrupting our state if something in the render
    // loop accidentally pumps a message loop and causes another paint to come in.
    // It also lets us debug watson crashes by providing a clear stack with the problem
    // code on the call stack.

    if (m_pDrawReentrancyCheck == NULL) XAML_FAIL_FAST();
    m_pDrawReentrancyCheck = NULL; // Set to NULL so we XAML_FAIL_FAST() if we re-enter

    // Get or create the IPAL clock and snapshot frame start time
    if (!m_pPALClock)
    {
        IFC(gps->CreateClock(m_pPALClock.ReleaseAndGetAddressOf()));
    }

    rFrameStartTime = static_cast<XFLOAT>(m_pPALClock->GetAbsoluteTimeInSeconds());

    if (m_isSuspended)
    {
        IGNOREHR(ProcessMediaEvents());
        // Suspend-exempt media apps can still run when minimized. We want to keep DispatcherTimers running for them, while not ticking other animations.
        pTimeMgrNoRef->TickTimers(GetBrowserHost()->GetFrameScheduler());
    }
    else
    {
        // Tick the timing manager, event manager, and deferred media event queue.
        IFC(Tick(TRUE /* tickForDrawing */, &checkForAnimationComplete, &hasActiveFiniteAnimations));
    }

    // Get any DM viewport changes. Any such changes require a render walk.
    IFC(GetDirectManipulationChanges(
        directManipulationViewports,
        &directManipulationViewportsChanged
        ));

    // TODO: MERGE: This layout code is often wrong unless rendering the root visual.
    //      In the case where a subtree is being rendered the surface size that the subtree is being
    //      drawn in to is used for calculating layout, which would be fine except that layout is
    //      then calculated from the root. Layout should be calculated for the specific element
    //      being rendered and constrained to the size of the surface.
    //
    //      In most cases, as long as layout is not dirty the layout manager will realize there is
    //      nothing to do and not update layout for all the elements. However, if the layout has been
    //      dirtied and then UpdateLayout is called it will incorrectly layout the tree.
    //
    //      This is an existing issue that causes hosted render target bitmap and writeable
    //      bitmap to behave poorly with layout in some cases. Existing behavior is preserved here
    //      for the merge but needs to be fixed to make RTB and WB work correctly with layout in
    //      all cases.

    // Run layout.
    GetWindowSize(pVisualRoot, nullptr /* we don't care about the physical size yet */, &layoutSize);
    uLayoutWidth = layoutSize.Width;
    uLayoutHeight = layoutSize.Height;
    // If there is nothing to do layout on, skip this work...
    if (pVisualRoot != nullptr)
    {
        // When zoom scaled is changed, we need to re-measure every elements in the visual tree because of layout rounding.
        // For example the app has a visual tree that looks like this:
        //    <Grid>
        //        <ContentPresenter Margin="6">
        //            <TextBlock Margin="0,0,0,-6" Height="20" />
        //        </ContentPresenter>
        //    </Grid>
        //
        // The grid has a row with Height="Auto". It goes to decide how big the row is by measuring its children.
        // The TextBlock's height is effectively 20 (not explicitly set, but measured correctly from its text), and it adds its margin of -6 to get a height of 14.
        // Its parent ContentPresenter adds its margins of 6 to get a height of 26px, which is used by the Grid as its row height.
        //
        // Come arrange time, the grid arranges the ContentPresenter at height 26. If it decides to apply a layout clip on the ContentPresenter, it will apply a clip of 26 - 6 - 6 = 14 (CFrameworkElement::UpdateLayoutClip subtracts the margin from the final rect).
        // The TextBlock needs 20px, so a clip of 14 would cause it to clip about 6 pixels.
        //
        // Whether layout clip should be applied is determined in CFrameworkElement::ArrangeCore,
        // and one of the comparisons involved is comparing the arrangeSize against the UnclippedDesiredSize.
        // When the arrange size was slightly smaller than the UnclippedDesiredSize, the layout clip will be applied.
        // The reason is that arrangeSize was put through layout rounding in CUIElement::Arrange,
        // but UnclippedDesiredSize was put through layout rounding the last time that the element was measured.
        // This matters because CUIElement::LayoutRound takes the zoom scale into account in order to snap to physical pixels rather than logical pixels.
        // When the element was measured, it was rounded to whole pixels. When the zoom scale changes, it isn't measured again, but it gets laid out at a size that rounds to (1/1.4) of a pixel.
        // The measured size is larger than the size given by arrange, so we apply the 14(ish, it's rounded too) pixel clip.
        if (m_zoomScaleChanged_ForceRelayout || m_localeSettingChanged)
        {
            // Don't force measure for the first frame because the measure is already dirty on all elements.
            if (m_uFrameNumber != 1)
            {
                if (m_zoomScaleChanged_ForceRelayout)
                {
                    TraceRecursiveInvalidateMeasureInfo(L"Zoom Scale Changed");
                }
                if (m_localeSettingChanged)
                {
                    TraceRecursiveInvalidateMeasureInfo(L"Locale Setting Changed");
                }
                pVisualRoot->RecursiveInvalidateMeasure();
            }

            // Layout runs even if Xaml isn't visible, unlike rendering. Clear these flags now so they don't constantly trigger more
            // layout invalidations while the window stays invisible.
            m_localeSettingChanged = FALSE;
            m_zoomScaleChanged_ForceRelayout = FALSE;   // The re-render flag will be cleared after we render
        }

        pLayoutManager = VisualTree::GetLayoutManagerForElement(pVisualRoot);
        IFCPTR(pLayoutManager);

        // IMPORTANT: This is a synchronous callout to app code that could change state.
        if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46076120>())
        {
            auto endOnExit = m_themeWalkResourceCache->BeginCachingThemeResources();
            IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));
        }
        else
        {
            IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));
        }

        // VSIS's PreUpdateVirtual (and thereby EndDraw) method relies on tiling
        // information which could be  stale if the resources were lost since the last call,
        // hence call HandleLostResources on each VSIS. Usually lost resources for an image
        // source is handled while determining the brush parameters for its parent brush,
        // but that's too late for VSIS.
        if (m_fSurfaceContentsLostEnqueued)
        {
            HandleVirtualSurfaceImageSourceLostResources();
        }

        // Raise event to the user that tells them to recreate content in shared surfaces
        // because we lost the device or recreated atlases.
        // We do this after layout is updated so that the bounds are correct, but before CompositionTarget.Rendering so
        // that when Rendering happens everything has already been cleaned up.
        // TODO:  Think about ordering...should this be after CompositionTarget.Rendering?
        //
        // IMPORTANT: This is a synchronous callout to app code that could change state.
        IFC(RaiseQueuedSurfaceContentsLostEvent());

        // Now call any VirtualSurfaceImageSources (that are in the tree) so they re-compute visible bounds
        // and fire update events if needed
        IFC(VirtualSurfaceImageSourcePerFrameWork(&windowBounds));


        CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();
        if (xamlIslandRootCollection)
        {
            CDOCollection* collection = xamlIslandRootCollection->GetChildren();
            if (collection)
            {
                for (CDependencyObject* child : *collection)
                {
                    CXamlIslandRoot* island = do_pointer_cast<CXamlIslandRoot>(child);
                    CContentRoot* contentRoot = island->GetContentRootNoRef();
                    if (contentRoot->HasPendingXamlRootChangedEvent())
                    {
                        // IMPORTANT: This is a synchronous callout to app code that could change state.
                        contentRoot->RaisePendingXamlRootChangedEventIfNeeded(false /*shouldRaiseWindowChangedEvent*/ );
                    }
                }
            }
        }
        // Raise the Loaded event here (synchronously) if needed. The order is critical.
        // We want to do this after layout - so any templates will have been applied.
        // We want to do this before rendering - to prevent flashing if the Loaded handlers modify the tree.
        // Finally, we need to re-layout if we raise the Loaded event, since a handler may have modified the tree.
        //
        // IMPORTANT: This is a synchronous callout to app code that could change state.
        if (m_pEventManager && m_pEventManager->ShouldRaiseLoadedEvent())
        {
            IFC(m_pEventManager->RaiseLoadedEvent());

            IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));
        }

        // Call the user code per-frame callback (synchronous; doesn't call into managed unless someone is listening)
        //
        // IMPORTANT: This is a synchronous callout to app code that could change state.
        if (m_fWantsRenderingEvent && m_pTimeManager != NULL)
        {
            IFC(CallPerFrameCallback(m_pTimeManager->GetLastTickTime()));

            // User could have changed layout in the callback. We have to layout again if they're listening :(
            IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));
        }

        // IMPORTANT: this is a synchronous callout to app code that could change state
        bool shouldScheduleANewTick = false;
        // we are going to build trees. That might change locations of elements, but that should
        // not trigger animations. Basically we see these as hopefully invisible quick fill-ins of data
        pLayoutManager->SetAllowTransitionsToRunUnderCurrentSubtree(FALSE);

        IFC(FxCallbacks::FrameworkCallbacks_PhasedWorkDistributor_PerformWork(&shouldScheduleANewTick));
        if (shouldScheduleANewTick)
        {
            ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler)
            {
                IFC(pFrameScheduler->RequestAdditionalFrame(0, RequestFrameReason::PhasedWork));
            }
        }

        // User or framework could have changed layout in the callback.
        IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));

        pLayoutManager->SetAllowTransitionsToRunUnderCurrentSubtree(TRUE);

        if (isRenderEnabled)
        {
            // Calculate the sizes of the RTBs to render
            IFC(m_pRenderTargetBitmapManager->UpdateMetrics());

            // User or framework could have changed layout in the callback.
            IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));
        }

        // Re-position the focus rectangle if needed
        for (const auto& contentRoot : m_contentRootCoordinator.GetContentRoots())
        {
            contentRoot->GetFocusManagerNoRef()->UpdateFocusRect(DirectUI::FocusNavigationDirection::None);
        }

        // create transitions that have been deferred until now
        IFC(pLayoutManager->RealizeRegisteredLayoutTransitions());

        // Re-evaluate the state of implicit Show/Hide animations and "keep-visible" elements.
        // Note:  It's extremely important that this be called before we call pLayoutManager->IncrementLayoutCounter()
        // otherwise we will get the wrong layout counter applied as we call into UIElement_ShouldPlayImplicitShowHideAnimation().
        dcompTreeHost->UpdateImplicitShowHideAnimations();
        IFC(pLayoutManager->UpdateLayout(uLayoutWidth, uLayoutHeight));

        // do a dirty walk that will effect layout on elements
        IFC(pLayoutManager->TransitionLayout());

        // Let GetLayoutTickForTransition know that we're done processing transitions this tick. Any transitions scheduled now will be processed NEXT tick.
        pLayoutManager->IncrementLayoutCounter();

        if (isRenderEnabled)
        {
            IFC(m_pRenderTargetBitmapManager->PickupForRender());
        }
    }

    // Tick only the newly added storyboards - this is necessary for animated values to get set in this frame.
    if (pTimeMgrNoRef != nullptr && !m_isSuspended)
    {
        IXcpBrowserHost *pBH = GetBrowserHost();
        ASSERT(pBH != nullptr);

        bool newAnimationsCheckForAnimationComplete = false;
        bool newAnimationsHasActiveFiniteAnimations = false;

        // Note that layout can add more animations, so timelines are ticked in two separate places: once before layout
        // (to tick normal Storyboards) with newTimelinesOnly == false, and once after layout (to tick animations kicked
        // off by layout) with newTimelinesOnly == true. When we decide whether there are animations active, we need to
        // look at both these places and combine the result.
        IFC(pTimeMgrNoRef->Tick(
            TRUE /* newTimelinesOnly */,
            TRUE /* processIATargets */,
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetMainDevice(),
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetEasingFunctionStatics(),
            m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor(),
            pBH->GetFrameScheduler(),
            false /* tickOnlyTimers */,
            &newAnimationsCheckForAnimationComplete,
            &newAnimationsHasActiveFiniteAnimations
            ));

        checkForAnimationComplete |= newAnimationsCheckForAnimationComplete;
        hasActiveFiniteAnimations |= newAnimationsHasActiveFiniteAnimations;
    }

    if (GetInputServices())
    {
        GetInputServices()->GetKeyTipManager().NotifyFiniteAnimationIsRunning(this, hasActiveFiniteAnimations);
    }

    // NOTE: No user code callbacks should be made beyond this point.
    // It's possible that one of the previous user-code callbacks closed the Window.
    // We should not attempt to draw a frame after the window has closed.
    if (!m_pHostSite->IsWindowDestroyed())
    {
        // TODO: Only MediaBase uses this now, could probably be removed entirely
        // Update the state of elements that change their state asynchronously.
        IFC(UpdateDirtyState());

        // Ensure that device resources have been created before the render walk.
        if (isRenderEnabled)
        {
            IFC(pRenderTarget->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());
            IFC(EnsureDeviceLostListener());
        }

        switch (GetInitializationType())
        {
            case InitializationType::BackgroundTask:
            case InitializationType::IslandsOnly:
                IFC(dcompTreeHost->UpdateExplicitAtlasHint());
                break;
        }

        // Update the element tree for any changes in independent animations since the previous frame.
        // Cloning animation targets requires that the DComp device is initialized, since some targets have DComp resources.
        if (pTimeMgrNoRef)
        {
            IFC(pTimeMgrNoRef->UpdateIATargets());
        }

        if( WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46833401>() && isRenderEnabled && m_framesToSkip != 0)
        {
            m_framesToSkip--;
            
            ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler != NULL)
            {
                IFC(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::LayoutCompletedNeeded));
            }
        }
        else if (isRenderEnabled)
        {
            bool canSubmitFrame = true;

            // Also force redraw if the zoom scale changes. We need to recalculate the sizes of full window media and
            // SwapChainBackgroundPanel to make them fill the screen with the new zoom level.
            forceRedraw = forceRedraw || m_zoomScaleChanged_ForceRerender;

            // Check If there is any prepared connected animation that is too old and mark it Canceled.
            // We do this prior to the RenderWalk so that if we end up not needing some comp nodes any more
            // we will clean them up.
            if (m_connectedAnimationService)
            {
                IFC(m_connectedAnimationService->TimeoutAnimations());
            }

            // Check whether this frame needs rendering or not.
            // The frame never needs rendering when in a
            // background task.
            const bool needsRenderWalk =
                (!IsInBackgroundTask() &&
                pVisualRoot != nullptr &&
                    (  forceRedraw
                    || pVisualRoot->NWNeedsRendering()
                    || m_renderStateChanged));

            // We do a render walk only if it is determined to be needed by
            // logic above and the window is visible. Only exception is that
            // we do the render walk even if the window is not visible if this is the
            // first frame; this is needed for window activation calls to work properly.
            if (needsRenderWalk)
            {
                m_replayPointerUpdateAfterTick = true;

                // Check to see if GC pressure is required
                TrackMemory();

                // Ensure XamlIsland target roots have been created
                IFC(dcompTreeHost->EnsureXamlIslandTargetRoots());

                // Connected animations may need to prepare themselves to re-snapshot
                // visuals that are going to be rendered.
                if (m_connectedAnimationService)
                {
                    IFC(m_connectedAnimationService->PreRenderWalk());
                }

                const bool hasUIAClientsListeningToStructure =
                    (UIAClientsAreListening(UIAXcp::AEStructureChanged) == S_OK);

                hr = RenderWalk(
                    pHWWalk,
                    pRenderTarget,
                    pVisualRoot,
                    forceRedraw,
                    hasUIAClientsListeningToStructure,
                    &canSubmitFrame);

                // If there is a UIA client listening, fire the pending StructureChanged events.
                // We do it right after the render walk has completed because some of these events
                // are requested during rendering. Doing it here also guarantees that the tree is
                // not changing anymore.
                if (hasUIAClientsListeningToStructure)
                {
                    m_automationEventsHelper.RaiseStructureChangedEvents();
                }
                else
                {
                    // Clear the requests for StructureChanged events in case one or more were
                    // registered; we are not listening for structure changes anymore so these are
                    // not needed.
                    m_automationEventsHelper.CleanUpStructureChangedRequests();
                }

                IFC_DEVICE_LOST_OTHERWISE_FAIL_FAST(hr);
            }

            // Do the render walks for RenderTargetBitmaps. Each of these render walks creates a new comptree with a new set of
            // primitives and primitive groups. These render walks are specialized such that they do not mess up the
            // incremental render data, or modify the state of the UIElement tree in any way.
            bool hasRenderTargetBitmaps = false;
            IFC(m_pRenderTargetBitmapManager->RenderElements(
                pHWWalk,
                pRenderTarget,
                &hasRenderTargetBitmaps
                ));

            // Due to background thread image loading, there may be hardware surfaces that have been filled
            // but require a DComp update.  This is usually done during the render walk, but it is possible that
            // the element was culled.
            FlushPendingImageUpdates();

            // Submit all texture updates that were queued by rendering this frame.
            // This can occur outside the lock, since the independent thread never accesses textures or the graphics device.
            IFC(pHWWalk->GetTextureManager()->SubmitTextureUpdates());

            // A new frame should be submitted to DComp if
            // ... a main render walk was done, which might modify the DComp visual tree
            // ... the animation tree has changed, which might also modify DComp property values
            // ... debug settings changed, which requires updating DComp APIs
            // ... we need to call LayoutCompleted()
            // TODO: JCOMP: We shouldn't need to submit a frame if we're just updating RTB.
            // TODO: JCOMP: However, DComp requires a Commit call before DrawPrimitiveGroup works. This is bug 123667.
            // TODO: JCOMP: Once this is fixed, we can remove the extra condition here for rtb.
            const bool shouldSubmitFrame = needsRenderWalk
                || (pTimeMgrNoRef && pTimeMgrNoRef->HaveIndependentTimelinesChanged())
                || hasRenderTargetBitmaps
                || m_debugSettingsChanged
                || m_fLayoutCompletedNeeded
                || m_commitRequested;

            // We reset the CommitRequested flag here because Connected Animations may request a subsequent commit during
            // SubmitPrimitiveCompositionCommands (for example, if it is waiting on a manipulation to complete).
            m_commitRequested = FALSE;

            if (!canSubmitFrame)
            {
                m_skippedSubmittingFrameDueToRenderingError = true;
            }
            else if (shouldSubmitFrame)
            {
                if (m_forceDisconnectRootOnSuspend)
                {
                    // Ensure that the DComp visual root is reconnected if
                    // it was previously disconnected.
                    dcompTreeHost->EnsureRootConnected();
                }

                for (const xref_ptr<CContentRoot>& contentRoot: m_contentRootCoordinator.GetContentRoots())
                {
                    // Reset the counter as we're submitting the frame
                    contentRoot->GetInputManager().SetKeyDownCountBeforeSubmitFrame(0);
                }

                TraceSubmitFrameBegin();

                IFC(SubmitPrimitiveCompositionCommands(
                    pRenderTarget,
                    pVisualRoot,
                    m_pPALClock,
                    rFrameStartTime));

                // Once we have submitted the composition commands, we can consider that a frame was drawn.
                *pFrameDrawn = true;

                TraceSubmitFrameEnd();

                IFC(dcompTreeHost->ConnectXamlIslandTargetRoots());

                // If listeners have registered for CompositionTarget.Render, fire the event
                if (m_fWantsCompositionTargetRenderedEvent)
                {
                    // Calculate the total duration for this frame : total layout time + render time in milliseconds
                    XFLOAT frameDurationInMilliseconds = 1000.0f * (static_cast<XFLOAT>(m_pPALClock->GetAbsoluteTimeInSeconds()) - rFrameStartTime);

                    // Set RenderedEventArgs frame render duration as a TimeSpan in ticks
                    wf::TimeSpan timeSpan = {};
                    xref_ptr<CRenderedEventArgs> renderedEventArgs;
                    renderedEventArgs.attach(new CRenderedEventArgs());
                    timeSpan.Duration = static_cast<INT64>(frameDurationInMilliseconds * CCORESERVICES_TICKS_PER_MILLISECOND);
                    IFC(renderedEventArgs->put_FrameDuration(timeSpan));

                    // AddRef for renderedEventArgs is called on the managed side
                    IGNOREHR(FxCallbacks::JoltHelper_RaiseEvent(
                        /* target */ NULL,
                        DirectUI::ManagedEvent::ManagedEventRendered,
                        renderedEventArgs));

                    // Determine if there are still CompositionTarget.Rendered handlers present.
                    // Note that the handlers can only be removed without calling the API by the
                    // event handler returning RPC_E_DISCONNECTED.  This condition can only happen
                    // after the event has been raised, so it is reasonable to check the event source
                    // for handlers here.
                    if (!DXamlServices::HasCompositionTargetRenderedEventHandlers())
                    {
                        // Handlers were removed through a mechanism other than the Composition.Rendered API and
                        // the Rendered event callbacks are no longer required.
                        m_fWantsCompositionTargetRenderedEvent = false;
                    }
                }

                m_zoomScaleChanged_ForceRerender = FALSE;
                m_renderStateChanged = FALSE;
                m_debugSettingsChanged = FALSE;
                m_skippedSubmittingFrameDueToRenderingError = false;
            }

            // Draw the comp trees created for RTBs onto their
            // corresponding surfaces.
            IFC(m_pRenderTargetBitmapManager->DrawCompTrees(pRenderTarget));
        }
        else if (pTimeMgrNoRef != nullptr && pTimeMgrNoRef->HasTargetDOs())
        {
            // Even if rendering is disabled, we would have ticked Xaml animations, which created DComp animations. Those Xaml
            // animations will then wait for the completed notifications of those DComp animations. But since rendering is disabled,
            // we never attached the DComp animations to the tree, which means they never started, and will never deliver a completed
            // notification. Xaml will then tick forever waiting for those animations to finish.
            //
            // This is usually not a problem, since if rendering is disabled, then the window has gone invisible, so the process will
            // be suspended anyway. Some apps are exempt from PLM suspend though, and they'll tick forever.
            //
            // We have a mechanism to connect all DComp animations. Normally that's called as part of submitting a frame, but with
            // rendering disabled we're not submitting any frames. Call that mechanism explicitly here.
            //
            // Note that this will no longer be needed if we don't tick invisible animations in the DWM. Those animations can complete
            // without waiting for the DComp animation.
            pTimeMgrNoRef->UpdateTargetDOs(m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor());
            IFC(dcompTreeHost->CommitMainDevice());
        }
    }

    if (isRenderEnabled)
    {
        // The render walk may have enqueued requests to decode images when DecodeToRenderSize is used.
        // Now that we're outside the render walk, we can safely make those requests.
        IFC(FlushImageDecodeRequests());

        // Issue decode requests for the bitmap images that didn't get visited
        // during the render walk due to be culled out.
        IFC(ProcessTrackedImages());
    }

    // tell framework about this time
    IFC(FxCallbacks::FrameworkCallbacks_BudgetService_StoreFrameTime(FALSE /* beginning of tick */));

    // Release device resources on ImageSurfaceWrappers that have left the tree for a fixed amount of time
    // This is intentionally put at the end of the render walk to give the render walk a chance to resurrect
    // an image element before it is cleaned up.
    TrimMemoryOnSurfaceWrappers();

    if (pTimeMgrNoRef != nullptr && checkForAnimationComplete)
    {
        pTimeMgrNoRef->UpdateAnimationEvents(hasActiveFiniteAnimations);
    }

    if (m_isFrameAfterVisualTreeReset)
    {
        IFCFAILFAST(SetRootVisualResetEventSignaledStatus(TRUE));
        m_isFrameAfterVisualTreeReset = false;
    }

    if (m_replayPointerUpdateAfterTick)
    {
        const auto currentTime = gps->GetCPUMilliseconds();
        const auto timeSinceLastReplay = currentTime - m_lastPointerReplayTime;
        if (timeSinceLastReplay >= MIN_POINTER_REPLAY_PERIOD_IN_MS)
        {
            m_pBrowserHost->RequestReplayPreviousPointerUpdate();
            m_lastPointerReplayTime = currentTime;
            m_replayPointerUpdateAfterTick = false;
        }
        else
        {
            ITickableFrameScheduler *pScheduler = m_pBrowserHost->GetFrameScheduler();
            // The worst that can happen if this fails is the user actually has to move the pointer.
            VERIFYHR(pScheduler->RequestAdditionalFrame(static_cast<UINT32>(MIN_POINTER_REPLAY_PERIOD_IN_MS - timeSinceLastReplay), RequestFrameReason::ReplayPointerUpdate));
        }
    }

Cleanup:
    if (pLayoutManager)
    {
        pLayoutManager->SetAllowTransitionsToRunUnderCurrentSubtree(TRUE);
    }

    m_pDrawReentrancyCheck = this; // set to non-null so we can call draw again

    // NOTE: If the change to convert TraceFrameInfo is ported to OS, then rev the event instead
    //       of changing the data format.
    TraceFrameInfo(*pFrameDrawn);
    TraceFrameEnd();

    if (m_pendingFirstFrameTraceLoggingEvent)
    {
        const auto traceSession = Instrumentation::GetXamlTraceSession();
        TraceLoggingWrite(
            g_hTraceProvider,
            "FirstUiThreadFrameEnd",
            TraceLoggingValue(traceSession->GetSessionId(), "XamlSessionId"),
            TraceLoggingValue(traceSession->GetAppId(), "XamlAppId"),
            TraceLoggingValue(traceSession->GetProcessId(), "XamlProcessId"),
            TraceLoggingLevel(WINEVENT_LEVEL_LOG_ALWAYS),
            TelemetryPrivacyDataTag(PDT_ProductAndServicePerformance),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        m_pendingFirstFrameTraceLoggingEvent = false;
    }

    SetVisibilityToggled(FALSE); // clear the flag, any processing based on it should be complete by now.

    for (XUINT32 i = 0; i < directManipulationViewports.size(); i++)
    {
        ReleaseInterface(directManipulationViewports[i]);
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Private helper to check if there is a SwapChainBackgroundPanel
//      element as root visual or child of a Page root visual.
//
//------------------------------------------------------------------------
_Ret_maybenull_ CSwapChainBackgroundPanel*
CCoreServices::FindSCBP() const
{
    CSwapChainBackgroundPanel *pSCBP = NULL;
    CUIElement *pPublicRootVisual = m_pMainVisualTree->GetPublicRootVisual();

    // Check if SCBP is root visual.
    if (pPublicRootVisual)
    {
        pSCBP = do_pointer_cast<CSwapChainBackgroundPanel>(pPublicRootVisual);
        if (pSCBP == NULL)
        {
            // Check if SCBP is a child of root visual. We cannot validate that the root visual type must be a Page element here,
            // since Jupiter types are opaque to core, so that constraint is enforced in dxaml layer.

            CUIElement **ppChildren = NULL;
            XUINT32 childCount = 0;
            pPublicRootVisual->GetChildrenInRenderOrder(&ppChildren, &childCount);

            if (childCount > 0)
            {
                CUIElement *pChild = ppChildren[0];
                pSCBP = do_pointer_cast<CSwapChainBackgroundPanel>(pChild);

                if (pSCBP != NULL)
                {
                    // SCBP can be the only child of Page element.
                    ASSERT(childCount == 1);
                }
            }
        }
    }

    return pSCBP;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Private helper to NWDrawTree, performs primitive composition
//      render walk on the given visual root.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::RenderWalk(
    _In_ HWWalk *pHWWalk,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ CUIElement *pVisualRoot,
    bool forceRedraw,
    bool hasUIAClientsListeningToStructure,
    _Out_ bool* canSubmitFrame)
{
    ASSERT(m_decodeRequests.Count() == 0);

    *canSubmitFrame = true;

    TraceRenderWalkBegin();

    pHWWalk->ResetEtwData();

    // Ensure that redirected elements are marked dirty before the render walk, if necessary.
    // This only needs to happen when a render walk is going to occur - if there were independent
    // property only, then we don't want to invalidate the redirection elements and force a render walk.
    InvalidateRedirectionElements();

    CSwapChainBackgroundPanel *pSCBPNoRef = FindSCBP();
    if (pSCBPNoRef != NULL)
    {
        IFC_RETURN(pSCBPNoRef->PreRender());
    }

    // Ensure any selection grippers have updated their visibility now that all animations for the frame have
    // started or stopped.
    IFC_RETURN(UpdateGripperVisibility());

    // Remove all temporary comp nodes from the previous frame before rendering this one. These temporary comp nodes were
    // created for redirected rendering, when we needed a reference for SetTransformParent2 and encountered a UIElement that
    // should have a comp node but doesn't have one yet. We'll create more temporary nodes this frame as needed.
    //
    // Note that we can't do this cleanup as part of RenderRoot, because in the case of XamlIslands we'll call RenderRoot
    // multiple times. We don't want island2 to remove all the temporary nodes that island1 just inserted.
    IFC_RETURN(pRenderTarget->GetCompositorTreeHost()->RemoveTemporaryNodes());

    float scale = 1.0f;
    if (const auto root = GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        scale = RootScale::GetRasterizationScaleForContentRoot(root);
    }

    // Render all the island roots
    if (HasXamlIslands())
    {
        DCompTreeHost* dcompTreeHost = pRenderTarget->GetDCompTreeHost();

        xref_ptr<WindowsPresentTarget> savedTarget = pRenderTarget->GetPresentTarget();

        auto restoreTargetOnScopeExit = wil::scope_exit([&]
        {
            // Restore the original render target when leaving the current scope
            pRenderTarget->Retarget(savedTarget);
        });

        bool cleanIslandsRenderFlags = true;

        for (auto& islandData : dcompTreeHost->GetXamlIslandRenderData())
        {
            CXamlIslandRoot* xamlIslandRoot = islandData.first;
            ASSERT(xamlIslandRoot);

            WindowsPresentTarget* newWindowsPresentTarget = islandData.second.windowsPresentTarget;
            if (newWindowsPresentTarget && xamlIslandRoot->GetVisualTreeNoRef() != nullptr)
            {
                pRenderTarget->Retarget(newWindowsPresentTarget); // returns void

                // During test cleanup we can have XamlIslandRoot that are still alive but have no content.
                // Don't try to render them.
                if (xamlIslandRoot->IsActive())
                {
                    IFC_RETURN(pHWWalk->RenderRoot(
                        xamlIslandRoot,
                        pRenderTarget,
                        forceRedraw,
                        scale,
                        hasUIAClientsListeningToStructure));

                    //
                    // After a render walk, the entire tree should be clean. If it isn't, that means we encountered
                    // some error on some element during the render walk, and we want to try again next frame. If
                    // that's the case, then that element currently has no content, so we don't want to submit the
                    // frame. Otherwise we'll end up with a flicker - on the previous frame that element had content,
                    // on this frame the element released its content and can't generate new content, and on the next
                    // frame the element will have new content.
                    //
                    // We also don't want to infinitely skip submitting frames if an element repeatedly fails, so we
                    // mark when we've skipped submitting a frame and force the next one through regardless.
                    //
                    // We can avoid this complicated mechanism with smarter recycling of an element's content visuals.
                    // See Task 26308130: <Render walk should better tolerate errors - don't release old content until new content is ready>
                    //
                    if (xamlIslandRoot->NWNeedsRendering())
                    {
                        if (!m_skippedSubmittingFrameDueToRenderingError)
                        {
                            // We're not submitting this frame because there's another one coming up.
                            auto pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
                            if (pFrameScheduler != NULL)
                            {
                                FAIL_FAST_ASSERT(pFrameScheduler->HasRequestedAdditionalFrame());
                            }

                            *canSubmitFrame = false;
                        }

                        cleanIslandsRenderFlags = false;
                    }
                }
            }
        }

        if (GetInitializationType() == InitializationType::IslandsOnly &&
            cleanIslandsRenderFlags)
        {
            // In island-only scenario we don't visit root visual and therefore it never has its dirty flags cleared.
            // Walk down the tree clearing dirty flags along the way.  This is ok as long as all islands were rendered
            // and none of other children of root visual are used.
            pVisualRoot->PropagateNWClean();
        }
    }

    if (GetInitializationType() != InitializationType::IslandsOnly)
    {
        IFC_RETURN(pHWWalk->RenderRoot(
            pVisualRoot,
            pRenderTarget,
            forceRedraw,
            scale,
            hasUIAClientsListeningToStructure));

        //
        // After a render walk, the entire tree should be clean. If it isn't, that means we encountered
        // some error on some element during the render walk, and we want to try again next frame. If
        // that's the case, then that element currently has no content, so we don't want to submit the
        // frame. Otherwise we'll end up with a flicker - on the previous frame that element had content,
        // on this frame the element released its content and can't generate new content, and on the next
        // frame the element will have new content.
        //
        // We also don't want to infinitely skip submitting frames if an element repeatedly fails, so we
        // mark when we've skipped submitting a frame and force the next one through regardless.
        //
        // We can avoid this complicated mechanism with smarter recycling of an element's content visuals.
        // See Task 26308130: <Render walk should better tolerate errors - don't release old content until new content is ready>
        //
        if (pVisualRoot->NWNeedsRendering() && !m_skippedSubmittingFrameDueToRenderingError)
        {
            // We're not submitting this frame because there's another one coming up.
            auto pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler != NULL)
            {
                FAIL_FAST_ASSERT(pFrameScheduler->HasRequestedAdditionalFrame());
            }

            *canSubmitFrame = false;
        }
    }

    TraceRenderWalkEnd1(pHWWalk->GetElementsVisited(), pHWWalk->GetElementsRendered());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Private helper to NWDrawTree, submits primitive composition draw commands to the HW compositor.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SubmitPrimitiveCompositionCommands(
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_opt_ CUIElement *pVisualRoot,
    _In_opt_ IPALClock *pClock,
    _In_ XFLOAT rFrameStartTime)
{
    const auto& compositorScheduler = m_pBrowserHost->GetCompositorScheduler();
    const auto& dcompTreeHost = pRenderTarget->GetDCompTreeHost();
    const auto& graphicsDevice = pRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();

    CTimeManager* timeManagerNoRef = GetTimeManager();

    CTimeManager* pTimeManager = GetTimeManager();
    ASSERT(pTimeManager != NULL);

    IFC_RETURN(dcompTreeHost->PreCommitMainDevice());

    //
    // Synchronize the UI thread with the independent thread.
    //
    // This is required to snap up-to-date property values committed on the UI thread, and to prevent the independent
    // thread from committing newer values until the UI thread commits. Failure to do either of these tasks makes it
    // possible for the UI thread to commit stale values for independently-animating properties, causing them to glitch
    // by jumping backwards to value from an earlier time.
    //
    {
        // Scope the lock.
        auto lock = compositorScheduler->GetDrawListsLock();

        TraceCompositorLockBegin();

        // NOTE: This should happen as late as possible before submitting the frame, so we can account for as much
        //           of the UI thread frame cost as we can when adjusting animation start times.

        // Give connected animations a chance to take snapshots and do some final tweaks.
        if (m_connectedAnimationService)
        {
            IFC_RETURN(m_connectedAnimationService->PreCommit());
        }

        IFC_RETURN(SubmitRenderCommandsToCompositor(
            pVisualRoot != nullptr ? pVisualRoot->GetCompositionPeer() : nullptr,
            pTimeManager));

        // Push any ThemeShadow updates to WUC
        if (!CThemeShadow::IsDropShadowMode())
        {
            dcompTreeHost->GetProjectedShadowManager()->UpdateShadowScenes(this);
        }

        // RenderTargetBitmap needs an opportunity to run before commit to run any composition operations like CaptureAsync
        // on visuals that were created.
        // This must run after SubmitRenderCommandsToCompositor since it needs to query properties on the visual that it will
        // be capturing.  These properties may be updated in SubmitRenderCommandsToCompositor first.
        IFC_RETURN(m_pRenderTargetBitmapManager->PreCommit(pRenderTarget));

        // Now that we've rendered and walked the UIElement and compositor trees, we've started any WUC animations that are
        // attached to the tree. There can still be WUC animations that aren't attached to the tree (e.g. their targets are
        // in Visibility="Collapsed" subtrees). Attach them and start them explicitly, so that we get notifications of WUC
        // animation completed. We also want them started in case their subtrees become visible while the animation is supposed
        // to be in progress.
        if (timeManagerNoRef != nullptr)
        {
            timeManagerNoRef->UpdateTargetDOs(m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor());
        }

        // Submit the render state changed command under draw lock so that it's picked up in the same frame as the
        // other work just submitted to the render thread.
        if (m_renderStateChanged && m_isRenderEnabled)
        {
            IFC_RETURN(compositorScheduler->OnRenderStateChanged(TRUE));
        }

        TraceCompositorLockEnd();
    }

    // DComp animations have been committed. Clear the flag for "new independent animations started this frame".
    pTimeManager->ResetIndependentTimelinesChanged();

    // Wake the independent thread since the UI thread submitted work.
    IFC_RETURN(compositorScheduler->WakeCompositionThread());

    // Commit the main device outside the lock. It is possible that the content of the frame and its animations will
    // get out of sync. The render thread can start ticking animations before the content changes are picked up.
    // However, the main device Commit() of the content changes can take a long time (> 1 vBlank) to return.
    // If the UI thread holds the lock this entire time, it prevents the render thread from ticking any active
    // animations or manipulations. It is more preferable to allow for de-synchronization at this point than to glitch
    // the render thread for the duration.
    IFC_RETURN(dcompTreeHost->CommitMainDevice());

    // Ignore any errors that get returned. This call goes through DwmSetWindowAttribute, which does a IsShellManagedWindow
    // check on the hwnd. LogonUI runs outside the shell and will fail the check, and this call will return E_INVALIDARG.
    // We should ignore that error and continue.
    IGNOREHR(dcompTreeHost->UnfreezeDWMSnapshotIfFrozen());

    // Post-commit ConnectedAnimationService needs to remove all the element it has retained.
    if (m_connectedAnimationService != nullptr)
    {
        IFC_RETURN(m_connectedAnimationService->PostCommit());
    }

    if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::TraceDCompSurfaces) ||
        runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::TraceDCompSurfaceUpdates))
    {
        IGNOREHR(DCompSurfaceMonitor::DumpSurfacesDrawn(m_uFrameNumber));
    }

    // Call LayoutCompleted() to notify the system that we've committed an updated frame.
    if (m_fLayoutCompletedNeeded)
    {
        IFC_RETURN(dcompTreeHost->NotifyUWPWindowLayoutComplete(*pRenderTarget));
    }

    // Handle debug settings.
    if (m_pHostSite != NULL)
    {
        const bool isFrameRateCounterEnabled = m_pHostSite->GetEnableFrameRateCounter();
        if (m_debugSettingsChanged)
        {
            IFC_RETURN(dcompTreeHost->UpdateDebugSettings(isFrameRateCounterEnabled));
        }

        if (isFrameRateCounterEnabled)
        {
            // The frame rate counter visual is inserted as a sibling of the root visual, so it won't have the zoom scale applied.
            // It can use the real width of the window.
            dcompTreeHost->UpdateUIThreadCounters(
                static_cast<XUINT32>(XcpRound(GetBrowserHost()->get_CurrFrameRate())),
                (pClock != NULL)
                    ? 1000.0f * (static_cast<XFLOAT>(pClock->GetAbsoluteTimeInSeconds()) - rFrameStartTime)
                    : 0.0f);
        }

        // TODO: JCOMP: This logic can all move above the normal frame Commit() once this branch picks up the
        // TODO: JCOMP: change where the Commit moved outside the lock. Safer to do this work outside
        // TODO: JCOMP: the lock and Commit again, until the changes merge.
        if (m_debugSettingsChanged || isFrameRateCounterEnabled)
        {
            IFC_RETURN(dcompTreeHost->CommitMainDevice());
        }
    }

    // Give the graphics device a chance to clean up unused memory allocations.
    if (graphicsDevice != nullptr)
    {
        graphicsDevice->TrimMemory();
    }

    m_fLayoutCompletedNeeded = FALSE;

    // Only update the frame counter if we actually drew something
    m_uFrameNumber++;

    // Finally make sure the time manager is unlocked. In normal apps we don't want to unlock the time manager right
    // away, since we'll be getting frames while the splash screen is still up. The StartApplication event will tell
    // us when the splash screen drops, at which point we mark a flag and use it to unlock the time manager. In the
    // designer there's no splash screen, so we can unlock the time manager right away.
    if ((m_isFirstFrameAfterAppStart || DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
        && m_pTimeManager->IsLocked())
    {
        m_pTimeManager->UnlockTime(); // Start running animations

        auto pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::UnlockTime));
        }
    }

    if (m_isFirstFrameAfterAppStart)
    {
        // Download requests during start-up are added to a queue and delayed until the very end
        // of the first frame.  This prevents the background worker threads from contending with the
        // UI thread until the first frame has been submitted.
        IFC_RETURN(ProcessDownloadRequests(FALSE));

        IGNOREHR(m_pHostSite->OnFirstFrameDrawn());

        m_isFirstFrameAfterAppStart = false;
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Submits render commands to the given compositor.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SubmitRenderCommandsToCompositor(
    _In_opt_ HWCompTreeNode *pRootCompNode,
    _In_ CTimeManager *pTimeManager)
{
    // The root comp node is not created when in background tasks or from a test that has already cleaned up the visual tree.
    // Hence do not submit the comp tree in such cases.
    DCompTreeHost* dcompTreeHost = m_pNWWindowRenderTarget->GetDCompTreeHost();
    if (pRootCompNode != NULL)
    {
        IFC_RETURN(pRootCompNode->UpdateTreeRoot(dcompTreeHost, true /* useDCompAnimations */));

        IFC_RETURN(dcompTreeHost->SetRoot(static_cast<HWCompTreeNodeWinRT*>(pRootCompNode)));
    }

    // For each tree, ensure that commands are sent to the compositor.
    // Note that the composition trees for each XamlIsland are not connected
    // to each other or the root, so commands for each disconnected sub-tree
    // must be sent to the compositor separately.
    for (auto& islandData : dcompTreeHost->GetXamlIslandRenderData())
    {
        auto xamlIslandRoot = islandData.first;

        if(xamlIslandRoot && xamlIslandRoot->GetVisualTreeNoRef() != nullptr)
        {
            auto compPeer = xamlIslandRoot->GetCompositionPeer();
            if (compPeer)
            {
                IFC_RETURN(compPeer->UpdateTreeRoot(dcompTreeHost, true /* useDCompAnimations */));
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a compositor scheduler.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CreateCompositorScheduler(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _Outptr_ CompositorScheduler** compositorScheduler)
{
    xref_ptr<CompositorScheduler> pCompositorScheduler;

    IFC_RETURN(CompositorScheduler::Create(
        pGraphicsDeviceManager,
        pCompositorScheduler.ReleaseAndGetAddressOf()));

    *compositorScheduler = pCompositorScheduler.detach();

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a UI-thread frame scheduler.
//
//------------------------------------------------------------------------------
void CCoreServices::CreateUIThreadScheduler(
    _In_ IXcpDispatcher *pIDispatcher,
    _In_ CompositorScheduler* compositorScheduler,
    _Outptr_ ITickableFrameScheduler **ppUIThreadScheduler)
{
    UIThreadScheduler *pFrameScheduler = NULL;  // TODO: delete ITickableFrameScheduler

    UIThreadScheduler::Create(
        pIDispatcher,
        compositorScheduler,
        &pFrameScheduler);

    *ppUIThreadScheduler = pFrameScheduler; // pass ownership of the object to the caller
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a render target that will manage the host's rendering area.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CreateWindowRenderTarget(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _In_ CompositorScheduler* compositorScheduler,
    _In_ WindowsPresentTarget *initialPresentTarget)
{
    xref_ptr<CWindowRenderTarget> pWindowRenderTarget;

    IFC_RETURN(CWindowRenderTarget::Create(
        this,
        TRUE /* isPrimaryWindowTarget */,
        compositorScheduler,
        pGraphicsDeviceManager,
        initialPresentTarget,
        pWindowRenderTarget.ReleaseAndGetAddressOf()));

    m_pNWWindowRenderTarget = pWindowRenderTarget.detach();

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Register an object  for suspend/resume notification.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RegisterPLMListener(
    _In_ IPLMListener *pPLMListener
    )
{
#if DBG
    // Check for adding listener when it has already been added.
    PLMListenerVector::const_iterator end = m_plmListeners.end();
    for (PLMListenerVector::const_iterator it = m_plmListeners.begin(); it != end; ++it)
    {
        if (*it == pPLMListener)
        {
            ASSERT(FALSE);
            break;
        }
    }
#endif
    RRETURN(m_plmListeners.push_back(pPLMListener));
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Unregister an object for suspend/resume notification.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UnregisterPLMListener(
    _In_ IPLMListener *pPLMListener
    )
{
    PLMListenerVector::const_iterator end = m_plmListeners.end();
    for (PLMListenerVector::const_iterator it = m_plmListeners.begin(); it != end; ++it)
    {
        if (*it == pPLMListener)
        {
            IFC_RETURN(m_plmListeners.erase(it));
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for suspending the application.
//
//      Flag isTriggeredByResourceTimer identifies whether this is triggered
//      by a true suspend or the firing of the Xaml background resource timer.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::OnSuspend(bool isTriggeredByResourceTimer, bool allowOfferResources)
{
    HRESULT hr = S_OK;
    bool wasSuspended = m_isSuspended;

    m_currentSuspendReason = isTriggeredByResourceTimer ? SuspendReason::WindowHiddenTimeout : SuspendReason::PLMSuspend;

    if (!m_isSuspended)
    {
        PLMListenerVector::const_iterator end = m_plmListeners.end();
        for (PLMListenerVector::const_iterator it = m_plmListeners.begin(); it != end; ++it)
        {
            IFC((*it)->OnSuspend(isTriggeredByResourceTimer));
        }

        IFC(PauseDCompAnimationsOnSuspend());

        // The notification behavior of the IHM (Input Pane) is somewhat odd.  If we suspend and the input
        // pane is hidden, we don't ever get notified that it is gone.  That means when we are brought back
        // (resumed) we still think the IHM is occluding part of our window so we don't paint it.  A partial
        // and possible temporary solution is to simply assume that the IHM was hidden when we suspended.  This
        // will prevent the 'holes' in the rendering, but if the input pane is up, the user may (probably)
        // won't be able to pan.  However, they will most likely close the IHM to see what is under it and
        // when the reopen it, all will be back to normal.
        // Note that even though we don't care about putting the focused element into view, we still need to
        // pass true for that parameter, otherwise the InputPaneHandler won't actually expand the content back
        // to the original size.
        for (const xref_ptr<CContentRoot>& contentRoot: m_contentRootCoordinator.GetContentRoots())
        {
            CInputPaneHandler* pInputPaneHandlerNoRef = static_cast<CInputPaneHandler*>(contentRoot->GetInputManager().GetInputPaneHandler());
            if (pInputPaneHandlerNoRef && pInputPaneHandlerNoRef->IsInputPaneShowed())
            {
                pInputPaneHandlerNoRef->Hiding(true/*ensureFocusedElementInView*/);
            }
        }

        static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ReleaseGraphicsAndDCompDevicesOnSuspend))
        {
            IFC(ReleaseDeviceResources(true /* releaseDCompDevice */, false /* isDeviceLost */));
        }
        else if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::ReleaseGraphicsDeviceOnSuspend))
        {
            IFC(ReleaseDeviceResources(false /* releaseDCompDevice */, false /* isDeviceLost */));
        }

        m_isSuspended = TRUE;
        TraceCCoreServicesOnSuspendInfo(isTriggeredByResourceTimer);
    }

    if (m_pBackgroundResourceTimer != NULL)
    {
        IFC(m_pBackgroundResourceTimer->Stop());
        TraceCCoreServicesStopSoftSuspendTimerInfo();
    }

    if (m_pNWWindowRenderTarget != NULL)
    {
        IFC(DisconnectRoot(allowOfferResources, !!wasSuspended));
    }

    if (m_attachMemoryManagerEvents)
    {
        AttachMemoryManagerEvents();

        // Since we have already suspended, check memory.
        if (m_memoryManager)
        {
            IFC(CheckMemoryUsage(false /* simulateLowMemory */));
        }
    }


Cleanup:
    if (GraphicsUtility::IsDeviceLostError(hr))
    {
        if (m_deviceLost == DeviceLostState::None)
        {
            m_deviceLost = DeviceLostState::HardwareOnly;
        }
        hr = S_OK;
    }
    RRETURN(hr);
}

_Check_return_ HRESULT
CCoreServices::DisconnectRoot(bool allowOfferResources, bool wasSuspended)
{
    DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
    if (!wasSuspended &&
        pDCompTreeHostNoRef != NULL &&
        m_deviceLost == DeviceLostState::None)
    {
        if (m_forceDisconnectRootOnSuspend)
        {
            // DCOMP hides offered surfaces. These stay hidden
            // until the first commit post-resume. DWM starts presenting
            // whatever is in the visual tree immediately after resume.
            // This may result in momentary partial content (due to the hidden surfaces).
            //
            // Hence we disconnect the DComp root visual from the hwnd target
            // during suspend and we will reconnect it along with the first
            // commit post resume.
            //
            // We do it before the Offer, because Offer already implicitly commits
            // DComp and it will convey the disconnection request too.
            XSIZE_LOGICAL layoutSize = {};
            GetWindowSize(GetMainRootVisual(), nullptr, &layoutSize);
            XUINT32 uLayoutWidth = layoutSize.Width;
            XUINT32 uLayoutHeight = layoutSize.Height;
            {
                XRECTF layoutRect = { 0, 0, static_cast<float>(uLayoutWidth), static_cast<float>(uLayoutHeight) };

                IFC_RETURN(pDCompTreeHostNoRef->DisconnectRoot(m_spTheming->GetRootVisualBackground(), layoutRect));
            }
        }

        if (allowOfferResources)
        {
            IFC_RETURN(pDCompTreeHostNoRef->OfferResources());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Event handler for resuming the application.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::OnResume()
{
    HRESULT hr = S_OK;
    bool wasSuspended = m_isSuspended;

    m_currentSuspendReason = SuspendReason::NotSuspended;

    if (m_isSuspended)
    {
        IFC(ResumeDCompAnimationsOnResume());

        m_isSuspended = FALSE;
        TraceCCoreServicesOnResumeInfo();

        // Ensure another frame is scheduled to re-draw on Resume.
        ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::AfterResume));
        }
    }

    if (m_pBackgroundResourceTimer != NULL)
    {
        IFC(m_pBackgroundResourceTimer->Stop());
        TraceCCoreServicesStopSoftSuspendTimerInfo();
    }

    if (wasSuspended)
    {
        IFC(ReclaimResources());
    }

    if (!m_plmListeners.empty())
    {
        for (PLMListenerVector::const_iterator it = m_plmListeners.begin(); it != m_plmListeners.end(); ++it)
        {
            IFC((*it)->OnResume());
        }
    }

Cleanup:
    if (GraphicsUtility::IsDeviceLostError(hr))
    {
        if (m_deviceLost == DeviceLostState::None)
        {
            m_deviceLost = DeviceLostState::HardwareOnly;
        }
        hr = S_OK;
    }
    RRETURN(hr);
}

void CCoreServices::OnLowMemory()
{
    for (auto it = m_plmListeners.begin(); it != m_plmListeners.end(); ++it)
    {
        (*it)->OnLowMemory();
    }
}

_Check_return_ HRESULT CCoreServices::ReclaimResources()
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
        if (pDCompTreeHostNoRef != NULL)
        {
            bool discarded = false;
            IFC_RETURN(pDCompTreeHostNoRef->ReclaimResources(&discarded));
            if (discarded && m_deviceLost == DeviceLostState::None)
            {
                IFC_RETURN(CheckForLostSurfaceContent());

                if (m_fSurfaceContentsLostEnqueued)
                {
                    HandleVirtualSurfaceImageSourceLostResources();
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::PauseDCompAnimationsOnSuspend()
{
    GetTimeManager()->PauseDCompAnimationsOnSuspend();

    ASSERT(m_pNWWindowRenderTarget != nullptr);
    DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDCompTreeHostNoRef->CommitMainDevice());

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::ResumeDCompAnimationsOnResume()
{
    GetTimeManager()->ResumeDCompAnimationsOnResume();

    ASSERT(m_pNWWindowRenderTarget != nullptr);
    DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
    IFC_RETURN_DEVICE_LOST_OTHERWISE_FAIL_FAST(pDCompTreeHostNoRef->CommitMainDevice());

    return S_OK;
}

void CCoreServices::SetIsSuspended(bool isSuspended)
{
    m_isSuspended= isSuspended;
}

void CCoreServices::SetIsRenderEnabled(bool value)
{
    m_isWindowVisible = value;
    LogCoreServicesEvent(CoreServicesEvent::TestHook);
    if (value)
    {
        LogCoreServicesEvent(CoreServicesEvent::WindowShown);
    }
    else
    {
        LogCoreServicesEvent(CoreServicesEvent::WindowHidden);
    }

    VERIFYHR(EnableRender(value));
}

//------------------------------------------------------------------------
//
//  Method: CCoreServices::ExecuteOnUIThread
//
//  Synopsis:
//      Schedule an executor for running on the UI thread
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ExecuteOnUIThread(_In_ IPALExecuteOnUIThread *pExecutor, const ReentrancyBehavior reentrancyBehavior)
{
    IFCEXPECT_RETURN(m_pBrowserHost);

    IFC_RETURN(m_pBrowserHost->FireExecuteOnUIThreadMessage(pExecutor, reentrancyBehavior));

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::UpdateColorAndBrushResources(bool* pWasFirstCreateForResources)
{
    auto resourceInfoList = m_spTheming->GetColorAndBrushResourceInfoList();

    if (!m_pSystemColorsResources)
    {
        IFC_RETURN(CResourceDictionary::CreateSystemColors(this, &m_pSystemColorsResources));
        *pWasFirstCreateForResources = true;
    }
    else
    {
        *pWasFirstCreateForResources = false;

        // Clear existing values.
        IFC_RETURN(m_pSystemColorsResources->Clear());
    }

    // Add new values.
    for (size_t i = 0; i < resourceInfoList.size(); ++i)
    {
        CValue valColorRaw;
        valColorRaw.SetColor(resourceInfoList[i].OverrideAlpha ? (0xFF000000 | resourceInfoList[i].rgbValue) : resourceInfoList[i].rgbValue);

        // Create color.
        if (!resourceInfoList[i].ColorKey.IsNullOrEmpty())
        {
            CValue valColor;
            xref_ptr<CDependencyObject> spColor;

            CREATEPARAMETERS cpColor(this, valColorRaw);
            IFC_RETURN(CColor::Create(spColor.ReleaseAndGetAddressOf(), &cpColor));
            valColor.WrapObjectNoRef(spColor);

            // Add color to the dictionary.
            IFC_RETURN(m_pSystemColorsResources->Add(resourceInfoList[i].ColorKey, &valColor, nullptr /* pResult */, FALSE /* bKeyIsType */));
        }

        // Create brush.
        if (!resourceInfoList[i].BrushKey.IsNullOrEmpty())
        {
            CValue valBrush;
            xref_ptr<CDependencyObject> spBrush;

            CREATEPARAMETERS cpBrush(this, valColorRaw);
            IFC_RETURN(CSolidColorBrush::Create(spBrush.ReleaseAndGetAddressOf(), &cpBrush));
            valBrush.WrapObjectNoRef(spBrush);

            // Add brush to the dictionary.
            IFC_RETURN(m_pSystemColorsResources->Add(resourceInfoList[i].BrushKey, &valBrush, nullptr /* pResult */, FALSE /* bKeyIsType */));
        }
    }

    IFC_RETURN(AddFocusVisualKindIsRevealToSystemColorsResources());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: AddFocusVisualKindIsRevealToSystemColorsResources
//
//  Synopsis:
//      SystemColorsResources is a dictionary of calculated color and brush
//      resources. There are additional resources that we calculate and populate
//      into that dictionary. Do that now. This is done in response to theme
//      change (when color and brush updates clear the dictionary and repopulate
//      it) or when the FocusVisualKind changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::AddFocusVisualKindIsRevealToSystemColorsResources()
{
    if (m_pSystemColorsResources)
    {
        // Add or update the IsApplicationFocusVisualKindReveal system resource key.
        DECLARE_CONST_XSTRING_PTR_STORAGE(c_IsApplicationFocusVisualKindRevealKey, L"IsApplicationFocusVisualKindReveal");

        CValue rawBoolValue;
        rawBoolValue.Set<valueBool>(m_isApplicationFocusVisualKindReveal);

        xref_ptr<CDependencyObject> boolDO;
        CREATEPARAMETERS createParameters(this, rawBoolValue);
        IFC_RETURN(CBoolean::Create(boolDO.ReleaseAndGetAddressOf(), &createParameters));

        CValue isFocusVisualKindRevealValue;
        isFocusVisualKindRevealValue.WrapObjectNoRef(boolDO);

        IFC_RETURN(m_pSystemColorsResources->Remove(
            XSTRING_PTR_FROM_STORAGE(c_IsApplicationFocusVisualKindRevealKey)));
        IFC_RETURN(m_pSystemColorsResources->Add(
            XSTRING_PTR_FROM_STORAGE(c_IsApplicationFocusVisualKindRevealKey),
            &isFocusVisualKindRevealValue,
            nullptr /* pResult */,
            FALSE /* bKeyIsType */));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: SetFocusVisualKindIsReveal
//
//  Synopsis:
//      Lets FrameworkApplication notify us that the FocusVisualKind changed
//      so that we can update the system resources.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetFocusVisualKindIsReveal(bool isReveal)
{
    if (m_isApplicationFocusVisualKindReveal != isReveal)
    {
        m_isApplicationFocusVisualKindReveal = isReveal;

        IFC_RETURN(AddFocusVisualKindIsRevealToSystemColorsResources());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: GetSystemColorsResources
//
//  Synopsis:
//      Returns the theme resource dictionary with system/immersive colors.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::GetSystemColorsResources(_Out_ CResourceDictionary** ppSystemColorsResources)
{
    HRESULT            hr   = S_OK;
    CDependencyObject *pIDO = nullptr;

    IFCPTR(ppSystemColorsResources);

    if (!m_strSystemColorsResourcesXaml.IsNullOrEmpty() && m_pSystemColorsResources == nullptr)
    {
        IFC(GetBrowserHost()->CreateFromXaml(
            m_strSystemColorsResourcesXaml.GetCount(),
            m_strSystemColorsResourcesXaml.GetBuffer(),
            true, // bCreateNamescope
            true, // bRequiresDefaultNamespace
            false, // bExpandTemplatesDuringParse
            &pIDO));
        if (pIDO->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
        {
            m_pSystemColorsResources = reinterpret_cast<CResourceDictionary *>(pIDO);
            // We need to remove the AddRef on the core so that we don't have
            // a circular reference on ourselves
            m_pSystemColorsResources->ContextRelease();
            pIDO = nullptr;
        }
    }

    // m_pSystemColorsResources is cleared between test runs, so it might be null.
    // If so, we should repopulate it to ensure that it always has a value.
    if (m_pSystemColorsResources == nullptr)
    {
        bool ignored = false;
        IFC(UpdateColorAndBrushResources(&ignored));
    }

Cleanup:
    if (ppSystemColorsResources != nullptr)
    {
        *ppSystemColorsResources = m_pSystemColorsResources;
    }

    ReleaseInterface(pIDO);

    RRETURN(hr);
}

void CCoreServices::SetThemeResources(_In_opt_ CResourceDictionary *pThemeResources)
{
    if (m_pThemeResources)
    {
        m_pThemeResources->UnpegManagedPeer();
    }

    ReplaceInterface(m_pThemeResources, pThemeResources);
    if (m_pThemeResources)
    {
        // Parsing the ThemeResources dictionary in the DXaml layer caused a ContextAddRef(),
        // so we need to call ContextRelease() here.
        m_pThemeResources->ContextRelease();

        // TODO: Find a better way to protect this resource dictionary.
        IGNOREHR(m_pThemeResources->PegManagedPeer());

        // The ResourceDictionary peer will get a PegNoRef when created through StyleCache::LoadThemeResources
        // which needs to be taken off to prevent it from leaking when ThemeDictionary is reset.
        // Usually the PegNoRef is taken off when it becomes part of the visual tree or an RCW is connected to it,
        // but in this case the ThemeDictionary is protected by an actual PegManagedPeer at which point the
        // the PegNoRef can be removed.
        m_pThemeResources->UnpegManagedPeerNoRef();

        m_pThemeResources->MarkAllIsGlobal();
    }

    // Ensure that the default text and selection brushes get invalidated.
    ResetThemedBrushes();

    if (m_pTextCore != NULL)
    {
        m_pTextCore->ClearDefaultTextFormatting();
    }
}

//------------------------------------------------------------------------
//
//  Method:   NotifyThemeChange
//
//  Synopsis:
//      Notifies the entire visual tree that the theme has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::NotifyThemeChange()
{
    auto endOnExit = WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46076120>() ?
        m_themeWalkResourceCache->BeginCachingThemeResources() :
        m_themeWalkResourceCache->BeginThemeWalk();

    bool wasFirstCreateForResources = false;
    IFC_RETURN(UpdateColorAndBrushResources(&wasFirstCreateForResources));

    // We skip the rest of the theme notification if we just created
    // our color and brush resources
    if (wasFirstCreateForResources)
    {
        return S_OK;
    }

    ResetThemedBrushes();

    if (m_pTextCore != nullptr)
    {
        m_pTextCore->ClearDefaultTextFormatting();
    }

    xref_ptr<IPALResourceManager> spResourceManager;
    IFC_RETURN(GetResourceManager(spResourceManager.ReleaseAndGetAddressOf()));
    IFC_RETURN(spResourceManager->NotifyThemeChanged());

    // If already switching theme, then we don't need to re-notify of theme change
    if (m_fIsSwitchingTheme)
    {
        return S_OK;
    }

    if (m_pMainVisualTree)
    {
        IFC_RETURN(m_pMainVisualTree->GetRootVisual()->SetBackgroundColor(m_spTheming->GetRootVisualBackground()));
    }

    m_fIsSwitchingTheme = true;
    auto resetSwitchingThemeAfterChange = wil::scope_exit([this]() { m_fIsSwitchingTheme = false; });
    m_hasThemeEverChanged = true;

    // Notify global theme resources since they are not part of the tree.
    auto pGlobalThemeResourcesNoRef = GetThemeResources();
    if (pGlobalThemeResourcesNoRef)
    {
        IFC_RETURN(pGlobalThemeResourcesNoRef->NotifyThemeChanged(m_spTheming->GetTheme(), true /*fForceRefresh*/));
    }

    // Notify app resources since they are not part of the tree.
    auto pAppResourcesNoRef = GetApplicationResourceDictionary();
    if (pAppResourcesNoRef)
    {
        IFC_RETURN(pAppResourcesNoRef->NotifyThemeChanged(m_spTheming->GetTheme(), true /*fForceRefresh*/));
    }

    // Notify open popups of theme change.
    auto pPopupRootNoRef = GetMainPopupRoot();
    if (pPopupRootNoRef)
    {
        IFC_RETURN(pPopupRootNoRef->NotifyThemeChanged(m_spTheming->GetTheme(), true /*fForceRefresh*/));
    }

    auto root = getVisualRoot();
    if (root)
    {
        // Although this is the root, it can inherit default property values
        // (specifically, Text foreground). Freeze this inheritance, to pick up
        // values corresponding to the new theme, instead of the default values.
        auto rootAsFE = do_pointer_cast<CFrameworkElement>(root);
        if (rootAsFE)
        {
            IFC_RETURN(rootAsFE->NotifyThemeChangedForInheritedProperties(
                m_spTheming->GetTheme(),
                true /* freezeInheritedPropertiesAtSubTreeRoot */));
        }

        // Notify visual tree root of theme change.
        IFC_RETURN(root->NotifyThemeChanged(m_spTheming->GetTheme(), true /*fForceRefresh*/));
    }

    auto xamlIslandRootCollection = m_pMainVisualTree ? m_pMainVisualTree->GetXamlIslandRootCollection() : nullptr;
    if (xamlIslandRootCollection)
    {
        IFC_RETURN(xamlIslandRootCollection->NotifyThemeChangedForInheritedProperties(
            m_spTheming->GetTheme(),
            true /* freezeInheritedPropertiesAtSubTreeRoot */));
        IFC_RETURN(xamlIslandRootCollection->NotifyThemeChanged(m_spTheming->GetTheme(), true /*fForceRefresh*/));
    }

    // Notify registered theme change listeners.
    for(auto& item : m_elementsWithThemeChangedListener)
    {
        item.first->NotifyThemeChangedListeners(m_spTheming->GetTheme());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called to request that an element's theme be used during resource
//      lookup.
//
//------------------------------------------------------------------------
void
CCoreServices::SetRequestedThemeForSubTree(_In_ Theming::Theme requestedTheme)
{
    m_requestedThemeForSubTree = Theming::GetBaseValue(requestedTheme);

    m_themeWalkResourceCache->SetSubTreeTheme(m_requestedThemeForSubTree);
}

bool
CCoreServices::IsThemeRequestedForSubTree() const
{
    return Theming::GetBaseValue(m_requestedThemeForSubTree) != Theming::Theme::None;
}

_Ret_maybenull_ HWTextureManager*
CCoreServices::GetHWTextureManagerNoRef()
{
    return GetHWWalk()->GetTextureManager();
}

uint32_t CCoreServices::GetMaxTextureSize()
{
    WindowsGraphicsDeviceManager *deviceMgr = GetBrowserHost()->GetGraphicsDeviceManager();

    // We may not have completed creating the DComp device.
    HRESULT hr = deviceMgr->WaitForD3DDependentResourceCreation();
    if (FAILED(hr))
    {
        if (GraphicsUtility::IsDeviceLostError(hr))
        {
            // If the device was lost, don't bother plumbing this error up through every caller,
            // as another device lost error is inevitable.  Just return a hard-coded value
            // of 2045 (2048 - 3 pixels of padding for gutters/white pixel, see DCompTreeHost::GetMaxTextureSize()).
            return 2045;
        }
        else
        {
            IFCFAILFAST(hr);
        }
    }

    return NWGetWindowRenderTarget()->GetDCompTreeHost()->GetMaxTextureSize();
}

MaxTextureSizeProvider& CCoreServices::GetMaxTextureSizeProvider()
{
    return m_maxTextureSizeProvider;
}

bool CCoreServices::AtlasRequest(uint32_t width, uint32_t height, PixelFormat pixelFormat)
{
    return FxCallbacks::Window_AtlasRequest(width, height, pixelFormat);
}

AtlasRequestProvider& CCoreServices::GetAtlasRequestProvider()
{
    return m_atlasRequestProvider;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the HWWalk object used for the primitive composition walk
//      from the CWindowRenderTarget, if it's attached. Otherwise return
//      NULL.
//
//------------------------------------------------------------------------
_Out_opt_ HWWalk*
CCoreServices::GetHWWalk()
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        return m_pNWWindowRenderTarget->GetHwWalk();
    }
    return NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the SurfaceCache object associated with the HWWalk object.
//      TODO: Merge the hardware surface cache into the ImageProvider.
//
//------------------------------------------------------------------------
_Ret_maybenull_ SurfaceCache*
CCoreServices::GetSurfaceCache()
{
    HWWalk* pHWWalk = GetHWWalk();
    if (pHWWalk)
    {
        return pHWWalk->GetSurfaceCache();
    }
    return NULL;
}

//------------------------------------------------------------------------
//
//  Method:   IsObjectAnActiveRootVisual
//
//  Synopsis:
//          Determine whether the object is the root visual of one of
//          the active VisualTree objects
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::IsObjectAnActiveRootVisual(_In_ CDependencyObject *pObject, _Out_ bool *pIsActiveRoot)
{
    bool isActiveRoot = false;

    IFCPTR_NOTRACE_RETURN(pObject);
    IFCPTR_RETURN(pIsActiveRoot);

    isActiveRoot = pObject == GetMainRootVisual();

    *pIsActiveRoot = isActiveRoot;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsObjectAnActivePublicRootVisual
//
//  Synopsis:
//          Determine whether the object is the public root visual of one of
//          the active VisualTree objects.
//          The root ScrollViewer will be active root if it is available on
//          the visual tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::IsObjectAnActivePublicRootVisual(_In_ CDependencyObject *pObject, _Out_ bool *pIsActiveRoot)
{
    bool isActiveRoot = false;
    CDependencyObject *pRootScrollViewer = NULL;

    IFCPTR_RETURN(pObject);
    IFCPTR_RETURN(pIsActiveRoot);

    // Compare with the root SV that is the parent of visual root if root SV is available on the visual tree
    pRootScrollViewer = getRootScrollViewer();
    if (pRootScrollViewer)
    {
        isActiveRoot = pObject == pRootScrollViewer;
    }
    else
    {
        isActiveRoot = pObject == getVisualRoot();
    }

    *pIsActiveRoot = isActiveRoot;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   IsObjectAnActivePopupRoot
//
//  Synopsis:
//          Determine whether the object is the popup root of one of
//          the active VisualTree objects
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::IsObjectAnActivePopupRoot(_In_ CDependencyObject *pObject, _Out_ bool *pIsActivePopupRoot)
{
    bool isActivePopupRoot = false;

    IFCPTR_RETURN(pObject);
    IFCPTR_RETURN(pIsActivePopupRoot);

    isActivePopupRoot = pObject == GetMainPopupRoot();

    if (!isActivePopupRoot && HasXamlIslands())
    {
        CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();
        CDOCollection* collection = xamlIslandRootCollection->GetChildren();
        if (collection != nullptr)
        {
            for (CDependencyObject* child : *collection)
            {
                CXamlIslandRoot* island = do_pointer_cast<CXamlIslandRoot>(child);

                if (pObject == island->GetPopupRootNoRef())
                {
                    isActivePopupRoot = true;
                    break;
                }
            }
        }
    }

    *pIsActivePopupRoot = isActivePopupRoot;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   StartApplication
//
//  Synopsis:
//          Start an application
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::StartApplication(_In_ CApplication *pApplication)
{
    HRESULT hr = S_OK;
    CUIElement *pPublicRootVisual = NULL;
    IXcpBrowserHost *pBrowserHost = GetBrowserHost();
    bool fNeedToUnpegRootVisual = false;

    IFCEXPECT_ASSERT(pBrowserHost);

    // Set the public root visual if we have it.
    if (m_pMainVisualTree && pApplication->m_fRootVisualSet)
    {
        // NOTE CAREFULLY: pApplication->m_pRootVisual is allowed to be null,
        // this handles the case where app code is re-setting a window's content to nothing.

        CDependencyObject *pParent = NULL;

        pPublicRootVisual = pApplication->m_pRootVisual;
        if (pPublicRootVisual && pPublicRootVisual->HasManagedPeer())
        {
            // Need to keep the root visual managed peer alive while we re-parent it
            IFC(pPublicRootVisual->PegManagedPeer());
            fNeedToUnpegRootVisual = TRUE;
        }

        // TODO: How is it okay to just remove an element's parent?
        pParent = pPublicRootVisual ? pPublicRootVisual->GetParentInternal(false) : NULL;
        if (pParent)
        {
            IFC(pPublicRootVisual->RemoveParent(pParent));
        }

        // As we're resetting the tree, we'll crash due to reentrancy guard if we fire a synchronous
        // event to release pointer capture.  Avoid this by clearing out input state first.  We only
        // need to do this when content is going away (i.e., the GetPublicRootVisual returns null)
        // This was found in watson, bug 7249381
        if (m_inputServices && m_pMainVisualTree->GetPublicRootVisual() != nullptr)
        {
            m_inputServices->DestroyPointerObjects();
        }

        IFC(m_pMainVisualTree->SetPublicRootVisual(
            pPublicRootVisual,
            pApplication->m_pRootScrollViewer,
            pApplication->m_pRootContentPresenter
            ));

        pApplication->SetIsStandardNameScopeOwner(FALSE);

        // Tell the event manager to raise the loaded event on the next tick.
        IFC(RaisePendingLoadedRequests());

        // Reset the holding flag so that the next tick also renders the first frame.
        m_isMainTreeLoading = FALSE;

        pBrowserHost->FireApplicationLoadComplete();
        IFC(SetRootVisualResetEventSignaledStatus(TRUE));

        StartMemoryTracking();

        // Mark that the next frame drawn will be the first frame for the new content.
        m_isFirstFrameAfterAppStart = true;
    }

Cleanup:

    if (fNeedToUnpegRootVisual)
    {
        pPublicRootVisual->UnpegManagedPeer();
    }

    if (FAILED(hr))
    {
        IErrorService *pErrorService = NULL;
        VERIFYHR(getErrorService(&pErrorService));

        if ((pErrorService) && (pBrowserHost))
        {
            VERIFYHR(pErrorService->ReportGenericError(hr, InitializeError, AG_E_INIT_ROOTVISUAL, 0, 0, 0, NULL, 0));
            pBrowserHost->ReportError();
        }

    }

    ReleaseInterface(pApplication->m_pRootVisual);
    pApplication->m_fRootVisualSet = FALSE;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   RaisePendingLoadedRequests
//
//  Synopsis:
//          Tell the event manager to process its queue of Loaded event
//          requests.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::RaisePendingLoadedRequests()
{
    if (GetEventManager())
    {
        IFC_RETURN(GetEventManager()->RequestRaiseLoadedEventOnNextTick());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//          Checks if any of the SurfaceImageSources lost their hardware resources
//          and queues SurfaceContentsLost event accordingly.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::CheckForLostSurfaceContent()
{
    CXcpList<CSurfaceImageSource>::XCPListNode *pCurrent = NULL;
    bool enqueueSurfaceContentsLostEvent = false;

    IFC_RETURN(m_pRenderTargetBitmapManager->CheckForLostSurfaceContent());
    enqueueSurfaceContentsLostEvent = m_pRenderTargetBitmapManager->NeedsSurfaceContentsLost();

    if (!enqueueSurfaceContentsLostEvent &&
        m_pAllSurfaceImageSources != NULL)
    {
        pCurrent = m_pAllSurfaceImageSources->GetHead();
        while (pCurrent)
        {
            if(pCurrent->m_pData->CheckForLostHardwareResources())
            {
                enqueueSurfaceContentsLostEvent = TRUE;
                break;
            }
            pCurrent = pCurrent->m_pNext;
        }
    }

    if (enqueueSurfaceContentsLostEvent)
    {
        //
        // Enqueue the SurfaceContentsLost event which will eventually notify the application that
        // it must regenerate its surface contents.
        //
        IFC_RETURN(EnqueueSurfaceContentsLostEvent());
    }

    m_pRenderTargetBitmapManager->ResetNeedsSurfaceContentsLost();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//          Checks if any of the VirtualSurfaceImageSources lost their hardware resources
//          and handles accordingly.
//
//------------------------------------------------------------------------
void
CCoreServices::HandleVirtualSurfaceImageSourceLostResources()
{
    CXcpList<CVirtualSurfaceImageSource>::XCPListNode *pCurrent = NULL;

    if (m_pAllVirtualSurfaceImageSources != NULL)
    {
        pCurrent = m_pAllVirtualSurfaceImageSources->GetHead();

        while (pCurrent)
        {
            pCurrent->m_pData->HandleLostResources();
            pCurrent = pCurrent->m_pNext;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      if we aren't currently rendering, check to see if we have exceeded our
//      memory limit.  If so, release our graphics resources.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::CheckMemoryUsage(bool simulateLowMemory)
{
    // The callback may still get called after the event handler was detached so do an extra null check
    bool overLimit = false;
    if (!m_isRenderEnabled && m_memoryManager != nullptr)
    {
        wsy::AppMemoryUsageLevel usageLevel;
        Microsoft::WRL::ComPtr<wsy::IMemoryManagerStatics2> memoryManager2;
        Microsoft::WRL::ComPtr<wsy::IAppMemoryReport> memoryReport;
        UINT64 memoryLimit;
        UINT64 memoryUsage;

        IFC_RETURN(m_memoryManager->get_AppMemoryUsageLevel(&usageLevel));
        IFC_RETURN(m_memoryManager.As(&memoryManager2));
        IFC_RETURN(memoryManager2->GetAppMemoryReport(&memoryReport));
        IFC_RETURN(memoryReport->get_TotalCommitUsage(&memoryUsage));
        IFC_RETURN(memoryReport->get_TotalCommitLimit(&memoryLimit));

        if ((usageLevel == wsy::AppMemoryUsageLevel_OverLimit) ||
            (memoryUsage >= memoryLimit))
        {
            overLimit = true;
        }
    }

    if (simulateLowMemory || overLimit)
    {
        // We're using too much memory.  Drop all device-dependent resources.
        IFC_RETURN(ReleaseDeviceResources(false /* releaseDCompDevice */, true /* isDeviceLost */));

        // release the unused textFormatters to reduce memory usage
        if (m_pTextCore != NULL)
        {
            m_pTextCore->ReleaseUnusedTextFormatters();
        }

        OnLowMemory();
    }

    return S_OK;
}

// Dispatcher mechanism used to enqueue request to query D3D device for lost state.
// Used from device lost listener (see EnsureDeviceLostListener()).
class DeviceLostDispatcher : public CXcpObjectBase<IPALExecuteOnUIThread>
{
public:
    DeviceLostDispatcher(_In_ CCoreServices* core)
    : m_core(core)
    {
    }

    _Check_return_ HRESULT Execute() override
    {
        return m_core->DetermineDeviceLost(nullptr);
    }

private:
    CCoreServices* m_core;
};

_Check_return_ HRESULT CCoreServices::EnsureDeviceLostListener()
{
    // Register ourselves with the D3D device to receive a callback when the device is lost.
    // This mechanism is proactive rather than requiring polling on the device and is required
    // on OS SKUs where no WM_PAINT message is sent to the framework from DWM (WCOS).
    if (m_deviceLostWaiter == nullptr)
    {
        m_deviceLostEvent.reset(::CreateEvent(
            nullptr, // no security descriptor
            FALSE,   // not a manual reset event
            FALSE,   // initially unsignaled
            nullptr  // no name
            ));
        IFCW32FAILFAST(m_deviceLostEvent != nullptr);

        // Note:  It's assumed that WaitForD3DDependentResourceCreation() was called prior to getting here.
        CD3D11Device *device = m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();
        IFC_RETURN(device->RegisterDeviceRemovedEvent(m_deviceLostEvent.get(), &m_deviceLostEventCookie));

        m_deviceLostWaiter.reset(CreateThreadpoolWait([](PTP_CALLBACK_INSTANCE, void *context, TP_WAIT *, TP_WAIT_RESULT)
        {
            auto core = static_cast<CCoreServices*>(context);
            xref_ptr<DeviceLostDispatcher> deviceLostDispatcher;
            deviceLostDispatcher.init(new DeviceLostDispatcher(core));
            core->ExecuteOnUIThread(deviceLostDispatcher, ReentrancyBehavior::CrashOnReentrancy);
        }, this, nullptr));
        IFCW32FAILFAST(m_deviceLostWaiter != nullptr);
        SetThreadpoolWait(m_deviceLostWaiter.get(), m_deviceLostEvent.get(), nullptr /*timeout*/);
    }

    return S_OK;
}

void CCoreServices::ReleaseDeviceLostListener()
{
    if (m_deviceLostWaiter != nullptr)
    {
        CD3D11Device *device = m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetGraphicsDevice();
        if (device != nullptr)
        {
            device->UnregisterDeviceRemoved(m_deviceLostEventCookie);
        }
        m_deviceLostWaiter.reset();
        m_deviceLostEvent.reset();
        m_deviceLostEventCookie = 0;
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks for recoverable errors and requests another frame to handle recovery.
//
//------------------------------------------------------------------------
void
CCoreServices::HandleDeviceLost(_Inout_ HRESULT *pHRESULT)
{
    // Make sure this is a device lost error
    if (!GraphicsUtility::IsDeviceLostError(*pHRESULT)) return;

    // Bug 32531355: [Watson Failure] caused by STOWED_EXCEPTION_88990015_Windows.UI.Xaml.dll!DCompTreeHost::PreCommitMainDevice
    // We're seeing failures with D2DERR_WRONG_RESOURCE_DOMAIN where we're rendering text with a foreground brush
    // for the wrong D2D device context. This looks like the result of incorrect device lost recovery. Rather than
    // tolerate the error and trying to recover again, we're trying to find and fix the place that put us in a mismatched
    // state to begin with.
    if (*pHRESULT == D2DERR_WRONG_RESOURCE_DOMAIN)
    {
        LogCoreServicesEvent(CoreServicesEvent::D2DWrongTargetError);
    }

    if (m_deviceLost == DeviceLostState::None)
    {
        m_deviceLost = DeviceLostState::HardwareOnly;

        // If we get an internal driver error we want to try to recover it, but there are situations
        // where this might not be possible, such as if the driver is bad and constantly returns
        // the error.
        if (*pHRESULT == DXGI_ERROR_DRIVER_INTERNAL_ERROR || *pHRESULT == DXGI_ERROR_DEVICE_HUNG)
        {
            if (!GetBrowserHost()->GetGraphicsDeviceManager()->CanContinueOnInternalDriverError())
            {
                // we have already done everthing we can to work around an internal driver error, with no
                // luck, we so will just return without updating the HRESULT and crash
                return;
            }
        }
    }

    // If we are rendering then schedule a tick to give us a chance to recover the device.
    if (m_isRenderEnabled)
    {
        ITickableFrameScheduler* pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            VERIFYHR(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::DeviceLost));
        }
    }

    // Reset the error to prevent a fail-fast, since this error will be recoverable on the next frame.
    *pHRESULT = S_OK;
}

_Check_return_ HRESULT CCoreServices::RecoverFromDeviceLost()
{
    if (m_deviceLost != DeviceLostState::None)
    {
        // Remove pending requests and data.
        // Note that we intentionally do not clear out the m_trackedImages collection on device lost, as
        // ImageSource's will only add themselves to this collection once, just after retrieving their natural size,
        // and thus will not automatically re-request for RenderWalk tracking after a device lost.
        m_decodeRequests.Clear();
        ClearPendingImageUpdates();

        // Release existing resources (unless they have previously been released)
        if (m_deviceLost != DeviceLostState::HardwareReleased)
        {
            const bool cleanupDComp = m_deviceLost == DeviceLostState::HardwareAndVisuals;
            CleanupDeviceRelatedResources(cleanupDComp, true /* isDeviceLost */);
            m_deviceLost = DeviceLostState::HardwareReleased;
            LogCoreServicesEvent(CoreServicesEvent::HardwareResourcesReleased);

            // The cleanupDComp is only specified when we reset everything between tests. It will cause the
            // DCompTreeHost to release its compositor, content bridge, and input site, which will tear down
            // the InProcInputHost and the master input thread. The next test needs to restart the MIT in order
            // for DManip to work, which is triggered by the initial DirectManipulationManager being activated.
            // We have a DMM in our input services for cross slide, which we need to explicitly deactivate and
            // release here, otherwise it will prevent the MIT from being restarted on the next test and will
            // break DManip for every test afterwards.
            //
            // Note that cleanupDComp is only set by tests (via DeviceLostState::HardwareAndVisuals). Normally
            // we do not release the compositor, content bridge, and the input site to recover from device lost,
            // so we don't need to worry about restarting the MIT, and the cross slide DMM does not need to be
            // explicitly cleaned up.
            if (cleanupDComp && m_inputServices)
            {
                m_inputServices->ResetCrossSlideService();
            }
        }

        // Rebuild the resources (but only if we are rendering)
        if (IsRenderingFrames())
        {
            IFC_RETURN(BuildDeviceRelatedResources());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleans up all the device related resources every where.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::CleanupDeviceRelatedResources(_In_ bool cleanupDComp, _In_ bool isDeviceLost)
{
    TraceReleaseGraphicsDeviceResourcesBegin(cleanupDComp, isDeviceLost);

    // Release our D3D device lost listener before moving on to releasing D3D device.
    ReleaseDeviceLostListener();

#if DBG
#define USE_RELEASE_ASSERTER
#endif

    auto guard = wil::scope_exit([&]()
    {
        TraceReleaseGraphicsDeviceResourcesEnd();
    });

#ifdef USE_RELEASE_ASSERTER
    xref_ptr<IPALDebugDeviceFinalReleaseAsserter> asserter;

    if (cleanupDComp && m_isDCompLeakDetectionEnabled)
    {
        IFC_RETURN(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->CreateFinalReleaseAsserter(asserter.ReleaseAndGetAddressOf()));
    }
#endif

    // Cleanup all the dictionaries on core.
    if (m_pDeploymentTree != nullptr)
    {
        m_pDeploymentTree->GetApplicationObjectForResourceLookup()->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_pThemeResources != nullptr)
    {
        m_pThemeResources->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_pSystemColorsResources != nullptr)
    {
        m_pSystemColorsResources->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    // Cleanup all the brushes on core.
    if (m_defaultTextBrush != nullptr)
    {
        m_defaultTextBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_textSelectionGripperFillBrush != nullptr)
    {
        m_textSelectionGripperFillBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_textSelectionBackgroundBrush != nullptr)
    {
        m_textSelectionBackgroundBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_textSelectionForegroundBrush != nullptr)
    {
        m_textSelectionForegroundBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_systemColorWindowBrush != nullptr)
    {
        m_systemColorWindowBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_systemColorWindowTextBrush != nullptr)
    {
        m_systemColorWindowTextBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_systemColorDisabledTextBrush != nullptr)
    {
        m_systemColorDisabledTextBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_systemColorHotlightBrush != nullptr)
    {
        m_systemColorHotlightBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_defaultFocusVisualSecondarySolidColorBrush != nullptr)
    {
        m_defaultFocusVisualSecondarySolidColorBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }
    if (m_defaultFocusVisualPrimarySolidColorBrush != nullptr)
    {
        m_defaultFocusVisualPrimarySolidColorBrush->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    // Release devices associated with our text core
    if (m_pTextCore)
    {
        m_pTextCore->GetWinTextCore()->ReleaseDeviceDependentResources();
    }

    m_pRenderTargetBitmapManager->CleanupDeviceRelatedResources(cleanupDComp);

    if (m_pAllSurfaceImageSources != nullptr)
    {
        CXcpList<CSurfaceImageSource>::XCPListNode *pCurrent = m_pAllSurfaceImageSources->GetHead();
        while (pCurrent)
        {
            pCurrent->m_pData->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
            pCurrent = pCurrent->m_pNext;
        }
    }

    CleanupDeviceRelatedResourcesOnSurfaceWrappers();

    for (auto it = m_deviceListeners.begin(); it != m_deviceListeners.end();)
    {
        VERIFYHR((*it++)->OnDeviceRemoved(cleanupDComp));
    }

    //
    // DX resources must be completely cleaned up before any recovery begins. A new DComp target
    // can't be created until the existing one is released.
    //

    // Animation targets can hold onto DComp resources, so clear them.
    if (m_pTimeManager != nullptr)
    {
        if (cleanupDComp)
        {
            // Processing targets outside the middle of a frame will clear all existing markings.
            // These are freed in CWindowRenderTarget::CleanupResources below.
            IFC_RETURN(m_pTimeManager->UpdateIATargets());

            // Notify the time manager that it's independent animations need to be cloned again, since all their
            // targets were freed.
            m_pTimeManager->NotifyIndependentAnimationChange();
        }

        m_pTimeManager->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    for (const auto& contentRoot: m_contentRootCoordinator.GetContentRoots())
    {
        contentRoot->GetFocusManagerNoRef()->CleanupDeviceRelatedResources(cleanupDComp);
    }

    if (cleanupDComp)
    {
        m_facadeStorage.CleanupCompositorResources();

        DXamlServices::GetCurrentJupiterWindow()->UninitializeInputSiteAdapterForCoreWindow();
    }

    // Recursively cleanup the entire tree. This also cleans up DComp related resources.
    CRootVisual *pRootVisualNoRef = GetMainRootVisual();
    if (pRootVisualNoRef != nullptr)
    {
        pRootVisualNoRef->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    }

    // Cleanup the primary render target.
    if (m_pNWWindowRenderTarget != nullptr)
    {
        IFC_RETURN(m_pNWWindowRenderTarget->CleanupResources(cleanupDComp, isDeviceLost));

        // Ensure debug settings will be updated if DComp resources are re-created.
        m_debugSettingsChanged = TRUE;
    }

    // If we are not cleaning up all the DComp objects and this isn't a device lost scenario (In other
    // (words, we are being requested to release our resources), then we need to call commit on the
    // DComp device to get it to give up all the stuff (surfaces) that we have released
    if (!cleanupDComp && !isDeviceLost)
    {
        IFC_RETURN(m_pNWWindowRenderTarget->GetDCompTreeHost()->CommitMainDevice());
    }

    // We need to make sure ICompositionEasingFunction is released from CConnectedAnimationService.
    // Otherwise, we'll continue to hold on to this function from the old DComp device when injecting/removing MockDComp.
    if (cleanupDComp && m_connectedAnimationService)
    {
        IFC_RETURN(m_connectedAnimationService->CancelAllAnimationsAndResetDefaults());
    }

#ifdef USE_RELEASE_ASSERTER
    if (asserter) asserter->ReleaseAllWithAssert();
#endif

    // Finish DCompTreeHost::ReleaseResources work. We delayed force closing and releasing the interop compositor
    // in order to give the DebugDeviceFinalReleaseAsserter a chance to do its leak verification.
    if (cleanupDComp)
    {
        m_pNWWindowRenderTarget->GetDCompTreeHost()->CloseAndReleaseInteropCompositor();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Rebuilds the minimum needed device related stuff so that
//      render walk could start. Rest of the stuff is rebuilt
//      by render walk as and when needed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::BuildDeviceRelatedResources()
{
    TraceRebuildGraphicsDeviceResourcesBegin();

    if ((m_pAllSurfaceImageSources != NULL &&
        m_pAllSurfaceImageSources->GetHead() != NULL) ||
        m_pRenderTargetBitmapManager->NeedsSurfaceContentsLost())
    {
        IFC_RETURN(EnqueueSurfaceContentsLostEvent());
        m_pRenderTargetBitmapManager->ResetNeedsSurfaceContentsLost();
    }

    //
    // Set state back to recovered. Do this before calling CWindowRenderTarget::RebuildResources.
    //
    // We do this to correctly handle errors that we encounter while we're trying to recover from a previous error.
    // If we're recovering from device lost, then the current state is HardwareReleased, which prevents us from
    // releasing resources in CCoreServices::RecoverFromDeviceLost. CWindowRenderTarget::RebuildResources is going
    // to create the D3D resources again, and if we hit errors during device creation then we must release the new
    // device and try again later. We reset the device lost state here so that we know to re-release the D3D
    // resources if something goes wrong during device creation. Otherwise, the device lost state stays as
    // HardwareReleased, and the new device stays around forever even though we hit errors during device creation.
    //
    m_deviceLost = DeviceLostState::None;

    //
    // Recover the main render target first.
    //
    // If there are no surface presenters, this is the only render target that exists.
    //
    // If there are surface presenters, then the scheduler assumes that the first registered compositor
    // is the primary compositor and gets refresh rate information from it. So it needs to be recover
    // before the surface presenter compositors recover.
    //
    IFC_RETURN(m_pNWWindowRenderTarget->RebuildResources());

    IFC_RETURN(GetBrowserHost()->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());

    m_deviceLost = DeviceLostState::None;

    // We reset the device lost state above to allow us to release the new resources if the recovery process hits
    // an error. We should still be able to release the new resources if needed.
    ASSERT(m_deviceLost != DeviceLostState::HardwareReleased);

    for (auto it = m_deviceListeners.begin(); it != m_deviceListeners.end();)
    {
        IFC_RETURN((*it++)->OnDeviceCreated());
    }

    LogCoreServicesEvent(CoreServicesEvent::HardwareResourcesRebuilt);

    TraceRebuildGraphicsDeviceResourcesEnd();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Method to check the state of DComp device.
//      In case the state is not valid, then record it
//      and request a frame. The new frame will do
//      the cleanup and recovery.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::DetermineDeviceLost(_Out_opt_ bool *pIsDeviceLost)
{
    if (m_deviceLost == DeviceLostState::None &&
        m_pNWWindowRenderTarget != NULL)
    {
        bool deviceValid = m_pNWWindowRenderTarget->GetDCompTreeHost()->CheckMainDeviceState();
        if (!deviceValid)
        {
            // DComp will return the state of the associated hardware device.
            m_deviceLost = DeviceLostState::HardwareOnly;
            ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::DeviceLost));
            }
        }
    }
    if (pIsDeviceLost != NULL)
    {
        *pIsDeviceLost = m_deviceLost != DeviceLostState::None;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Method to track the given surface wrapper for cleanup.
//      Such a tracking list is used to cleanup device related
//      resources on elements off the tree when device gets lost.
//
//------------------------------------------------------------------------
void
CCoreServices::RegisterSurfaceWrapperForDeviceCleanup(
    _In_ ImageSurfaceWrapper *pSurfaceWrapper
    )
{
    if (!IsDestroyingCoreServices())
    {
        m_imageSurfaceWrapperCleanupList.PushBack(pSurfaceWrapper->GetDeviceCleanupLink());
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Method to remove the entry from surface wrapper cleanup list.
//      This gets called when the element reenters the tree or when
//      the surface wrapper is about to be destroyed.
//
//------------------------------------------------------------------------
void
CCoreServices::UnregisterSurfaceWrapperForDeviceCleanup(
    _In_ ImageSurfaceWrapper *pSurfaceWrapper
    )
{
    m_imageSurfaceWrapperCleanupList.Remove(pSurfaceWrapper->GetDeviceCleanupLink());
}

void CCoreServices::AddDeviceListener(_In_ DeviceListener *deviceListener)
{
    ASSERT(m_deviceListeners.count(deviceListener) == 0);
    m_deviceListeners.insert(deviceListener);
}

void CCoreServices::RemoveDeviceListener(_In_ DeviceListener *deviceListener)
{
    ASSERT(m_deviceListeners.count(deviceListener) == 1);
    m_deviceListeners.erase(deviceListener);
}

void CCoreServices::AddDisplayListener(_In_ DisplayListener *displayListener)
{
    if (const auto root = GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
        {
            rootScale->AddDisplayListener(displayListener);
        }
    }
}

void CCoreServices::RemoveDisplayListener(_In_ DisplayListener *displayListener)
{
    if (const auto root = GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
        {
            rootScale->RemoveDisplayListener(displayListener);
        }
    }
}

void CCoreServices::AddRenderStateListener(_In_ RenderStateListener *renderStateListener)
{
    ASSERT(m_renderStateListeners.count(renderStateListener) == 0);
    m_renderStateListeners.insert(renderStateListener);
}

void CCoreServices::RemoveRenderStateListener(_In_ RenderStateListener *renderStateListener)
{
    ASSERT(m_renderStateListeners.count(renderStateListener) == 1);
    m_renderStateListeners.erase(renderStateListener);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to cleanup device related resources on
//      all the surface wrappers whose owners are off the tree.
//
//------------------------------------------------------------------------
void
CCoreServices::CleanupDeviceRelatedResourcesOnSurfaceWrappers()
{
    IntrusiveList<ImageSurfaceWrapper>::Iterator it =
        m_imageSurfaceWrapperCleanupList.Begin(ImageSurfaceWrapper::DeviceCleanupLinkOffset());
    while (it != m_imageSurfaceWrapperCleanupList.End())
    {
        ImageSurfaceWrapper *pSurfaceWrapper = *it;

        // Clean device code will unregister the surface wrapper and there by
        // removing it from the cleanup list. Hence get the next pointer before
        // calling cleanup.
        ++it;

        pSurfaceWrapper->CleanupDeviceRelatedResources();
    }
    ASSERT(m_imageSurfaceWrapperCleanupList.IsEmpty());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to cleanup device related resources on
//      all surface wrappers that are off the tree for a pre-determined
//      amount of time.
//
//      This operates similar to CD3D11Device::TrimMemory
//
//------------------------------------------------------------------------
void
CCoreServices::TrimMemoryOnSurfaceWrappers()
{
    const XUINT64 currentTime = gps->GetCPUMilliseconds();
    const XUINT64 releaseDelayMs = m_testOverrideImageSurfaceWrapperReleaseDelay ? 0 : sc_imageSurfaceWrapperReleaseDelayMilliseconds;

    IntrusiveList<ImageSurfaceWrapper>::Iterator it =
        m_imageSurfaceWrapperCleanupList.Begin(ImageSurfaceWrapper::DeviceCleanupLinkOffset());
    while (it != m_imageSurfaceWrapperCleanupList.End())
    {
        ImageSurfaceWrapper *pSurfaceWrapper = *it;

        // Clean device code will unregister the surface wrapper and there by
        // removing it from the cleanup list. Hence get the next pointer before
        // calling cleanup.
        ++it;

        // This was built to operate similar to CD3D11Device::TrimMemory
        if (((pSurfaceWrapper->GetDeviceCleanupTimestamp() + releaseDelayMs) < currentTime))
        {
            pSurfaceWrapper->CleanupDeviceRelatedResources();
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   InvalidateImplicitStylesOnRoots
//
//  Synopsis:
//          Invalidate implicit styles on each root visual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::InvalidateImplicitStylesOnRoots(_In_opt_ CResourceDictionary *pOldResources)
{
    IFC_RETURN(CFrameworkElement::InvalidateImplicitStyles(GetMainRootVisual(), pOldResources));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   UpdateImplicitStylesOnRoots
//
//  Synopsis:
//          Update implicit styles on each root
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UpdateImplicitStylesOnRoots(_In_opt_ CStyle *pOldStyle, _In_opt_ CStyle *pNewStyle, bool forceUpdate)
{
    IFC_RETURN(GetMainRootVisual()->UpdateImplicitStyle(pOldStyle, pNewStyle, forceUpdate));

    return S_OK;
}

_Check_return_
HRESULT CCoreServices::GetResourceManager(_Outptr_ IPALResourceManager **ppResourceManager)
{
    if (!m_pResourceManager)
    {
        IFC_RETURN(m_pHostSite->CreateResourceManager(&m_pResourceManager));
    }

    *ppResourceManager = m_pResourceManager;
    m_pResourceManager->AddRef();

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::ShimGetResourcePropertyBagRaw(
    _In_ const xstring_ptr& spKeyString,
    _In_ const xref_ptr<IPALUri>& spBaseUri,
    _Out_ std::vector<std::pair<xstring_ptr, xstring_ptr>>& propertyBag)
{
    // Use override if it's set.
    if (m_propertyBagOverride != nullptr)
    {
        propertyBag.clear();
        auto iter = m_propertyBagOverride->find(spKeyString);
        if (iter != m_propertyBagOverride->end())
        {
            propertyBag = *(iter->second);
        }
    }
    else
    {
        const IPALUri *pBaseUri = spBaseUri ? spBaseUri : GetBaseUriNoRef();
        xref_ptr<IPALResourceManager> resourceManager;
        IFC_RETURN(GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
        IFC_RETURN(resourceManager->GetPropertyBag(spKeyString, pBaseUri, propertyBag));
    }

    return S_OK;
}

// Used for testing. Overrides the resource property bag normally retrieved from the resource manager.
void CCoreServices::OverrideResourcePropertyBag(_In_opt_ std::map<std::wstring, std::vector<std::pair<std::wstring, std::wstring>>>* map)
{
    m_propertyBagOverride.reset();

    if (map != nullptr)
    {
        std::shared_ptr<std::map<xstring_ptr, std::shared_ptr<PropertyBag>>> newPropertyBagMap
            = std::make_shared<std::map<xstring_ptr, std::shared_ptr<PropertyBag>>>();

        for (const auto& mapEntry : *map)
        {
            std::shared_ptr<PropertyBag> newPropertyBag = std::make_shared<PropertyBag>();

            for (const auto& propEntry : mapEntry.second)
            {
                xstring_ptr propertyName;
                xstring_ptr propertyValue;
                IFCFAILFAST(xstring_ptr::CloneBuffer(propEntry.first.data(), &propertyName));
                IFCFAILFAST(xstring_ptr::CloneBuffer(propEntry.second.data(), &propertyValue));
                newPropertyBag->push_back(std::make_pair(propertyName, propertyValue));
            }

            xstring_ptr uid;
            IFCFAILFAST(xstring_ptr::CloneBuffer(mapEntry.first.data(), &uid));
            newPropertyBagMap->emplace(uid, newPropertyBag);
        }

        m_propertyBagOverride = newPropertyBagMap;
    }
}

_Check_return_ void CCoreServices::SetCustomResourceLoader(_In_ ICustomResourceLoader *pResourceLoader)
{
    ReleaseInterface(m_pCustomResourceLoader);
    m_pCustomResourceLoader = pResourceLoader;
    AddRefInterface(pResourceLoader);
}

_Check_return_ ICustomResourceLoader* CCoreServices::GetCustomResourceLoader()
{
    return m_pCustomResourceLoader;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get or create the imaging factory associated with the core.
//
//------------------------------------------------------------------------
IAsyncImageFactory* CCoreServices::GetImageFactory()
{
    if (!m_imageTaskDispatcher)
    {
        m_imageTaskDispatcher = make_xref<ImageTaskDispatcher>(this);
    }

    if (!m_imageFactory)
    {
        IFCFAILFAST(AsyncImageFactory::Create(m_pWorkItemFactory, m_imageTaskDispatcher, m_imageFactory.ReleaseAndGetAddressOf()));
    }

    return m_imageFactory.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Get or create the imaging provider associated with the core.
//
//------------------------------------------------------------------------
ImageProvider* CCoreServices::GetImageProvider()
{
    if (!m_imageTaskDispatcher)
    {
        m_imageTaskDispatcher = make_xref<ImageTaskDispatcher>(this);
    }

    if (!m_imageProvider)
    {
        IFCFAILFAST(ImageProvider::Create(this, m_imageTaskDispatcher, GetImageFactory(), m_imageProvider.ReleaseAndGetAddressOf()));
    }

    return m_imageProvider.get();
}

CResourceDictionary* CCoreServices::GetApplicationResourceDictionary()
{
    if (m_pDeploymentTree != NULL && m_pDeploymentTree->GetApplicationObjectForResourceLookup() != NULL)
    {
        return m_pDeploymentTree->GetApplicationObjectForResourceLookup()->m_pResources;
    }
    return NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CCoreServices::GetLastLayoutExceptionElement
//
//  Synopsis:
//          Gets the last element that threw an exception during layout,
//          regardless of which visual tree/layout manager it is associated
//          with.
//
//  Notes:
//          This is equivalent to the most recent element passed into
//          CLayoutManager::SetLastExceptionElement across all layout
//          managers.
//
//          If there is one visual tree (the standard Jupiter app case),
//          this is equivalent to:
//              GetMainLayoutManager()->GetLastExceptionElement().
//
//          Note that this value will be set to NULL upon a successful
//          layout pass on any visual tree.
//
//------------------------------------------------------------------------
CUIElement *CCoreServices::GetLastLayoutExceptionElement()
{
    return m_pLastLayoutExceptionElement;
}

//------------------------------------------------------------------------
//
//  Method:   CCoreServices::SetLastLayoutExceptionElement
//
//  Synopsis:
//          Sets the last element that threw an exception during layout,
//          regardless of which visual tree/layout manager it is associated
//          with.
//
//  Notes:
//          Called by CLayoutManager. Note that we don't care about
//          pegging the framework peer here since we know that the calling
//          layout manager has already done this if necessary.
//
//------------------------------------------------------------------------
void CCoreServices::SetLastLayoutExceptionElement(_In_opt_ CUIElement *pElement)
{
    ReplaceInterface(m_pLastLayoutExceptionElement, pElement);
}

//static
HINSTANCE CCoreServices::GetInstanceHandle()
{
    return ::GetModuleHandle(L"Microsoft.UI.Xaml.dll");
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Return the size of the window in DIP, There may be multiple windows like in design
//  mode.  In this case the root visual tree host associated with the object must be used for
//  the size of the window.
//
//------------------------------------------------------------------------
void CCoreServices::GetWindowSize(
    _In_opt_ CDependencyObject *pObject,
    _Out_opt_ XSIZE_PHYSICAL* physicalSize,
    _Out_opt_ XSIZE_LOGICAL* logicalSize)
{
    float width = 0.0f;
    float height = 0.0f;

    ASSERT(m_pNWWindowRenderTarget != NULL);

    width = static_cast<XFLOAT>(m_pNWWindowRenderTarget->GetWidth());
    height = static_cast<XFLOAT>(m_pNWWindowRenderTarget->GetHeight());

    if (physicalSize != nullptr)
    {
        // Don't bother dividing out the zoom scale for the physical size. We're going to use the physical size to calculate the
        // orientation transform on the phone, so it's important to be precise. We don't want to divide out the zoom scale, take
        // the ceiling, then multiply the zoom scale back. That could result in being 1 pixel off with the offset, which turns
        // off DWM optimizations and leaves a line of pixels on the screen.
        physicalSize->Width = XcpRound(width);  // We expect these to be integers anyway.
        physicalSize->Height = XcpRound(height);
    }

    if (logicalSize != nullptr)
    {
        const auto rootScale = RootScale::GetRootScaleForElementWithFallback(pObject);
        if (rootScale == nullptr)
        {
            logicalSize->Width = 0;
            logicalSize->Height = 0;
        }
        else
        {
            const auto scale = rootScale->GetEffectiveRasterizationScale();
            // Ensure the logical window size is always at least as large as the actual window size
            // when the plateau scale is applied back to it.
            logicalSize->Width = XcpCeiling(width / scale);
            logicalSize->Height = XcpCeiling(height / scale);
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates render state information.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::EnableRender(bool enable)
{
    if (enable != m_isRenderEnabled)
    {
        m_isRenderEnabled = enable;

        if (m_isRenderEnabled)
        {
            LogCoreServicesEvent(CoreServicesEvent::RenderEnabled);
        }
        else
        {
            LogCoreServicesEvent(CoreServicesEvent::RenderDisabled);
        }

        // Propagate render state change info to render target.
        // We do so only if the render is disabled; for enabled
        // we defer it until the frame is produced.
        // TODO: Consider an event if there are more listeners.
        if (m_pNWWindowRenderTarget && !enable)
        {
            const auto& compositorScheduler = m_pBrowserHost->GetCompositorScheduler();
            IFC_RETURN(compositorScheduler->OnRenderStateChanged(FALSE));
        }
        m_renderStateChanged = TRUE;
        if (enable)
        {
            for (auto it = m_renderStateListeners.begin(); it != m_renderStateListeners.end();)
            {
                IFC_RETURN((*it++)->OnRenderEnabled());
            }

            ITickableFrameScheduler *pFrameScheduler = GetBrowserHost()->GetFrameScheduler();
            if (pFrameScheduler != NULL)
            {
                IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::EnableRender))
            }
        }
        else if (m_attachMemoryManagerEvents)
        {
            AttachMemoryManagerEvents();

            // Do an extra check in case we missed a memory event in the past
            if (m_memoryManager != nullptr)
            {
                IFC_RETURN(CheckMemoryUsage(false /* simulateLowMemory */));
            }
        }
    }

    return S_OK;
}

void CCoreServices::LogCoreServicesEvent(CoreServicesEvent coreServicesEvent)
{
    m_coreServicesEventLog.Log( { m_uFrameNumber, coreServicesEvent } );
}

Diagnostics::ResourceLookupLogger* CCoreServices::GetResourceLookupLogger()
{
    if (!m_resourceLookupLogger)
    {
        m_resourceLookupLogger = std::make_unique<Diagnostics::ResourceLookupLogger>();
    }
    return m_resourceLookupLogger.get();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a flag that will cause us to call LayoutCompleted() when
//      committing the next frame.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::SetLayoutCompletedNeeded(const LayoutCompletedNeededReason reason)
{
    m_fLayoutCompletedNeeded = TRUE;

    switch (reason)
    {
        case LayoutCompletedNeededReason::WindowSizeChanged:
            LogCoreServicesEvent(CoreServicesEvent::LayoutCompletedNeeded_SizeChanged);
            break;
        case LayoutCompletedNeededReason::WindowMadeVisible:
            LogCoreServicesEvent(CoreServicesEvent::LayoutCompletedNeeded_Visible);
            break;
    }

    // Raise an ETW event to signal that we've requested a LayoutCompleted()
    // call on the next frame.
    XHANDLE hwnd = NULL;
    if (GetHostSite())
    {
        hwnd = GetHostSite()->GetXcpControlWindow();
    }
    TraceSubmitNotifyWindowLayoutCompletedInfo1((UINT64)hwnd);

    // LayoutCompleted() will be called on the next frame. We need to request a frame here
    // because there may be nothing else that will do so. For example, if the window
    // is currently up-to-date and becoming visible, and nothing is animating, nothing else
    // will cause us to produce a frame. If we don't produce a frame, we'll never call
    // LayoutCompleted(), and the window won't be un-ghosted.
    IXcpBrowserHost *pBH = GetBrowserHost();
    if (pBH != NULL)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != NULL)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::LayoutCompletedNeeded));
        }
    }

    return S_OK;
}

// Returns whether the window or an island is visible.
bool CCoreServices::IsXamlVisible() const
{
    if (m_forceWindowInvisible_TestHook)
    {
        return false;
    }
    else if (m_isWindowVisible)
    {
        return true;
    }

    if (HasXamlIslands())
    {
        // Check to see if we have a XamlIsland that has active UIA
        CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();

        CDOCollection* collection = xamlIslandRootCollection->GetChildren();
        if (collection)
        {
            for (CDependencyObject* child : *collection)
            {
                CXamlIslandRoot* island = do_pointer_cast<CXamlIslandRoot>(child);
                if (island->IsVisible())
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void CCoreServices::ForceXamlInvisible_TestHook(bool isVisible)
{
    m_forceWindowInvisible_TestHook = !isVisible;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates window visibility information.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CCoreServices::SetWindowVisibility(bool value, bool isStartingUp, bool freezeDWMSnapshotIfHidden)
{
    // In background tasks, behave as though the window is always visible.
    if (!IsInBackgroundTask() && m_isWindowVisible != value)
    {
        m_isWindowVisible = value;

        if (m_isWindowVisible)
        {
            LogCoreServicesEvent(CoreServicesEvent::WindowShown);
        }
        else
        {
            LogCoreServicesEvent(CoreServicesEvent::WindowHidden);
        }

        IFC_RETURN(OnVisibilityChanged(isStartingUp, freezeDWMSnapshotIfHidden));
    }

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::OnVisibilityChanged(bool isStartingUp, bool freezeDWMSnapshotIfHidden)
{
    if (!m_pNWWindowRenderTarget)
    {
        // The tree may have already been Reset, so bail out.
        return S_OK;
    }

    //
    // We're seeing cases of Explorer crashes where a Xaml island being torn down during DXamlCore deinitialization
    // calls into here via CXamlIslandRoot::Dispose. Xaml thinks it's still visible, so we call OnResume to notify
    // things that are listening. There's a LoadedImageSource listening, and it responds to OnResume by starting
    // to decode. It asks for the max texture size, but CCoreServices::GetMaxTextureSize AVs because the
    // WindowsGraphicsDeviceManager has already been released.
    //
    // The idea behind CXamlIslandRoot::Dispose calling this method is to check whether the last island was
    // released. In Explorer's case, m_isWindowVisible is marked true during startup and is never cleared, so we
    // think Xaml is always visible.
    //
    // We could have CCoreServices::GetMaxTextureSize explicitly check for a null WindowsGraphicsDeviceManager,
    // but that might devolve into an onion-peeling exercise where something else crashes due to the OnResume
    // notification. The better fix is to explicitly check for DXamlCore shutting down and no-op here.
    //
    // FrameworkCallbacks_IsDXamlCoreShuttingDown checks DXamlCore::IsShuttingDown(), which checks the DXamlCore's
    // m_state for Deinitializing. This state transition only happens during DXamlCore::DeinitializeInstance.
    // Notably, we don't set it during the recoverable DXamlCore::DeinitializeInstanceToIdle.
    //
    if (DirectUI::DXamlServices::IsDXamlCoreShuttingDown())
    {
        LogCoreServicesEvent(CoreServicesEvent::OnVisibilityChangedDuringShutdown);
    }
    else
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();

        const bool isXamlVisible = IsXamlVisible();

        TraceCCoreServicesSetWindowVisibilityInfo(isXamlVisible);

        if (isXamlVisible)
        {
            m_shouldUpdateRotationManagerAfterWindowMadeVisible = true;
            m_wasWindowEverMadeVisible = true;
        }

        // Always enable rendering when the window is visible.
        // When the window isn't visible, disable rendering unless
        // the app is starting up. In that case, we want to render
        // proactively to get the first first frame ready as early as possible.
        if (isXamlVisible || !isStartingUp)
        {
            EnableRender(isXamlVisible);
        }

        // Start timer to offer resources while the app is running in the background.
        if (!isXamlVisible)
        {
            // Freeze the DWM snapshot immediately on window invisibility.
            // Freezing at this time is more optimal than waiting for the resource timer,
            // as the DWM already takes a snapshot to animate the window out.  By freezing
            // now, the DWM can reuse this snapshot if it's picked up while the animation
            // is happening, thus avoiding having to take a 2nd snapshot.
            //
            // It is possible that the window has already been destroyed.
            // This happens when the app's suspend is timed such that it happens
            // while processing the WM_DESTROY. In such a case CoreApplicationView
            // processes the suspend before Deinitializing itself. But since the window
            // is already destroyed, taking its snapshot doesn't make sense.
            if (m_pNWWindowRenderTarget != nullptr && freezeDWMSnapshotIfHidden)
            {
                if (pDCompTreeHostNoRef != nullptr &&
                    GetHostSite() != nullptr &&
                    !GetHostSite()->IsWindowDestroyed())
                {
                    // Ignore any errors that get returned. This call goes through DwmSetWindowAttribute, which does a
                    // IsShellManagedWindow check on the hwnd. LogonUI runs outside the shell and will fail the check,
                    // and this call will return E_INVALIDARG. We should ignore that error and kick off the 2 second
                    // timeout for soft suspend, otherwise LogonUI (which doesn't get PLM suspended) will never suspend
                    // and will tick animations forever.
                    IGNOREHR(pDCompTreeHostNoRef->FreezeDWMSnapshot());
                }
            }

            // Ensure the background timer has been created and initialized
            IFC_RETURN(EnsureBackgroundTimer());

            // WINBLUE Bug #558370
            // Reader is drawing to a SIS very early on startup.  On low-end systems, it can
            // take more than 2 seconds until this first drawing happens.  Reader is not properly
            // detecting that the window is hidden in this scenario and is drawing after we
            // offer DComp resources, causing a failure and subsequent fail-fast.
            // This is a tactical fix for the Spring GDR:
            // Avoid setting the background timer when the app is starting up,
            // this has no practical downside as the app is launching and will need resources
            // very soon anyways.
            if (!isStartingUp)
            {
                IFC_RETURN(m_pBackgroundResourceTimer->Start());
                TraceCCoreServicesStartSoftSuspendTimerInfo();
            }
        }
        // If the app came back to the foreground, make sure the app doesn't remain suspended by the timer.
        else if (m_pBackgroundResourceTimer != nullptr)
        {
            IFC_RETURN(OnResume());
        }

        if (GetInitializationType() != InitializationType::IslandsOnly)
        {
            if (CContentRoot* coreWindowContentRoot = m_contentRootCoordinator.Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
            {
                coreWindowContentRoot->RaiseXamlRootChanged(CContentRoot::ChangeType::IsVisible);
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Event listener for background resource timer 'tick'.
//
//------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
CCoreServices::OnBackgroundResourceTimeout(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
    )
{
    CDispatcherTimer *pTimerNoRef = static_cast<CDispatcherTimer*>(pSender);

    // Since the event is raised asynchronously, check if the window became visible again after the
    // event was raised but before it was handled.
    if (!pTimerNoRef->GetContext()->IsXamlVisible() && g_backgroundResourceTimerEnabled )
    {
        // If the timer fired and the window is still in the background, treat the app like it suspended.
        // This will release expensive resources, but will allow the app logic to continue to run.
        IFC_RETURN(pTimerNoRef->GetContext()->OnSuspend(true /* isTriggeredByResourceTimer */, true /* allowOfferResources */));
    }

    IFC_RETURN(pTimerNoRef->WorkComplete());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Return if animation/touch tracking is enabled: defer to the DComp tree
//      host since it provides the means for tracking.
//
//------------------------------------------------------------------------
bool CCoreServices::IsAnimationTrackingEnabled()
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
        if (pDCompTreeHostNoRef != NULL)
        {
            return pDCompTreeHostNoRef->IsAnimationTrackingEnabled();
        }
    }
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the beginning of an animation scenario for animation tracking.
//
//------------------------------------------------------------------------
void CCoreServices::AnimationTrackingScenarioBegin(_In_ AnimationTrackingScenarioInfo* pScenarioInfo)
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        // It's possible that animations started during start-up might not be capture because the DComp
        // device initialization happens in parallel on a background thread. This would be most likely
        // to happen on a slow machine with a simple app that starts up very quickly. Since this case
        // is expected to be rare, we are OK with not being able to miss those for animation tracking.
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
        if (pDCompTreeHostNoRef != NULL)
        {
            // Fill in the QPC if one was not specified.
            if (!pScenarioInfo->qpcInitiate)
            {
                // If we are processing a frame, use the start time for that frame.
                if (m_qpcDrawMainTreeStart)
                {
                    pScenarioInfo->qpcInitiate = m_qpcDrawMainTreeStart;
                }
                else
                {
                    if (GetPALCoreServices())
                    {
                        XINT64_LARGE_INTEGER liQpc = {0};
                        GetPALCoreServices()->PerformanceCounter(&liQpc);
                        pScenarioInfo->qpcInitiate = liQpc.QuadPart;
                    }
                }
            }

            // Fill in the input QPC if the input manager has recorded one.
            if (m_inputServices)
            {
                pScenarioInfo->qpcInput = m_inputServices->GetFirstPointerUpQPCSinceLastFrame();
            }

            IGNOREHR(pDCompTreeHostNoRef->AnimationTrackingScenarioBegin(pScenarioInfo));
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the beginning of a sub-animation for animation tracking.
//
//------------------------------------------------------------------------
void CCoreServices::AnimationTrackingScenarioReference(XUINT64 uniqueKey)
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
        if (pDCompTreeHostNoRef != NULL)
        {
            IGNOREHR(pDCompTreeHostNoRef->AnimationTrackingScenarioReference(/*pScenarioGuid*/ nullptr, uniqueKey));
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the end of a sub-animation for animation tracking.
//
//------------------------------------------------------------------------
void CCoreServices::AnimationTrackingScenarioUnreference(XUINT64 uniqueKey)
{
    if (m_pNWWindowRenderTarget != NULL)
    {
        DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
        if (pDCompTreeHostNoRef != NULL)
        {
            IGNOREHR(pDCompTreeHostNoRef->AnimationTrackingScenarioUnreference(/*pScenarioGuid*/ nullptr, uniqueKey));
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Clear the image provider and surface caches.  Created so we can reset these
//      caches when the designer resets its tree.
//      TODO: Merge the hardware surface cache into the ImageProvider.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ClearMainRootImageCaches()
{
    if (m_imageProvider)
    {
        IFC_RETURN(m_imageProvider->Clear());
    }
    SurfaceCache *pSurfaceCache = GetSurfaceCache();
    if (pSurfaceCache != nullptr)
    {
        IFC_RETURN(pSurfaceCache->Clear());
    }
    if (const auto root = GetContentRootCoordinator()->Unsafe_XamlIslandsIncompatible_CoreWindowContentRoot())
    {
        if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))
        {
            auto& imageReloadManager = rootScale->GetImageReloadManager();
            imageReloadManager.ClearImages();
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Enqueue a request to decode an image to a specified size,
//      used by the DecodeToRenderSize feature.
//
//      We queue these requests as the act of making an image decode request
//      can have the side-effect of marking the XAML tree dirty for render.
//      This cannot be done from the render walk, so we enqueue the requests
//      and flush those requests after the render walk has completed
//      (see FlushImageDecodeRequests).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::EnqueueImageDecodeRequest(
    _In_ CImageSource* pImageSource,    // The CImageSource to make the request on
    XUINT32 width,                      // Desired decode width
    XUINT32 height,                     // Desired decode height
    bool retainPlaybackState
    )
{
    DecodeRequestEntry decodeRequestEntry;
    bool update = false;
    bool exists = false;

    // All images during the walk are processed, if there is a format change
    if (m_decodeRequests.Get(pImageSource, decodeRequestEntry) == S_OK)
    {
        exists = TRUE;

        // Check if there is a size difference and a new size needs to be decoded
        if ((width > decodeRequestEntry.width) ||
            (height > decodeRequestEntry.height))
        {
            decodeRequestEntry.width = MAX(width, decodeRequestEntry.width);
            decodeRequestEntry.height = MAX(height, decodeRequestEntry.height);

            update = TRUE;
        }
    }
    else
    {
        decodeRequestEntry.width = width;
        decodeRequestEntry.height = height;
        decodeRequestEntry.retainPlaybackState = retainPlaybackState;

        update = TRUE;
    }

    if (update)
    {
        if (exists)
        {
            DecodeRequestEntry dummyRequest;

            IFC_RETURN(m_decodeRequests.Remove(
                pImageSource,
                dummyRequest));
        }

        IFC_RETURN(m_decodeRequests.Add(
            pImageSource,
            decodeRequestEntry));
    }

    // Now that this request has been enqueued, we can stop tracking it
    // as the decode request is guaranteed to be made.
    IFC_RETURN(StopTrackingImageForRenderWalk(pImageSource));

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::RemoveImageDecodeRequest(
    _In_ CImageSource* pImageSource
    )
{
    DecodeRequestEntry dummy;

    return m_decodeRequests.Remove(pImageSource, dummy);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Flush all pending image decode requests.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::FlushImageDecodeRequests()
{
    HRESULT hr = S_OK;

    for (DecodeRequestMap::const_iterator it = m_decodeRequests.begin();
         it != m_decodeRequests.end();
         it++)
    {
        IFC(it->first->RequestDecode(it->second.width, it->second.height, it->second.retainPlaybackState));
    }

Cleanup:
    m_decodeRequests.Clear();
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Track this CImageSource as needing to request image decoding.
//      This collection tracks the bitmap images that don't get visited
//      during the render walk and still need to issue decode requests.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::StartTrackingImageForRenderWalk(_In_ CImageSource* pImageSource)
{
    bool dummy = false;

    // Do not track bitmap images when in background task.
    // This is because there is no live tree in background tasks
    // and render walks for RTBs  are expected to be skipped
    // in some frames without meaning that the bitmap images
    // should be decoded at natural size.
    if (!IsInBackgroundTask())
    {
        HRESULT hrAdd = m_trackedImages.Add(pImageSource, dummy);

        // We don't expect to add the same CImageSource to the collection more than once.
        // TODO: Disabled for further investigation after adding animated gif pipeline.
        //ASSERT(hrAdd != S_FALSE);
        IFC_RETURN(hrAdd);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Stop tracking this CImageSource as needing to request image decoding.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::StopTrackingImageForRenderWalk(_In_ CImageSource* pImageSource)
{
    bool dummy;

    IFC_RETURN(m_trackedImages.Remove(pImageSource, dummy));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Request image decodes for all bitmap images we're still tracking.
//      This tracking is needed because of a compatibility issue with
//      DecodeToRenderSize:  Before DecodeToRenderSize was introduced,
//      image decode requests would be initiated when the ImageSource enters
//      the live tree.  Some applications listen for the ImageOpened event
//      as a signal to make the image visible.  With DecodeToRenderSize
//      we require a render walk to the element to request decode.  But the
//      render walk culls out non-visible elements, so we won't walk to them
//      and thus won't request decoding, and thus won't fire the ImageOpened
//      event.  This causes the image to never be rendered.
//
//      The fix is to track all ImageSource's that request a render walk
//      due to wanting to initiate an image decode.  After the render walk
//      any items that weren't walked to will remain in this collection and
//      we will issue a decode request for them now.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ProcessTrackedImages()
{
    HRESULT hr = S_OK;

    for (xchainedmap<CImageSource*, bool>::const_iterator it = m_trackedImages.begin();
         it != m_trackedImages.end();
         it++)
    {
        CImageSource* pImageSource = it->first;

        // Instead of falling back to natural size unconditionally,
        // run an algorithm to figure out when we can still use DecodeToRenderSize
        // and if so, compute the same bounds that would have been computed in the RenderWalk
        ImageDecodeBoundsFinder boundsFinder(pImageSource);
        boundsFinder.FindReasonableDecodeBounds();

        if (!boundsFinder.m_skipDecode)
        {
            IFC(pImageSource->RequestDecode(boundsFinder.m_width, boundsFinder.m_height, true /*retainPlaybackState*/));
        }
    }

    for (auto& imageSource: m_animatedImages)
    {
        imageSource->SuspendAnimation();
    }

Cleanup:
    m_trackedImages.Clear();
    m_animatedImages.clear();
    RRETURN(hr);
}

void CCoreServices::StartTrackingAnimatedImage(_In_ CImageSource* imageSource)
{
    m_animatedImages.insert(imageSource);
}

void CCoreServices::StopTrackingAnimatedImage(_In_ CImageSource* imageSource)
{
    m_animatedImages.erase(imageSource);
}

void
CCoreServices::EnqueueImageUpdateRequest(
    _In_ ImageSurfaceWrapper* pImageSurfaceWrapper
    )
{
    // This ignores adding duplicate entries.  During the flush, it will skip
    // duplicate entries because the update state will be set when an image is updated.
    // It is more efficient to just add the duplicate entry and skip during the flush
    // than it is to search for duplicate entries here.
    m_pendingImageUpdates.push_back(pImageSurfaceWrapper);
}

void
CCoreServices::RemoveImageUpdateRequest(
    _In_ ImageSurfaceWrapper* pImageSurfaceWrapper
    )
{
    m_pendingImageUpdates.erase(
        std::remove_if(
            m_pendingImageUpdates.begin(),
            m_pendingImageUpdates.end(),
            [pImageSurfaceWrapper](ImageSurfaceWrapper* pElement) { return (pElement == pImageSurfaceWrapper); }),
        m_pendingImageUpdates.end()
        );
}

void
CCoreServices::FlushPendingImageUpdates()
{
    for (auto pImageSurfaceWrapper : m_pendingImageUpdates)
    {
        if (pImageSurfaceWrapper->IsUpdateRequired())
        {
            pImageSurfaceWrapper->UpdateHardwareResources();
        }
    }

    ClearPendingImageUpdates();
}

void
CCoreServices::ClearPendingImageUpdates()
{
    m_pendingImageUpdates.clear();

#if XCP_MONITOR
    m_pendingImageUpdates.shrink_to_fit();
#endif
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the device resources (recreated on next tick)
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::ReleaseDeviceResources(bool releaseDCompDevice, bool isDeviceLost)
{
    if (m_deviceLost != DeviceLostState::HardwareReleased)
    {
        m_deviceLost = DeviceLostState::HardwareReleased;
        IFC_RETURN(CleanupDeviceRelatedResources(releaseDCompDevice, isDeviceLost));
    }
    return S_OK;
}

WUComp::ICompositor* CCoreServices::GetCompositor() const
{
    ASSERT(m_pNWWindowRenderTarget != nullptr);

    IFCFAILFAST(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->EnsureDCompDevice());
    return m_pNWWindowRenderTarget->GetDCompTreeHost()->GetCompositor();
}

void CCoreServices::EnsureCompositionIslandCreated(_In_ wuc::ICoreWindow* const coreWindow) const
{
    ASSERT(m_pNWWindowRenderTarget != nullptr);

    m_pNWWindowRenderTarget->GetDCompTreeHost()->SetCoreWindow(coreWindow);
    IFCFAILFAST(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->EnsureDCompDevice());
}

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
UINT64 CCoreServices::GetCoreWindowCompositionIslandId()
{
    return GetDCompTreeHost()->GetCompositionIslandId();
}
#endif

#pragma region IXamlTestHooks

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Simulate a device-lost error to trigger re-creation of DComp device(s).
//      Called by the test framework.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SimulateDeviceLost(bool resetVisuals, bool resetDManip)
{
    m_deviceLost = resetVisuals ? DeviceLostState::HardwareAndVisuals : DeviceLostState::HardwareOnly;
    ITickableFrameScheduler *pScheduler = GetBrowserHost()->GetFrameScheduler();
    ASSERT(pScheduler != nullptr);
    IFC_RETURN(pScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::DeviceLost));

    if (resetVisuals && m_inputServices)
    {
        m_inputServices->ResetSharedDManipCompositor();
    }

    if (resetDManip && m_inputServices)
    {
        IFC_RETURN(m_inputServices->ResetDManipCompositors());
    }


    if (WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_46468883>())
    {
        if (resetVisuals && resetDManip)
        {
            // MockDComp may have just been injected. Reset the cache so we can create mock objects.
            ActivationFactoryCache::GetActivationFactoryCache()->ResetCache();
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the DComp device. Called by the test framework.
//
//------------------------------------------------------------------------------
void
CCoreServices::GetDCompDevice(
    _Outptr_ IDCompositionDesktopDevicePartner **ppDCompDevice
    ) const
{
    ASSERT(m_pNWWindowRenderTarget != nullptr);

    IFCFAILFAST(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->WaitForD3DDependentResourceCreation());
    DCompTreeHost *pDCompTreeHostNoRef = m_pNWWindowRenderTarget->GetDCompTreeHost();
    SetInterface(*ppDCompDevice, pDCompTreeHostNoRef->GetMainDevice());
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Forces the window to a certain size. Called by the test framework.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetWindowSizeOverride(
    _In_ const XSIZE *pWindowSize,
    _In_ XHANDLE hwnd,
    float testOverrideScale)
{
    // Only used for testing. Calling without a browser host is unsupported.
    XCP_FAULT_ON_FAILURE(m_pBrowserHost != nullptr);

    IFC_RETURN(m_pBrowserHost->SetWindowSizeOverride(pWindowSize, hwnd));

    const auto& roots = GetContentRootCoordinator()->GetContentRoots();
    for (const auto& root : roots)
    {
        if (const auto rootScale = RootScale::GetRootScaleForContentRoot(root))  // rootScale maybe gone if the main visual tree has been reset
        {
            IFC_RETURN(rootScale->SetTestOverride(testOverrideScale));
        }
    }

    ITickableFrameScheduler *pScheduler = GetBrowserHost()->GetFrameScheduler();
    if (pScheduler)
    {
        IFC_RETURN(pScheduler->RequestAdditionalFrame(0 /*immediate*/, RequestFrameReason::WindowSize));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Overrides the expiration timer for releasing for trimming
//      ImageSurfaceWrapper device resources.  If this is enabled, all device
//      resources related to ImageSurfaceWrapper will be cleaned up on the next
//      render for all wrappers that have left the tree.
//
//------------------------------------------------------------------------------
void
CCoreServices::OverrideTrimImageResourceDelay(bool enabled)
{
    m_testOverrideImageSurfaceWrapperReleaseDelay = enabled;
}

void CCoreServices::SetTimeManagerClockOverrideConstant(double newTime)
{
    if (m_pTimeManager != nullptr)
    {
        m_pTimeManager->SetTimeManagerClockOverrideConstant(newTime);
    }
}

HRESULT CCoreServices::CleanUpAfterTest()
{
    if (m_pTimeManager != nullptr)
    {
        m_pTimeManager->StopAllTimelinesAfterTest();
    }

    return S_OK;
}

// This method is intended to deallocate everything that was allocated during a test run.
// It goes hand in hand with DXamlCore::DeinitializeInstanceToIdle and right now is only
// called from within the test framework (see WindowHelper::ShutdownXaml). This method
// assumes some previous cleanup had been performed. We should
// probably move all the cleanup into this method eventually.
_Check_return_
HRESULT CCoreServices::ShutdownToIdle()
{
    LOG_INFO_EX(L"Shutting down to idle");

    m_spXamlSchemaContext.reset();
    m_spXamlNodeStreamCacheManager.reset();

    IFC_RETURN(ResetState());

    delete m_pTextCore;
    m_pTextCore = NULL;

    // Proactively release our D3D device lost listener to guarantee we synchronize with any pending callback that might be in-flight.
    ReleaseDeviceLostListener();

    // Turn off our frame counter so that we don't potentially attempt to use a bad dcomp visual stored
    // in the DCompTreeHost.
    IFC_RETURN(m_pNWWindowRenderTarget->GetDCompTreeHost()->UpdateDebugSettings(false /* isFrameRateCounterEnabled */))

    // We need to release the render target before calling CleanupRenderTarget
    ReleaseInterface(m_pNWWindowRenderTarget);

    ASSERT(m_pThemeResources == nullptr);

    IFC_RETURN(m_pSystemColorsResources->Clear());
    ReleaseInterface(m_pSystemColorsResources);

    m_pFontDownloadListener.reset();

    // We'll pretend the main tree is loading so that we don't try to render anything until we setup for the next test
    m_isMainTreeLoading = TRUE;

    ClearContextObjects();

    // Do this after ClearContextObjects call which changes
    // a property in the shared state which could end up creating
    // new entries in the FlyweightState list of entires.
    m_flyweightState.Reset();

    m_resourceDictionaryUriCache.Clear();

    IFC_RETURN(ClearMainRootImageCaches());

    m_appliedStyleTables.shrink_to_fit();

    const auto traceSession = Instrumentation::GetXamlTraceSession();
    // Release the trace session. This isn't needed after the first frame anyways.
    if (traceSession)
    {
        DependencyLocator::Internal::ReleaseInstance<Instrumentation::XamlTraceSession>();
    }

    const auto gcInstrumentationAggregator = Instrumentation::GetTelemetryAggregator();
    if (gcInstrumentationAggregator)
    {
        DependencyLocator::Internal::ReleaseInstance<Instrumentation::CTelemetryAggregatorGC>();
    }

    m_pDefaultContentPresenterTemplate.reset();

    // Release namescope root.
    m_nameScopeRoot.ReleaseAllTables(false /*finalShutdown*/);

    DetachMemoryManagerEvents();

    // These should be clear, but we'll just shrink to fit and verify it is empty
    // in CheckForLeaks
    m_pendingImageUpdates.shrink_to_fit();

    auto browserHostNoRef = GetBrowserHost();
    if (browserHostNoRef != nullptr)
    {
        IFC_RETURN(browserHostNoRef->CleanupRenderTarget());

        static_cast<CommonBrowserHost*>(browserHostNoRef)->CleanupErrorService();

        browserHostNoRef->ResetDownloader();
    }

    ReleaseInterface(m_pErrorServiceForXbf);

    m_pPALClock.reset();

    // Only delete this if it's empty, that way it will show as being leaked if it isn't
    if (m_pAllSurfaceImageSources != nullptr && m_pAllSurfaceImageSources->GetHead() == nullptr)
    {
        delete m_pAllSurfaceImageSources;
        m_pAllSurfaceImageSources = nullptr;
    }

    // Only delete this if it's empty, that way it will show as being leaked if it isn't
    if (m_pAllVirtualSurfaceImageSources != nullptr && m_pAllVirtualSurfaceImageSources->GetHead() == nullptr)
    {
        delete m_pAllVirtualSurfaceImageSources;
        m_pAllVirtualSurfaceImageSources = nullptr;
    }

    SetDefaultNavigationTransition(nullptr);

    m_connectedAnimationService.reset();

    ReleaseInterface(m_pRenderTargetBitmapManager);

    m_transparentBrush.reset();
    ResetThemedBrushes();

    m_elementsWithThemeChangedListener.shrink_to_fit();

    m_automationEventsHelper.CleanUpStructureChangedRequests();

#if XCP_MONITOR
    Parser::XamlPredicateService::ClearEvaluationCache();
#endif

    m_facadeStorage.ShrinkToFit();

    SimpleProperty::Property::DestroyAllProperties();

    m_facadeAnimationCount = 0;
    if (m_hasFacadeAnimationsEvent != nullptr)
    {
        VERIFYHR(m_hasFacadeAnimationsEvent->Reset());
    }
    if (m_facadeAnimationsCompleteEvent != nullptr)
    {
        VERIFYHR(m_facadeAnimationsCompleteEvent->Reset());
    }

    m_brushTransitionCount = 0;
    if (m_hasBrushTransitionsEvent != nullptr)
    {
        VERIFYHR(m_hasBrushTransitionsEvent->Reset());
    }
    if (m_brushTransitionsCompleteEvent != nullptr)
    {
        VERIFYHR(m_brushTransitionsCompleteEvent->Reset());
    }

    if (m_pEventManager)
    {
        m_pEventManager->Reset();
    }

    // std::vectors need a shrink_to_fit call, otherwise leak detection thinks the memory they allocated was leaked.
    m_valuesWithExpectedReference.shrink_to_fit();

    if (m_deferredMapping && m_deferredMapping->Empty())
    {
        // Reset only if all deferred elements unregistered.  If this map is not empty then not all dtors were called
        // and there is a leak unrelated to x:Load.
        m_deferredMapping.reset();
    }

    return S_OK;
}

void CCoreServices::DetachApartmentEvents()
{
    DetachMemoryManagerEvents();

    if (m_pResourceManager != nullptr)
    {
        m_pResourceManager->DetachEvents();
    }
}

void CCoreServices::DetachMemoryManagerEvents()
{
    if (m_memoryManager)
    {
        m_memoryManager->remove_AppMemoryUsageLimitChanging(m_appMemoryUsageLimitChangingToken);
        m_appMemoryUsageLimitChangingToken = EventRegistrationToken();
        m_memoryManager->remove_AppMemoryUsageIncreased(m_appMemoryUsageIncreasedToken);
        m_appMemoryUsageIncreasedToken = EventRegistrationToken();
        m_memoryManager.Reset();
    }
}

void CCoreServices::AttachMemoryManagerEvents()
{
    m_attachMemoryManagerEvents = false; // We only attempt this once.

    // When we are in the background, we don't pump windows messages, so we can't get events on the
    // ui thread.
    if (IsInBackgroundTask()) return;

    // Attach the memory manager event for detecting when we need to free up memory.  Note that
    // this may fail when we aren't running under WinRT.  That is ok, we will just ignore if
    // that happens.
    Microsoft::WRL::ComPtr<wsy::IMemoryManagerStatics> memoryManager;
    if (FAILED(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Windows_System_MemoryManager).Get(),
        &memoryManager
    )))
    {
        return;
    }

    // Memory limit changing event
    auto limitChangingCallback = Make<AppMemoryUsageLimitChangingCallback>();
    if (SUCCEEDED(memoryManager->add_AppMemoryUsageLimitChanging(limitChangingCallback.Get(), &m_appMemoryUsageLimitChangingToken)))
    {
        m_memoryManager = memoryManager;
    }

    // Memory usage increased event
    auto usageIncreasedCallback = Make<AppMemoryUsageIncreasedCallback>();
    if (SUCCEEDED(memoryManager->add_AppMemoryUsageIncreased(usageIncreasedCallback.Get(), &m_appMemoryUsageIncreasedToken)) && m_memoryManager == nullptr)
    {
        m_memoryManager = memoryManager;
    }
}

void
CCoreServices::CheckForLeaks()
{
    if (!m_pendingImageUpdates.empty())
    {
        LOG_LEAK_EX(L"Pending image updates on CCoreServices is not empty");
    }

    // Ensure this is empty, we mark it as leak ignoring because the deque is special. It even reallocates
    // when we call shrink_to_fit.
    if (!m_deferredAnimationOperationQueue.empty())
    {
        LOG_LEAK_EX(L"Deferred Animation queue on CCoreServices is not empty");
    }

    if (!m_PegNoRefCoreObjectsWithoutPeers.empty())
    {
        // See comment in corep.h why we mark the m_PegNoRefCoreObjectsWithoutPeers with the leak ignoring allocator.
        auto iter = m_PegNoRefCoreObjectsWithoutPeers.begin();
        if (m_PegNoRefCoreObjectsWithoutPeers.size() != 1u || !(*iter)->OfTypeByIndex<KnownTypeIndex::Application>())
        {
            // If CDependencyObjects's are being leaked, it's likely those leaked objects are what is causing this to complain.
            // Fix why they are leaking first, and this should go away.
            LOG_LEAK_EX(L"The CApplication object is not the only object in CCoreService::m_PegNoRefCoreObjectsWithoutPeers");
        }
    }

    if (!m_grippersNoRef.empty())
    {
        LOG_LEAK_EX(L"The grippers collection on CCoreService is not empty");
    }
}

_Check_return_
HRESULT CCoreServices::InitializeFromIdle()
{
    ASSERT(!IsCoreReady());

    IFC_RETURN(RefreshXamlSchemaContext());

    // Create a new compositor scheduler and render target to host the next
    // application loaded into this host. This needs to be called before Initialize because
    // we need the FrameScheduler
    if (m_pNWWindowRenderTarget == nullptr)
    {
        IFC_RETURN(GetBrowserHost()->CreateRenderTarget());
    }

    // We have to re-initialize here so that the foreground window is happy in our TestServices.
    IFC_RETURN(Initialize());

    // Ensure the background timer has been created and initialized
    IFC_RETURN(EnsureBackgroundTimer());

    if (m_pEventManager->m_pfnScriptCallbackAsync == nullptr)
    {
        IFC_RETURN(RegisterScriptCallback(
            GetBrowserHost(),
            CXcpBrowserHost::PostAsyncScriptCallbackRequest,
            CXcpBrowserHost::SyncScriptCallbackRequest));
    }

    InitializeContextObjects();

    InitCoreWindowContentRoot();

    //  Initialize the TextCore
    IFC_RETURN(GetTextCore(NULL));

    if (m_pRenderTargetBitmapManager == nullptr)
    {
        IFC_RETURN(CRenderTargetBitmapManager::Create(this, &m_pRenderTargetBitmapManager));
    }

    // Build device related resources now. Usually this happens when we recover from device lost,
    // however we weren't rendering so we'll do it now
    IFC_RETURN(BuildDeviceRelatedResources());

    m_isMainTreeLoading = FALSE;

    return S_OK;
}

_Check_return_
HRESULT CCoreServices::EnsureBackgroundTimer()
{
    if (m_pBackgroundResourceTimer == nullptr)
    {
        xref_ptr<CTimeSpan> pTimeout;
        CREATEPARAMETERS cp(this);

        // Resources timeout 2s after the app goes to the background.
        CValue resourceTimeout;
        resourceTimeout.SetFloat(2.0f);

        IFC_RETURN(CDispatcherTimer::Create(reinterpret_cast<CDependencyObject**>(&m_pBackgroundResourceTimer), &cp));

        CREATEPARAMETERS cpTimeout(this, resourceTimeout);
        IFC_RETURN(CTimeSpan::Create(reinterpret_cast<CDependencyObject**>(pTimeout.ReleaseAndGetAddressOf()), &cpTimeout));

        resourceTimeout.WrapObjectNoRef(pTimeout.get());
        IFC_RETURN(m_pBackgroundResourceTimer->SetValueByIndex(KnownPropertyIndex::DispatcherTimer_Interval, resourceTimeout));

        CValue handler;
        handler.SetInternalHandler(&OnBackgroundResourceTimeout);
        IFC_RETURN(m_pBackgroundResourceTimer->AddEventListener(EventHandle(KnownEventIndex::DispatcherTimer_Tick), &handler, EVENTLISTENER_INTERNAL, NULL, FALSE));
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Sets a custom font collection held by the core text services for testing.
//      It also invalidates the measurement of controls because with a different
//      set of fonts, layout sizes can change.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetSystemFontCollectionOverride(_In_opt_ IDWriteFontCollection* pFontCollection)
{
    CTextCore* pTextCore = nullptr;
    IFC_RETURN(GetTextCore(&pTextCore));
    IFC_RETURN(pTextCore->GetWinTextCore()->SetSystemFontCollectionOverride(pFontCollection));

    CUIElement *pVisualRoot = m_pMainVisualTree->GetRootVisual();
    TraceRecursiveInvalidateMeasureInfo(L"Set System Font Collection Override");
    pVisualRoot->RecursiveInvalidateMeasure();

    return S_OK;
}

void
CCoreServices::SetTransparentBackground(bool isTransparent)
{
    if (m_isTransparentBackground != isTransparent)
    {
        m_isTransparentBackground = isTransparent;

        // Need the HWWalk to rebuild the root visual which will check the
        // IsTransparentBackground field in core services to skip the root
        // visual primitive
        CUIElement::NWSetContentDirty(GetMainRootVisual(), DirtyFlags::Render);

        if (HasXamlIslands())
        {
            CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();
            CDOCollection* collection = xamlIslandRootCollection->GetChildren();
            if (collection != nullptr)
            {
                for (CDependencyObject* child : *collection)
                {
                    CXamlIslandRoot* island = do_pointer_cast<CXamlIslandRoot>(child);
                    CUIElement::NWSetContentDirty(island, DirtyFlags::Render);
                }
            }
        }
    }
}

bool
CCoreServices::IsDeviceLost()
{
    // Pending device lost indicates a device lost that will be detected during the next tick since there is an
    // asynchronous work item that initializes expensive graphics resources.
    // It is always initialized to S_OK before creation begins and it will be updated to a device lost error if
    // initialization has already run and failed for such a reason.
    bool pendingDeviceLost =
        (m_pNWWindowRenderTarget != nullptr) &&
        (m_pNWWindowRenderTarget->GetGraphicsDeviceManager() != nullptr) &&
        GraphicsUtility::IsDeviceLostError(m_pNWWindowRenderTarget->GetGraphicsDeviceManager()->GetResourceCreationHR());

    return (m_deviceLost != DeviceLostState::None) ||
           pendingDeviceLost;
}

// Mark a core object that doesn't have a peer as a GC root
void CCoreServices::PegNoRefCoreObjectWithoutPeer(_In_ CDependencyObject *pObject)
{
    AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());

    // Don't need to AddRef pObject because CDependencyObject's dtor calls
    // UnpegNoRefCoreObjectWithoutPeer.
    m_PegNoRefCoreObjectsWithoutPeers.insert(pObject);
}

// Unmark a core object that doesn't have a peer as a GC root
void CCoreServices::UnpegNoRefCoreObjectWithoutPeer(_In_ CDependencyObject *pObject)
{
    if (!m_bIsDestroyingCoreServices)
    {
        AutoReentrantReferenceLock lock(DXamlServices::GetPeerTableHost());
        m_PegNoRefCoreObjectsWithoutPeers.erase(pObject);
    }
}

// Walks core objects that are GC roots. These didn't have peers when
// PegNoRef was called.
void CCoreServices::ReferenceTrackerWalkOnCoreGCRoots(
    _In_ DirectUI::EReferenceTrackerWalkType walkType)
{
    for (auto it = m_PegNoRefCoreObjectsWithoutPeers.begin(); it != m_PegNoRefCoreObjectsWithoutPeers.end(); ++it)
    {
        (*it)->ReferenceTrackerWalk(
            walkType,
            true,   // isRoot,
            true);  // shouldWalkPeer
    }
}

void CCoreServices::SetThreadingAssertOverride(bool enable)
{
    m_fDbgEnableThreadingAssert = enable;
}

xref_ptr<CLayoutTransitionElement> CCoreServices::AddTestLTE(
    _In_ CUIElement *lteTarget,
    _In_ CUIElement *lteParent,
    bool parentIsRootVisual,
    bool parentIsPopupRoot,
    bool isAbsolutelyPositioned)
{
    // Only used for testing. Calling without a visual tree is unsupported.
    XCP_FAULT_ON_FAILURE(m_pMainVisualTree != nullptr);

    return m_pMainVisualTree->AddTestLTE(lteTarget, lteParent, parentIsRootVisual, parentIsPopupRoot, isAbsolutelyPositioned);
}

void CCoreServices::RemoveTestLTE(_In_ CUIElement *lte)
{
    // Only used for testing. Calling without a visual tree is unsupported.
    XCP_FAULT_ON_FAILURE(m_pMainVisualTree != nullptr);

    m_pMainVisualTree->RemoveTestLTE(lte);
}

void CCoreServices::ClearTestLTEs()
{
    // Only used for testing. Some tests reset the entire visual tree right before the end to test for crashes.
    // No-op the call in those cases.
    if (m_pMainVisualTree != nullptr)
    {
        m_pMainVisualTree->ClearTestLTEs();
    }
}

void CCoreServices::SetCanTickWithNoContent(bool canTickWithNoContent)
{
    m_canTickWithNoContent = canTickWithNoContent;
}

#pragma endregion

void CCoreServices::EnsureOleIntialized()
{
    if (!m_calledOleInitialize && IsOleInitializePresent())
    {
        if (SUCCEEDED(OleInitialize(NULL)))
        {
            m_calledOleInitialize = true;
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes the events used in our wait for idle test infra.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::InitWaitForIdleEvents()
{
    IFC_RETURN(gps->NamedEventCreate(
        &m_pHasAnimationsEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"HasAnimations"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pAnimationsCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"AnimationsComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pHasDeferredAnimationOperationsEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"HasDeferredAnimationOperations"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pDeferredAnimationOperationsCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"DeferredAnimationOperationsComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pRootVisualResetEvent,
        InitModeOpenOrCreate,
        TRUE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"RootVisualReset"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pImageDecodingIdleEvent,
        InitModeOpenOrCreate,
        TRUE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"ImageDecodingIdle"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pFontDownloadsIdleEvent,
        InitModeOpenOrCreate,
        TRUE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"FontDownloadsIdle"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pPopupMenuCommandInvokedEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE  /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"PopupMenuCommandInvoked"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pHasBuildTreeWorksEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"HasBuildTreeWorks"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pBuildTreeServiceDrainedEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"BuildTreeServiceDrained"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pKeyboardInputEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"KeyboardInputReceived"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pPointerInputEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"PointerInputReceived"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_pImplicitShowHideCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"ImplicitShowHideComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_hasFacadeAnimationsEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"HasFacadeAnimations"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_facadeAnimationsCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"FacadeAnimationsComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_animatedFacadePropertyChangesCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"AnimatedFacadePropertyChangesComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_hasBrushTransitionsEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        TRUE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"HasBrushTransitions"),
        FALSE /* bReturnFailureIfCreationFailed */));

    IFC_RETURN(gps->NamedEventCreate(
        &m_brushTransitionsCompleteEvent,
        InitModeOpenOrCreate,
        FALSE /* bInitialState */,
        FALSE /* bManualReset */,
        XSTRING_PTR_EPHEMERAL(L"BrushTransitionsComplete"),
        FALSE /* bReturnFailureIfCreationFailed */));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns a value indicating whether or not the has animations and animations
//      complete events have been successfully retrieved.
//
//------------------------------------------------------------------------
bool
CCoreServices::HasAnimationEvents()
{
    return m_pHasAnimationsEvent != nullptr && m_pAnimationsCompleteEvent != nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the signaled status of the has animations event to a given state, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetHasAnimationsEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pHasAnimationsEvent != nullptr)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pHasAnimationsEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pHasAnimationsEvent->Reset());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the signaled status of the root visual reset event to a given
//      state, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetRootVisualResetEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pRootVisualResetEvent != nullptr)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pRootVisualResetEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pRootVisualResetEvent->Reset());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the signaled status of the image decoding idle event to a given
//      state, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetImageDecodingIdleEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pImageDecodingIdleEvent != nullptr)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pImageDecodingIdleEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pImageDecodingIdleEvent->Reset());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the signaled status of the font downloads idle event to a given
//      state, if it exists. This is for tests to be able to wait on the
//      idle state before testing conditions.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetFontDownloadsIdleEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pFontDownloadsIdleEvent != nullptr)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pFontDownloadsIdleEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pFontDownloadsIdleEvent->Reset());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the animations complete event, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetAnimationsCompleteEvent()
{
    if (m_pAnimationsCompleteEvent != nullptr)
    {
        IFC_RETURN(m_pAnimationsCompleteEvent->Set());
    }

    return S_OK;
}

// Sets the signaled status of the has animations event to a given state, if it exists.
_Check_return_ HRESULT
CCoreServices::SetHasDeferredAnimationOperationsEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pHasDeferredAnimationOperationsEvent)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pHasDeferredAnimationOperationsEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pHasDeferredAnimationOperationsEvent->Reset());
        }
    }

    return S_OK;
}

// Sets the deferred animation operations complete event, if it exists.
_Check_return_ HRESULT
CCoreServices::SetDeferredAnimationOperationsCompleteEvent()
{
    if (m_pDeferredAnimationOperationsCompleteEvent)
    {
        IFC_RETURN(m_pDeferredAnimationOperationsCompleteEvent->Set());
    }

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::SetPopupMenuCommandInvokedEvent()
{
    if (m_pPopupMenuCommandInvokedEvent != nullptr)
    {
        return m_pPopupMenuCommandInvokedEvent->Set();
    }
    return S_FALSE;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Are the BuildTreeEvents initialized
//
//------------------------------------------------------------------------
bool
CCoreServices::HasBuildTreeWorkEvents()
{
    return m_pHasBuildTreeWorksEvent != nullptr && m_pBuildTreeServiceDrainedEvent != nullptr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the signalled status of the has BuildTreeWorks event to a given state, if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::SetHasBuildTreeWorksEventSignaledStatus(_In_ bool bSignaled)
{
    if (m_pHasBuildTreeWorksEvent != nullptr)
    {
        if (bSignaled)
        {
            IFC_RETURN(m_pHasBuildTreeWorksEvent->Set());
        }
        else
        {
            IFC_RETURN(m_pHasBuildTreeWorksEvent->Reset());
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::SetBuildTreeServiceDrainedEvent()
{
    if (m_pBuildTreeServiceDrainedEvent != nullptr)
    {
        IFC_RETURN(m_pBuildTreeServiceDrainedEvent->Set());
    }

    return S_OK;
}

_Check_return_ HRESULT
CCoreServices::SetKeyboardInputEvent()
{
    if (m_fireKeyboardInputEvent && (m_pKeyboardInputEvent != nullptr))
    {
        IFC_RETURN(m_pKeyboardInputEvent->Set());
    }

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::EnableKeyboardInputEvent()
{
    m_fireKeyboardInputEvent = true;
    if (m_pKeyboardInputEvent != nullptr)
    {
        IFC_RETURN(m_pKeyboardInputEvent->Reset());
    }

    return S_OK;
}

void CCoreServices::DisableKeyboardInputEvent()
{
    m_fireKeyboardInputEvent = false;
}

bool CCoreServices::CanFireKeyboardInputEvent() const
{
    return m_fireKeyboardInputEvent;
}
_Check_return_ HRESULT
CCoreServices::SetPointerInputEvent()
{
    if (m_firePointerInputEvent && (m_pPointerInputEvent != nullptr))
    {
        IFC_RETURN(m_pPointerInputEvent->Set());
    }

    return S_OK;
}

_Check_return_ HRESULT CCoreServices::EnablePointerInputEvent()
{
    m_firePointerInputEvent = true;
    if (m_pPointerInputEvent != nullptr)
    {
        IFC_RETURN(m_pPointerInputEvent->Reset());
    }

    return S_OK;
}

void CCoreServices::DisablePointerInputEvent()
{
    m_firePointerInputEvent = false;
}

bool CCoreServices::CanFirePointerInputEvent() const
{
    return m_firePointerInputEvent;
}

void CCoreServices::IncrementPendingImplicitShowHideCount()
{
    if (::InterlockedIncrement(&m_pendingImplicitShowHideCount) == 1)
    {
        if (m_pImplicitShowHideCompleteEvent != nullptr)
        {
            VERIFYHR(m_pImplicitShowHideCompleteEvent->Reset());
        }
    }
}

void CCoreServices::DecrementPendingImplicitShowHideCount()
{
    ASSERT(m_pendingImplicitShowHideCount > 0);
    if (::InterlockedDecrement(&m_pendingImplicitShowHideCount) == 0)
    {
        if (m_pImplicitShowHideCompleteEvent != nullptr)
        {
            VERIFYHR(m_pImplicitShowHideCompleteEvent->Set());
        }
    }
}

void CCoreServices::IncrementActiveFacadeAnimationCount()
{
    if (::InterlockedIncrement(&m_facadeAnimationCount) == 1)
    {
        if (m_hasFacadeAnimationsEvent != nullptr)
        {
            VERIFYHR(m_hasFacadeAnimationsEvent->Set());
        }
    }
}

void CCoreServices::DecrementActiveFacadeAnimationCount()
{
    ASSERT(m_facadeAnimationCount > 0);
    if (::InterlockedDecrement(&m_facadeAnimationCount) == 0)
    {
        if (m_facadeAnimationsCompleteEvent != nullptr)
        {
            VERIFYHR(m_facadeAnimationsCompleteEvent->Set());
        }
    }
}

void CCoreServices::IncrementActiveBrushTransitionCount()
{
    if (::InterlockedIncrement(&m_brushTransitionCount) == 1)
    {
        if (m_hasBrushTransitionsEvent != nullptr)
        {
            VERIFYHR(m_hasBrushTransitionsEvent->Set());
        }
    }
}

void CCoreServices::DecrementActiveBrushTransitionCount()
{
    ASSERT(m_brushTransitionCount > 0);
    if (::InterlockedDecrement(&m_brushTransitionCount) == 0)
    {
        if (m_brushTransitionsCompleteEvent != nullptr)
        {
            VERIFYHR(m_brushTransitionsCompleteEvent->Set());
        }
    }
}

void CCoreServices::ScheduleWaitForAnimatedFacadePropertyChanges(int count)
{
    ASSERT(count > 0);

    m_pendingAnimatedFacadePropertyChanges = count;
    if (m_animatedFacadePropertyChangesCompleteEvent != nullptr)
    {
        VERIFYHR(m_animatedFacadePropertyChangesCompleteEvent->Reset());
    }
}

void CCoreServices::DecrementPendingAnimatedFacadePropertyChangeCount()
{
    if (m_pendingAnimatedFacadePropertyChanges > 0)
    {
        m_pendingAnimatedFacadePropertyChanges--;
        if (m_pendingAnimatedFacadePropertyChanges == 0)
        {
            if (m_animatedFacadePropertyChangesCompleteEvent != nullptr)
            {
                VERIFYHR(m_animatedFacadePropertyChangesCompleteEvent->Set());
            }
        }
    }
}

void CCoreServices::RequestAdditionalFrame(RequestFrameReason reason)
{
    IXcpBrowserHost *pBH = GetBrowserHost();
    if (pBH != nullptr)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != nullptr)
        {
            IFCFAILFAST(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, reason));
        }
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the stored font scale with the current value from settings.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CCoreServices::UpdateFontScale(_In_ XFLOAT newFontScale)
{
    XFLOAT oldFontScale = m_fontScale;

    m_fontScale = newFontScale;

    // If the font scale changed, then that's going to fundamentally alter layout
    // as the sizes of every single block of text will change that hasn't opted out
    // of font scaling, so we should invalidate font size on the whole visual tree
    // to ensure that we pick up all the changes.
    // The only exception is when we're on the first frame,
    // in which case everything is already dirty, so we don't need to bother.
    if (m_fontScale != oldFontScale && m_uFrameNumber != 1)
    {
        CUIElement *pVisualRoot = m_pMainVisualTree->GetRootVisual();

        if (pVisualRoot != NULL)
        {
            IFC_RETURN(pVisualRoot->RecursiveInvalidateFontSize());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the stored font scale.
//
//------------------------------------------------------------------------
XFLOAT
CCoreServices::GetFontScale()
{
    return m_fontScale;
}

_Check_return_ HRESULT CCoreServices::CancelAllConnectedAnimationsAndResetDefaults()
{
    if (m_connectedAnimationService != nullptr)
    {
        IFC_RETURN(m_connectedAnimationService->CancelAllAnimationsAndResetDefaults());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Queues a Commit to the main DComp device
//
//------------------------------------------------------------------------
HRESULT
CCoreServices::RequestMainDCompDeviceCommit(RequestFrameReason reason)
{
    m_commitRequested = TRUE;

    if (auto browserHost = GetBrowserHost())
    {
        if (auto frameScheduler = browserHost->GetFrameScheduler())
        {
            IFC_RETURN(frameScheduler->RequestAdditionalFrame(0 /* immediate */, reason));
        }
    }

    return S_OK;
}

void
CCoreServices::AddThemeChangedListener(_In_ CFrameworkElement* pFE)
{
    auto it = m_elementsWithThemeChangedListener.find(pFE);
    if (it == m_elementsWithThemeChangedListener.end())
    {
        m_elementsWithThemeChangedListener.emplace(pFE, 1);
    }
    else
    {
        it->second++;
    }
}

void
CCoreServices::RemoveThemeChangedListener(_In_ CFrameworkElement* pFE)
{
    auto it = m_elementsWithThemeChangedListener.find(pFE);
    if (it == m_elementsWithThemeChangedListener.end())
    {
        ASSERT(FALSE);
        return;
    }

    auto count = --(it->second);
    if (count == 0)
    {
        m_elementsWithThemeChangedListener.erase(it);
    }

#if XCP_MONITOR
    if (m_elementsWithThemeChangedListener.empty())
    {
        m_elementsWithThemeChangedListener.clear();
    }
#endif
}

void CCoreServices::SetPostTickCallback(_In_opt_ std::function<void()> callback)
{
    m_postTickCallback = std::move(callback);
}

bool CCoreServices::GetIsTextPerformanceVisualizationEnabled() const
{
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    return runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen);
}

_Check_return_ HRESULT CCoreServices::SetIsTextPerformanceVisualizationEnabled(bool enabled)
{
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    runtimeEnabledFeatureDetector->SetFeatureOverride(RuntimeEnabledFeature::DrawDWriteTextLayoutInGreen, enabled);
    // We need to redraw the text (in CTextBlock::ArrangeOverride) to desired color when the DebugSettings is changed.
    // So we need to invalidate the layout here.
    if (m_pMainVisualTree != nullptr && m_pMainVisualTree->GetRootVisual() != nullptr)
    {
        TraceRecursiveInvalidateMeasureInfo(L"Text Performance Visualization Enabled");
        m_pMainVisualTree->GetRootVisual()->RecursiveInvalidateMeasure();
    }

    // Request a tick to pick up the debug settings change.
    IXcpBrowserHost *pBH = GetBrowserHost();
    if (pBH != nullptr)
    {
        ITickableFrameScheduler *pFrameScheduler = pBH->GetFrameScheduler();
        if (pFrameScheduler != nullptr)
        {
            IFC_RETURN(pFrameScheduler->RequestAdditionalFrame(0 /* immediate */, RequestFrameReason::SettingsChanged));
        }
    }
    return S_OK;
}

void CCoreServices::EnsureConnectedAnimationService()
{
    if (m_connectedAnimationService == nullptr)
    {
        m_connectedAnimationService.attach(new CConnectedAnimationService(this));
    }
}

CConnectedAnimationService* CCoreServices::GetConnectedAnimationServiceNoRef()
{
    return m_connectedAnimationService.get();
}

DCompTreeHost* CCoreServices::GetDCompTreeHost()
{
    return m_pNWWindowRenderTarget->GetDCompTreeHost();
}

CDeferredMapping* CCoreServices::GetDeferredMapping()
{
    if (!m_deferredMapping)
    {
        m_deferredMapping.reset();
        m_deferredMapping = std::make_unique<CDeferredMapping>();
    }

    return m_deferredMapping.get();
}

void CCoreServices::AddXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    CXamlIslandRootCollection* xamlIslandRootCollection = m_pMainVisualTree->GetXamlIslandRootCollection();

    CUIElementCollection* collection = static_cast<CUIElementCollection*>(xamlIslandRootCollection->GetChildren());

    IFCFAILFAST(collection->Append(xamlIslandRoot));

    m_pNWWindowRenderTarget->GetDCompTreeHost()->AddXamlIslandTarget(xamlIslandRoot);
}

void CCoreServices::RemoveXamlIslandRoot(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    m_isTearingDownIsland = true;

    CXamlIslandRootCollection* xamlIslandRootCollection {
        do_pointer_cast<CXamlIslandRootCollection>(xamlIslandRoot->GetParentInternal(false /*publicOnly*/)) };
    if (m_pMainVisualTree)
    {
        ASSERT(m_pMainVisualTree->GetXamlIslandRootCollection() == xamlIslandRootCollection);
    }

    CDOCollection* xamlIslands = xamlIslandRootCollection->GetChildren();
    void* result = xamlIslands->Remove(xamlIslandRoot);

    if (result)
    {
        xamlIslandRoot->Release();
    }

    m_pNWWindowRenderTarget->GetDCompTreeHost()->RemoveXamlIslandTarget(xamlIslandRoot);

    if (m_inputServices
        && GetInitializationType() == InitializationType::IslandsOnly
        && xamlIslands->size() == 0)
    {
        // If the last island is going away and we're in "islands-only" mode, we have some extra cleanup to do.
        // When tests run with non-island hosting, they set "Window.Content = null" at the end of the test,
        // which calls CCoreServices::StartApplication and cleans up the pointer objects.  The equivalent in
        // the islands-only case is to do this when the last island gets removed.
        m_inputServices->DestroyPointerObjects();
    }

    m_isTearingDownIsland = false;
}

void CCoreServices::UpdateXamlIslandRootTargetSize(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    m_pNWWindowRenderTarget->GetDCompTreeHost()->UpdateXamlIslandTargetSize(xamlIslandRoot);
}

bool CCoreServices::HasXamlIslands() const
{
    return m_pNWWindowRenderTarget
        && m_pNWWindowRenderTarget->GetDCompTreeHost()
        && m_pNWWindowRenderTarget->GetDCompTreeHost()->HasXamlIslandData();
}

CDependencyObject* CCoreServices::GetRootForElement(_In_ CDependencyObject* dependencyObject)
{
    if (m_pMainVisualTree)
    {
        CXamlIslandRoot* xamlIslandRoot = m_pMainVisualTree->GetXamlIslandRootForElement(dependencyObject);
        if (xamlIslandRoot)
        {
            return xamlIslandRoot;
        }
        return m_pMainVisualTree->GetRootVisual();
    }
    else
    {
        return nullptr;
    }
}

CResourceDictionary* CCoreServices::GetThemeResources()
{
    if (!m_pThemeResources)
    {
        IFCFAILFAST(FxCallbacks::FrameworkCallbacks_LoadThemeResources());
    }

    return m_pThemeResources;
}

_Check_return_ HRESULT
CCoreServices::GetPointerInfoFromPointerPoint(
        _In_ ixp::IPointerPoint* pointerPoint,
        _Out_ PointerInfo* pointerInfoResult)
{
    IFC_RETURN(GetInputServices()->GetPointerInfoFromPointerPoint(pointerPoint, pointerInfoResult));
    return S_OK;
}

VisualTree* CCoreServices::GetMainVisualTree()
{
    return m_pMainVisualTree;
}

void CCoreServices::AddMutableStyleValueChangedListener(
    _In_ CStyle* const pStyle,
    xref::weakref_ptr<CFrameworkElement>& elementWeakRef)
{
    const bool shouldStoreSourceInfo = DXamlServices::ShouldStoreSourceInformation();
    ASSERT(pStyle && (pStyle->HasMutableSetters() || shouldStoreSourceInfo));

    if (pStyle->HasMutableSetters(false /* considerBasedOnStyle */) || shouldStoreSourceInfo)
    {
        auto& associatedElements = m_appliedStyleTables.emplace(xref::get_weakref(pStyle), std::vector<xref::weakref_ptr<CFrameworkElement>>()).first->second;
        associatedElements.emplace_back(elementWeakRef);
    }

    // Recursively add the BasedOn Styles
    auto const basedOnStyle = pStyle->GetBasedOnStyleNoRef();
    if (basedOnStyle && (basedOnStyle->HasMutableSetters() || shouldStoreSourceInfo))
    {
        AddMutableStyleValueChangedListener(basedOnStyle, elementWeakRef);
    }
}

void CCoreServices::RemoveMutableStyleValueChangedListener(
    _In_ CStyle* const pStyle,
    xref::weakref_ptr<CFrameworkElement>& elementWeakRef)
{
    const bool shouldStoreSourceInfo = DXamlServices::ShouldStoreSourceInformation();
    ASSERT(pStyle && (pStyle->HasMutableSetters() || shouldStoreSourceInfo));

    auto resultItr = m_appliedStyleTables.find(xref::get_weakref(pStyle));
    if (resultItr != m_appliedStyleTables.end())
    {
        auto& listeners = resultItr->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), elementWeakRef), listeners.end());

        if (listeners.empty())
        {
            m_appliedStyleTables.erase(resultItr);
        }
    }

    // Recursively remove the BasedOn Styles
    auto const basedOnStyle = pStyle->GetBasedOnStyleNoRef();
    if (basedOnStyle && (basedOnStyle->HasMutableSetters() || shouldStoreSourceInfo))
    {
        RemoveMutableStyleValueChangedListener(basedOnStyle, elementWeakRef);
    }
}

const std::vector<xref::weakref_ptr<CFrameworkElement>>&
CCoreServices::GetMutableStyleValueChangedListeners(_In_ CStyle* const pStyle)
{
    static const std::vector<xref::weakref_ptr<CFrameworkElement>> empty_vector;

    const auto resultItr = m_appliedStyleTables.find(xref::get_weakref(pStyle));
    if (resultItr != m_appliedStyleTables.end())
    {
        return resultItr->second;
    }

    return empty_vector;
}

void CCoreServices::NotifyMutableStyleValueChangedListeners(
    _In_ CStyle* const pStyle,
    KnownPropertyIndex propertyIndex)
{
    ASSERT(pStyle && (pStyle->HasMutableSetters() || DXamlServices::ShouldStoreSourceInformation()));

    auto resultItr = m_appliedStyleTables.find(xref::get_weakref(pStyle));
    if (resultItr != m_appliedStyleTables.end())
    {
        std::size_t expiredElements = 0;
        auto& listeners = resultItr->second;
        for (auto& elementWeakRef : listeners)
        {
            auto element = elementWeakRef.lock();
            if (element)
            {
                element->NotifyMutableStyleValueChanged(propertyIndex);
            }
            else
            {
                ++expiredElements;
            }
        }

        // If the number of expired listeners is greater than half (arbitrarily chosen) the number of total listeners,
        // do some cleanup
        if (expiredElements > (listeners.size() / 2))
        {
            listeners.erase(std::remove_if(listeners.begin(), listeners.end(), [](auto& listener) { return listener.expired(); }), listeners.end());
        }

        if (listeners.empty())
        {
            m_appliedStyleTables.erase(resultItr);
        }
    }
}

bool CCoreServices::IsTSF3Enabled() const
{
    if (m_isTsf3Disabled)
    {
        return false;
    }

    if (DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
    {
        return false;
    }

    return true;
}

void CCoreServices::AddValueWithExpectedReference(_In_ CModifiedValue* value)
{
    m_valuesWithExpectedReference.push_back(value);
}

void CCoreServices::RemoveValueWithExpectedReference(_In_ CModifiedValue* value)
{
    for (auto it = m_valuesWithExpectedReference.begin(); it != m_valuesWithExpectedReference.end(); it++)
    {
        if (*it == value)
        {
            m_valuesWithExpectedReference.erase(it);
            break;
        }
    }
}

void CCoreServices::ClearValuesWithExpectedReference()
{
    std::vector<CModifiedValue*> valuesCopy = m_valuesWithExpectedReference;
    for (auto it : valuesCopy)
    {
        it->RemoveExpectedReference();
    }
    ASSERT(m_valuesWithExpectedReference.empty());
}

HRESULT AppMemoryUsageLimitChangingCallback::Invoke(_In_opt_ IInspectable* sender, _In_ wsy::IAppMemoryUsageLimitChangingEventArgs* args)
{
    CCoreServices* services = DirectUI::DXamlServices::GetSafeHandle();
    if (!services)
    {
        return S_OK;
    }
    return services->CheckMemoryUsage(false /* simulateLowMemory */);
}

HRESULT AppMemoryUsageIncreasedCallback::Invoke(_In_opt_ IInspectable* sender, _In_ IInspectable* args)
{
    CCoreServices* services = DirectUI::DXamlServices::GetSafeHandle();
    if (!services)
    {
        return S_OK;
    }
    return services->CheckMemoryUsage(false /* simulateLowMemory */);
}

wf::Size CCoreServices::GetContentRootMaxSize()
{
    wf::Size maxSize{ 0, 0 };

    for (const xref_ptr<CContentRoot>& contentRoot : GetContentRootCoordinator()->GetContentRoots())
    {
        wf::Size rootSize = contentRoot->GetVisualTreeNoRef()->GetSize();

        if (rootSize.Width > maxSize.Width)
        {
            maxSize.Width = rootSize.Width;
        }

        if (rootSize.Height > maxSize.Height)
        {
            maxSize.Height = rootSize.Height;
        }
    }

    return maxSize;
}
