// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DCompTreeHost.h"
#include <touchtelemetry.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <windows.applicationmodel.core.h>
#include <GraphicsUtility.h>
#include <DCompSurface.h>
#include <DCompSurfaceFactoryManager.h>
#include <D3D11SharedDeviceGuard.h>
#include <D3D11Device.h>
#include <dwrite.h>
#include <core.h>
#include <MUX-ETWEvents.h>
#include <windows.foundation.h>
#include <microsoft.ui.composition.h>
#include <microsoft.ui.composition.experimental.h>
#include <microsoft.ui.composition.experimental.interop.h>
#include <WindowsGraphicsDeviceManager.h>
#include <corep.h>
#include <d2dutils.h>
#include <DCompInteropCompositorPartnerCallback.h>
#include <DoubleUtil.h>
#include <HWCompNode.h>
#include <UIElement.h>
#include <Popup.h>
#include <WindowRenderTarget.h>
#include <OfferTracker.h>

#include <FxCallbacks.h>
#include "CoreWindowIslandAdapter.h"

// XamlIslandRoots
#include <framework.h>
#include <DOPointerCast.h>
#include <XamlIslandRoot.h>
#include <UIElementCollection.h>

// For XcpDispatcher
#include <host.h>
#include <xcpwindow.h>

#include <windows.ui.core.h>

#include <dwmapi.h>
#include <isapipresent.h>  // IsDwmSetWindowAttributePresent

#include <XamlOneCoreTransforms.h>

#include <DXamlServices.h>

#include <DesignMode.h>

using namespace DirectUI;
using namespace Microsoft::WRL::Wrappers;
using namespace RuntimeFeatureBehavior;
using namespace ABI::Windows::UI::Core;

typedef HRESULT (WINAPI *DCOMPOSITIONCREATEDEVICE3FUNC)(
    _In_opt_ IUnknown *renderingDevice,
    _In_ REFIID iid,
    _Outptr_ void **dcompositionDevice
    );

typedef HRESULT(WINAPI * PFN_DllGetActivationFactory)(HSTRING activatableClassId, IActivationFactory **ppFactory);

bool DCompTreeHost::IsFullCompNodeTree()
{
    if (!s_isFullCompNodeTreeInitialized)
    {
        s_isFullCompNodeTree = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableFullCompNodeTree);
        s_isFullCompNodeTreeInitialized = true;
    }
    return s_isFullCompNodeTree;
}

/* static */ bool
DCompTreeHost::VisualDebugTagsEnabled()
{
    if (!s_visualDebugTagsEnabledInitialized)
    {
        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        s_visualDebugTagsEnabled = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableVisualDebugTags);
        s_visualDebugTagsEnabledInitialized = true;
    }
    return s_visualDebugTagsEnabled;
}

/* static */ bool
DCompTreeHost::WUCShapesEnabled()
{
    if (!s_WUCShapesEnabledInitialized)
    {
        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        s_WUCShapesEnabled = runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableWUCShapes);
        s_WUCShapesEnabledInitialized = true;

        if (s_WUCShapesEnabled)
        {
            LOG_INFO_EX(L"WUCShapeVisuals ENABLED");
        }
        else
        {
            LOG_INFO_EX(L"WUCShapeVisuals DISABLED");
        }
    }
    return s_WUCShapesEnabled;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Static create method
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::Create(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager,
    _Outptr_ DCompTreeHost **ppDCompTreeHost
    )
{
    HRESULT hr = S_OK;

    *ppDCompTreeHost = NULL;

    DCompTreeHost *pDCompTreeHost = new DCompTreeHost(pGraphicsDeviceManager);

    *ppDCompTreeHost = pDCompTreeHost;
    RRETURN(hr);//RRETURN_REMOVAL
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Ctor
//
//----------------------------------------------------------------------------
DCompTreeHost::DCompTreeHost(
    _In_ WindowsGraphicsDeviceManager *pGraphicsDeviceManager)
    : m_pGraphicsDeviceManagerNoRef(pGraphicsDeviceManager)
    , m_strAnimationTrackingAppId()
    , m_animationTrackingAppIdSetOnDevice(FALSE)
    , m_hasNative8BitSurfaceSupport(FALSE)
    , m_pFrameRateTextFormat(NULL)
    , m_pFrameRateScratchBrush(NULL)
    , m_targetHwnd(NULL)
    , m_thumbnailState(DwmThumbnailState::Unfrozen)
    , m_primaryMonitorWidth(0)
    , m_primaryMonitorHeight(0)
    , m_atlasSizeHintWidth(0)
    , m_atlasSizeHintHeight(0)
    , m_useExplicitAtlasHint(false)
    , m_disableAtlas(FALSE)
    , m_isInitialized(false)
    , m_isCallbackThreadRegistered(false)
{
    XCP_WEAK(&m_pGraphicsDeviceManagerNoRef);
    ComputeAndCachePrimaryMonitorSize();
    EmptyRectF(&m_backgroundRect);
    m_offerTracker = make_xref<OfferTracker>();
    m_projectedShadowManager = std::make_shared<ProjectedShadowManager>(this);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Dtor
//
//----------------------------------------------------------------------------
DCompTreeHost::~DCompTreeHost()
{
    VERIFYHR(ReleaseResources(false /* shouldDeferClosingInteropCompostior */));
}

// In the device loss recovery case, do this after DCompTreeHost::ReleaseResources to allow the DebugDeviceFinalReleaseAsserter
// to test for Comp object leaks prior to applying "big hammer" RealClose() cleanup on interop compositor
void
DCompTreeHost::CloseAndReleaseInteropCompositor()
{
    if (m_spInteropCompositorPartner != nullptr)
    {
        IFCFAILFAST(m_spInteropCompositorPartner->ClearCallback());
        IFCFAILFAST(m_spInteropCompositorPartner->RealClose());
    }

    if (m_spInteropCompositorPartnerCallback != nullptr)
    {
        m_spInteropCompositorPartnerCallback->Disconnect();
        m_spInteropCompositorPartnerCallback = nullptr;
    }
    m_spInteropCompositorPartner = nullptr;
    m_frameRateVisual = nullptr;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Release all DComp resources, in cases like device loss recovery,
//      they will be recreated later. Called on UI thread.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::ReleaseResources(bool shouldDeferClosingInteropCompostior)
{
    if (m_systemBackdropBrush != nullptr)
    {
        CCoreServices* coreServicesNoRef = GetCoreServicesNoRef();
        // Keep transparent Xaml Islands by default
        if (coreServicesNoRef != nullptr && coreServicesNoRef->GetInitializationType() != InitializationType::IslandsOnly)
        {
            coreServicesNoRef->SetTransparentBackground(false);
        }

        m_systemBackdropBrush = nullptr;
    }

    m_hwndVisual.Reset();
    m_disconnectedHWndVisual.Reset();
    m_mockDCompDummyVisual1.Reset();
    m_mockDCompDummyVisual2.Reset();
    m_compNodeRootVisual.Reset();
    m_frameRateVisual.Reset();

    ReleaseInterface(m_pFrameRateTextFormat);
    ReleaseInterface(m_pFrameRateScratchBrush);

    m_projectedShadowManager->ReleaseResources();

    if (m_spMainSurfaceFactoryPartner3)
    {
        //we store the pointers of offered surfacefactories in a list
        //in OfferTracker, so we need to inform OfferTracker this SurfaceFactory
        //is being released
        m_offerTracker->DeleteReleasedSurfaceFactoryFromList(m_spMainSurfaceFactoryPartner3.Get());
    }

    DCompSurfaceFactoryManager* instance = DCompSurfaceFactoryManager::Instance();
    if (instance)
    {
        IFC_RETURN(instance->CleanupSurfaceFactoryMapForCurrentThread());
    }

    m_dcompObjectRegistry.ReleaseDCompResources();
    m_wucBrushManager.ReleaseDCompResources();
    m_sharedTransitionAnimations.ReleaseDCompResources();

    // It's possible in multi-view scenarios that there are still some UIElements with handoff (or handin) visuals at this point. There is no need
    // force close these since the WUC Compositor will be destroyed shortly, and do a full force close of all of its visuals at that point.
    // However, we still need to detach our DCompPropertyChangedListener's from any handoff visuals.
    if (!m_handOffVisualDataMap.empty())
    {
        for (const auto& handOffPair : m_handOffVisualDataMap)
        {
            ASSERT(handOffPair.second.handOffVisual != nullptr);

            // We must unregister the property listener before calling RealClose on the visual, otherwise we'll leak references on the visual due to
            // the visual going into "zombie" state.  DComp also takes a reference on the listener via registration which we need to explicitly release via unregistration.
            Microsoft::WRL::ComPtr<DCompPropertyChangedListener> listener = handOffPair.second.dcompPropertyChangedListener;
            if (listener)
            {
                listener->DetachFromHandOffVisual();
                listener->DetachFromPrependVisual();
                listener->DetachFromWUCClip();
            }
        }
    }

    // Remove all entries from m_hoverPointerSourceMap since the underlying composition property sets
    // will be disposed when we close the compositor.
    m_hoverPointerSourceMap.clear();

    // There is the possibility of objects leaking in DComp if we don't commit the device before releasing it
    const HRESULT commitMainDeviceHr = CommitMainDevice();
    if (commitMainDeviceHr == RO_E_CLOSED)
    {
        // If the device has already been closed, release these objects so we don't try to use them later
        // (e.g. in CloseAndReleaseInteropCompositor).  They're not useful to us anymore anyway.
        m_spInteropCompositorPartnerCallback = nullptr;
        m_spInteropCompositorPartner = nullptr;
    }

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    if (m_onecoreIslandAdapter.get() != nullptr)
    {
        ReleaseVisualTreeCompositionIslandAdapter();
    }
#endif

    //  Explicitly close the content bridge before closing the compositor.  If we close the compositor
    //  first then we lose some linkages that allow the bridge to be cleaned up properly.
    if (m_contentBridge != nullptr)
    {
        wrl::ComPtr<wf::IClosable> spClosable;
        m_contentBridge.As(&spClosable);
        IFCFAILFAST(spClosable->Close());

        m_contentBridge = nullptr;
    }

    if (!shouldDeferClosingInteropCompostior)
    {
        CloseAndReleaseInteropCompositor();
    }

    m_spMainSurfaceFactoryPartner = nullptr;
    m_spMainSurfaceFactoryPartner2 = nullptr;
    m_spMainSurfaceFactoryPartner3 = nullptr;
    m_spMainDevice = nullptr;
    m_spMainDeviceInternal = nullptr;
    m_spCompositor = nullptr;
    m_spCompositor2 = nullptr;
    m_spCompositor5 = nullptr;
    m_spCompositor6 = nullptr;
    m_spCompositorPrivate = nullptr;
    m_spCompositorInterop = nullptr;

    m_contentBridgeCW = nullptr;
    m_inprocIslandRootVisual = nullptr;

    m_compositionGraphicsDevice.Reset();

    m_hasNative8BitSurfaceSupport = FALSE;
    m_targetHwnd = NULL;
    m_coreWindowContentIsland = nullptr;
    m_offerTracker->Reset();
    m_animationTrackingAppIdSetOnDevice = FALSE;

    m_isInitialized = false;
    m_isCallbackThreadRegistered = false;

    // The background visual got released along with the background primitive. Clear the rect so that we'll make a new background
    // primitive the next time someone tries to attach the background visual.
    EmptyRectF(&m_backgroundRect);

    // Check the regkey again. This could be a test toggling modes
    s_visualDebugTagsEnabledInitialized = false;
    s_WUCShapesEnabledInitialized = false;

    return S_OK;
}

void DCompTreeHost::AbandonDCompObjectRegistry()
{
    m_dcompObjectRegistry.AbandonRegistry();
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Freezes the DWM's snapshot of the Jupiter window.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::FreezeDWMSnapshot()
{
    if (IsDwmSetWindowAttributePresent_PD_Replacement())
    {
        if (m_targetHwnd != NULL)
        {
            BOOL fFreeze = TRUE;
            IFC_RETURN(DwmSetWindowAttribute(m_targetHwnd, DWMWA_FREEZE_REPRESENTATION, &fFreeze, sizeof(fFreeze)));
            m_thumbnailState = DwmThumbnailState::Frozen;
        }

        // Note: This needs to happen after DWM snapshots are frozen, otherwise the snapshot will be blank.
        IFC_RETURN(CommitMainDevice());
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Unfreezes the DWM's snapshot of the Jupiter window.
//      Happens the first time we commit a frame after resuming.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::UnfreezeDWMSnapshotIfFrozen()
{
    // TODO: JCOMP: Do we need to wait for anything? GPU work? DWM notification?
    // TODO: JCOMP: Should this happen on the UI thread?

    if (IsDwmSetWindowAttributePresent_PD_Replacement())
    {
        // A suspend some time ago triggered this unfreeze. Check the freeze count to make sure a second
        // freeze hasn't happened since then.
        if (m_targetHwnd != NULL && m_thumbnailState == DwmThumbnailState::Frozen)
        {
            BOOL fFreeze = FALSE;
            IFC_RETURN(DwmSetWindowAttribute(m_targetHwnd, DWMWA_FREEZE_REPRESENTATION, &fFreeze, sizeof(fFreeze)));
            m_thumbnailState = DwmThumbnailState::Unfrozen;
        }
    }
    else
    {
        ASSERT(m_thumbnailState != DwmThumbnailState::Frozen);
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Offers DComp surfaces. Called on suspend.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::OfferResources()
{
    std::vector<IDCompositionSurfaceFactoryPartner3*> surfaceFactoryVector;
    //Acquire all the surfaceFactories, including the main SF and secondary SFs
    GetSurfaceFactoriesForCurrentThread(&surfaceFactoryVector);

    if ((surfaceFactoryVector.size() > 0) &&
        !m_offerTracker->IsOffered())
    {
        TraceOfferResourcesBegin();
        IFC_RETURN(m_offerTracker->OfferResources(&surfaceFactoryVector));

        IFC_RETURN(CommitMainDevice());
        TraceOfferResourcesEnd(FALSE /*_IsSurfaceBeingRendered*/);
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Reclaims DComp surfaces. Called on resume.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::ReclaimResources(_Out_ bool *pDiscarded)
{
    *pDiscarded = FALSE;
    if (m_offerTracker->IsOffered())
    {
        TraceReclaimResourcesBegin();
        BOOL discarded = FALSE;
        IFC_RETURN(m_offerTracker->ReclaimResources(&discarded));
        *pDiscarded = !!discarded;
        TraceReclaimResourcesEnd(discarded /*_WasDiscarded*/);
    }
    return S_OK;
}

//-------------------------------------------------------------------------------
//Synopsis:
//    Get the main and secondary SFs of the current thread, put them into the
//    vector, which will consumed by Offer and Reclaim
//-------------------------------------------------------------------------------
void DCompTreeHost::GetSurfaceFactoriesForCurrentThread(_Out_ std::vector<IDCompositionSurfaceFactoryPartner3*>* surfaceFactoryVector)
{
    //First, we put the main SurfaceFactory into the vector
    if (m_spMainSurfaceFactoryPartner3 != NULL) {
        surfaceFactoryVector->push_back(m_spMainSurfaceFactoryPartner3.Get());
    }

    //We also need to put secondary SFs into the vector
    DCompSurfaceFactoryManager::Instance()->GetSurfaceFactoriesForCurrentThread(surfaceFactoryVector);
}

// Uncomment LOADMOCKDEVICE and rebuild XAML to turn on auto-loading of MockDComp
// USE ONLY FOR DEBUGGING
//#define LOADMOCKDEVICE
#ifdef LOADMOCKDEVICE
typedef HRESULT (*STARTDETOURMOCKDCOMPDEVICEFUNC)();

void LoadMockDevice()
{
    static bool attemptedLoad = false;
    if (!attemptedLoad)
    {
        HMODULE mockDComp = LoadLibraryEx(L"MockDComp.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (mockDComp != nullptr)
        {
            STARTDETOURMOCKDCOMPDEVICEFUNC startDetourFunc = reinterpret_cast<STARTDETOURMOCKDCOMPDEVICEFUNC>(GetProcAddress(mockDComp, "StartDetourMockDCompDevice"));
            if (startDetourFunc != nullptr)
            {
                startDetourFunc();
            }
        }
        attemptedLoad = true;
    }
}
#endif


//----------------------------------------------------------------------------
//
//  Synopsis:
//      Ensure that we have created a DComp Device.  This will allow us to create
//      and manipulate all non-surface/graphics related objects.
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::EnsureDCompDevice() noexcept
{
    if (m_spMainDevice != nullptr) return S_OK;

    // We should not be holding onto any DComp resources.
    ASSERT(m_targetHwnd == NULL);
    ASSERT(!m_isInitialized);
    ASSERT(m_spMainSurfaceFactoryPartner == nullptr);
    ASSERT(m_spMainSurfaceFactoryPartner2 == nullptr);
    ASSERT(m_spMainSurfaceFactoryPartner3 == nullptr);
    ASSERT(m_spMainDeviceInternal == nullptr);
    ASSERT(m_spInteropCompositorPartnerCallback == nullptr);
    ASSERT(m_spInteropCompositorPartner == nullptr);
    ASSERT(m_spCompositor == nullptr);
    ASSERT(m_spCompositor2 == nullptr);
    ASSERT(m_spCompositor5 == nullptr);
    ASSERT(m_spCompositor6 == nullptr);
    ASSERT(m_spCompositorPrivate == nullptr);
    ASSERT(m_spCompositorInterop == nullptr);
    ASSERT(m_systemBackdropBrush == nullptr);

    #ifdef LOADMOCKDEVICE
    LoadMockDevice();
    #endif

    // The static factory can survive device loss. No need to create it again.
    if (m_easingFunctionStatics == nullptr)
    {
        m_easingFunctionStatics = ActivationFactoryCache::GetActivationFactoryCache()->GetCompositionEasingFunctionStatics();
    }

    IFC_RETURN(CreateWinRTInteropCompositionDevice(
        __uuidof(IDCompositionDesktopDevicePartner),
        reinterpret_cast<void**>(m_spMainDevice.GetAddressOf())));

    // Cache the WUComp::IInteropCompositorPartner implementation for
    // the ability to mark the interop compositor dirty during commit deferrals.
    IFC_RETURN(m_spMainDevice.As(&m_spInteropCompositorPartner));
    IFC_RETURN(m_spMainDevice.As(&m_spCompositor));
    IFC_RETURN(m_spCompositor.As(&m_spCompositor2));
    IFC_RETURN(m_spCompositor.As(&m_spCompositor5));
    IFC_RETURN(m_spCompositor.As(&m_spCompositor6));
    IFC_RETURN(m_spCompositor.As(&m_spCompositorPrivate));
    IFC_RETURN(m_spCompositor.As(&m_spCompositorInterop));

    if (XamlOneCoreTransforms::IsEnabled() && !DirectUI::DXamlServices::IsInBackgroundTask())
    {
        // When running in a background task, the visualTreeCompositionIslandAdapter
        // won't work correctly because the current CoreWindow does not have an HWND.
        CreateVisualTreeCompositionIslandAdapter();
    }

    auto coreServicesNoRef = GetCoreServicesNoRef();
    if ((coreServicesNoRef != nullptr) &&
        (coreServicesNoRef->GetInitializationType() == InitializationType::IslandsOnly))
    {
        wrl::ComPtr<ixp::ICompositorInternal> compositorInternal;
        if (SUCCEEDED(m_spMainDevice.As(&compositorInternal)))
        {
            // If we are being hosted by XAML islands we auto enable completion of
            // keyframe animations on screen occluded to prevent animation leaks.
            compositorInternal->put_AutoCompleteKeyFrameAnimationsOnScreenOccluded(true);
        }
    }

    m_spMainDevice->DisableD2DStatePreservation();

    // This should be kept in sync with the adjustment made in GetMaxTextureSize().
    m_spMainDevice->EnableWhitePixelOptimization(TRUE);

    // Cache the internal device interfaces used for animation/touch tracking.
    IFC_RETURN(m_spMainDevice.As(&m_spMainDeviceInternal));

    m_isInitialized = true;
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Called by a background thread whenever we create a new D3D11 device,
//      initially on start up and later during recovery from device loss.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::EnsureResources() noexcept
{
    IFC_RETURN(EnsureDCompDevice());

    if (m_compositionGraphicsDevice == nullptr && m_spCompositorInterop != nullptr)
    {
        CD3D11Device* d3d11DeviceInternal = m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();

        // If we don't have a device then we are most likely ticking while rendering is disabled.
        if (d3d11DeviceInternal != nullptr)
        {
            CD3D11SharedDeviceGuard guard;
            IFC_RETURN(d3d11DeviceInternal->TakeLockAndCheckDeviceLost(&guard));
            IFCFAILFAST(m_spCompositorInterop->CreateGraphicsDevice(d3d11DeviceInternal->GetDevice(&guard), m_compositionGraphicsDevice.ReleaseAndGetAddressOf()));
        }

        m_wucBrushManager.EnsureResources(&m_sharedTransitionAnimations, m_easingFunctionStatics.Get(), m_spCompositor.Get(), m_compositionGraphicsDevice.Get());
    }

    if (m_spMainSurfaceFactoryPartner == nullptr)
    {
        // The surface factory is created with the DComposition device, but can also be recreated
        // if we had a Graphics device lost situation.
        CD3D11Device *pD3D11DeviceInternal = m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();

        // If we don't have a device then we are most likely ticking while rendering is disabled.
        if (pD3D11DeviceInternal == nullptr) return S_OK;

        CD3D11SharedDeviceGuard guard;
        IFC_RETURN(pD3D11DeviceInternal->TakeLockAndCheckDeviceLost(&guard));

        // The D2D device is optional - it is required only for desktop, for phone we avoid D2D for performance reasons.
        // See WindowsGraphicsDeviceManager::InitializeExpensiveResources().
        ID2D1Device* pD2DDevice = pD3D11DeviceInternal->GetD2DDevice(&guard);

        // Create our surface factory
        {
            Microsoft::WRL::ComPtr<IDCompositionSurfaceFactory> pMainSurfaceFactory;
            if (nullptr != pD2DDevice)
            {
                IFC_RETURN(m_spMainDevice->CreateSurfaceFactory(static_cast<IUnknown*>(pD2DDevice), &pMainSurfaceFactory));
            }
            else
            {
                IFC_RETURN(m_spMainDevice->CreateSurfaceFactory(pD3D11DeviceInternal->GetDevice(&guard), &pMainSurfaceFactory));
            }
            VERIFYHR(pMainSurfaceFactory.As(&m_spMainSurfaceFactoryPartner));
            VERIFYHR(pMainSurfaceFactory.As(&m_spMainSurfaceFactoryPartner2));

            // XAML_DIM_DOWN: What do we lose here?
            // m_spMainSurfaceFactoryPartner3 not available on RS4
            IGNOREHR(pMainSurfaceFactory.As(&m_spMainSurfaceFactoryPartner3));
        }

        // Test for 8-bit DCompSurface support.
        //
        // Note that if Jupiter ends up using WARP in-proc while the system compositor
        // is using actual hardware device, we might end up in a situation where we
        // successfully create a surface that dwmcore cannot render with -- 9.2 and
        // 9.3 devices are not required to support sampling and blendable render
        // targets, and D2D (used by dwmcore to render) currently requires both.
        m_hasNative8BitSurfaceSupport = FALSE;
        if (pD3D11DeviceInternal->ShouldAttemptToUseA8Textures())
        {
            IDCompositionSurface *p8BitSupportCheckSurface = NULL;
            HRESULT createTextureHR = m_spMainSurfaceFactoryPartner->CreateSurface(
                1, // width
                1, // height
                DXGI_FORMAT_A8_UNORM, // 8-bit
                DXGI_ALPHA_MODE_PREMULTIPLIED,
                &p8BitSupportCheckSurface
                );
            if (SUCCEEDED(createTextureHR))
            {
                m_hasNative8BitSurfaceSupport = TRUE;
                ReleaseInterface(p8BitSupportCheckSurface);
            }
        }

        // RS5 Bug #17365115:  If something requires XAML to create an atlas surface before the first RenderWalk
        // (eg LoadedImageSurface), we may not have updated the atlas hint yet and will use the default atlas size.
        // Push any explicit atlas hint down into DComp just after creating the primary SurfaceFactory, to
        // guarantee we will use any hint that was set by the app early on app launch.
        UpdateExplicitAtlasHint();
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Releases the Graphics device resources associated with the Dcomp device
//
//----------------------------------------------------------------------------
void DCompTreeHost::ReleaseGraphicsResources()
{
    //we store the pointers of offered surfacefactories in a list
    //in OfferTracker
    m_offerTracker->DeleteReleasedSurfaceFactoryFromList(m_spMainSurfaceFactoryPartner3.Get());

    m_spMainSurfaceFactoryPartner = nullptr;
    m_spMainSurfaceFactoryPartner2 = nullptr;
    m_spMainSurfaceFactoryPartner3 = nullptr;
    ReleaseInterface(m_pFrameRateScratchBrush);

    m_compositionGraphicsDevice.Reset();
    m_wucBrushManager.ReleaseGraphicsDevice();
}

_Check_return_
HRESULT DCompTreeHost::CreateCompositionSurfaceForHandle(
    _In_ HANDLE swapChainHandle,
    _Outptr_ WUComp::ICompositionSurface** compositionSurface)
{
    *compositionSurface = nullptr;

    IFC_RETURN(EnsureDCompDevice());

    ComPtr<ixp::IExpCompositorInterop> expCompositorInterop;
    IFC_RETURN(m_spCompositorInterop->QueryInterface(IID_PPV_ARGS(expCompositorInterop.ReleaseAndGetAddressOf())));
    IFC_RETURN(expCompositorInterop->CreateCompositionSurfaceForHandle(swapChainHandle, compositionSurface));

    return S_OK;
}

// Creates a WinRT Composition compositor and returns its underlying legacy COM interop device.
_Check_return_
HRESULT DCompTreeHost::CreateWinRTInteropCompositionDevice(
    _In_ REFIID iid,
    _Out_ void **ppDevice)
{
    ixp::IInteropCompositorFactoryPartner* factoryPartnerNoRef = ActivationFactoryCache::GetActivationFactoryCache()->GetInteropCompositorFactoryPartner();

    ASSERT(m_spInteropCompositorPartnerCallback == nullptr);
    IFCFAILFAST(DCompInteropCompositorPartnerCallback::Create(this, reinterpret_cast<DCompInteropCompositorPartnerCallback**>(&m_spInteropCompositorPartnerCallback)));

    IFCFAILFAST(factoryPartnerNoRef->CreateInteropCompositor(nullptr /*pRenderingDevice*/, m_spInteropCompositorPartnerCallback, iid, ppDevice));

    return S_OK;
}

CCoreServices* DCompTreeHost::GetCoreServicesNoRef() const
{
    if (m_pGraphicsDeviceManagerNoRef != nullptr)
    {
        auto renderTargetNoRef = m_pGraphicsDeviceManagerNoRef->GetRenderTarget();
        if (renderTargetNoRef != nullptr)
        {
            return renderTargetNoRef->GetCoreServicesNoRef();
        }
    }

    return nullptr;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Caches the primary monitor size
//
//----------------------------------------------------------------------------
void DCompTreeHost::ComputeAndCachePrimaryMonitorSize()
{
    DISPLAY_DEVICE dd = { 0 };
    UINT primaryMonitorWidth = 0;
    UINT primaryMonitorHeight = 0;

    dd.cb = sizeof(dd);

    for (UINT i = 0; EnumDisplayDevices(nullptr, i, &dd, 0); i++)
    {
        if (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        {
            // Verify that this is not a disconnected device
            if ((dd.StateFlags & DISPLAY_DEVICE_DISCONNECT) != 0)
            {
                continue;
            }

            // Skip mirroring and accessibility drivers.
            if ((dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER) || (dd.StateFlags & DISPLAY_DEVICE_ACC_DRIVER))
            {
                continue;
            }

            DEVMODE dm = { 0 };
            dm.dmSize = sizeof(dm);
            if (EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm))
            {
                // Record the resolution of the Primary monitor
                if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
                {
                    primaryMonitorWidth = dm.dmPelsWidth;
                    primaryMonitorHeight = dm.dmPelsHeight;
                }
            }
        }

        dd.cb = sizeof(dd);
    }

    m_primaryMonitorWidth = primaryMonitorWidth;
    m_primaryMonitorHeight = primaryMonitorHeight;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Called on the UI thread after resources have been initialized to create the DComp
//      resources for the target HWnd. This is only get called for UWP app
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::SetTargetWindowUWP(HWND targetHwnd)
{
    ASSERT(m_spMainDevice != nullptr);

    if (targetHwnd != nullptr)
    {
        if (m_targetHwnd == nullptr)
        {
            // Create a composition target object that is bound to the window that is represented by the specified window handle.
            m_targetHwnd = targetHwnd;
            ASSERT(m_spMainDevice);

            if (!DesignerInterop::GetDesignerMode(DesignerMode::V2Only))
            {
                wrl::ComPtr<ixp::ICoreWindowSiteBridgeStatics> bridgeStatics;
                IFCFAILFAST(wf::GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(
                    RuntimeClass_Microsoft_UI_Content_CoreWindowSiteBridge).Get(), &bridgeStatics));

                wuc::ICoreWindow* coreWindow = DirectUI::DXamlServices::GetCurrentCoreWindowNoRef();
                IFCEXPECT_ASSERT_RETURN(coreWindow);
                // Create ICoreWindowSiteBridge
                IFC_RETURN(bridgeStatics->Create(
                    m_spCompositor.Get(),
                    coreWindow,
                    m_contentBridgeCW.ReleaseAndGetAddressOf()));

                IFCFAILFAST(m_contentBridgeCW.As(&m_contentBridge));

                wrl::ComPtr<IInspectable> islandAsInspectable;

                // Create a new island
                wrl::ComPtr<ixp::IContentIslandStatics> contentStatics;
                IFCFAILFAST(wf::GetActivationFactory(Microsoft::WRL::Wrappers::HStringReference(
                    RuntimeClass_Microsoft_UI_Content_ContentIsland).Get(), &contentStatics));

                wrl::ComPtr<ixp::IContainerVisual> containerRootVisual;
                IFCFAILFAST(m_spCompositor->CreateContainerVisual(&containerRootVisual));

                wrl::ComPtr<ixp::IVisual> rootVisual;
                IFCFAILFAST(containerRootVisual.As(&rootVisual));

                IFCFAILFAST(contentStatics->Create(rootVisual.Get(), &m_coreWindowContentIsland));

                if (auto contentRoot =
                        DXamlServices::GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot())
                {
                    IFCFAILFAST(contentRoot->SetContentIsland(m_coreWindowContentIsland.Get()));
                }

                // Connect DWLiftedDB with Island and input
                IFC_RETURN(m_contentBridgeCW->Connect(m_coreWindowContentIsland.Get()));
            }

            // Initialize the root DComp visual, and attach it to the hwnd target, or shared target in the
            // DesignModeV2 case.
            ASSERT(m_hwndVisual == nullptr);
            IFC_RETURN(SetTargetHelper());

            if (m_systemBackdropBrush != nullptr)
            {
                // Use the backdrop brush that was previously set while m_contentBridge was still null.
                IFC_RETURN(SetSystemBackdropBrush(m_systemBackdropBrush.Get()));

                m_systemBackdropBrush = nullptr;
            }
        }

        IFC_RETURN(UpdateAtlasHint());
    }

    return S_OK;
}

_Check_return_ HRESULT
DCompTreeHost::SetTargetHelper()
{
    IFCFAILFAST(GetCompositor()->CreateContainerVisual(m_hwndVisual.ReleaseAndGetAddressOf()));

    // Mark this visual as a standard visual root to differentiate it from the root of Xaml Islands.
    // This is necessary because using lifted IXP we now put standard visual roots into a composition
    // island and we need to differentiate because the trees can be different.
    {
        xref_ptr<WUComp::ICompositionObject> compositionObject;
        VERIFYHR(m_hwndVisual->QueryInterface(IID_PPV_ARGS(compositionObject.ReleaseAndGetAddressOf())));
        xref_ptr<WUComp::ICompositionPropertySet> compositionPropertySet;
        IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

        // Property sets don't support strings as property values, so store our tag as a scalar value.
        IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(L"_XAML_DEBUG_TAG_RootVisual").Get(), 1.0f));
    }

    // Insert dummy visuals to keep MockDComp masters the same.
    {
        wrl::ComPtr<WUComp::IVisualCollection> childCollection;
        IFCFAILFAST(m_hwndVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));

        wrl::ComPtr<WUComp::IContainerVisual> containerVisual;
        IFCFAILFAST(GetCompositor()->CreateContainerVisual(containerVisual.ReleaseAndGetAddressOf()));
        IFCFAILFAST(containerVisual.As(&m_mockDCompDummyVisual1));
        IFC_RETURN(childCollection->InsertAtTop(m_mockDCompDummyVisual1.Get()));
        IFCFAILFAST(GetCompositor()->CreateContainerVisual(containerVisual.ReleaseAndGetAddressOf()));
        IFCFAILFAST(containerVisual.As(&m_mockDCompDummyVisual2));
        IFC_RETURN(childCollection->InsertAtTop(m_mockDCompDummyVisual2.Get()));
    }

    wrl::ComPtr<WUComp::IVisual> hwndVisualAsVisual;
    IFCFAILFAST(m_hwndVisual.As(&hwndVisualAsVisual));
    IFC_RETURN(SetRootForCorrectContext(hwndVisualAsVisual.Get()));

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Reconnects the previously disconnected root DCOMP visual
//      to the appropriate target.
//
//-----------------------------------------------------------------------------
void DCompTreeHost::EnsureRootConnected()
{
    if (m_disconnectedHWndVisual != nullptr)
    {
        wrl::ComPtr<WUComp::IVisual> hwndVisualAsVisual;
        IFCFAILFAST(m_hwndVisual.As(&hwndVisualAsVisual));
        IFCFAILFAST(SetRootForCorrectContext(hwndVisualAsVisual.Get()));
        m_disconnectedHWndVisual.Reset();
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Disconnects the root DComp visual from the DCOMP
//      target if not already disconnected. A visual
//      with theme based background is connected as
//      root instead.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::DisconnectRoot(UINT32 backgroundColor, _In_ const XRECTF &backgroundRect)
{
    if (m_disconnectedHWndVisual == nullptr && HasDCompTarget())
    {
        wrl::ComPtr<WUComp::ICompositionBrush> brush;
        wrl::ComPtr<WUComp::ICompositionColorBrush> colorBrush;
        IFCFAILFAST(GetCompositor()->CreateColorBrush(colorBrush.ReleaseAndGetAddressOf()));
        IFCFAILFAST(colorBrush->put_Color(ColorUtils::GetWUColor(backgroundColor)));
        IFCFAILFAST(colorBrush.As(&brush));

        ComPtr<WUComp::ISpriteVisual> spriteVisual;
        IFCFAILFAST(GetCompositor()->CreateSpriteVisual(&spriteVisual));
        IFCFAILFAST(spriteVisual->put_Brush(brush.Get()));
        IFCFAILFAST(spriteVisual.As(&m_disconnectedHWndVisual));

        wfn::Vector2 size;
        size.X = backgroundRect.Width;
        size.Y = backgroundRect.Height;
        IFCFAILFAST(m_disconnectedHWndVisual->put_Size(size));

        IFC_RETURN(SetRootForCorrectContext(m_disconnectedHWndVisual.Get()));
    }

    return S_OK;
}

// Hooks up the visual as the root in the appropriate way:
// - via the m_onecoreIslandAdapter (OneCoreTransforms enabled - WCOS?)
// - via the inproc island
_Check_return_ HRESULT DCompTreeHost::SetRootForCorrectContext(_In_ WUComp::IVisual *visual)
{
// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    if (XamlOneCoreTransforms::IsEnabled())
    {
        // Running in OneCore Transforms mode, set the root of the target obtained from
        // calling Compositor->CreateTargetForCurrentView().
        // This will work for both cases, running as Top-Level or Component. We want this method
        // of connecting the host to the compoment to take precedence over the legacy
        // put_RootLegacyVisual when we are running in XamlOneCoreTransforms mode.
        // Note that when we're running as a background task we won't have a m_onecoreIslandAdapter
        if (visual != nullptr && m_onecoreIslandAdapter)
        {
            IFCFAILFAST(m_onecoreIslandAdapter->GetCompositionTargetNoRef()->put_Root(visual));
        }
    }
    else
#endif
    ixp::IContentIsland* compositionContent = nullptr;
    if(auto contentRoot =
         DXamlServices::GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot())
    {
        compositionContent = contentRoot->GetCompositionContentNoRef();
    }

    if (compositionContent != nullptr)
    {
        // For CoreWindow scenarios, the CompositionContent is also listening for the CoreWindow's closed event.
        // CompositionContent will get the notification first and close the entire visual tree, then Xaml will
        // exit its message loop and tear down the tree. Since CompositionContent already closed everything,
        // Xaml will get lots of RO_E_CLOSED errors. These are all safe to ignore. So tolerate RO_E_CLOSED if
        // we're also in the middle of tearing down the tree.
        ComPtr<ixp::IContentIsland2> contentIsland2;
        IFCFAILFAST(compositionContent->QueryInterface(IID_PPV_ARGS(&contentIsland2)));
        HRESULT hr = contentIsland2->put_Root(visual);
        if (FAILED(hr))
        {
            if ( hr != RO_E_CLOSED)
            {
                IFCFAILFAST(hr);
            }
        }

        m_inprocIslandRootVisual = visual;
    }

    return S_OK;//RRETURN_REMOVAL
}

bool DCompTreeHost::HasDCompTarget()
{
    ixp::IContentIsland* compositionContent = nullptr;
    if (auto contentRoot =
         DXamlServices::GetHandle()->GetContentRootCoordinator()->Unsafe_IslandsIncompatible_CoreWindowContentRoot())
    {
        compositionContent = contentRoot->GetCompositionContentNoRef();
    }

    return
        // CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
        (m_onecoreIslandAdapter != nullptr) || // TODO: do we need to change this to XamlOneCoreTransforms::IsEnabled() ?
#endif
        HasXamlIslandData()
        || (DesignerInterop::GetDesignerMode(DesignerMode::V2Only) && m_hwndVisual != nullptr)
        || compositionContent != nullptr;
}

// Called from CCoreServices::NWDrawTree() __before__ RenderWalk() and RenderElements().
_Check_return_ HRESULT DCompTreeHost::EnsureXamlIslandTargetRoots()
{
    for (auto& islandData : GetXamlIslandRenderData())
    {
        CXamlIslandRoot* xamlIslandRoot = islandData.first;
        XamlIslandRenderData& renderData = islandData.second;

        // Application has explicitly requested a CompositionIsland, and Xaml hasn't created the
        // island's content yet.
        if (xamlIslandRoot->GetContentRequested() &&
            (renderData.windowsPresentTarget == nullptr))
        {
            // Create a WindowsPresentTarget for this XamlIslandRoot
            const wf::Size xamlIslandRootSize = xamlIslandRoot->GetSize();

            IFC_RETURN(WindowsPresentTarget::CreateCompositedWindowlessPresentTarget(
                static_cast<XUINT32>(xamlIslandRootSize.Width),  // No fractional pixels
                static_cast<XUINT32>(xamlIslandRootSize.Height),
                nullptr /*m_pPresentSite*/,
                renderData.windowsPresentTarget.ReleaseAndGetAddressOf()));
        }
    }

    return S_OK;
}

// Called from CCoreServices::NWDrawTree() __after__ RenderWalk() and RenderElements().
_Check_return_ HRESULT DCompTreeHost::ConnectXamlIslandTargetRoots()
{
    // Hookup our Xaml presentation to their CompositionIslands
    for (auto& islandData : GetXamlIslandRenderData())
    {
        auto xamlIslandRoot = islandData.first;
        ASSERT(xamlIslandRoot);

        auto & renderData = islandData.second;

        // Get the tree and visual for this XamlIslandRoot
        HWCompTreeNode * compositionPeer = xamlIslandRoot->GetCompositionPeer();
        if (compositionPeer && !renderData.contentConnected)
        {
            WUComp::IVisual * wucVisual = compositionPeer->GetWUCVisual();

            // The visual may be NULL
            if (wucVisual && xamlIslandRoot->GetContentRequested())
            {
                FAIL_FAST_ASSERT(renderData.windowsPresentTarget != nullptr);

                auto content = xamlIslandRoot->GetContentIsland();

                // CONTENT-TODO: This assumes that only one Visual would be connected into the
                // Content.  If Xaml needs multiple Visuals, it would need to create its own
                // ContainerVisual.
                ComPtr<ixp::IContentIsland2> contentIsland2;
                IFCFAILFAST(content->QueryInterface(IID_PPV_ARGS(&contentIsland2)));
                IFC_RETURN(contentIsland2->put_Root(wucVisual));

                xamlIslandRoot->SetRootVisual(wucVisual);

                renderData.contentConnected = true;

                // If this is our first island and we need a Frame visual then add it here.  Note that at this point the
                // commit has already been done so we need to recommit.
                if (m_needsFrameRateVisual)
                {
                    ShowUIThreadCounters();
                    IFC_RETURN(CommitMainDevice());
                }

                continue;
            }
        }
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Checks the state of the D3D device and returns the result.
//
//-----------------------------------------------------------------------------
bool DCompTreeHost::CheckMainDeviceState()
{
    bool result = true;

    CD3D11Device* d3d11DeviceInternal = m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();
    if (d3d11DeviceInternal != nullptr)
    {
        result = !d3d11DeviceInternal->IsDeviceLost();
    }

    return result;
}

void DCompTreeHost::RegisterDCompAnimationCompletedCallbackThread()
{
    if (m_isInitialized && !m_isCallbackThreadRegistered)
    {
        IFCFAILFAST(m_spMainDevice->RegisterCallbackThread());
        m_isCallbackThreadRegistered = true;
    }
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      "Pre-Commit" the primary DComp device.  This flushes D2D work and gutters.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::PreCommitMainDevice()
{
    if (m_spMainDevice)
    {
        IFC_RETURN(m_spMainDevice->Flush());
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Commit DComp device.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::CommitMainDevice()
{
    if (m_spMainDevice)
    {
        ASSERT(m_pGraphicsDeviceManagerNoRef != nullptr);

        TraceCommitMainDeviceBegin();

        IFC_RETURN(m_spMainDevice->Commit());

        TraceCommitMainDeviceEnd();
    }

    return S_OK;
}

uint32_t DCompTreeHost::GetMaxTextureSize() const
{
    // Pad all allocations with 2 pixels for gutters, and 1 pixel for the "white pixel".
    const uint32_t padding = 3;
    return GetMainDevice()->GetMaxTextureSize() - padding;
}

// Requests a new render frame by calling CCoreServices::RequestMainDCompDeviceCommit()
_Check_return_ HRESULT
DCompTreeHost::RequestMainDCompDeviceCommit()
{
    if (m_pGraphicsDeviceManagerNoRef != nullptr)
    {
        CWindowRenderTarget* pWindowRenderTarget = m_pGraphicsDeviceManagerNoRef->GetRenderTarget();
        if (pWindowRenderTarget != nullptr)
        {
            IFC_RETURN(pWindowRenderTarget->RequestMainDCompDeviceCommit());
        }
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Submits a work item to notify completion of window layout when the current
//      frame finishes on the GPU.
//
//  TODO: m_targetHwnd is only set for UWP. We may need to customize this function to support desktop/ island. Task# 29571334
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::NotifyUWPWindowLayoutComplete(CWindowRenderTarget &renderTarget)
{
    if (m_targetHwnd != nullptr)
    {
        ComPtr<ICoreWindowResizeManagerStatics> resizeMgrStatics;
        ComPtr<ICoreWindowResizeManager> resizeMgr;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_UI_Core_CoreWindowResizeManager).Get(), &resizeMgrStatics));
        IFC_RETURN(resizeMgrStatics->GetForCurrentView(&resizeMgr));

        IFC_RETURN(resizeMgr->NotifyLayoutCompleted());

        TraceProcessNotifyWindowLayoutCompletedInfo1((UINT64)m_targetHwnd);
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates an WUComp::IContainerVisual.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::CreateContainerVisual(_Outptr_ WUComp::IContainerVisual **ppContainerVisual)
{
    xref_ptr<WUComp::IContainerVisual> spContainerVisual;
    IFC_RETURN(GetCompositor()->CreateContainerVisual(spContainerVisual.ReleaseAndGetAddressOf()));

    if (VisualDebugTagsEnabled())
    {
        xref_ptr<WUComp::ICompositionObject> compositionObject;
        VERIFYHR(spContainerVisual->QueryInterface(IID_PPV_ARGS(compositionObject.ReleaseAndGetAddressOf())));
        xref_ptr<WUComp::ICompositionPropertySet> compositionPropertySet;
        IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

        // This constant needs to be kept in sync with test code, see WinRTMockDComp.cpp
        static const wchar_t* s_contentVisualTag = L"_XAML_DEBUG_TAG_ContentVisual";

        // Property sets don't support strings as property values, so store our tag as a scalar value.
        IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(s_contentVisualTag).Get(), 0.0f));
    }

    *ppContainerVisual = spContainerVisual.detach();

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a DCompSurface object.
//
//      Note: DComp will pad out the surface with gutters as necessary. The
//      dimensions passed in should not include any gutters.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::CreateSurface(
    XUINT32 widthWithoutGutters,
    XUINT32 heightWithoutGutters,
    bool isOpaque,
    bool isAlphaMask,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    _Outptr_ DCompSurface **ppSurface
    )
{
    CD3D11Device *pD3D11Device = m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();

    // The device may be in offered state. In this case, we temporarily reclaim so that we can allocate a surface without error.
    std::unique_ptr<OfferTracker::UnofferRevoker> unofferRevoker;
    if (m_offerTracker->IsOffered() && m_spMainSurfaceFactoryPartner3)
    {
        IFC_RETURN(m_offerTracker->Unoffer(m_spMainSurfaceFactoryPartner3.Get(), &unofferRevoker));
    }

    IFC_RETURN(DCompSurface::Create(
        pD3D11Device,
        this,
        isOpaque,
        isAlphaMask,
        isVirtual,
        isHDR,
        requestAtlas,
        widthWithoutGutters,
        heightWithoutGutters,
        ppSurface
        ));

    if (unofferRevoker)
    {
        unofferRevoker.reset();
        IFC_RETURN(CommitMainDevice());
    }

    return S_OK;
}

// Helper function to allocate legacy hardware surface for a DCompSurface and resize it.
_Check_return_ HRESULT DCompTreeHost::EnsureLegacyDeviceSurface(_In_ DCompSurface* dcompSurface, unsigned int width, unsigned int height)
{
    // The device may be in offered state.  In this case, we temporarily reclaim so that we can allocate a surface without error.
    std::unique_ptr<OfferTracker::UnofferRevoker> unofferRevoker;
    if (m_offerTracker->IsOffered() && m_spMainSurfaceFactoryPartner3)
    {
        IFC_RETURN(m_offerTracker->Unoffer(m_spMainSurfaceFactoryPartner3.Get(), &unofferRevoker));
    }

    if (dcompSurface->GetIDCompSurface() == nullptr)
    {
        IFC_RETURN(dcompSurface->InitializeSurface(GetGraphicsDevice(), this, dcompSurface->IsVirtual()));
    }

    IFC_RETURN(dcompSurface->Resize(width, height));

    if (unofferRevoker)
    {
        unofferRevoker.reset();
        IFC_RETURN(CommitMainDevice());
    }

    return S_OK;
}

// Helper function to create a DCompSurface which only has a WinRT surface wrapper and no actual hardware surface.
xref_ptr<DCompSurface>
DCompTreeHost::CreateSurfaceWithNoHardware(bool isVirtual)
{
    return DCompSurface::CreateWithNoHardware(this, isVirtual);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      For the given device, obtains a SurfaceFactory, creating one if one
//      does not already exist, otherwise returning the SurfaceFactory
//      associated with this device.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::ObtainSurfaceFactory(
    _In_ IUnknown *pIUnk,
    _Outptr_ DCompSurfaceFactory **ppSurfaceFactory
    )
{
    IFC_RETURN(DCompSurfaceFactoryManager::Instance()->ObtainSurfaceFactory(this, pIUnk, GetMainDevice(), ppSurfaceFactory));

    return S_OK;
}

_Check_return_ HRESULT
DCompTreeHost::OnSurfaceFactoryCreated(_In_ DCompSurfaceFactory* newSurfaceFactory)
{
    //if the SurfaceFactory is a new one, we offer it if we are in the offered state
    if (m_offerTracker->IsOffered())
    {
        IFC_RETURN(m_offerTracker->OfferSurfaceFactory(newSurfaceFactory->GetSurfaceFactoryPartner()));
    }

    return S_OK;
}

_Check_return_ HRESULT
DCompTreeHost::GetSystemBackdropBrush(_Outptr_result_maybenull_ ABI::Windows::UI::Composition::ICompositionBrush** systemBackdropBrush)
{
    ASSERT(systemBackdropBrush != nullptr);

    *systemBackdropBrush = nullptr;

    if (m_contentBridge)
    {
        Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;

        IFC_RETURN(m_contentBridge.As(&compositionSupportsSystemBackdrop));
        IFC_RETURN(compositionSupportsSystemBackdrop->get_SystemBackdrop(systemBackdropBrush));
    }
    else
    {
        // Return potentially cached m_systemBackdropBrush.
        m_systemBackdropBrush.CopyTo(systemBackdropBrush);
    }

    return S_OK;
}

_Check_return_ HRESULT
DCompTreeHost::SetSystemBackdropBrush(_In_opt_ ABI::Windows::UI::Composition::ICompositionBrush* systemBackdropBrush)
{
    if (m_contentBridge)
    {
        Microsoft::WRL::ComPtr<ABI::Microsoft::UI::Composition::ICompositionSupportsSystemBackdrop> compositionSupportsSystemBackdrop;

        IFC_RETURN(m_contentBridge.As(&compositionSupportsSystemBackdrop));

#ifdef DBG
        Microsoft::WRL::ComPtr<ABI::Windows::UI::Composition::ICompositionBrush> systemBackdropBrushDbg;

        IGNOREHR(compositionSupportsSystemBackdrop->get_SystemBackdrop(&systemBackdropBrushDbg));
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DCompTreeHost::SetSystemBackdropBrush - old systemBackdroBrushp=%p, new systemBackdropBrush=%p\r\n", systemBackdropBrushDbg.Get(), systemBackdropBrush));
#endif

        IFC_RETURN(compositionSupportsSystemBackdrop->put_SystemBackdrop(systemBackdropBrush));
    }
    else
    {
        // Cache systemBackdropBrush for subsequent call to SetBackgroundBrush when m_contentBridge is set.
        m_systemBackdropBrush = systemBackdropBrush;
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Swaps the root of the compositor's DComp visual tree.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::SetRoot(_In_ HWCompTreeNodeWinRT* root)
{
    if (HasDCompTarget()
        && m_compNodeRootVisual.Get() != root->GetWUCVisual().get())
    {
        ASSERT(m_hwndVisual != nullptr);

        wrl::ComPtr<WUComp::IVisualCollection> childCollection;
        IFCFAILFAST(m_hwndVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));

        if (m_compNodeRootVisual != nullptr)
        {
            IFC_RETURN(childCollection->Remove(m_compNodeRootVisual.Get()));
        }

        m_compNodeRootVisual = root->GetWUCVisual();
        IFC_RETURN(childCollection->InsertAbove(m_compNodeRootVisual.Get(), m_mockDCompDummyVisual2.Get()));
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
// Stores the atlas size hint. The size hint will be applied to the dcomp
// device the next time SetTarget() is called.
//
//----------------------------------------------------------------------------
void DCompTreeHost::SetAtlasSizeHint(XUINT32 width, XUINT32 height)
{
    m_atlasSizeHintWidth = width;
    m_atlasSizeHintHeight = height;
    m_useExplicitAtlasHint = true;

    TraceExternalAtlasSizeOverrideInfo(
        reinterpret_cast<XUINT64>(this),
        width,
        height);
}

void DCompTreeHost::ResetAtlasSizeHint()
{
    m_atlasSizeHintWidth = 0;
    m_atlasSizeHintHeight = 0;
    m_useExplicitAtlasHint = false;
}

//----------------------------------------------------------------------------
//
// Sets the flag to disable the atlas. Atlasing once disabled
// stays disabled.
//
//----------------------------------------------------------------------------
void DCompTreeHost::DisableAtlas()
{
    m_useExplicitAtlasHint = true;
    m_disableAtlas = TRUE;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Dynamically adjusts the DComp atlas hint based on incoming surface request.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::UpdateAtlasHintForRequest(XUINT32 requestWidth, XUINT32 requestHeight)
{
    // Do not make any adjustment if someone has called SetAtlasSizeHint().
    // We do not want to override an explicitly set hint.
    if (!m_useExplicitAtlasHint)
    {
        // Do not make any adjustment on platforms that don't support querying
        // the primary monitor for its size (ie Phone).
        if (m_primaryMonitorWidth != 0 && m_primaryMonitorHeight != 0)
        {
            // The policy to adjust the atlas hint is based on 2 factors:
            // 1) The request must not fit in DComp's default atlas size.
            // 2) The request must fit in an atlas sized to the primary monitor size (+2 for gutters).
            //
            // In order to implement the first part of this policy, we've copied the algorithm
            // DComp uses to compute its default atlas size, as well as the large surface threshold.
            // DComp's algorithm for computing the default atlas size goes as follows:
            // Use 3/4 of primary monitor size, rounded up to nearest multiple of 64, and clamped to 1024 in each dimension.
            // The large surface threshold algorithm goes as follows:
            // If the request fits in the atlas, and also fits in 1/2 of the atlas in either dimension, then it is atlased.
            // TODO: JCOMP: In vNext we really should remove this and get an API from DComp.
            const XUINT32 alignment = 64;
            XUINT32 defaultDCompAtlasWidth = (m_primaryMonitorWidth * 75) / 100;
            XUINT32 defaultDCompAtlasHeight = (m_primaryMonitorHeight * 75) / 100;
            defaultDCompAtlasWidth = ((defaultDCompAtlasWidth + alignment-1) & ~(alignment-1));
            defaultDCompAtlasHeight = ((defaultDCompAtlasHeight + alignment-1) & ~(alignment-1));
            defaultDCompAtlasWidth = std::min(defaultDCompAtlasWidth, static_cast<XUINT32>(1024));
            defaultDCompAtlasHeight = std::min(defaultDCompAtlasHeight, static_cast<XUINT32>(1024));

            // Check #1:  Would it not fit in DComp's default atlas size?
            if (requestWidth > defaultDCompAtlasWidth ||
                requestHeight > defaultDCompAtlasHeight ||
                  (requestWidth > defaultDCompAtlasWidth/2 &&
                  requestHeight > defaultDCompAtlasHeight/2))
            {
                // Check #2:  Would it fit in screen-size?
                if (requestWidth <= m_primaryMonitorWidth &&
                    requestHeight <= m_primaryMonitorHeight &&
                      (requestWidth <= m_primaryMonitorWidth/2 ||
                       requestHeight <= m_primaryMonitorHeight/2))
                {
                    // Switch the atlas size to primary + 2.
                    // Only update if we haven't already done so.
                    if (m_atlasSizeHintWidth != m_primaryMonitorWidth + 2 &&
                        m_atlasSizeHintHeight != m_primaryMonitorHeight + 2)
                    {
                        m_atlasSizeHintWidth = m_primaryMonitorWidth + 2;
                        m_atlasSizeHintHeight = m_primaryMonitorHeight + 2;
                        IFC_RETURN(UpdateAtlasHint());
                    }
                }
            }
        }
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Method to ensure that the cached explicit atlas hint
//      size is actually communicated to DComp.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::UpdateExplicitAtlasHint()
{
    if (m_useExplicitAtlasHint || m_disableAtlas)
    {
        IFC_RETURN(UpdateAtlasHint());
    }
    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to hint the DComp atlas size to our cached hint size.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::UpdateAtlasHint()
{
    // Pass the atlas size hint (if set) to the device.
    if (m_disableAtlas ||
       (m_atlasSizeHintWidth && m_atlasSizeHintHeight))
    {
        XUINT32 atlasSizeHintWidth = m_atlasSizeHintWidth;
        XUINT32 atlasSizeHintHeight = m_atlasSizeHintHeight;

        // Atlas disabled flag takes precedence over explicit hint size. Atlases are disabled by hinting their size to 0x0.
        // This will make things slow but memory is a bigger concern in some cases like background tasks. Atlasing also
        // must be disabled when rendering RTBs, otherwise the RTB's subrect could share the same atlas texture as something
        // in the subtree being rendered, in which case that atlas texture is both being read from and written to, which
        // isn't something that D3D allows.
        if (m_disableAtlas)
        {
            atlasSizeHintWidth = 0;
            atlasSizeHintHeight = 0;
        }

        Microsoft::WRL::ComPtr<IDCompositionDeviceInternal> pDeviceInternal;
        IFC_RETURN(m_spMainDevice.As(&pDeviceInternal));
        IFC_RETURN(pDeviceInternal->HintSize(atlasSizeHintWidth, atlasSizeHintHeight));
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Updates DComp debug settings.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::UpdateDebugSettings(
    bool isFrameRateCounterEnabled
)
{
    if (!isFrameRateCounterEnabled)
    {
        HideUIThreadCounters();
    }
    else
    {
        ShowUIThreadCounters();
    }

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the UI thread frame counters.
//
//------------------------------------------------------------------------------
void DCompTreeHost::UpdateUIThreadCounters(
    XUINT32 uiThreadFrameRate,
    XFLOAT uiThreadCPUTime)
{
    // If there is a case where we can't create the frame rate visual, don't crash.  Just
    // don't try to render it.
    if (!m_frameRateVisual) return;

    wrl::ComPtr<DCompSurface> frameRateSurface;
    IFCFAILFAST(CreateFrameRateSurface(
        uiThreadFrameRate,
        uiThreadCPUTime,
        frameRateSurface.ReleaseAndGetAddressOf()));

    wrl::ComPtr<WUComp::ICompositionBrush> brush;
    wrl::ComPtr<WUComp::ICompositionSurfaceBrush> surfaceBrush;
    IFCFAILFAST(GetCompositor()->CreateSurfaceBrush(&surfaceBrush))
    IFCFAILFAST(surfaceBrush->put_HorizontalAlignmentRatio(0));
    IFCFAILFAST(surfaceBrush->put_VerticalAlignmentRatio(0));
    IFCFAILFAST(surfaceBrush->put_Stretch(WUComp::CompositionStretch::CompositionStretch_Fill));
    IFCFAILFAST(surfaceBrush->put_Surface(frameRateSurface->GetWinRTSurface()));
    IFCFAILFAST(surfaceBrush.As(&brush));

    ComPtr<WUComp::ISpriteVisual> spriteVisual;
    IFCFAILFAST(m_frameRateVisual.As(&spriteVisual));
    IFCFAILFAST(spriteVisual->put_Brush(brush.Get()));

    wfn::Vector2 size;
    size.X = c_FrameRateTotalWidth;
    size.Y = c_FrameRateHeight;
    IFCFAILFAST(m_frameRateVisual->put_Size(size));
}

void DCompTreeHost::ShowUIThreadCounters()
{
    m_needsFrameRateVisual = false;  // We are setting up the frame visual here.

    if (m_frameRateVisual) return; // Already have one and don't need another;

    wrl::ComPtr<WUComp::IContainerVisual> hostVisual = m_hwndVisual;

    if (!hostVisual && m_islandRenderData.size() > 0)
    {
        // We are running in an island scenario so we will put the counters on the first island assuming that the first island is a window for a desktop app.
        // If we have a scenario where the Application object can request frame rate counters in a traditional island application or we need to do something smarter
        // in a multiwindow single-thread application, then we should actually design the feature.
        auto iter = m_islandRenderData.begin();
        if (iter != m_islandRenderData.end())
        {
            wrl::ComPtr<WUComp::IVisual> rootVisual = iter->first->GetRootVisual();
            LPCWSTR ROOT_HOST_TAG = L"_XAML_Root_Frame_Host";

            // If we don't have a root visual then this frame is being rendered before have completely set up our environment This can happen on the first
            // frame or a frame where all existing islands are torn down and a new one is added.  If so, we have to wait until the visual is setup as
            // our root visual.
            if (!rootVisual)
            {
                m_needsFrameRateVisual = true;
                return;
            }

            // We want to add the frame rate visual as a sibling to the root visual, but in our initial configuration, we have added the Xaml root visual as
            // the root of the content island.  So we start by attempting to get the parent of the root visual and if null is returned, we know that our
            // our root visual is directly under the content island and so we create our own container visual to be used as the content island root and
            // move our root visual to under that container with the frame rate visual.  Future attempts to get the parent of the root visual will return
            // this container visual and we can just use it.
            IFCFAILFAST(rootVisual->get_Parent(hostVisual.ReleaseAndGetAddressOf()));

            if (hostVisual)
            {
                // Make sure that this is our host visual in case something changes with DComp and we for some reason get a parent that isn't what we expect.
                wrl::ComPtr<WUComp::ICompositionObject> compositionObject;
                wrl::ComPtr<WUComp::ICompositionPropertySet> compositionPropertySet;
                IFCFAILFAST(hostVisual.As(&compositionObject));
                IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

                WUComp::CompositionGetValueStatus status;
                float dummy;
                IFCFAILFAST(compositionPropertySet->TryGetScalar(wrl::Wrappers::HStringReference(ROOT_HOST_TAG).Get(), &dummy, &status));
                if (status != WUComp::CompositionGetValueStatus_Succeeded)
                {
                    // Not the one we expected.
                    hostVisual.Reset();
                }
            }

            if (!hostVisual)
            {
                // We don't have the host visual so create one.
                ComPtr<ixp::IContentIsland2> contentIsland2;
                IFCFAILFAST(iter->first->GetContentIsland()->QueryInterface(IID_PPV_ARGS(&contentIsland2)));
                IFCFAILFAST(contentIsland2->put_Root(nullptr));
                IFCFAILFAST(GetCompositor()->CreateContainerVisual(hostVisual.ReleaseAndGetAddressOf()));

                // mark this as our FrameCount/Root host.
                wrl::ComPtr<WUComp::ICompositionObject> compositionObject;
                wrl::ComPtr<WUComp::ICompositionPropertySet> compositionPropertySet;
                IFCFAILFAST(hostVisual.As(&compositionObject));
                IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));
                IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(ROOT_HOST_TAG).Get(), 1.0f));

                // Update the tree structure
                wrl::ComPtr<WUComp::IVisualCollection> childCollection;
                IFCFAILFAST(hostVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));
                IFCFAILFAST(childCollection->InsertAtTop(rootVisual.Get()));

                rootVisual.Reset();
                IFCFAILFAST(hostVisual.As(&rootVisual))
                IFCFAILFAST(contentIsland2->put_Root(rootVisual.Get()));
            }
        }
    }

    // If we can't find a host visual, we can't show the counters;
    if (hostVisual)
    {
        wrl::ComPtr<WUComp::ISpriteVisual> spriteVisual;
        IFCFAILFAST(GetCompositor()->CreateSpriteVisual(spriteVisual.ReleaseAndGetAddressOf()));
        IFCFAILFAST(spriteVisual.As(&m_frameRateVisual));

        wrl::ComPtr<WUComp::IVisualCollection> childCollection;
        IFCFAILFAST(hostVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));
        IFCFAILFAST(childCollection->InsertAtTop(m_frameRateVisual.Get()));
    }
}

void DCompTreeHost::HideUIThreadCounters()
{
    if (!m_frameRateVisual) return; // Don't have a frame rate visual so don't need to hide it.

    wrl::ComPtr<WUComp::IContainerVisual> parentVisual;
    IFCFAILFAST(m_frameRateVisual->get_Parent(parentVisual.ReleaseAndGetAddressOf()));

    if (parentVisual)
    {
        wrl::ComPtr<WUComp::IContainerVisual> rootVisual;
        IFCFAILFAST(parentVisual.As(&rootVisual));

        wrl::ComPtr<WUComp::IVisualCollection> childCollection;
        IFCFAILFAST(rootVisual->get_Children(childCollection.ReleaseAndGetAddressOf()));
        IFCFAILFAST(childCollection->Remove(m_frameRateVisual.Get()));
    }
    m_frameRateVisual.Reset();
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Draws a counter using D2D.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DrawCounter(
    _In_ const D2D1_RECT_F *pRect,
    XUINT32 counter,
    _In_ ID2D1DeviceContext *pD2DDeviceContext,
    _In_ ID2D1Brush *pD2DBrush,
    _In_ IDWriteTextFormat *pDWriteTextFormat
    )
{
    WCHAR buffer[4] = {0};

    // Clamp to 3-digit number.
    counter = MIN(counter, 999);

    // Iterate over the digits and pad them with zeros
    IFCCHECK_RETURN(swprintf_s(buffer, 4, L"%03d", counter));

    // Draw the counter.
    pD2DDeviceContext->DrawText(
        buffer,
        3,
        pDWriteTextFormat,
        pRect,
        pD2DBrush
        );

    return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new DComp primitive to draw the current counters
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::CreateFrameRateSurface(
    XUINT32 uiThreadFrameRate,
    XFLOAT uiThreadCPUTime,
    _Outptr_ DCompSurface** ppFrameRateSurface
    )
{
    HRESULT hr = S_OK;

    DCompSurface *pFrameRateSurface = NULL;
    IDXGISurface *pDXGISurface = NULL;
    ID2D1DeviceContext *pD2DDeviceContext = NULL;
    ID2D1Bitmap1 *pSurfaceBitmap = NULL;
    bool callEndDrawOnD2DContext = false;

    //
    // Create a surface for the frame-rate counter.
    //
    const XRECTF totalRect = {
        0.0f,
        0.0f,
        c_FrameRateTotalWidth,
        c_FrameRateHeight
    };

    IFC(CreateSurface(
        static_cast<XUINT32>(totalRect.Width),
        static_cast<XUINT32>(totalRect.Height),
        TRUE, // isOpaque
        FALSE, // isAlphaMask
        FALSE, // isVirtual
        false, // isHDR
        true,  // requestAtlas
        &pFrameRateSurface
        ));

    IFC(EnsureTextFormat());

    //
    // Update the frame-rate counter surface.
    //
    {
        const XRECT updateRect = {
            static_cast<XUINT32>(totalRect.X),
            static_cast<XUINT32>(totalRect.Y),
            static_cast<XUINT32>(totalRect.Width),
            static_cast<XUINT32>(totalRect.Height)
        };

        XPOINT drawOffset;
        hr = pFrameRateSurface->BeginDraw(
            &updateRect,
            __uuidof(ID2D1DeviceContext),
            reinterpret_cast<IUnknown**>(&pD2DDeviceContext),
            &drawOffset
            );
        if (hr == E_NOINTERFACE)
        {
            //
            // On phone we delay-create D2D resources, so DComp only has a IDXGISurface and we need to
            // make a context and wrap a D2D surface around the IDXGISurface we get back.
            //

            IFC(pFrameRateSurface->BeginDraw(
                &updateRect,
                __uuidof(IDXGISurface),
                reinterpret_cast<IUnknown**>(&pDXGISurface),
                &drawOffset
                ));

            IFC(m_pGraphicsDeviceManagerNoRef->EnsureD2DResources());

            CD3D11Device* pD3DDevice = m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();

            CD3D11SharedDeviceGuard guard;
            IFC(pD3DDevice->TakeLockAndCheckDeviceLost(&guard));
            IFC(pD3DDevice->GetD2DDevice(&guard)->CreateDeviceContext(
                D2D1_DEVICE_CONTEXT_OPTIONS_ENABLE_MULTITHREADED_OPTIMIZATIONS,
                &pD2DDeviceContext
                ));

            IFC(pD2DDeviceContext->CreateBitmapFromDxgiSurface(pDXGISurface, NULL, &pSurfaceBitmap));
            pD2DDeviceContext->SetTarget(pSurfaceBitmap);
            pD2DDeviceContext->BeginDraw();
            callEndDrawOnD2DContext = true;
        }
        else
        {
            IFC(hr);
        }

        pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());

        XPOINTF offset = { static_cast<XFLOAT>(drawOffset.x), static_cast<XFLOAT>(drawOffset.y) };

        if (m_pFrameRateScratchBrush == NULL)
        {
            IFC(pD2DDeviceContext->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White),
                &m_pFrameRateScratchBrush
                ));
        }

        // Clear to black.
        pD2DDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));

        // Draw the frame-rate counter.
        {
            D2D1_RECT_F frameCountRect = D2D1::RectF(
                offset.x + c_FrameRateWhiteSpaceWidth,
                offset.y,
                offset.x + c_FrameRateWhiteSpaceWidth + c_FrameRateFrameCountWidth,
                offset.y + c_FrameRateHeight
                );

            IFC(DrawCounter(
                &frameCountRect,
                uiThreadFrameRate,
                pD2DDeviceContext,
                m_pFrameRateScratchBrush,
                m_pFrameRateTextFormat
                ));
        }

        // Draw the frame-time counter.
        {
            D2D1_RECT_F cpuTimeRect = D2D1::RectF(
                offset.x + 2 * c_FrameRateWhiteSpaceWidth + c_FrameRateFrameCountWidth,
                offset.y,
                offset.x + 2 * c_FrameRateWhiteSpaceWidth + c_FrameRateFrameCountWidth + c_FrameRateCpuTimeWidth,
                offset.y + c_FrameRateHeight
                );

            IFC(DrawCounter(
                &cpuTimeRect,
                static_cast<XUINT32>(DoubleUtil::Round(uiThreadCPUTime, 0)),
                pD2DDeviceContext,
                m_pFrameRateScratchBrush,
                m_pFrameRateTextFormat
                ));
        }

        if (callEndDrawOnD2DContext)
        {
            pD2DDeviceContext->EndDraw();
            callEndDrawOnD2DContext = false;
        }
        IFC(pFrameRateSurface->EndDraw());
    }

    *ppFrameRateSurface = pFrameRateSurface;
    pFrameRateSurface = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pDXGISurface);
    ReleaseInterfaceNoNULL(pSurfaceBitmap);
    ReleaseInterfaceNoNULL(pFrameRateSurface);
    if (callEndDrawOnD2DContext && (pD2DDeviceContext != NULL))
    {
        pD2DDeviceContext->EndDraw();
    }
    ReleaseInterfaceNoNULL(pD2DDeviceContext);

    RRETURN(hr);
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures a IDWriteTextFormat exists to draw frame-rate counter text.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::EnsureTextFormat()
{
    if (m_pFrameRateTextFormat == nullptr)
    {
        Microsoft::WRL::ComPtr<IDWriteFactory> pDWriteFactory;
        IFC_RETURN(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &pDWriteFactory));

        static const WCHAR msc_fontName[] = L"Lucida Console";
        static const FLOAT msc_fontSize = 20;

        // Create a DirectWrite text format object.
        IFC_RETURN(pDWriteFactory->CreateTextFormat(
            msc_fontName,
            NULL,
            DWRITE_FONT_WEIGHT_NORMAL,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            msc_fontSize,
            L"", //locale
            &m_pFrameRateTextFormat
            ));

        // Center the text horizontally and vertically.
        m_pFrameRateTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
        m_pFrameRateTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Return if animation/touch tracking is enabled.
//
//------------------------------------------------------------------------
bool DCompTreeHost::IsAnimationTrackingEnabled()
{
    // dcompi.dll gives us a local channel, which doesn't support animation tracking.
    return false;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the beginning of an animation scenario for animation tracking.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::AnimationTrackingScenarioBegin(_In_ const AnimationTrackingScenarioInfo* pScenarioInfo)
{
    return S_OK;

    // Removing for change: https://microsoft.visualstudio.com/OS/_git/os.2020/pullrequest/5624071
    // IFC_RETURN(EnsureAnimationTrackingAppId());

    // IFCCHECK_RETURN(m_spMainDeviceInternal);

    // DCOMPOSITION_TELEMETRY_ANIMATION_SCENARIO_INFO info;
    // ZeroMemory(&info, sizeof(info));
    // info.version = DCOMPOSITION_TELEMETRY_ANIMATION_SCENARIO_INFO_VERSION;
    // info.scenarioPriority = pScenarioInfo->priority;
    // info.qpcInitiate = pScenarioInfo->qpcInitiate;
    // info.qpcInput = pScenarioInfo->qpcInput;
    // info.msIntendedDuration = pScenarioInfo->msIntendedDuration;
    // info.pszScenarioName = pScenarioInfo->scenarioName;
    // info.pszScenarioDetails = pScenarioInfo->scenarioDetails;

    // IFC_RETURN(m_spMainDeviceInternal->TelemetryAnimationScenarioBegin(&info));

    // return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the begining of a sub-animation for animation tracking.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::AnimationTrackingScenarioReference(
    _In_opt_ const GUID* pScenarioGuid,
    XUINT64 uniqueKey)
{
    return S_OK;

    // Removing for change: https://microsoft.visualstudio.com/OS/_git/os.2020/pullrequest/5624071
    // IFC_RETURN(EnsureAnimationTrackingAppId());

    // IFCCHECK_RETURN(m_spMainDeviceInternal);

    // IFC_RETURN(m_spMainDeviceInternal->TelemetryAnimationScenarioReference(pScenarioGuid, uniqueKey));

    // return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Signals the end of an sub-animation for animation tracking.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT DCompTreeHost::AnimationTrackingScenarioUnreference(
    _In_opt_ const GUID* pScenarioGuid,
    XUINT64 uniqueKey)
{
    return S_OK;

    // Removing for change: https://microsoft.visualstudio.com/OS/_git/os.2020/pullrequest/5624071
    // IFC_RETURN(EnsureAnimationTrackingAppId());

    // IFCCHECK_RETURN(m_spMainDeviceInternal);

    // IFC_RETURN(m_spMainDeviceInternal->TelemetryAnimationScenarioUnreference(pScenarioGuid, uniqueKey));

    // return S_OK;
}

//------------------------------------------------------------------------------
//
//  Synopsis:
//      Ensures that we have captured the application ID string to be used for
//      animation/touch tracking and it is set on the current DComp devices.
//
//------------------------------------------------------------------------------
_Check_return_ HRESULT
DCompTreeHost::EnsureAnimationTrackingAppId()
{
    // If Storyboard::Begin was called as part of app startup, we could end up here while the DComp device was still being created
    // in the background. We need to call out to the DComp device, so wait for device creation first.
    IFCFAILFAST(EnsureDCompDevice());

    if (m_strAnimationTrackingAppId.IsNullOrEmpty())
    {
        // Try to get the modern app id. If it fails or succeeds without
        // returning the app id (when we are not in a modern app) fallback
        // to using the process image name with the patih stripped.
        IGNOREHR(gps->GetProcessModernAppId(&m_strAnimationTrackingAppId));
        if (m_strAnimationTrackingAppId.IsNullOrEmpty())
        {
            IFC_RETURN(gps->GetProcessImageName(&m_strAnimationTrackingAppId));
            ASSERT(!m_strAnimationTrackingAppId.IsNullOrEmpty());
        }
    }

    // Make sure that the internal device is available.
    IFCCHECK_RETURN(m_spMainDeviceInternal);

    if (!m_animationTrackingAppIdSetOnDevice)
    {
        XUINT16 cchAppId = static_cast<XUINT16>(std::min(static_cast<XUINT32>(XUINT16_MAX), m_strAnimationTrackingAppId.GetCount()));
        IFC_RETURN(m_spMainDeviceInternal->TelemetrySetApplicationId(cchAppId, m_strAnimationTrackingAppId.GetBuffer()));
        m_animationTrackingAppIdSetOnDevice = TRUE;
    }

    return S_OK;
}

DependencyObjectDCompRegistry* DCompTreeHost::GetDCompObjectRegistry()
{
    return &m_dcompObjectRegistry;
}

WUCBrushManager* DCompTreeHost::GetWUCBrushManager()
{
    return &m_wucBrushManager;
}

CD3D11Device* DCompTreeHost::GetGraphicsDevice() const
{
    return m_pGraphicsDeviceManagerNoRef->GetGraphicsDevice();
}

_Check_return_ HRESULT
DCompTreeHost::WaitForCommitCompletion()
{
    if (m_spMainDevice)
    {
        IFC_RETURN(m_spMainDevice->WaitForCommitCompletion());
    }
    return S_OK;
}

bool DCompTreeHost::HasInteropCompositorPartner() const
{
    return m_spInteropCompositorPartner != nullptr;
}

// Start/Stop tracking the effective visibility of this element.
// Note carefully, effective visibility for implicit Show/Hide animations has special semantics.
// See additional notes in CUIElement::ComputeEffectiveVisibility();
void DCompTreeHost::SetTrackEffectiveVisibility(_In_ CUIElement* uiElement, bool track)
{
    auto itFind = m_effectiveVisibilityMap.find(uiElement);
    if (itFind != m_effectiveVisibilityMap.end())
    {
        if (!track)
        {
            m_effectiveVisibilityMap.erase(itFind);
        }
    }
    else
    {
        if (track)
        {
            m_effectiveVisibilityMap.emplace(uiElement, uiElement->ComputeEffectiveVisibility());
        }
    }
}

bool DCompTreeHost::IsTrackingEffectiveVisibility(_In_ CUIElement* uiElement) const
{
    auto itFind = m_effectiveVisibilityMap.find(uiElement);
    return (itFind != m_effectiveVisibilityMap.end());
}

// Do processing for implicit Show/Hide animations, done once per frame, just before the RenderWalk.
// This function has two responsibilities:
// 1) Recompute the effective visibility of each element with an implicit Show/Hide animation,
//    and trigger the appropriate animation when effective visibility changes.
// 2) For every element that's being "kept visible" due to implicit Hide animation running in it subtree,
//    recompute if this element still needs to be kept visible, and clean it up if no longer being kept visible.
void DCompTreeHost::UpdateImplicitShowHideAnimations()
{
    // First update the effective visibility of any modern panels that have live child items with implicit animations.
    // We'll use this effective visibility information to help decide if a container should actually play its animation.
    std::vector<CUIElement*> modernPanelsToUpdate;
    for (auto it = m_effectiveVisibilityMap.begin(); it !=  m_effectiveVisibilityMap.end(); it++)
    {
        CUIElement* uiElement = it->first;

        // It's only necessary to evaluate if the element is in the live tree
        if (uiElement->IsActive())
        {
            if (uiElement->GetIsItemContainer())
            {
                // OK we found an item container for a modern panel.
                // Put the panel in a collection which we'll run through below.
                CUIElement* parentPanel = static_cast<CUIElement*>(uiElement->GetParentInternal());
                ASSERT(parentPanel != nullptr);

                auto itFind = std::find(modernPanelsToUpdate.begin(), modernPanelsToUpdate.end(), parentPanel);
                if (itFind == modernPanelsToUpdate.end())
                {
                    modernPanelsToUpdate.push_back(parentPanel);
                }
            }
        }
    }

    // Now that we have the minimal collection of panels to evaluate, update their effective visibility.
    for (auto it = modernPanelsToUpdate.begin(); it != modernPanelsToUpdate.end(); it++)
    {
        UpdateEffectiveVisibilityTracker(*it);
    }

    // We'll collect these elements and fire events for them at the end. That way we're not calling out to app code in
    // between computing effective visibility for elements.
    std::vector<CUIElement*> shownElements;
    std::vector<CUIElement*> hiddenElements;

    // Recompute effective visibility of each element with implicit Show/Hide animation
    for (auto it = m_effectiveVisibilityMap.begin(); it !=  m_effectiveVisibilityMap.end(); it++)
    {
        CUIElement* uiElement = it->first;

        EffectiveVisibilityInfo effectiveVisibility = ComputeEffectiveVisibility(uiElement);

        if (effectiveVisibility != it->second)
        {
            it->second = effectiveVisibility;

            bool shouldPlay = uiElement->IsActive();

            if (!shouldPlay && uiElement->GetParentInternal() == nullptr && uiElement->OfTypeByIndex<KnownTypeIndex::Popup>())
            {
                // If the element is a parentless popup, it might be allowed to play animations even though it's not in the live tree.
                // (No parentless popup is considered "in the live tree".)
                CPopup* popupNoRef = static_cast<CPopup*>(uiElement);
                bool isPopupVisible = popupNoRef->IsOpen() && popupNoRef->IsVisible();
                if (isPopupVisible && effectiveVisibility.vis == EffectiveVisibility::Visible
                    || !isPopupVisible && effectiveVisibility.vis == EffectiveVisibility::Hidden)
                {
                    shouldPlay = true;
                }
            }

            if (shouldPlay)
            {
                if (effectiveVisibility.isExempt)
                {
                    // Playing animations on sub-elements of Modern Panel item containers is currently not supported.
                    // This case is too complex to support for now as they must adhere to the special policies above, and also
                    // allow "normal" effective visibility changes within the sub-element tree to trigger animations.
                    // This scenario will be revisited in RS3 when we tackle per-item animations on item containers.
                    shouldPlay = false;
                }
                else if (uiElement->GetIsItemContainer())
                {
                    // Modern Panel item containers are subject to some special policies for RS2:
                    // 1) Do trigger animations in recursive scenario (container or ancestor changed effective visibility)
                    // 2) Do trigger animations on initial load of items, or when ItemsSource changes
                    // 3) Do not trigger for individual item add/remove, or during virtualization
                    // To detect #1 we detect a change in the effective visibility of the item's parent panel
                    // To detect #2 we leverage information in the panel used for ThemeTransitions.  This information
                    // can tell us if on this frame, an ItemsSource was loading its items, or was changed.
                    // Detection of #3 is implicit - if neither condition #1 or #2 is detected, this must be condition #3
                    CUIElement* parentPanel = static_cast<CUIElement*>(uiElement->GetParentInternal());
                    ASSERT(parentPanel != nullptr);
                    auto effectiveVisTracker = GetEffectiveVisibilityTracker(parentPanel);
                    bool parentEffectiveVisChanged = effectiveVisTracker.previous.vis != effectiveVisTracker.current.vis;
                    shouldPlay = parentEffectiveVisChanged || FxCallbacks::UIElement_ShouldPlayImplicitShowHideAnimation(uiElement);
                }

                if (shouldPlay)
                {
                    switch (effectiveVisibility.vis)
                    {
                    case EffectiveVisibility::Visible:
                        // We just transitioned to visible.  Cancel any ongoing Hide and request Show animation.
                        if (uiElement->HasImplicitAnimation(ImplicitAnimationType::Show))
                        {
                            uiElement->CancelImplicitAnimation(ImplicitAnimationType::Hide);
                            uiElement->SetImplicitAnimationRequested(ImplicitAnimationType::Show, true);
                        }
                        break;

                    case EffectiveVisibility::Hidden:
                        // We just transitioned to hidden.  Cancel any ongoing Show and request Hide animation.
                        if (uiElement->HasImplicitAnimation(ImplicitAnimationType::Hide))
                        {
                            uiElement->CancelImplicitAnimation(ImplicitAnimationType::Show);
                            uiElement->SetImplicitAnimationRequested(ImplicitAnimationType::Hide, true);
                        }
                        break;
                    }
                }
            }

            // Shown/Hidden events should be raised even if we're not playing implicit animations. This can be the case for elements
            // that just left the tree without going into unloading storage, for example.
            // TODO: These will need to go into unloading storage if they're going to be animated.
            switch (effectiveVisibility.vis)
            {
            case EffectiveVisibility::Visible:
                shownElements.push_back(uiElement);
                break;

            case EffectiveVisibility::Hidden:
                hiddenElements.push_back(uiElement);
                break;
            }
        }
    }

    // Fire the shown/hidden events now that we're done computing everything.
    for (const auto& shownElement : shownElements)
    {
        shownElement->FireShownHiddenEvent(KnownEventIndex::UIElement_Shown);
    }
    for (const auto& hiddenElement : hiddenElements)
    {
        hiddenElement->FireShownHiddenEvent(KnownEventIndex::UIElement_Hidden);
    }

    // There could have been some elements that were removed from the tree this frame, but had Hidden event handlers
    // on them. We kept these elements in the tree long enough to raise the Hidden event. Now that the Hidden event
    // has been raised, we no longer need to keep them alive. If the Hidden event handler didn't keep them alive either,
    // then they will be taken out of the tree.
    // Note: Since this affects which elements get taken out of the tree this frame, we do this before collecting
    // elementsToNotKeepVisible below.
    for (CUIElement* elementWithKeepAliveCount : m_elementsWithKeepAliveCount)
    {
        if (elementWithKeepAliveCount->IsKeepingAliveUntilHiddenEventIsRaised())
        {
            elementWithKeepAliveCount->ReleaseKeepAlive();
            elementWithKeepAliveCount->SetIsKeepingAliveUntilHiddenEventIsRaised(false);
        }
    }

    // Update the "keep visible" status for elements being kept visible due to implicit Hide animation running in their subtree.
    //
    // Take a reference to these elements. These might include popups that are only kept alive by being in the list
    // of open popups while they're kept visible for a hide animation. When we clean them up they will be removed
    // from the open popup list, but we don't want them to be deleted before we finish cleaning them up. Some of these
    // might also be kept alive because of an earlier entry in the list, and we don't want to AV when we iterate through
    // the removal list because some previous cleanup deleted a later entry.
    std::vector<xref_ptr<CUIElement>> elementsToNotKeepVisible;
    for (auto it = m_keepVisible.begin(); it != m_keepVisible.end(); it++)
    {
        CUIElement* keepVisibleElement = *it;
        bool keepVisible = false;

        // Check for running ECP hide animations under the element
        // Note: We're not looking for any element with an ECP hide animation. It needs to be running.
        for (auto itVis = m_effectiveVisibilityMap.begin(); itVis !=  m_effectiveVisibilityMap.end(); itVis++)
        {
            CUIElement* hideElement = itVis->first;
            if (hideElement->IsImplicitAnimationRequested(ImplicitAnimationType::Hide) ||
                hideElement->IsImplicitAnimationPlaying(ImplicitAnimationType::Hide))
            {
                if (ShouldKeepVisible(keepVisibleElement, hideElement))
                {
                    keepVisible = true;
                    break;
                }
            }
        }

        // Check for RequestKeepAlive elements under the element
        if (!keepVisible && HasDescendantWithKeepAliveCount(keepVisibleElement))
        {
            keepVisible = true;
            break;
        }

        if (!keepVisible)
        {
            // Since m_keepVisible is a vector, we can't remove while iterating.
            // Transfer to another vector.
            elementsToNotKeepVisible.push_back(xref_ptr<CUIElement>(keepVisibleElement));
        }
    }

    // Cleanup all the elements no longer being kept visible.
    for (auto it = elementsToNotKeepVisible.begin(); it != elementsToNotKeepVisible.end(); it++)
    {
        (*it)->SetKeepVisible(false);
    }
}

// Helper function, computes effective visibility for the given CUIElement, leveraging already computed information about potential parent elements.
EffectiveVisibilityInfo DCompTreeHost::ComputeEffectiveVisibility(_In_ CUIElement* uiElement)
{
    EffectiveVisibilityInfo effectiveVisibility;

    if (uiElement->GetIsItemContainer())
    {
        // Optimization for ItemContainers:  Since we're likely to see implicit animations attached to all containers in a modern panel,
        // we'll reuse the effective visibility information we just computed for the panel and save a lot of extra walks up the tree.
        if (uiElement->IsActive())
        {
            // Get the current effective visibility for the panel
            CUIElement* parentPanel = static_cast<CUIElement*>(uiElement->GetParentInternal());
            ASSERT(parentPanel != nullptr);
            auto it = m_effectiveVisibilityTrackerMap.find(parentPanel);
            ASSERT(it != m_effectiveVisibilityTrackerMap.end());

            effectiveVisibility.isExempt = it->second.current.isExempt;
            switch (it->second.current.vis)
            {
                case EffectiveVisibility::Visible:
                {
                    CUIElementCollection* children = parentPanel->GetChildren();
                    effectiveVisibility.vis = uiElement->IsActive() && uiElement->IsVisible() && !children->IsUnloadingElement(uiElement) ? EffectiveVisibility::Visible : EffectiveVisibility::Hidden;
                    break;
                }
                case EffectiveVisibility::Hidden:
                    effectiveVisibility.vis = EffectiveVisibility::Hidden;
                    break;
            }
        }
        else
        {
            effectiveVisibility.vis = EffectiveVisibility::Hidden;
        }
    }
    else
    {
        effectiveVisibility = uiElement->ComputeEffectiveVisibility();

        // Special case for popup here - we look at whether the popup is open as well.
        if (uiElement->OfTypeByIndex<KnownTypeIndex::Popup>())
        {
            CPopup* popupNoRef = static_cast<CPopup*>(uiElement);
            if (!popupNoRef->IsOpen())
            {
                effectiveVisibility.vis = EffectiveVisibility::Hidden;
            }
        }
    }

    return effectiveVisibility;
}

// Update and store the EffectiveVisibilityTracker for this element
void DCompTreeHost::UpdateEffectiveVisibilityTracker(_In_ CUIElement* element)
{
    EffectiveVisibilityInfo currentVisibility = element->ComputeEffectiveVisibility();

    auto it = m_effectiveVisibilityTrackerMap.find(element);
    if (it == m_effectiveVisibilityTrackerMap.end())
    {
        EffectiveVisibilityTracker effectiveVisibilityTracker;
        ASSERT(currentVisibility.vis == EffectiveVisibility::Visible);
        effectiveVisibilityTracker.previous = {EffectiveVisibility::Hidden, false};
        effectiveVisibilityTracker.current = currentVisibility;
        m_effectiveVisibilityTrackerMap.emplace(element, effectiveVisibilityTracker);
    }
    else
    {
        it->second.previous = it->second.current;
        it->second.current = currentVisibility;
    }
}

// Retrieve the EffectiveVisibilityTracker for this element
EffectiveVisibilityTracker DCompTreeHost::GetEffectiveVisibilityTracker(_In_ CUIElement* element) const
{
    auto it = m_effectiveVisibilityTrackerMap.find(element);

    // It's required that the element be in the map to retrieve an entry as there's no reasonable default
    ASSERT(it != m_effectiveVisibilityTrackerMap.end());

    return it->second;
}

// Remove the EffectiveVisibilityTracker map entry for this element
void DCompTreeHost::CleanupEffectiveVisibilityTracker(_In_ CUIElement* element)
{
    auto it = m_effectiveVisibilityTrackerMap.find(element);
    if (it != m_effectiveVisibilityTrackerMap.end())
    {
        m_effectiveVisibilityTrackerMap.erase(it);
    }
}

// Start/stop tracking this element for "keep visible" purposes.
// The basic idea is, for any element with an implicit Hide animation running in its subtree,
// we keep track of these elements, and every frame we evaluate whether or not it still has
// any animations running in its subtree.  Once we reach a state of no animations running,
// we can complete the action that was originally requested (finish Unloading, or Collapsing).
// We also need to keep elements visible if it has RequestKeepAlive called on it.
void DCompTreeHost::SetTrackKeepVisible(_In_ CUIElement* uiElement, bool track)
{
    auto itFind = std::find(m_keepVisible.begin(), m_keepVisible.end(), uiElement);
    if (track)
    {
        if (itFind == m_keepVisible.end())
        {
            m_keepVisible.push_back(uiElement);
        }
    }
    else
    {
        if (itFind != m_keepVisible.end())
        {
            m_keepVisible.erase(itFind);
        }
    }
}

// Return true if this element is being tracked for "keep visible" purposes
bool DCompTreeHost::IsKeepVisible(_In_ CUIElement* uiElement)
{
    auto itFind = std::find(m_keepVisible.begin(), m_keepVisible.end(), uiElement);
    return (itFind != m_keepVisible.end());
}

void DCompTreeHost::AddElementWithKeepAliveCount(_In_ CUIElement* element)
{
    ASSERT(std::find(m_elementsWithKeepAliveCount.begin(), m_elementsWithKeepAliveCount.end(), element) == m_elementsWithKeepAliveCount.end());
    m_elementsWithKeepAliveCount.push_back(element);
}

void DCompTreeHost::RemoveElementWithKeepAliveCount(_In_ CUIElement* element)
{
    const auto& itFind = std::find(m_elementsWithKeepAliveCount.begin(), m_elementsWithKeepAliveCount.end(), element);
    ASSERT(itFind != m_elementsWithKeepAliveCount.end());

    m_elementsWithKeepAliveCount.erase(itFind);
}

bool DCompTreeHost::HasElementsWithKeepAliveCount() const
{
    return !m_elementsWithKeepAliveCount.empty();
}

bool DCompTreeHost::HasDescendantWithKeepAliveCount(_In_ CUIElement* ancestor) const
{
    for (CUIElement* elementWithKeepAliveCount : m_elementsWithKeepAliveCount)
    {
        if (ShouldKeepVisible(ancestor, elementWithKeepAliveCount))
        {
            return true;
        }
    }
    return false;
}

bool DCompTreeHost::IsElementInKeepAliveVector(_In_ CUIElement* element)
{
    const auto& itFind = std::find(m_elementsWithKeepAliveCount.begin(), m_elementsWithKeepAliveCount.end(), element);
    return (itFind != m_elementsWithKeepAliveCount.end());
}

void DCompTreeHost::RequestKeepAliveForHiddenEventHandlersInSubtree(_In_ CUIElement* ancestor)
{
    // Elements with Hidden event handlers will be in the effective visibility map.
    for (auto it = m_effectiveVisibilityMap.begin(); it !=  m_effectiveVisibilityMap.end(); it++)
    {
        CUIElement* uiElement = it->first;

        // If this element has a Hidden handler, and isn't being kept alive...
        if (!uiElement->HasKeepAliveCount()
            && uiElement->HasHiddenHandlers())
        {
            // ...and it's in this subtree...
            if (uiElement->HasAncestorIncludingPopups(ancestor))
            {
                // ...then keep it alive until we raise the Hidden event at the end of the frame.

                ASSERT(!IsElementInKeepAliveVector(uiElement));
                ASSERT(!uiElement->IsKeepingAliveUntilHiddenEventIsRaised());   // If this flag is marked, then the element should have a KeepAliveCount already.

                uiElement->RequestKeepAlive();
                uiElement->SetIsKeepingAliveUntilHiddenEventIsRaised(true);

                // That RequestKeepAlive call above should have been the first call, which put it in this vector.
                // We'll look for it in this vector later when making the matching ReleaseKeepAlive call after raising
                // Hidden events.
                ASSERT(IsElementInKeepAliveVector(uiElement));
            }
        }
    }
}

bool DCompTreeHost::ShouldKeepVisible(_In_ CUIElement* uiElement)
{
    for (auto& it : m_implicitHideAnimationsMap)
    {
        CUIElement* uiElementWithHideAnimation = it.first;
        if (ShouldKeepVisible(uiElement, uiElementWithHideAnimation))
        {
            return true;
        }
    }

    // There are no ECP hide animations under the element. Check whether there are RequestKeepAlive elements.
    return HasDescendantWithKeepAliveCount(uiElement);
}

bool DCompTreeHost::ShouldKeepVisible(_In_ CUIElement* uiElement, _In_ CUIElement* uiElementWithKeepAlive) const
{
    if (uiElementWithKeepAlive == uiElement)
    {
        // This element needs to be kept alive.  Keep it visible.
        return true;
    }
    else if (uiElementWithKeepAlive->HasAncestorIncludingPopups(uiElement))
    {
        // This element is the ancestor of an element that needs to be kept alive.  Keep it visible.
        return true;
    }

    return false;
}

XamlLightTargetIdMap& DCompTreeHost::GetXamlLightTargetIdMap()
{
    return m_xamlLightTargetIdMap;
}

XamlLightTargetMap& DCompTreeHost::GetXamlLightTargetMap()
{
    return m_xamlLightTargetMap;
}


bool DCompTreeHost::HasXamlIslandData() const
{
    return !m_islandRenderData.empty();
}

// Add render data for a new XamlIslandRoot
void DCompTreeHost::AddXamlIslandTarget(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    if(xamlIslandRoot)
    {
        auto islandData = m_islandRenderData.find(xamlIslandRoot);
        if (islandData == m_islandRenderData.end())
        {
            m_islandRenderData.emplace(xamlIslandRoot, XamlIslandRenderData());
        }
        else
        {
            // Islands should already have render data
            IFCFAILFAST(E_FAIL);
        }
    }
}

void DCompTreeHost::UpdateXamlIslandTargetSize(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    if(xamlIslandRoot)
    {
        auto islandData = m_islandRenderData.find(xamlIslandRoot);
        if (islandData != m_islandRenderData.end())
        {
            // If XamlIslandRoot size is set before WindowsPresentTarget has been
            // created, the target size will be set in EnsureXamlIslandTargetRoots
            // on WindowsPresentTarget's creation.
            if(islandData->second.windowsPresentTarget)
            {
                auto xamlIslandRootSize = xamlIslandRoot->GetSize();
                islandData->second.windowsPresentTarget->SetWidth(static_cast<XUINT32>(xamlIslandRootSize.Width));
                islandData->second.windowsPresentTarget->SetHeight(static_cast<XUINT32>(xamlIslandRootSize.Height));
            }
        }

        // It's possible that a layout pass happens while the CXamlIslandRoot is in the "unloading" state.  In that
        // case, we may not have any render data for that island.  In past releases we had a FailFast here, but now
        // we just silently no-op.
    }
}

void DCompTreeHost::RemoveXamlIslandTarget(_In_ CXamlIslandRoot* xamlIslandRoot)
{
    // Remove render data for XamlIslandRoot, if it exists
    auto islandData = m_islandRenderData.find(xamlIslandRoot);

    if (islandData == m_islandRenderData.end())
    {
        // Islands should already have render data
        IFCFAILFAST(E_FAIL);
    }

    // If this is the first island and we are showing frame data, then remove it and reshow it on the next island
    bool refreshFrameRateVisual = m_frameRateVisual && islandData == m_islandRenderData.begin();
    if (refreshFrameRateVisual)
    {
        HideUIThreadCounters();
    }
    
    m_islandRenderData.erase(islandData);

    if (refreshFrameRateVisual)
    {
        ShowUIThreadCounters();
    }
}

_Check_return_ HRESULT DCompTreeHost::GetPointerSourceForElement(
    _In_ CUIElement *element,
    _Out_ std::shared_ptr<void> *pointerSourceWrapper)
{
    *pointerSourceWrapper = nullptr;

    auto it = m_hoverPointerSourceMap.find(element);
    if (it != m_hoverPointerSourceMap.end())
    {
        *pointerSourceWrapper = it->second;
    }

    return S_OK;
}

void DCompTreeHost::StorePointerSourceForElement(
    _In_ CUIElement *element,
    _In_ std::shared_ptr<void> pointerSourceWrapper)
{
    m_hoverPointerSourceMap[element] = pointerSourceWrapper;
}

void DCompTreeHost::ReleasePointerSourceForElement(_In_ CUIElement *element)
{
    m_hoverPointerSourceMap.erase(element);
}

void DCompTreeHost::CreateVisualTreeCompositionIslandAdapter()
{
    FAIL_FAST_ASSERT(m_spCompositor.Get());

    if (m_coreWindowNoRef == nullptr)
    {
        m_coreWindowNoRef = DirectUI::DXamlServices::GetCurrentCoreWindowNoRef();
    }

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    m_onecoreIslandAdapter = std::make_unique<CoreWindowIslandAdapter>(m_coreWindowNoRef, m_spCompositor.Get());

    WUComp::ICompositionIsland * island = m_visualTreeCompIslandAdapter->GetCompostionIslandNoRef();

    // This results in XAML registering for IContentIsland::StateChanged event
    IFCFAILFAST(FxCallbacks::DxamlCore_OnCompositionIslandCreated(content));
#endif
}

void DCompTreeHost::ReleaseVisualTreeCompositionIslandAdapter()
{
// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
    IFCFAILFAST(FxCallbacks::DxamlCore_OnCompositionIslandDestroyed());
    m_onecoreIslandAdapter.reset();
#endif
}

// CONTENT-TODO: Lifted IXP doesn't support OneCoreTransforms UIA yet.
#if false
UINT64 DCompTreeHost::GetCompositionIslandId()
{
    ASSERT(XamlOneCoreTransforms::IsEnabled());
    ASSERT(m_onecoreIslandAdapter != nullptr);

    auto content = m_onecoreIslandAdapter->GetCompostionCompositionIslandNoRef();
    UINT64 id;
    IFCFAILFAST(content->get_Id(&id));
    id = 0;

    return id;
}
#endif

/* static */ void DCompTreeHost::SetTag(_In_ WUComp::IVisual* visual, _In_ const wchar_t* tag, float value)
{
    xref_ptr<WUComp::ICompositionObject> compositionObject;
    VERIFYHR(visual->QueryInterface(IID_PPV_ARGS(compositionObject.ReleaseAndGetAddressOf())));
    xref_ptr<WUComp::ICompositionPropertySet> compositionPropertySet;
    IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

    // Property sets don't support strings as property values, so store our tag as a scalar value.
    IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(tag).Get(), value));
}

/* static */ void DCompTreeHost::SetTagIfEnabled(_In_ WUComp::IVisual* visual, VisualDebugTags tag)
{
    if (VisualDebugTagsEnabled())
    {
        xref_ptr<WUComp::ICompositionObject> compositionObject;
        VERIFYHR(visual->QueryInterface(IID_PPV_ARGS(compositionObject.ReleaseAndGetAddressOf())));
        xref_ptr<WUComp::ICompositionPropertySet> compositionPropertySet;
        IFCFAILFAST(compositionObject->get_Properties(compositionPropertySet.ReleaseAndGetAddressOf()));

        // Property sets don't support strings as property values, so store our tag as a scalar value.
        IFCFAILFAST(compositionPropertySet->InsertScalar(wrl_wrappers::HStringReference(debugTagPropertyName).Get(), static_cast<uint8_t>(tag) ));
    }
}

std::weak_ptr<ProjectedShadowManager> DCompTreeHost::GetProjectedShadowManagerWeakPtr()
{
    std::weak_ptr<ProjectedShadowManager> weakProjectedShadowManager = m_projectedShadowManager;

    return weakProjectedShadowManager;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Ctor
//
//----------------------------------------------------------------------------
DCompSurfaceFactory::DCompSurfaceFactory(
    _In_ DCompTreeHost *pDCompTreeHost,
    _In_ IDCompositionSurfaceFactory *pSurfaceFactory
    )
    : m_DCompTreeHostNoRef(pDCompTreeHost)
    , m_SurfaceFactory(pSurfaceFactory)
{
    m_SurfaceFactory->AddRef();
    m_SurfaceFactory->QueryInterface(IID_PPV_ARGS(&m_SurfaceFactoryPartner));
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//       Dtor
//
//----------------------------------------------------------------------------
DCompSurfaceFactory::~DCompSurfaceFactory()
{
    IGNOREHR(DCompSurfaceFactoryManager::Instance()->OnSurfaceFactoryDestroyed(this));

    //we store pointers of offered surfacefactory
    //a list in OfferTracker, need to guarantee that
    //they are deleted before released
    OfferTracker* pOfferTrackerNoRef = m_DCompTreeHostNoRef->GetOfferTrackerNoRef();
    if (pOfferTrackerNoRef != nullptr)
    {
        pOfferTrackerNoRef->DeleteReleasedSurfaceFactoryFromList(m_SurfaceFactoryPartner.Get());
    }

    ReleaseInterface(m_SurfaceFactory);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a DCompSurfaceFactory object.
//
//----------------------------------------------------------------------------
/*static*/ _Check_return_ HRESULT
DCompSurfaceFactory::Create(
    _In_ DCompTreeHost *pDCompTreeHost,
    _In_ IDCompositionDesktopDevicePartner *pMainDevice,
    _In_ IUnknown *pIUnknownDevice,
    _Outptr_ DCompSurfaceFactory **ppSurfaceFactoryWrapper
    )
{
    HRESULT hr = S_OK;

    DCompSurfaceFactory *pSurfaceFactoryWrapper = NULL;
    IDCompositionSurfaceFactory *pDCompSurfaceFactory = NULL;

    ID3D11Device *pD3D11Device = NULL;
    XUINT32 creationFlags;

    IFC(DCompHelpers::D3D11DeviceFromUnknownDevice(pIUnknownDevice, &pD3D11Device));

    // D3D11_CREATE_DEVICE_BGRA_SUPPORT is required because the surface created with the
    // surface factory on the provided device will have BGRA format.
    creationFlags = pD3D11Device->GetCreationFlags();

    if (0 == (creationFlags & D3D11_CREATE_DEVICE_BGRA_SUPPORT))
    {
        IFC(E_INVALIDARG);
    }

    IFC(pMainDevice->CreateSurfaceFactory(pIUnknownDevice, &pDCompSurfaceFactory));

    pSurfaceFactoryWrapper = new DCompSurfaceFactory(pDCompTreeHost, pDCompSurfaceFactory);

    *ppSurfaceFactoryWrapper = pSurfaceFactoryWrapper;
    pSurfaceFactoryWrapper = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactoryWrapper);
    ReleaseInterfaceNoNULL(pDCompSurfaceFactory);
    ReleaseInterfaceNoNULL(pD3D11Device);

    RRETURN(hr);
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a DCompSurface object associated with this device.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactory::CreateSurface(
    XUINT32 width,
    XUINT32 height,
    bool isOpaque,
    bool isVirtual,
    bool isHDR,
    bool requestAtlas,
    _Outptr_ DCompSurface **ppSurface
    )
{
    HRESULT hr = S_OK;

    DCompSurface *pDCompSurface = NULL;

    IFC(DCompSurface::Create(
        m_DCompTreeHostNoRef,
        m_SurfaceFactory,
        isOpaque,
        isVirtual,
        isHDR,
        requestAtlas,
        width,
        height,
        &pDCompSurface
        ));

    *ppSurface = pDCompSurface;
    pDCompSurface = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pDCompSurface);

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Flushes all GPU work for all SIS surfaces created with this SurfaceFactory.
//      This includes GPU work done by DComp on the app's behalf (gutters).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
DCompSurfaceFactory::Flush()
{
    HRESULT hr = S_OK;

    IDCompositionSurfaceFactoryPartner *pSurfaceFactoryPartner = NULL;

    IFC(m_SurfaceFactory->QueryInterface(&pSurfaceFactoryPartner));

    IFC(pSurfaceFactoryPartner->Flush());

Cleanup:
    ReleaseInterfaceNoNULL(pSurfaceFactoryPartner);

    RRETURN(hr);
}

namespace DCompHelpers
{
//----------------------------------------------------------------------------
//
//  Synopsis:
//      Given an IUnknown which must be either a D3D device or a D2D device,
//      return the underlying D3D11Device, otherwise return failure.
//
//----------------------------------------------------------------------------
_Check_return_ HRESULT
D3D11DeviceFromUnknownDevice(
    _In_ IUnknown *pUnknownDevice,
    _Outptr_ ID3D11Device **ppD3D11Device
    )
{
    HRESULT hr = S_OK;

    ID2D1Device2 *pD2DDevice2 = NULL;
    IDXGIDevice *pDXGIDevice = NULL;
    ID3D11Device *pD3D11Device = NULL;

    // First see if this is a D3D device
    hr = pUnknownDevice->QueryInterface(IID_PPV_ARGS(&pD3D11Device));
    if (FAILED(hr))
    {
        // Since QI failed we know this must not be a D3D device.
        // The only other possible valid device is a D2D device.
        // If so, fish the DXGI Device out of it.
        IFC(pUnknownDevice->QueryInterface(IID_PPV_ARGS(&pD2DDevice2)));
        IFC(pD2DDevice2->GetDxgiDevice(&pDXGIDevice));
        IFC(pDXGIDevice->QueryInterface(IID_PPV_ARGS(&pD3D11Device)));
    }

    *ppD3D11Device = pD3D11Device;
    pD3D11Device = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pD2DDevice2);
    ReleaseInterfaceNoNULL(pDXGIDevice);
    ReleaseInterfaceNoNULL(pD3D11Device);

    RRETURN(hr);
}

}  //namespace DCompHelpers
