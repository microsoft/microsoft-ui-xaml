// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CDirectManipulationService class used to interact with the
//    DirectManipulation APIs.

#include "precomp.h"
#include <Microsoft.DirectManipulation.h>
#include "DirectManipulationService.h"
#include "DirectManipulationServiceSharedState.h"
#include "DirectManipulationViewportEventHandler.h"
#include "DirectManipulationFrameInfoProvider.h"
#include "DirectManipulationHelper.h"
#include <d2d1.h>
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DMDeferredRelease.h>
#include "RemapVirtualKey.h"
#include <ComPtr.h>
#include <ErrorContext.h>
#include <DXamlServices.h>
#include "corep.h"
#include "LoadLibraryAbs.h"

using namespace ATL;

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get debug outputs, and 0 otherwise
#define DMS_DBG 0

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get verbose debug outputs, and 0 otherwise
#define DMSv_DBG 0

const float CDirectManipulationService::s_maxOverpanDistance = 200.0f;
const float CDirectManipulationService::s_scaleOverpanValue = 0.91f;
const float CDirectManipulationService::s_minOverpanDistance = 1.0f;
const float CDirectManipulationService::s_centerPointScaleFactor = 1.94f;
const float CDirectManipulationService::s_curveSuppressionValueForZoom = 1.0f;
const float CDirectManipulationService::s_curveSuppressionValueForTranslate = 0.0f;
const float CDirectManipulationService::s_linearCurvePassThroughSlope = 1.0f;
float CDirectManipulationService::s_range[] = { 0, FLT_MAX };

// Used for the DirectManipulation event handler CDirectManipulationViewportEventHandler.
CComModule _Module;

using namespace RuntimeFeatureBehavior;

#ifdef DM_DEBUG
bool CDirectManipulationService::DMS_TraceDbg() const
{
    bool result = gps->IsDebugTraceTypeActive(XCP_TRACE_DM_PAL_SERVICE);

    if constexpr (DMS_DBG)
    {
        result = true;
    }

    return result;
}

bool CDirectManipulationService::DMSv_TraceDbg() const
{
    bool result = gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE));

    if constexpr (DMSv_DBG)
    {
        result = true;
    }

    return result;
}
#endif // DM_DEBUG

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationService::Create (static)
//
//  Synopsis:
//      Creates an instance of the CDirectManipulationService class
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::Create(
    std::shared_ptr<DirectManipulationServiceSharedState> sharedState,
    _Outptr_ CDirectManipulationService** ppDMService)
{
    HRESULT hr = S_OK;
    CDirectManipulationService* pDMService = NULL;

    ASSERT(ppDMService);
    *ppDMService = NULL;

    pDMService = new CDirectManipulationService(std::move(sharedState));

    *ppDMService = pDMService;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationService::~CDirectManipulationService
//
//  Synopsis:
//      Destructor for the CDirectManipulationService class.
//      - unregisters the existing DM viewports
//      - deactivates DM for this window
//      - releases all DM interfaces
//
//------------------------------------------------------------------------
CDirectManipulationService::~CDirectManipulationService()
{
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ~CDirectManipulationService - destructor.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_mapViewports.Count() == 0);
    ASSERT(m_mapViewportOverpanReflexes.Count() == 0);
    ASSERT(m_mapUIThreadViewportEventHandlerCookies.Count() == 0);
    ASSERT(m_DCompTransformsMap.empty());

#ifdef DM_DEBUG
    ULONG cRefDMManager = m_pDMManager == nullptr ? 0L : m_pDMManager->Release();
    ULONG cRefDMUpdateManager = m_pDMUpdateManager == nullptr ? 0L : m_pDMUpdateManager->Release();
    ULONG cRefDMFrameInfoProvider = m_pDMFrameInfoProvider == nullptr ? 0L : m_pDMFrameInfoProvider->Release();
    ULONG cRefDMCompositor = m_pDMCompositor == nullptr ? 0L : m_pDMCompositor->Release();
    ULONG cRefUIThreadViewportEventHandler = m_pUIThreadViewportEventHandler == nullptr ? 0L : m_pUIThreadViewportEventHandler->Release();
    ULONG cRefViewportEventHandler = m_pViewportEventHandler == nullptr ? 0L : m_pViewportEventHandler->Release();

    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  ~CDirectManipulationService - cRefDMManager=%ld, cRefDMUpdateManager=%ld, cRefDMFrameInfoProvider=%ld, cRefDMCompositor=%ld, cRefUIThreadViewportEventHandler=%ld, cRefViewportEventHandler=%ld.",
            this, cRefDMManager, cRefDMUpdateManager, cRefDMFrameInfoProvider, cRefDMCompositor, cRefUIThreadViewportEventHandler, cRefViewportEventHandler));
    }
#else
    ReleaseInterface(m_pDMManager);
    ReleaseInterface(m_pDMUpdateManager);
    ReleaseInterface(m_pDMFrameInfoProvider);
    m_sharedState->ReleaseSharedDCompManipulationCompositor(m_pDMCompositor);
    ReleaseInterface(m_pUIThreadViewportEventHandler);
    ReleaseInterface(m_pViewportEventHandler);
#endif // DM_DEBUG

    ASSERT(!m_pMapSecondaryContents || m_pMapSecondaryContents->Count() == 0);
    SAFE_DELETE(m_pMapSecondaryContents);

    ASSERT(!m_pMapSecondaryClipContents || m_pMapSecondaryClipContents->Count() == 0);
    SAFE_DELETE(m_pMapSecondaryClipContents);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::EnsureDirectManipulationManager
//
//  Synopsis:
//    Create a DirectManipulation manager for m_islandInputSite if it was not
//    created already.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::EnsureDirectManipulationManager(
    _In_ InputSiteHelper::IIslandInputSite* pIslandInputSite,
    _In_ bool fIsForCrossSlideViewports)
{
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   EnsureDirectManipulationManager - entry. pIslandInputSite=0x%x, fIsForCrossSlideViewports=%d.", this, pIslandInputSite, fIsForCrossSlideViewports));
    }
#endif // DM_DEBUG

    ASSERT(nullptr != pIslandInputSite);

    if (!m_pDMManager)
    {
        // Create a DirectManipulation manager instance
        wrl::ComPtr<IDirectManipulationManager3> spDMManager = CDirectManipulationService::CreateDirectManipulationManager();

        m_pDMManager = spDMManager.Detach();

        m_islandInputSite = pIslandInputSite;

        IFC_RETURN(m_sharedState->GetSharedDCompManipulationCompositor(&m_pDMCompositor));
        ASSERT(m_pDMCompositor);

        if (!fIsForCrossSlideViewports)
        {
            IFC_RETURN(EnsureFrameInfoProvider());
            ASSERT(m_pDMFrameInfoProvider);

            ctl::ComPtr<IDirectManipulationUpdateManager> spDMUpdateManager;
            IFC_RETURN(m_pDMManager->GetUpdateManager(IID_PPV_ARGS(&spDMUpdateManager)));
            ASSERT(spDMUpdateManager.Get());
            m_pDMUpdateManager = spDMUpdateManager.Detach();

            IFC_RETURN(m_pDMCompositor->SetUpdateManager(m_pDMUpdateManager));
        }

        m_pDMHelper.Initialize(m_pDMCompositor, m_pDMManager);
    }

    return S_OK;
}

// Note: This is a stopgap measure that updates the IslandInputSite for the CDirectManipulationService that already exists. We
// can't update any viewports that already exist with the old IslandInputSite, so it's unlikely that the ScrollViewer actually
// works under the new IslandInputSite. This is just enough to remove the initial crashes caused by E_INVALIDARG. To fix this for
// real, we should be resetting all DManip-related state (this DM service, CUIDMContainer and friends, along with all
// scrolling offsets/viewports/extents that we've calculated that are stored in ScrollViewer and ScrollContentPresenter)
// when we detect the IslandInputSite changed. That's a much more involved change and is tracked by Bug 43760760: Xaml's DManip
// viewports need a way to reset their input hwnd.
_Check_return_ HRESULT CDirectManipulationService::EnsureElementIslandInputSite(_In_ InputSiteHelper::IIslandInputSite* pIslandInputSite)
{
    ASSERT(m_pDMManager);

    if (m_islandInputSite.Get() != pIslandInputSite)
    {
        // If the IDirectManipulationManager is active, deactivate it and reactivate for the new input hwnd.
        if (m_fManagerActive)
        {
            FAIL_FAST_IF(nullptr == m_islandInputSite);
            HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
            FAIL_FAST_IF(m_activeInputHwnd != inputHwnd || nullptr == m_activeInputHwnd);
            IFC_RETURN(static_cast<IDirectManipulationManager*>(m_pDMManager)->Deactivate(inputHwnd));
            m_activeInputHwnd = nullptr;
        }

        if (nullptr != pIslandInputSite)
        {
            m_islandInputSite = pIslandInputSite;
        }
        else
        {
            m_islandInputSite = nullptr;
        }

        if (m_fManagerActive)
        {
            if (nullptr != m_islandInputSite)
            {
                HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
                IFC_RETURN(static_cast<IDirectManipulationManager*>(m_pDMManager)->Activate(inputHwnd));
                FAIL_FAST_IF(nullptr != m_activeInputHwnd);
                m_activeInputHwnd = inputHwnd;
            }
            else
            {
                // We're updating to a null IslandInputSite while our manager was active with another IslandInputSite.
                // We have already deactivated the previously set IslandInputSite, so we should properly update our state.
                FAIL_FAST_IF(nullptr != m_activeInputHwnd);
                m_fManagerActive = false;
            }
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::RegisterViewportEventHandler
//
//  Synopsis:
//    Sets the IXcpDirectManipulationViewportEventHandler implementation
//    that wants to be forwarded the DirectManipulation feedback.
//    Provided pViewportEventHandler maybe null to reset the handler.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::RegisterViewportEventHandler(
    _In_opt_ IXcpDirectManipulationViewportEventHandler* pViewportEventHandler)
{
    ReleaseInterface(m_pViewportEventHandler);

    m_pViewportEventHandler = pViewportEventHandler;
    AddRefInterface(m_pViewportEventHandler);

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::ActivateDirectManipulationManager
//
//  Synopsis:
//    Activates the DirectManipulation manager.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::ActivateDirectManipulationManager()
{
#ifdef DBG
    WINDOWPLACEMENT wp = { sizeof(WINDOWPLACEMENT) };
#endif // DBG

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   ActivateDirectManipulationManager - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(nullptr != m_islandInputSite);
    ASSERT(m_pDMManager);

    if (!m_fManagerActive)
    {
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Activating."));
        }
#endif // DM_DEBUG
        HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
        IFC_RETURN(static_cast<IDirectManipulationManager*>(m_pDMManager)->Activate(inputHwnd));
        m_fManagerActive = TRUE;
        FAIL_FAST_IF(nullptr != m_activeInputHwnd);
        m_activeInputHwnd = inputHwnd;
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::DeactivateDirectManipulationManager
//
//  Synopsis:
//    Deactivates the DirectManipulation manager.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::DeactivateDirectManipulationManager()
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   DeactivateDirectManipulationManager - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(nullptr != m_islandInputSite);
    ASSERT(m_pDMManager);

    if (m_fManagerActive)
    {
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Deactivating."));
        }
#endif // DM_DEBUG
        // This method can be invoked on teardown triggered by destroying the application window.
        // When the application window is destroyed it immediately nulls out the input hwnd in the IslandInputSite so we cannot retrieve it from there.
        // Use our cached m_activeInputHwnd instead.
        FAIL_FAST_IF(nullptr == m_activeInputHwnd);
        IFC(static_cast<IDirectManipulationManager*>(m_pDMManager)->Deactivate(m_activeInputHwnd));
        m_fManagerActive = FALSE;
        m_activeInputHwnd = nullptr;
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::UnregisterViewport
//
//  Synopsis:
//    Removes the viewport from our internal m_mapViewports storage,
//    unhooks the two event listeners and releases the viewport DM interface.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::UnregisterViewport(
    _In_ IObject* pViewport)
{
    HRESULT hr = S_OK;
    bool fViewportRegistered = false;
    XDWORD viewportEventHandlerCookie = 0;
    IDirectManipulationViewport* pDMViewport = NULL;
    CDirectManipulationViewportEventHandler* pDMViewportEventHandler = NULL;
    Microsoft::WRL::ComPtr<IDirectManipulationViewport2> spDMViewport2;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   UnregisterViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(IsViewportHandleRegistered(pViewport, &fViewportRegistered));
    if (fViewportRegistered)
    {
        IFC(m_mapViewports.Remove(static_cast<XHANDLE>(pViewport), pDMViewport));
        ASSERT(pDMViewport);

        if (m_mapUIThreadViewportEventHandlerCookies.ContainsKey(static_cast<XHANDLE>(pDMViewport)))
        {
            IFC(m_mapUIThreadViewportEventHandlerCookies.Remove(static_cast<XHANDLE>(pDMViewport), viewportEventHandlerCookie));
        }
        if (viewportEventHandlerCookie != 0)
        {
            IFC(pDMViewport->RemoveEventHandler(viewportEventHandlerCookie));
        }
        if (m_mapUIThreadViewportEventHandlerCookies.Count() == 0 && m_pUIThreadViewportEventHandler)
        {
            pDMViewportEventHandler = static_cast<CDirectManipulationViewportEventHandler*>(m_pUIThreadViewportEventHandler);
            ASSERT(pDMViewportEventHandler);
            IFC(pDMViewportEventHandler->SetDMService(NULL));
        }

        IFC(pDMViewport->QueryInterface(IID_PPV_ARGS(&spDMViewport2)));
        IFC(CleanupOverpanReflexData(pViewport, spDMViewport2.Get()));

        if (m_autoScrollBehaviorCookie != 0)
        {
            IFC(spDMViewport2->RemoveBehavior(m_autoScrollBehaviorCookie));
            m_autoScrollBehaviorCookie = 0;
        }

        if (pDMViewport == m_spDragDropViewport.Get())
        {
            ASSERT(m_dragDropBehaviorCookie != 0);
            IFC(spDMViewport2->RemoveBehavior(m_dragDropBehaviorCookie));
            m_dragDropBehaviorCookie = 0;
            m_spDragDropViewport = nullptr;
        }
        spDMViewport2 = nullptr;

        ReleaseInterface(pViewport);

        IFC(pDMViewport->Abandon());

#ifdef DM_DEBUG
        ULONG cRefDMViewport = pDMViewport->Release();

        if (DMSv_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
                L"DMSv[0x%p]:  UnregisterViewport - pDMViewport=0x%p, cRefDMViewport=%ld.", this, pDMViewport, cRefDMViewport));
        }
//#elif DBG
//        ULONG cRefDMViewport = pDMViewport->Release();

        // After running an inertial animation, DManip might still temporarily hold on to the IDirectManipulationViewport.
        // Otherwise it is expected to be destroyed.
//        ASSERT(m_wasInInertiaStatusDbg || cRefDMViewport == 0);
#else
        ReleaseInterface(pDMViewport);
#endif
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::ProcessInput
//
//  Synopsis:
//    Forwards a keyboard or mouse input message to DirectManipulation
//    for processing.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::ProcessInput(
    _In_ IObject* pViewport,
    _In_ XHANDLE hMsg,
    _In_ MessageMap msgID,
    _In_ bool fIsSecondaryMessage,
    _In_ bool fInvertForRightToLeft,
    _In_ XDMConfigurations activatedConfiguration,
    _Out_ bool& fHandled)
{
    HRESULT hr = S_OK;
    bool fIsKeyboardInput = false;
    bool fIsForHorizontalPan = false;
    BOOL handled = FALSE;
    WPARAM wParam = 0;
    MSG msg;
    MsgPacket* pMsgPack = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   ProcessInput - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(hMsg);

    fHandled = FALSE;

    pMsgPack = static_cast<MsgPacket*>(hMsg);

    if (msgID == XCP_KEYDOWN)
    {
        wParam = static_cast<UINT32>(InputUtility::RemapVirtualKey(static_cast<wsy::VirtualKey>(pMsgPack->m_wParam)));
    }
    else
    {
        wParam = pMsgPack->m_wParam;
    }

    if (IsWindowsMessageForHorizontalPan(msgID, wParam))
    {
        if ((activatedConfiguration & XcpDMConfigurationPanX) != 0)
        {
            fIsForHorizontalPan = TRUE;
        }
        else
        {
            // Message is dedicated to horizontal pan, which is turned off.
            // Do not forward to DM.
            goto Cleanup;
        }
    }

    if (IsWindowsMessageForVerticalPan(msgID, wParam))
    {
        if ((activatedConfiguration & XcpDMConfigurationPanY) == 0)
        {
            // Message is dedicated to vertical pan, which is turned off.
            // Do not forward to DM.
            goto Cleanup;
        }
    }

    if (!fIsForHorizontalPan && IsWindowsMessageForPan(msgID, wParam))
    {
        if ((activatedConfiguration & (XcpDMConfigurationPanX | XcpDMConfigurationPanY)) == 0)
        {
            goto Cleanup;
        }
        else if ((activatedConfiguration & (XcpDMConfigurationPanX | XcpDMConfigurationPanY)) == (XcpDMConfigurationPanX | XcpDMConfigurationPanY))
        {
            XUINT32 modifierKeys = 0;
            IFC(gps->GetKeyboardModifiersState(&modifierKeys));
            fIsForHorizontalPan = modifierKeys & KEY_MODIFIER_CTRL ? TRUE : FALSE;
        }
        else if ((activatedConfiguration & XcpDMConfigurationPanX) != 0)
        {
            fIsForHorizontalPan = TRUE;
        }
    }

    // A new MSG is reconstructed from the PAL MsgPacket and provided to DM.
    msg.hwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
    msg.message = GetWindowsMessageFromMessageMap(msgID, fIsSecondaryMessage, fIsKeyboardInput);
    msg.wParam = GetWindowsMessageWParam(msgID, wParam, fInvertForRightToLeft && fIsForHorizontalPan);
    msg.lParam = pMsgPack->m_lParam;
    msg.time = ::GetMessageTime();
    msg.pt.x = 0;
    msg.pt.y = 0;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    // Note that these pseudo pointer ids must still use the pointer id version
    // of SetContact and not the overload that takes a PointerPoint.
    IFC(pDMViewport->SetContact(fIsKeyboardInput ? DIRECTMANIPULATION_KEYBOARDFOCUS : DIRECTMANIPULATION_MOUSEFOCUS /*pointerId*/));

    if (msgID == XCP_POINTERWHEELCHANGED)
    {
        XUINT32 pointerId = GET_POINTERID_WPARAM(msg.wParam);
        auto pointerPoint = GetPointerPointFromPointerId(pointerId);
        IFC(m_pDMHelper->ProcessInputWithPointerPoint(&msg, pointerPoint.Get(), &handled));
    }
    else
    {
        IFC(m_pDMManager->ProcessInput(&msg, &handled));
    }

    fHandled = !!handled;
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Exit with hr=%d, fHandled=%d.", hr, fHandled));
    }
#endif // DM_DEBUG

    // Note that these pseudo pointer ids must still use the pointer id version
    // of ReleaseContact and not the overload that takes a PointerPoint.
    IFC(pDMViewport->ReleaseContact(fIsKeyboardInput ? DIRECTMANIPULATION_KEYBOARDFOCUS : DIRECTMANIPULATION_MOUSEFOCUS /*pointerId*/));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetContact
//
//  Synopsis:
//    Declares a contact ID to DirectManipulation that can potentially start
//    a manipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetContact(
    _In_ IObject* pViewport,
    _In_ XUINT32 pointerId,
    _Out_ bool* pfContactFailure)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   SetContact - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(pfContactFailure);
    *pfContactFailure = FALSE;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    TraceDmSetContactInfo((UINT64)pViewport);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   SetContact(pointerId=%d, hWnd=0x%x, pDMViewport=0x%p).", pointerId, inputHwnd, pDMViewport));
    }
#endif // DM_DEBUG

    {
        // http://osgvsowi/14575768 - Suspend until we have a touch driver reset solution
        SuspendFailFastOnStowedException suspender;
        auto pointerPoint = GetPointerPointFromPointerId(pointerId);
        IFC(m_pDMHelper->SetContact(pDMViewport, pointerPoint.Get()));
    }

Cleanup:
    if (FAILED(hr))
    {
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            if (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS))
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   SetContact failed with ERROR_OBJECT_ALREADY_EXISTS."));
            }
            else if (hr == HRESULT_FROM_WIN32(ERROR_OBJECT_NO_LONGER_EXISTS))
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   SetContact failed with ERROR_OBJECT_NO_LONGER_EXISTS."));
            }
        }
#endif // DM_DEBUG

        *pfContactFailure = TRUE;
        hr = S_FALSE;
    }
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::ReleaseContact
//
//  Synopsis:
//    Tells DirectManipulation to no longer track the provided contact ID.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::ReleaseContact(
    _In_ IObject* pViewport,
    _In_ XUINT32 pointerId)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   ReleaseContact - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    {
        auto pointerPoint = GetPointerPointFromPointerId(pointerId);
        IFC(m_pDMHelper->ReleaseContact(pDMViewport, pointerPoint.Get()));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::ReleaseAllContacts
//
//  Synopsis:
//    Tells DirectManipulation to stop tracking all contact IDs associated
//    with the provided viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::ReleaseAllContacts(
    _In_ IObject* pViewport)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   ReleaseAllContacts - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->ReleaseAllContacts());

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::EnableViewport
//
//  Synopsis:
//    Enables the provided viewport if its current status is
//    Building or Disabled.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::EnableViewport(
    _In_ IObject* pViewport,
    _Out_ bool& fCausedRunningStatus)
{
    HRESULT hr = S_OK;
    DIRECTMANIPULATION_STATUS dmStatus;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   EnableViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    fCausedRunningStatus = FALSE;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetStatus(&dmStatus));

    if (dmStatus == DIRECTMANIPULATION_BUILDING || dmStatus == DIRECTMANIPULATION_DISABLED)
    {
        IFC(pDMViewport->Enable());

        // Check if we ended on the Ready status because of a transitional Running status.
        IFC(pDMViewport->GetStatus(&dmStatus));
        if (dmStatus == DIRECTMANIPULATION_READY)
        {
            // In order to land on the expected Enabled status, the viewport gets disabled and re-enabled synchronously.
            IFC(pDMViewport->Disable());
            IFC(pDMViewport->Enable());
            fCausedRunningStatus = TRUE;
        }

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Completed."));
        }
#endif // DM_DEBUG

    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::DisableViewport
//
//  Synopsis:
//    Disables the provided viewport if its current status is not
//    Building or Disabled.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::DisableViewport(
    _In_ IObject* pViewport)
{
    HRESULT hr = S_OK;
    DIRECTMANIPULATION_STATUS dmStatus;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   DisableViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetStatus(&dmStatus));

    if (dmStatus != DIRECTMANIPULATION_BUILDING &&
        dmStatus != DIRECTMANIPULATION_DISABLED)
    {
        IFC(pDMViewport->Disable());
        IFC(pDMViewport->Disable());
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Completed."));
        }
#endif // DM_DEBUG

    }

Cleanup:
    RRETURN(hr);
}

// Interrupts the active manipulation for the provided viewport, triggering a status change to Ready.
_Check_return_ HRESULT
CDirectManipulationService::StopViewport(
    _In_ IObject* pViewport)
{
    IDirectManipulationViewport* pDMViewport = nullptr;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   StopViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pViewport);

    IFC_RETURN(GetDMViewportFromHandle(pViewport, &pDMViewport));
    if (pDMViewport != nullptr)
    {

#ifdef DM_DEBUG
        DIRECTMANIPULATION_STATUS dmStatusDbg;
        IGNOREHR(pDMViewport->GetStatus(&dmStatusDbg));
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   DM status=%d.", dmStatusDbg));
        }
#endif // DM_DEBUG

        IFC_RETURN(pDMViewport->Stop());
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::AddViewportConfiguration
//
//  Synopsis:
//    Adds a possible configuration to the provided viewport. This config
//    may be used in a future manipulation.
//    Creates an associated DM viewport if needed and adds the duo to the
//    viewport map.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::AddViewportConfiguration(
    _In_ IObject* pViewport,
    _In_ bool fIsCrossSlideViewport,
    _In_ bool fIsDragDrop,
    _In_ XDMConfigurations configuration)
{
    HRESULT hr = S_OK;
    RECT emptyRect = { 0, 0, 0, 0 };
    DIRECTMANIPULATION_STATUS dmStatus = DIRECTMANIPULATION_BUILDING;
    IDirectManipulationContent* pDMContent = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationViewport* pDMNewViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddViewportConfiguration - entry. pViewport=0x%p, configuration=%d.", this, pViewport, configuration));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pViewport);
    ASSERT(!fIsDragDrop || fIsCrossSlideViewport); //drag drop uses CrossSlide viewport

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    if (!pDMViewport)
    {
        if (fIsCrossSlideViewport)
        {
            IFC(CreateCrossSlideViewport(fIsDragDrop, &pDMViewport));
        }
        else
        {
            IFC(CreateViewport(&pDMViewport));
        }

        pDMNewViewport = pDMViewport;
        IFC(AddViewport(pViewport, pDMViewport));
    }
    ASSERT(pDMViewport);

    if (fIsCrossSlideViewport)
    {
        if (fIsDragDrop)
        {
            ASSERT(configuration == XcpDMConfigurationNone);
            IFC(AttachDragDropBehavior(pDMViewport));
        }
        else
        {
            ASSERT(configuration == XcpDMConfigurationZoom || configuration == XcpDMConfigurationPanX || configuration == XcpDMConfigurationPanY || configuration == (XcpDMConfigurationPanX | XcpDMConfigurationPanY));

            switch (configuration)
            {
            case XcpDMConfigurationPanX:
            case XcpDMConfigurationPanY:
                // Activate the configuration to include panning in one direction, although this viewport
                // should immediately be out of bounds in that direction.
                IFC(pDMViewport->ActivateConfiguration((configuration == XcpDMConfigurationPanX) ?
                DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X : DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y));

                // Indicate that this cross-slide viewport should chain input to its parent, for that same direction.
                IFC(pDMViewport->SetChaining((configuration == XcpDMConfigurationPanX) ?
                DIRECTMANIPULATION_MOTION_TRANSLATEX : DIRECTMANIPULATION_MOTION_TRANSLATEY));

                // Indicate that this viewport supports the default gestures as well as cross-slide in the perpendicular direction.
                IFC(pDMViewport->SetManualGesture(DIRECTMANIPULATION_GESTURE_DEFAULT | ((configuration == XcpDMConfigurationPanX) ?
                DIRECTMANIPULATION_GESTURE_CROSS_SLIDE_VERTICAL : DIRECTMANIPULATION_GESTURE_CROSS_SLIDE_HORIZONTAL)));
                break;

            #pragma warning(suppress : 4063)
            case XcpDMConfigurationPanX | XcpDMConfigurationPanY:
                // This viewport will block DManip until a second contact point is declared on the outer viewport.
                IFC(pDMViewport->SetInputMode(DIRECTMANIPULATION_INPUT_MODE_MANUAL));
                IFC(pDMViewport->ActivateConfiguration(DIRECTMANIPULATION_CONFIGURATION_NONE));
                break;

            case XcpDMConfigurationZoom:
                // This viewport will block DManip manipulations if a zoom gesture is recognized.
                IFC(pDMViewport->ActivateConfiguration(DIRECTMANIPULATION_CONFIGURATION_NONE));
                IFC(pDMViewport->SetManualGesture(DIRECTMANIPULATION_GESTURE_DEFAULT | DIRECTMANIPULATION_GESTURE_PINCH_ZOOM));
                break;
            }
        }

        // Cross-slide viewports are zero-sized...
        IFC(pDMViewport->SetViewportRect(&emptyRect));

        // ...and their content as well to trigger chaining immediately in case of a pan
        IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
        ASSERT(pDMContent);
        IFC(pDMContent->SetContentRect(&emptyRect));
    }
    else
    {
        IFC(pDMViewport->GetStatus(&dmStatus));
        if (dmStatus == DIRECTMANIPULATION_BUILDING || dmStatus == DIRECTMANIPULATION_DISABLED || dmStatus == DIRECTMANIPULATION_ENABLED || dmStatus == DIRECTMANIPULATION_READY)
        {
            hr = pDMViewport->AddConfiguration(GetDMConfigurations(configuration));
            if (hr == UI_E_OBJECT_SEALED)
            {
                // Because of an unavoidable race condition given DManip's APIs design, the viewport may already
                // be active even though the retrieved status is inactive. Details in Win Blue bug 38233.
                // Indicate that the configuration could not be added as requested.
                hr = S_FALSE;
            }
        }
        else
        {
            // Indicate that the configuration could not be added as requested because of an active status.
            hr = S_FALSE;
        }
    }

Cleanup:
#ifdef DM_DEBUG
    if (DMS_TraceDbg() && hr == S_FALSE)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddViewportConfiguration - configuration not added while viewport status=%d.", this, dmStatus));
    }
#endif // DM_DEBUG

    ReleaseInterface(pDMContent);
    ReleaseInterface(pDMNewViewport);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::RemoveViewportConfiguration
//
//  Synopsis:
//    Removes an existing configuration for the provided viewport. This
//    config may no longer be used in the future manipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::RemoveViewportConfiguration(
    _In_ IObject* pViewport,
    _In_ XDMConfigurations configuration)
{
    HRESULT hr = S_OK;
    DIRECTMANIPULATION_STATUS dmStatus = DIRECTMANIPULATION_BUILDING;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   RemoveViewportConfiguration - entry. pViewport=0x%p, configuration=%d.", this, pViewport, configuration));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetStatus(&dmStatus));
    if (dmStatus == DIRECTMANIPULATION_BUILDING || dmStatus == DIRECTMANIPULATION_DISABLED || dmStatus == DIRECTMANIPULATION_ENABLED || dmStatus == DIRECTMANIPULATION_READY)
    {
        hr = pDMViewport->RemoveConfiguration(GetDMConfigurations(configuration));
        if (hr == UI_E_OBJECT_SEALED)
        {
            // Because of an unavoidable race condition given DManip's APIs design, the viewport may already
            // be active even though the retrieved status is inactive. Details in Win Blue bug 38233.
            // Indicate that the configuration could not be removed as requested.
            hr = S_FALSE;
        }
    }
    else
    {
        // Indicate that the configuration could not be removed as requested because of an active status.
        hr = S_FALSE;
    }

Cleanup:
#ifdef DM_DEBUG
    if (DMS_TraceDbg() && hr == S_FALSE)
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   RemoveViewportConfiguration - configuration not removed while viewport status=%d.", this, dmStatus));
    }
#endif // DM_DEBUG

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::ActivateViewportConfiguration
//
//  Synopsis:
//    Activates an existing configuration for the provided viewport.
//    The active configuration can be changed during a manipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::ActivateViewportConfiguration(
    _In_ IObject* pViewport,
    _In_ XDMConfigurations configuration,
    _Out_ bool* activationFailed)
{
    IDirectManipulationViewport* pDMViewport = nullptr;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   ActivateViewportConfiguration - entry. pViewport=0x%p, configuration=%d.", this, pViewport, configuration));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(activationFailed);

    *activationFailed = false;

    IFC_RETURN(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    HRESULT hr = pDMViewport->ActivateConfiguration(GetDMConfigurations(configuration));
    if (hr == UI_E_OBJECT_SEALED)
    {
        // Workaround for RS1 bug 5697333.
        // This error occurs when attempting to activate a configuration that
        // was not registered with IDirectManipulationViewport::AddConfiguration
        // while the viewport is active and thus sealed.
        // Indicate that the configuration could not be activated as requested.
        *activationFailed = true;
        hr = S_FALSE;

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   ActivateViewportConfiguration - configuration not activated."));
        }
#endif // DM_DEBUG
    }
    IFC_RETURN(hr);

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::AddSecondaryContent
//
//  Synopsis:
//    Adds a secondary content to the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::AddSecondaryContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_ XDMContentType contentType)
{
    HRESULT hr = S_OK;
    bool fNewSecondaryContentAdded = false;
    bool fNewSecondaryContentsAdded = false;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pNewDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryContents = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pNewDMSecondaryContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddSecondaryContent - entry. pViewport=0x%p, pSecondaryContent=0x%p, contentType=%d.",
            this, pViewport, pSecondaryContent, contentType));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);
    ASSERT(contentType != XcpDMContentTypePrimary);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);
    IFC(m_pDMManager->CreateContent(NULL /*pFrameInfo*/, CLSID_Microsoft_ParametricMotionBehavior, IID_PPV_ARGS(&pNewDMSecondaryContent)));
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   IDMContent=0x%p.", pNewDMSecondaryContent));
    }
#endif // DM_DEBUG

    if (!m_pMapSecondaryContents)
    {
        m_pMapSecondaryContents = new xchainedmap<XHANDLE, xchainedmap<XHANDLE, IDirectManipulationContent*>*>();
    }

    IFC(m_pMapSecondaryContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));

    if (!pDMSecondaryContents)
    {
        pNewDMSecondaryContents = new xchainedmap<XHANDLE, IDirectManipulationContent*>();
        IFC(m_pMapSecondaryContents->Add(static_cast<XHANDLE>(pViewport), pNewDMSecondaryContents));
        fNewSecondaryContentsAdded = TRUE;
        pDMSecondaryContents = pNewDMSecondaryContents;
    }

#if DBG
    // Make sure this secondary content has not been added already.
    IDirectManipulationContent* pDMSecondaryContentDbg = NULL;
    IGNOREHR(pDMSecondaryContents->Get(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContentDbg));
    ASSERT(!pDMSecondaryContentDbg);
#endif // DBG

    IFC(pDMSecondaryContents->Add(static_cast<XHANDLE>(pSecondaryContent), pNewDMSecondaryContent));
    fNewSecondaryContentAdded = TRUE;

    IFC(pDMViewport->AddContent(pNewDMSecondaryContent));

    // Setup new secondary content according to contentType.
    IFC(SetupSecondaryContent(pNewDMSecondaryContent, contentType));

Cleanup:
    if (FAILED(hr))
    {
        if (pNewDMSecondaryContent)
        {
            if (fNewSecondaryContentAdded)
            {
                IGNOREHR(pDMSecondaryContents->Remove(static_cast<XHANDLE>(pSecondaryContent), pNewDMSecondaryContent));
            }
            ReleaseInterface(pNewDMSecondaryContent);
        }

        if (fNewSecondaryContentsAdded)
        {
            IGNOREHR(m_pMapSecondaryContents->Remove(static_cast<XHANDLE>(pViewport), pNewDMSecondaryContents));
            SAFE_DELETE(pNewDMSecondaryContents);
        }

        ReleaseInterface(pSecondaryContent);
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::AddSecondaryContent
//
//  Synopsis:
//    Adds a secondary content to the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::AddSecondaryContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_ XUINT32 cDefinitions,
    _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
    _In_ XFLOAT offsetX,
    _In_ XFLOAT offsetY)
{
    HRESULT hr = S_OK;
    bool fNewSecondaryContentAdded = false;
    bool fNewSecondaryContentsAdded = false;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryContents = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pNewDMSecondaryContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddSecondaryContent - entry. pViewport=0x%p, pSecondaryContent=0x%p, cDefinitions=%d.",
            this, pViewport, pSecondaryContent, cDefinitions));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);
    ASSERT(cDefinitions > 0 && pDefinitions != NULL);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    if (!m_pMapSecondaryContents)
    {
        m_pMapSecondaryContents = new xchainedmap<XHANDLE, xchainedmap<XHANDLE, IDirectManipulationContent*>*>();
    }

    IFC(m_pMapSecondaryContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));

    if (!pDMSecondaryContents)
    {
        pNewDMSecondaryContents = new xchainedmap<XHANDLE, IDirectManipulationContent*>();
        IFC(m_pMapSecondaryContents->Add(static_cast<XHANDLE>(pViewport), pNewDMSecondaryContents));
        fNewSecondaryContentsAdded = TRUE;
        pDMSecondaryContents = pNewDMSecondaryContents;
    }

    IFC(pDMSecondaryContents->Get(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));

    if (!pDMSecondaryContent)
    {
        IFC(m_pDMManager->CreateContent(NULL /*pFrameInfo*/, CLSID_Microsoft_ParametricMotionBehavior, IID_PPV_ARGS(&pDMSecondaryContent)));

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   IDMContent=0x%p.", pDMSecondaryContent));
        }
#endif // DM_DEBUG

        IFC(pDMSecondaryContents->Add(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
        IFC(pDMViewport->AddContent(pDMSecondaryContent));
    }

    fNewSecondaryContentAdded = TRUE;

    // Setup new secondary content according to contentType.
    IFC(SetupSecondaryContent(pDMSecondaryContent, cDefinitions, pDefinitions, offsetX, offsetY));

Cleanup:
    if (FAILED(hr))
    {
        if (pDMSecondaryContent)
        {
            if (fNewSecondaryContentAdded)
            {
                IGNOREHR(pDMSecondaryContents->Remove(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
            }
            ReleaseInterface(pDMSecondaryContent);
        }

        if (fNewSecondaryContentsAdded)
        {
            IGNOREHR(m_pMapSecondaryContents->Remove(static_cast<XHANDLE>(pViewport), pNewDMSecondaryContents));
            SAFE_DELETE(pNewDMSecondaryContents);
        }

        ReleaseInterface(pSecondaryContent);
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::RemoveSecondaryContent
//
//  Synopsis:
//    Removes the provided secondary content from the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::RemoveSecondaryContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_opt_ DMDeferredRelease* pDMDeferredRelease)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   RemoveSecondaryContent - entry. pViewport=0x%p, pSecondaryContent=0x%p, pDMDeferredRelease set=%d.",
            this, pViewport, pSecondaryContent, pDMDeferredRelease != nullptr));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(m_pMapSecondaryContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));

    ASSERT(pDMSecondaryContents);
    IFC(pDMSecondaryContents->Remove(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
    ASSERT(pDMSecondaryContent);

    if (pDMDeferredRelease != nullptr)
    {
        // We are in the midst of synchronizing a change.  Don't actually release the content,
        // instead, transfer it over to the DMDeferredRelease object.
        // See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
        pDMDeferredRelease->SetDMContent(pDMSecondaryContent);
        pDMDeferredRelease->SetDMCompositor(m_pDMCompositor);
        pDMDeferredRelease->SetDMViewport(pDMViewport);
    }
    else
    {
        IFC(pDMViewport->RemoveContent(pDMSecondaryContent));
    }
    ReleaseInterface(pDMSecondaryContent);

    if (pDMSecondaryContents->Count() == 0)
    {
        IFC(m_pMapSecondaryContents->Remove(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));
        SAFE_DELETE(pDMSecondaryContents);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::AddSecondaryClipContent
//
//  Synopsis:
//    Adds a secondary clip content to the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::AddSecondaryClipContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_ XUINT32 cDefinitions,
    _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
    _In_ XFLOAT offsetX,
    _In_ XFLOAT offsetY)
{
    HRESULT hr = S_OK;
    bool fNewSecondaryClipContentAdded = false;
    bool fNewSecondaryClipContentsAdded = false;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMSecondaryClipContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryClipContents = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pNewDMSecondaryClipContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddSecondaryClipContent - entry. pViewport=0x%p, pSecondaryContent=0x%p",
            this, pViewport, pSecondaryContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);
    ASSERT(cDefinitions > 0 && pDefinitions != NULL);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    if (!m_pMapSecondaryClipContents)
    {
        m_pMapSecondaryClipContents = new xchainedmap<XHANDLE, xchainedmap<XHANDLE, IDirectManipulationContent*>*>();
    }

    IFC(m_pMapSecondaryClipContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryClipContents));

    if (!pDMSecondaryClipContents)
    {
        pNewDMSecondaryClipContents = new xchainedmap<XHANDLE, IDirectManipulationContent*>();
        IFC(m_pMapSecondaryClipContents->Add(static_cast<XHANDLE>(pViewport), pNewDMSecondaryClipContents));
        fNewSecondaryClipContentsAdded = TRUE;
        pDMSecondaryClipContents = pNewDMSecondaryClipContents;
    }

    IFC(pDMSecondaryClipContents->Get(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryClipContent));

    if (!pDMSecondaryClipContent)
    {
        IFC(m_pDMManager->CreateContent(NULL /*pFrameInfo*/, CLSID_Microsoft_ParametricMotionBehavior, IID_PPV_ARGS(&pDMSecondaryClipContent)));
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   IDMContent=0x%p.", pDMSecondaryClipContent));
        }
#endif // DM_DEBUG

        IFC(pDMSecondaryClipContents->Add(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryClipContent));
        IFC(pDMViewport->AddContent(pDMSecondaryClipContent));
    }

    fNewSecondaryClipContentAdded = TRUE;

    // Setup new secondary content according to contentType.
    IFC(SetupSecondaryContent(pDMSecondaryClipContent, cDefinitions, pDefinitions, offsetX, offsetY));

Cleanup:
    if (FAILED(hr))
    {
        if (pDMSecondaryClipContent)
        {
            if (fNewSecondaryClipContentAdded)
            {
                IGNOREHR(pDMSecondaryClipContents->Remove(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryClipContent));
            }
            ReleaseInterface(pDMSecondaryClipContent);
        }

        if (fNewSecondaryClipContentsAdded)
        {
            IGNOREHR(m_pMapSecondaryClipContents->Remove(static_cast<XHANDLE>(pViewport), pNewDMSecondaryClipContents));
            SAFE_DELETE(pNewDMSecondaryClipContents);
        }

        ReleaseInterface(pSecondaryContent);
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::RemoveSecondaryClipContent
//
//  Synopsis:
//    Removes the provided secondary clip content from the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::RemoveSecondaryClipContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_opt_ DMDeferredRelease* pDMDeferredRelease)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryClipContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   RemoveSecondaryClipContent - entry. pViewport=0x%p, pSecondaryContent=0x%p.",
            this, pViewport, pSecondaryContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryClipContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(m_pMapSecondaryClipContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryClipContents));

    ASSERT(pDMSecondaryClipContents);
    IFC(pDMSecondaryClipContents->Remove(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
    ASSERT(pDMSecondaryContent);

    if (pDMDeferredRelease != nullptr)
    {
        // We are in the midst of synchronizing a change.  Don't actually release the content,
        // instead, transfer it over to the DMDeferredRelease object.
        // See more details on deferred DM Release in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
        pDMDeferredRelease->SetDMContent(pDMSecondaryContent);
        pDMDeferredRelease->SetDMCompositor(m_pDMCompositor);
        pDMDeferredRelease->SetDMViewport(pDMViewport);
    }
    else
    {
        IFC(pDMViewport->RemoveContent(pDMSecondaryContent));
    }

    ReleaseInterface(pDMSecondaryContent);

    if (pDMSecondaryClipContents->Count() == 0)
    {
        IFC(m_pMapSecondaryClipContents->Remove(static_cast<XHANDLE>(pViewport), pDMSecondaryClipContents));
        SAFE_DELETE(pDMSecondaryClipContents);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetViewportChaining
//
//  Synopsis:
//    Pushes a chaining setting to DirectManipulation for a given viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetViewportChaining(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionTypes)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    DIRECTMANIPULATION_MOTION_TYPES dmMotionTypes = GetDMMotionTypes(motionTypes);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetViewportChaining - entry. pViewport=0x%p, motionTypes=%d.", this, pViewport, motionTypes));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->SetChaining(dmMotionTypes));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetViewportInputTransform
//
//  Synopsis:
//    Declares an input matrix to DirectManipulation for the provided viewport.
//    This matrix is used in case the viewport has a render transform.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetViewportInputTransform(
    _In_ IObject* pViewport,
    _In_ CMILMatrix* pInputTransform)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    XFLOAT matrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetViewportInputTransform - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(pInputTransform);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   M11=%4.6lf, M12=%4.6lf, M21=%4.6lf, M22=%4.6lf, Dx=%4.6lf, Dy=%4.6lf.",
            pInputTransform->GetM11(), pInputTransform->GetM12(), pInputTransform->GetM21(), pInputTransform->GetM22(), pInputTransform->GetDx(), pInputTransform->GetDy()));
    }
#endif // DM_DEBUG

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    matrix[0] = pInputTransform->GetM11();
    matrix[1] = pInputTransform->GetM12();
    matrix[2] = pInputTransform->GetM21();
    matrix[3] = pInputTransform->GetM22();
    matrix[4] = pInputTransform->GetDx();
    matrix[5] = pInputTransform->GetDy();

    IFC(pDMViewport->SetViewportTransform(reinterpret_cast<XFLOAT*>(&matrix), 6));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetViewportStatus
//
//  Synopsis:
//    Accesses the status for the provided viewport. Returns a PAL version
//    of the status.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetViewportStatus(
    _In_ IObject* pViewport,
    _Out_ XDMViewportStatus& status)
{
    HRESULT hr = S_OK;
    bool fViewportRegistered = false;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetViewportStatus - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    status = XcpDMViewportBuilding;

    IFC(IsViewportHandleRegistered(pViewport, &fViewportRegistered));
    if (fViewportRegistered)
    {
        IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
        ASSERT(pDMViewport);
        IFC(GetViewportStatus(pDMViewport, status));

        if (m_autoScrollStatus != AutoScrollStopped && status == XcpDMViewportInertia)
        {
            // Expose the DManip auto-scroll inertia status as the XcpDMViewportAutoRunning internal status.
            status = XcpDMViewportAutoRunning;
        }
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetViewportCenterPoint
//
//  Synopsis:
//    Accesses the center point of the provided viewport.
//    Returns coordinates in relation to the top left corner of the
//    primary content.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetViewportCenterPoint(
    _In_ IObject* pViewport,
    _Out_ XFLOAT& centerX,
    _Out_ XFLOAT& centerY)
{
    HRESULT hr = S_OK;
    DIRECTMANIPULATION_STATUS dmStatus;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetViewportCenterPoint - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(nullptr != m_islandInputSite);

    ASSERT(pViewport);
    centerX = 0.0f;
    centerY = 0.0f;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetStatus(&dmStatus));
    if (dmStatus == DIRECTMANIPULATION_RUNNING)
    {
        IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
        ASSERT(pDMContent);

        IFC(pDMContent->GetCenterPoint(&centerX, &centerY));
    }

Cleanup:
    ReleaseInterface(pDMContent);
#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"                   centerX=%4.6lf, centerY=%4.6lf.", centerX, centerY));
    }
#endif // DM_DEBUG

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetContentInertiaEndTransform
//
//  Synopsis:
//    Retrieves the primary content's inertia end transform.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetContentInertiaEndTransform(
    _In_ IObject* pViewport,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& zoomFactor)
{
    HRESULT hr = S_OK;
    XFLOAT matrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    DIRECTMANIPULATION_STATUS dmStatus;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetContentInertiaEndTransform - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(nullptr != m_islandInputSite);

    ASSERT(pViewport);
    translationX = 0.0f;
    translationY = 0.0f;
    zoomFactor = 1.0f;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetStatus(&dmStatus));
    if (dmStatus == DIRECTMANIPULATION_INERTIA)
    {
        IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
        ASSERT(pDMContent);

        hr = pDMContent->GetInertiaEndTransform(reinterpret_cast<XFLOAT*>(&matrix), 6);
        if (hr == E_FAIL)
        {
            // Because of an unavoidable race condition given DManip's APIs, DManip occasionally returns E_FAIL
            // even though the retrieved status is DIRECTMANIPULATION_INERTIA. Details in Win Blue bug 38233.
            // Indicate that no inertia-end-transform was retrieved.
            hr = S_FALSE;
            goto Cleanup;
        }

        translationX = matrix[4];
        translationY = matrix[5];
        zoomFactor = matrix[0];
    }
    else
    {
        // Indicate that no inertia-end-transform was retrieved.
        hr = S_FALSE;
    }

Cleanup:
    ReleaseInterface(pDMContent);
#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"                   DM status=%d, translationX=%4.6lf, translationY=%4.6lf, zoomFactor=%4.8lf, hr=%d.", dmStatus, translationX, translationY, zoomFactor, hr));
    }
#endif // DM_DEBUG
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::BringIntoViewport
//
//  Synopsis:
//    Invokes the DirectManipulation ZoomToRect method to scroll/zoom
//    a section of the primary viewport content into view.
//    The move is instantaneous when fAnimate is False.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::BringIntoViewport(
    _In_ IObject* pViewport,
    _In_ XRECTF& bounds,
    _In_ bool fAnimate)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   BringIntoViewport - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   bounds.X=%4.6lf, bounds.Y=%4.6lf, bounds.Width=%4.6lf, bounds.Height=%4.6lf, fAnimate=%d.", bounds.X, bounds.Y, bounds.Width, bounds.Height, fAnimate));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->ZoomToRect(bounds.X, bounds.Y, bounds.X + bounds.Width, bounds.Y + bounds.Height, fAnimate));

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        DIRECTMANIPULATION_STATUS dmStatusDbg;
        IGNOREHR(pDMViewport->GetStatus(&dmStatusDbg));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   After ZoomToRect call: status=%d, hr=%d.", dmStatusDbg, hr));

        XFLOAT translationX = 0.0f;
        XFLOAT translationY = 0.0f;
        XFLOAT uncompressedZoomFactor = 1.0f;
        XFLOAT zoomFactorX = 1.0f;
        XFLOAT zoomFactorY = 1.0f;
        IGNOREHR(GetPrimaryContentTransform(pViewport, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   Current output transform: translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.",
            translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetViewportBounds
//
//  Synopsis:
//    Returns the bounds of the viewport in client coordinates.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetViewportBounds(
    _In_ IObject* pViewport,
    _Out_ XRECTF& clientBounds)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    RECT viewportBounds = { 0, 0, 0, 0 };

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetViewportBounds - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetViewportRect(&viewportBounds));
    clientBounds.X = static_cast<XFLOAT>(viewportBounds.left);
    clientBounds.Width = static_cast<XFLOAT>(viewportBounds.right - clientBounds.X);
    clientBounds.Y = static_cast<XFLOAT>(viewportBounds.top);
    clientBounds.Height = static_cast<XFLOAT>(viewportBounds.bottom - clientBounds.Y);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   clientBounds.X=%4.6lf, clientBounds.Y=%4.6lf, clientBounds.Width=%4.6lf, clientBounds.Height=%4.6lf.",
            clientBounds.X, clientBounds.Y, clientBounds.Width, clientBounds.Height));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetViewportBounds
//
//  Synopsis:
//    Pushes new bounds for the provided viewport to DirectManipulation.
//    The provided bounds are in client coordinates.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetViewportBounds(
    _In_ IObject* pViewport,
    _In_ const XRECTF& clientBounds)
{
    HRESULT hr = S_OK;
    RECT viewportBounds = { 0, 0, 0, 0 };
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   SetViewportBounds - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   clientBounds.X=%4.6lf, clientBounds.Y=%4.6lf, clientBounds.Width=%4.6lf, clientBounds.Height=%4.6lf.", clientBounds.X, clientBounds.Y, clientBounds.Width, clientBounds.Height));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    viewportBounds.left = GetRoundedLong(clientBounds.X);
    if (clientBounds.Width > 0.0f && clientBounds.Width < 1.0f)
    {
        // Avoid a nil viewport width when the actual width is between 0 and 1 pixel.
        // Declare a 1 pixel width instead.
        viewportBounds.right = viewportBounds.left + 1;
    }
    else
    {
        // To accommodate snap points, always round down the viewport width,
        // instead of picking the closest integer.
        viewportBounds.right = viewportBounds.left + GetRoundedDownLong(clientBounds.Width);
    }

    viewportBounds.top = GetRoundedLong(clientBounds.Y);
    if (clientBounds.Height > 0.0f && clientBounds.Height < 1.0f)
    {
        // Avoid a nil viewport height when the actual height is between 0 and 1 pixel.
        // Declare a 1 pixel height instead.
        viewportBounds.bottom = viewportBounds.top + 1;
    }
    else
    {
        // To accommodate snap points, always round down the viewport height,
        // instead of picking the closest integer.
        viewportBounds.bottom = viewportBounds.top + GetRoundedDownLong(clientBounds.Height);
    }

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   Calling SetViewportRect(L=%d, T=%d, R=%d, B=%d).", viewportBounds.left, viewportBounds.top, viewportBounds.right, viewportBounds.bottom));
    }
#endif // DM_DEBUG

    IFC(pDMViewport->SetViewportRect(&viewportBounds));

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetContentBounds
//
//  Synopsis:
//    Returns the bounds of the primary content for the provided viewport.
//    The bounds are in relation to the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetContentBounds(
    _In_ IObject* pViewport,
    _Out_ XRECTF& bounds)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMContent = NULL;
    RECT contentBounds = { 0, 0, 0, 0 };

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetContentBounds - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(pViewport);

    ASSERT(m_pDMManager);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(pDMContent->GetContentRect(&contentBounds));

    bounds.X = static_cast<XFLOAT>(contentBounds.left);
    bounds.Width = static_cast<XFLOAT>(contentBounds.right - bounds.X);
    bounds.Y = static_cast<XFLOAT>(contentBounds.top);
    bounds.Height = static_cast<XFLOAT>(contentBounds.bottom - bounds.Y);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   bounds.X=%4.6lf, bounds.Y=%4.6lf, bounds.Width=%4.6lf, bounds.Height=%4.6lf.", bounds.X, bounds.Y, bounds.Width, bounds.Height));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetContentBounds
//
//  Synopsis:
//    Pushes new primary content bounds for the provided viewport to
//    DirectManipulation. The provided bounds are in relation to the viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetContentBounds(
    _In_ IObject* pViewport,
    _In_ XRECTF& bounds)
{
    IDirectManipulationViewport* dmViewportNoRef = nullptr;
    Microsoft::WRL::ComPtr<IDirectManipulationContent> dmContent;
    RECT contentBounds = { 0, 0, 0, 0 };

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   SetContentBounds - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   bounds.X=%4.6lf, bounds.Y=%4.6lf, bounds.Width=%4.6lf, bounds.Height=%4.6lf.", bounds.X, bounds.Y, bounds.Width, bounds.Height));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFCFAILFAST(GetDMViewportFromHandle(pViewport, &dmViewportNoRef));
    ASSERT(dmViewportNoRef);

    IFCFAILFAST(dmViewportNoRef->GetPrimaryContent(IID_PPV_ARGS(&dmContent)));
    ASSERT(dmContent);

    // To accommodate snap points, always round up the content sizes, instead of picking the closest integers.
    contentBounds.left = GetRoundedLong(bounds.X);
    contentBounds.right = contentBounds.left + GetRoundedUpLong(bounds.Width);
    contentBounds.top = GetRoundedLong(bounds.Y);
    contentBounds.bottom = contentBounds.top + GetRoundedUpLong(bounds.Height);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"                   Calling SetContentRect(L=%d, T=%d, R=%d, B=%d).", contentBounds.left, contentBounds.top, contentBounds.right, contentBounds.bottom));
    }
#endif // DM_DEBUG

    IFCFAILFAST(dmContent->SetContentRect(&contentBounds));

    // We also need to re-evaluate overpan curves
    IFCFAILFAST(RefreshOverpanCurves(pViewport));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetPrimaryContentTransform
//
//  Synopsis:
//    Accesses the transformation matrix for the primary content of the
//    provided viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetPrimaryContentTransform(
    _In_ IObject* pViewport,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetPrimaryContentTransform - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG
    ASSERT(m_pDMManager);
    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(GetDMTransform(
        pDMContent,
        pViewport,
        XcpDMContentTypePrimary,
        translationX,
        translationY,
        uncompressedZoomFactor,
        zoomFactorX,
        zoomFactorY));

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"                   translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetSecondaryContentTransform
//
//  Synopsis:
//    Accesses the transformation matrix for the secondary content of the
//    provided viewport & content.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetSecondaryContentTransform(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _In_ XDMContentType contentType,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    IDirectManipulationContent* pDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryContents = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetSecondaryContentTransform - entry. pViewport=0x%p, pSecondaryContent=0x%p.",
            this, pViewport, pSecondaryContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);

    IFC(m_pMapSecondaryContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));
    if (pDMSecondaryContents)
    {
        IFC(pDMSecondaryContents->Get(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
        if (pDMSecondaryContent)
        {
            IFC(GetDMTransform(
                pDMSecondaryContent,
                pViewport,
                contentType,
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));
        }
    }

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"                   translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetSecondaryClipContentTransform
//
//  Synopsis:
//    Accesses the transformation matrix for the secondary clip content of the
//    provided viewport & content.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetSecondaryClipContentTransform(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryClipContent,
    _In_ XDMContentType contentType,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    IDirectManipulationContent* pDMSecondaryClipContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryClipContents = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetSecondaryClipContentTransform - entry. pViewport=0x%p, pSecondaryClipContent=0x%p.",
            this, pViewport, pSecondaryClipContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryClipContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryClipContent);

    IFC(m_pMapSecondaryClipContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryClipContents));
    if (pDMSecondaryClipContents)
    {
        IFC(pDMSecondaryClipContents->Get(static_cast<XHANDLE>(pSecondaryClipContent), pDMSecondaryClipContent));
        if (pDMSecondaryClipContent)
        {
            IFC(GetDMTransform(
                pDMSecondaryClipContent,
                pViewport,
                contentType,
                translationX,
                translationY,
                uncompressedZoomFactor,
                zoomFactorX,
                zoomFactorY));
        }
    }

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"                   translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetContentAlignment
//
//  Synopsis:
//    Pushes either horizontal or vertical alignment for the primary content
//    of the provided viewport to DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetContentAlignment(
    _In_ IObject* pViewport,
    _In_ XDMAlignment alignment,
    _In_ bool fIsHorizontal)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   SetContentAlignment - entry. pViewport=0x%p, alignment=%d, fIsHorizontal=%d.", this, pViewport, alignment, fIsHorizontal));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    if (fIsHorizontal)
    {
        IFC(pDMContent->SetHorizontalAlignment(GetDMHorizontalAlignment(alignment)));
    }
    else
    {
        IFC(pDMContent->SetVerticalAlignment(GetDMVerticalAlignment(alignment)));
    }

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentSnapPoints
//
//  Synopsis:
//    Pushes regular snap points to DirectManipulation for the provided
//    viewport's primary content and motion type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentSnapPoints(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType,
    _In_ XFLOAT offset,
    _In_ XFLOAT interval)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetPrimaryContentSnapPoints - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   motionType=%d, offset=%4.6lf, interval=%4.6lf.", motionType, offset, interval));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(pDMContent->SetSnapInterval(GetDMMotionTypes(motionType), interval, offset));

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentSnapPoints
//
//  Synopsis:
//    Pushes irregular snap points to DirectManipulation for the provided
//    viewport's primary content and motion type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentSnapPoints(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType,
    _In_ XUINT32 cSnapPoints,
    _In_reads_opt_(cSnapPoints) XFLOAT* pSnapPoints)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetPrimaryContentSnapPoints - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   motionType=%d, cSnapPoints=%d.", motionType, cSnapPoints));
        for (XUINT32 iSnapPoint = 0; iSnapPoint < cSnapPoints; iSnapPoint++)
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   pSnapPoints[%d]=%4.6lf", iSnapPoint, pSnapPoints[iSnapPoint]));
        }
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(pDMContent->SetSnapPoints(GetDMMotionTypes(motionType), pSnapPoints, cSnapPoints));

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentSnapPointsType
//
//  Synopsis:
//    Pushes the snap points type to DirectManipulation for the provided
//    viewport and motion type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentSnapPointsType(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType,
    _In_ bool fAreSnapPointsOptional,
    _In_ bool fAreSnapPointsSingle)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;
    DIRECTMANIPULATION_SNAPPOINT_TYPE snapPointType;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetPrimaryContentSnapPointsType - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   motionType=%d, optional=%d, single=%d.", motionType, fAreSnapPointsOptional, fAreSnapPointsSingle));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    {
        if (fAreSnapPointsSingle)
        {
            snapPointType = fAreSnapPointsOptional ? DIRECTMANIPULATION_SNAPPOINT_OPTIONAL_SINGLE : DIRECTMANIPULATION_SNAPPOINT_MANDATORY_SINGLE;
        }
        else
        {
            snapPointType = fAreSnapPointsOptional ? DIRECTMANIPULATION_SNAPPOINT_OPTIONAL : DIRECTMANIPULATION_SNAPPOINT_MANDATORY;
        }

        IFC(pDMContent->SetSnapType(GetDMMotionTypes(motionType), snapPointType));
    }

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentSnapPointsCoordinate
//
//  Synopsis:
//    Pushes the snap points coordinate to DirectManipulation for the provided
//    viewport and motion type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentSnapPointsCoordinate(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType,
    _In_ XDMSnapCoordinate coordinate,
    _In_ XFLOAT origin)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;
    DIRECTMANIPULATION_SNAPPOINT_COORDINATE DMCoordinate = GetDMSnapCoordinate(coordinate);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetPrimaryContentSnapPointsCoordinate - entry. pViewport=0x%p.", this, pViewport));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   motionType=%d, coordinate=%d, origin=%4.6lf.", motionType, coordinate, origin));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(pDMContent->SetSnapCoordinate(GetDMMotionTypes(motionType), DMCoordinate, origin));

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentZoomBoundaries
//
//  Synopsis:
//    Pushes zoom boundaries for the provided viewport's primary content
//    to DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentZoomBoundaries(
    _In_ IObject* pViewport,
    _In_ XFLOAT minZoomFactor,
    _In_ XFLOAT maxZoomFactor)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationPrimaryContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetPrimaryContentZoomBoundaries - entry. pViewport=0x%p, minZoomFactor=%4.8lf, maxZoomFactor=%4.8lf.", this, pViewport, minZoomFactor, maxZoomFactor));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(minZoomFactor >= DIRECTMANIPULATION_MINIMUM_ZOOM);
    ASSERT(minZoomFactor <= maxZoomFactor);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(pDMContent->SetZoomBoundaries(minZoomFactor, maxZoomFactor));

Cleanup:
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetPrimaryContentTransform
//
//  Synopsis:
//    Pushes translation and zoom transformation values for the viewport's
//    primary content to DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetPrimaryContentTransform(
    _In_ IObject* pViewport,
    _In_ float translationX,
    _In_ float translationY,
    _In_ float zoomFactor)
{
    HRESULT hr = S_OK;
    float matrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    IDirectManipulationViewport* pDMViewport = nullptr;
    Microsoft::WRL::ComPtr<IDirectManipulationContent> spDMContent;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   SetPrimaryContentTransform - entry. pViewport=0x%p, translationX=%4.6lf, translationY=%4.6lf, zoomFactor=%4.8lf.", this, pViewport, translationX, translationY, zoomFactor));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager != nullptr);
    ASSERT(pViewport != nullptr);
    ASSERT(zoomFactor > 0.0f);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport != nullptr);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&spDMContent)));
    ASSERT(spDMContent != nullptr);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        if (spDMContent != nullptr)
        {
            float dbgMatrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            IGNOREHR(spDMContent->GetContentTransform(reinterpret_cast<float*>(&dbgMatrix), 6));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
                L"                   old ContentTransform:     translationX=%4.6lf, translationY=%4.6lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", dbgMatrix[4], dbgMatrix[5], dbgMatrix[0], dbgMatrix[3]));
            IGNOREHR(spDMContent->GetOutputTransform(reinterpret_cast<float*>(&dbgMatrix), 6));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
                L"                   old OutputTransform:      translationX=%4.6lf, translationY=%4.6lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", dbgMatrix[4], dbgMatrix[5], dbgMatrix[0], dbgMatrix[3]));
        }
    }
#endif // DM_DEBUG

    matrix[0] = zoomFactor;
    matrix[3] = zoomFactor;
    matrix[4] = translationX;
    matrix[5] = translationY;

    IFC(m_pDMHelper->SetPrimaryContentTransform(spDMContent.Get(), matrix, ARRAYSIZE(matrix)));

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        if (spDMContent != nullptr)
        {
            float dbgMatrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
            IGNOREHR(spDMContent->GetContentTransform(reinterpret_cast<float*>(&dbgMatrix), 6));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
                L"                   new ContentTransform:     translationX=%4.6lf, translationY=%4.6lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", dbgMatrix[4], dbgMatrix[5], dbgMatrix[0], dbgMatrix[3]));
            IGNOREHR(spDMContent->GetOutputTransform(reinterpret_cast<float*>(&dbgMatrix), 6));
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
                L"                   new OutputTransform:      translationX=%4.6lf, translationY=%4.6lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", dbgMatrix[4], dbgMatrix[5], dbgMatrix[0], dbgMatrix[3]));
        }
    }
#endif // DM_DEBUG

Cleanup:
    return hr;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorService
//
//  Synopsis:
//    Returns an IPALDirectManipulationCompositorService implementation
//    that the compositor can use to interact with DirectManipulation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorService(
    _Outptr_ IPALDirectManipulationCompositorService** ppCompositorService)
{
    HRESULT hr = S_OK;

    ASSERT(ppCompositorService);
    *ppCompositorService = static_cast<IPALDirectManipulationCompositorService*>(this);
    AddRef();

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorViewport
//
//  Synopsis:
//    Returns a PAL-friendly ref-counted object representing the provided viewport.
//    That object is used by the compositor in its usage of
//    IPALDirectManipulationCompositorService.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorViewport(
    _In_ IObject* pViewport,
    _Outptr_ IObject** ppCompositorViewport)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorViewport = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetCompositorViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(ppCompositorViewport);
    *ppCompositorViewport = NULL;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(CCOMObjectWrapper::Create(pDMViewport, &pCompositorViewport));
    *ppCompositorViewport = pCompositorViewport;
    pCompositorViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Returning pCompositorViewport=0x%p.", *ppCompositorViewport));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pCompositorViewport);
    RRETURN(hr);
}

// Find the mapping from pViewport => IDirectManipulationViewport and return it if found
_Check_return_ HRESULT
CDirectManipulationService::GetDirectManipulationViewport(_In_ IObject* pViewport, _Outptr_ IDirectManipulationViewport** ppDirectManipulationViewport)
{
    IFC_RETURN(GetDMViewportFromHandle(pViewport, ppDirectManipulationViewport));
    ASSERT(*ppDirectManipulationViewport);
    AddRefInterface(*ppDirectManipulationViewport);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   GetDirectManipulationViewport returning viewport=0x%p.", *ppDirectManipulationViewport));
    }
#endif // DM_DEBUG

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorPrimaryContent
//
//  Synopsis:
//    Returns a PAL-friendly handle representing the primary content of
//    the provided viewport. That handle is used by the compositor in its
//    usage of IPALDirectManipulationCompositorService.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorPrimaryContent(
    _In_ IObject* pViewport,
    _Outptr_ IObject** ppCompositorContent)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorContent = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;
    IDirectManipulationContent* pDMContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetCompositorPrimaryContent - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);

    ASSERT(pViewport);
    ASSERT(ppCompositorContent);
    *ppCompositorContent = NULL;

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport);

    IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&pDMContent)));
    ASSERT(pDMContent);

    IFC(CCOMObjectWrapper::Create(pDMContent, &pCompositorContent));
    *ppCompositorContent = pCompositorContent;
    pCompositorContent = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Returning pCompositorContent=0x%p.", *ppCompositorContent));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pCompositorContent);
    ReleaseInterface(pDMContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorSecondaryContent
//
//  Synopsis:
//    Returns a PAL-friendly ref-counted object representing the secondary content of the provided viewport &
//    content. That handle is used by the compositor in its usage of IPALDirectManipulationCompositorService.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorSecondaryContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryContent,
    _Outptr_ IObject** ppCompositorSecondaryContent)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorSecondaryContent = NULL;
    IDirectManipulationContent* pDMSecondaryContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetCompositorSecondaryContent - entry. pViewport=0x%p, pSecondaryContent=0x%p.",
            this, pViewport, pSecondaryContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryContent);
    ASSERT(ppCompositorSecondaryContent);
    *ppCompositorSecondaryContent = NULL;

    IFC(m_pMapSecondaryContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryContents));
    if (pDMSecondaryContents)
    {
        IFC(pDMSecondaryContents->Get(static_cast<XHANDLE>(pSecondaryContent), pDMSecondaryContent));
        if (pDMSecondaryContent)
        {
            IFC(CCOMObjectWrapper::Create(pDMSecondaryContent, &pCompositorSecondaryContent));
            *ppCompositorSecondaryContent = pCompositorSecondaryContent;
            pCompositorSecondaryContent = NULL;
        }
    }

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Returning pCompositorSecondaryContent=0x%p.", *ppCompositorSecondaryContent));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pCompositorSecondaryContent);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorSecondaryClipContent
//
//  Synopsis:
//    Returns a PAL-friendly ref-counted object representing the secondary clip content of the provided viewport &
//    content. That handle is used by the compositor in its usage of IPALDirectManipulationCompositorService.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorSecondaryClipContent(
    _In_ IObject* pViewport,
    _In_ IObject* pSecondaryClipContent,
    _Outptr_ IObject** ppCompositorSecondaryClipContent)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorSecondaryClipContent = NULL;
    IDirectManipulationContent* pDMSecondaryClipContent = NULL;
    xchainedmap<XHANDLE, IDirectManipulationContent*>* pDMSecondaryClipContents = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   GetCompositorSecondaryClipContent - entry. pViewport=0x%p, pSecondaryClipContent=0x%p.",
            this, pViewport, pSecondaryClipContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(m_pMapSecondaryClipContents);
    ASSERT(pViewport);
    ASSERT(pSecondaryClipContent);
    ASSERT(ppCompositorSecondaryClipContent);
    *ppCompositorSecondaryClipContent = NULL;

    IFC(m_pMapSecondaryClipContents->Get(static_cast<XHANDLE>(pViewport), pDMSecondaryClipContents));
    if (pDMSecondaryClipContents)
    {
        IFC(pDMSecondaryClipContents->Get(static_cast<XHANDLE>(pSecondaryClipContent), pDMSecondaryClipContent));
        if (pDMSecondaryClipContent)
        {
            IFC(CCOMObjectWrapper::Create(pDMSecondaryClipContent, &pCompositorSecondaryClipContent));
            *ppCompositorSecondaryClipContent = pCompositorSecondaryClipContent;
            pCompositorSecondaryClipContent = NULL;
        }
    }

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   Returning pCompositorSecondaryClipContent=0x%p.", *ppCompositorSecondaryClipContent));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pCompositorSecondaryClipContent);
    RRETURN(hr);
}

// Re-creates DManip Compositor and FrameInfoProvider, used only by test hook.
_Check_return_ HRESULT
CDirectManipulationService::ResetCompositor()
{
    HRESULT hr = S_OK;

    if (m_pDMCompositor != nullptr)
    {
        m_sharedState->ReleaseSharedDCompManipulationCompositor(m_pDMCompositor);
        ReleaseInterface(m_pDMFrameInfoProvider);
        IFC(m_sharedState->GetSharedDCompManipulationCompositor(&m_pDMCompositor));
        IFC(EnsureFrameInfoProvider());
        IFC(m_pDMCompositor->SetUpdateManager(m_pDMUpdateManager));
    }

Cleanup:
    return hr;
}

// Primary and Secondary transforms are both created by DManip's DComp device.  Primary is always created, Secondary is
// only present if overpan mode is none.
_Check_return_ HRESULT
CDirectManipulationService::EnsureSharedContentTransform(
    _In_ ixp::ICompositor* compositor,
    _In_ IObject* pContent,
    _In_ XDMContentType contentType,
    _Outptr_result_nullonfailure_ IUnknown** sharedPrimaryTransform,
    _Outptr_result_maybenull_ IUnknown** sharedSecondaryTransform)
{
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   EnsureSharedContentTransform - pViewport=0x%p, pContent=0x%p, contentType=%d.",
            this, m_mapViewports.Count() ? m_mapViewports.begin()->first : nullptr, pContent, contentType));
    }
#endif // DM_DEBUG

    *sharedPrimaryTransform = nullptr;
    *sharedSecondaryTransform = nullptr;

    ASSERT(m_pDMCompositor != nullptr);

    xref_ptr<IObject> spContent(pContent);
    SharedTransformParts parts;

    auto itFind = m_DCompTransformsMap.find(spContent);
    if (itFind == m_DCompTransformsMap.end())
    {
        // DManip hasn't created any shared transforms yet.  Create those now.
        bool isSharedTransformForOverpanReflexes = ShouldUseOverpanReflexesForContentType(contentType);
        if (isSharedTransformForOverpanReflexes)
        {
            // Case 1:  Overpan mode None is in effect.  Create Primary and Secondary transforms.
            IFC_RETURN(CreateSharedContentTransformsForOverpanReflex(
                compositor,
                contentType,
                parts.spPrimarySharedTransform.ReleaseAndGetAddressOf(),
                parts.spSecondarySharedTransform.ReleaseAndGetAddressOf()));
        }
        else
        {
            // Case 2:  Default overpan.  Just Create Primary transform
            IFC_RETURN(CreateSharedContentTransformForContent(compositor, pContent, parts.spPrimarySharedTransform.ReleaseAndGetAddressOf()));
        }
        switch (contentType)
        {
            case XcpDMContentTypePrimary:
                m_isPrimarySharedTransformForOverpanReflexes = isSharedTransformForOverpanReflexes;
                break;
            case XcpDMContentTypeTopHeader:
                m_isTopHeaderSharedTransformForOverpanReflexes = isSharedTransformForOverpanReflexes;
                break;
            case XcpDMContentTypeLeftHeader:
                m_isLeftHeaderSharedTransformForOverpanReflexes = isSharedTransformForOverpanReflexes;
                break;
        }

        // Add an entry to the map to store the Content -> Parts relationship
        m_DCompTransformsMap.insert(std::make_pair(spContent, parts));
    }
    else
    {
        // We already have shared transforms.  There's no need to re-create them.
        parts = itFind->second;
    }

    parts.spPrimarySharedTransform.CopyTo(sharedPrimaryTransform);
    parts.spSecondarySharedTransform.CopyTo(sharedSecondaryTransform);

    TraceCreateDManipSharedTransformInfo();

    return S_OK;
}

// Release the shared DManip transform
_Check_return_ HRESULT
CDirectManipulationService::ReleaseSharedContentTransform(
    _In_ IObject* pContent,
    _In_ XDMContentType contentType)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ReleaseSharedContentTransform - pViewport=0x%p, pContent=0x%p, contentType=%d.",
            this, m_mapViewports.Count() ? m_mapViewports.begin()->first : nullptr, pContent, contentType));
    }
#endif // DM_DEBUG

    // For now the code is in a situation where we can toggle between DManip-on-DComp being
    // enabled/disabled on the fly.  Because of this, it is possible the service has had DManip-on-DComp
    // disabled before the CompNode has a chance to release the shared transform.
    // In this case we've already released the compositor.  There are no leaks or harmful side-effects
    // so for now we just check for the presence of our compositor and no-op if it's NULL.
    if (m_pDMCompositor != nullptr)
    {
        xref_ptr<IObject> spContent(pContent);
        DCompTransformsMap::iterator itFind = m_DCompTransformsMap.find(spContent);
        if (itFind != m_DCompTransformsMap.end())
        {
            bool isSharedTransformForOverpanReflexes = false;
            switch (contentType)
            {
            case XcpDMContentTypePrimary:
                isSharedTransformForOverpanReflexes = m_isPrimarySharedTransformForOverpanReflexes;
                break;
            case XcpDMContentTypeTopHeader:
                isSharedTransformForOverpanReflexes = m_isTopHeaderSharedTransformForOverpanReflexes;
                break;
            case XcpDMContentTypeLeftHeader:
                isSharedTransformForOverpanReflexes = m_isLeftHeaderSharedTransformForOverpanReflexes;
                break;
            }
            if (isSharedTransformForOverpanReflexes)
            {
                IFC(ReleaseSharedContentTransformForOverpanReflex(contentType));
            }
            else
            {
                IFC(ReleaseSharedContentTransformForContent(pContent));
            }

            m_DCompTransformsMap.erase(itFind);
        }
    }

    TraceReleaseDManipSharedTransformInfo();

Cleanup:
    return hr;
}

// Clear out the mapping we have for this content from content -> shared parts map
// This function helps carry out synchronizing a change to a sticky header curve.
// See more details in CInputServices::PrepareSecondaryContentRelationshipForCurveUpdate().
_Check_return_ HRESULT
CDirectManipulationService::RemoveSharedContentTransformMapping(_In_ IObject* pContent)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   RemoveSharedContentTransformMapping - pViewport=0x%p, pContent=0x%p.",
            this, m_mapViewports.Count() ? m_mapViewports.begin()->first : nullptr, pContent));
    }
#endif // DM_DEBUG

    if (m_pDMCompositor != nullptr)
    {
        xref_ptr<IObject> spContent(pContent);
        DCompTransformsMap::iterator itFind = m_DCompTransformsMap.find(spContent);
        if (itFind != m_DCompTransformsMap.end())
        {
            m_DCompTransformsMap.erase(itFind);
        }
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

// Determines, for the given content type, if we should create a shared DManip transform that
// takes overpan reflexes into account
bool CDirectManipulationService::ShouldUseOverpanReflexesForContentType(_In_ XDMContentType contentType)
{
    bool result = false;

    // Notes:  At one point CDirectManipulationService was designed to target multiple manipulatable ScrollViewers,
    // that is why the m_mapViewportOverpanReflexes exists.  Today there is a 1:1 relationship between a
    // DirectManipulationService and its corresponding ScrollViewer.  Thus we can assume there will only be
    // one entry in this map when performing this lookup.
    if (m_mapViewportOverpanReflexes.Count() > 0)
    {
        switch (contentType)
        {
            // These are the only types of content that we create overpan reflexes for
            case XcpDMContentTypePrimary:
            case XcpDMContentTypeTopHeader:
            case XcpDMContentTypeLeftHeader:
                result = true;
                break;
        }
    }

    return result;
}

// Helper function to create DManip shared transform for the given DManip content
_Check_return_ HRESULT
CDirectManipulationService::CreateSharedContentTransformForContent(
    _In_ ixp::ICompositor* compositor,
    _In_ IObject* pContent,
    _Outptr_result_nullonfailure_ IUnknown** ppSharedTransform)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   CreateSharedContentTransformForContent - pViewport=0x%p, pContent=0x%p.",
            this, m_mapViewports.Count() ? m_mapViewports.begin()->first : nullptr, pContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMCompositor != nullptr);
    *ppSharedTransform = nullptr;

    xref_ptr<IDirectManipulationContent> spDMContent;
    xref_ptr<IUnknown> spSharedTransform;
    IFC(static_cast<CCOMObjectWrapper*>(pContent)->GetCOMObjectNoRef()->QueryInterface(IID_PPV_ARGS(spDMContent.ReleaseAndGetAddressOf())));
    
    GetDirectManipulationHelper()->CreateSharedContentTransformForContent(compositor, spDMContent, spSharedTransform.ReleaseAndGetAddressOf());

    *ppSharedTransform = spSharedTransform.detach();

Cleanup:
    return hr;
}

// Release the DManip shared transform for the given DManip content
_Check_return_ HRESULT
CDirectManipulationService::ReleaseSharedContentTransformForContent(_In_ IObject* pContent)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ReleaseSharedContentTransformForContent - pViewport=0x%p, pContent=0x%p.",
            this, m_mapViewports.Count() ? m_mapViewports.begin()->first : nullptr, pContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMCompositor != nullptr);

    xref_ptr<IDirectManipulationContent> spDMContent;
    IFC(static_cast<CCOMObjectWrapper*>(pContent)->GetCOMObjectNoRef()->QueryInterface(IID_PPV_ARGS(spDMContent.ReleaseAndGetAddressOf())));

    // Managed tests set up some DM content before they can inject mockdcomp. Injecting MockDComp will then reset
    // the DM compositor. The new DM compositor won't be able to remove the old content from before MockDComp
    // was injected. It doesn't need to, so ignore those failures.
    IGNOREHR(m_pDMCompositor->RemoveContent(spDMContent));

Cleanup:
    return hr;
}

// Helper function to create DManip shared transform containing overpan reflexes
_Check_return_ HRESULT
CDirectManipulationService::CreateSharedContentTransformsForOverpanReflex(
    _In_ ixp::ICompositor* compositor,
    _In_ XDMContentType contentType,
    _Outptr_result_nullonfailure_ IUnknown** ppPrimarySharedTransform,
    _Outptr_result_nullonfailure_ IUnknown** ppSecondarySharedTransform)
{
    HRESULT hr = S_OK;
    xref_ptr<IDirectManipulationContent> spPrimaryContent;
    xref_ptr<IDirectManipulationContent> spSecondaryContent;
    xref_ptr<IUnknown> spPrimarySharedTransform;
    xref_ptr<IUnknown> spSecondarySharedTransform;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   CreateSharedContentTransformsForOverpanReflex - contentType=%d.", this, contentType));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMCompositor != nullptr);
    ASSERT(ShouldUseOverpanReflexesForContentType(contentType));

    *ppPrimarySharedTransform = nullptr;
    *ppSecondarySharedTransform = nullptr;

    // Notes:
    // The DManip shared transform when using overpan reflexes is actually multiple
    // transforms that we will combine together into a transform group.
    // These two transforms are simply multiplied together in the compositor, yielding
    // the desired overall transform.

    // First figure out which two pieces of DManip content we need to create transforms on
    ViewportOverpanReflexes* pReflexes = m_mapViewportOverpanReflexes.begin()->second;
    switch(contentType)
    {
        case XcpDMContentTypePrimary:
            spPrimaryContent = pReflexes->m_spContentPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spContentSecondaryReflex.Get();
            break;
        case XcpDMContentTypeTopHeader:
            spPrimaryContent = pReflexes->m_spTopHeaderPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spTopHeaderSecondaryReflex.Get();
            break;
        case XcpDMContentTypeLeftHeader:
            spPrimaryContent = pReflexes->m_spLeftHeaderPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spLeftHeaderSecondaryReflex.Get();
            break;
    }

    // Now create DManip shared transforms for the appropriate 2 pieces of content
    IFC(GetDirectManipulationHelper()->CreateSharedContentTransformForContent(
        compositor, spPrimaryContent, spPrimarySharedTransform.ReleaseAndGetAddressOf()));
    IFC(GetDirectManipulationHelper()->CreateSharedContentTransformForContent(
        compositor, spSecondaryContent, spSecondarySharedTransform.ReleaseAndGetAddressOf()));

    *ppPrimarySharedTransform = spPrimarySharedTransform.detach();
    *ppSecondarySharedTransform = spSecondarySharedTransform.detach();

Cleanup:
    return hr;
}

// Release the DManip shared transform for overpan reflexes
_Check_return_ HRESULT
CDirectManipulationService::ReleaseSharedContentTransformForOverpanReflex(_In_ XDMContentType contentType)
{
    HRESULT hr = S_OK;
    xref_ptr<IDirectManipulationContent> spPrimaryContent;
    xref_ptr<IDirectManipulationContent> spSecondaryContent;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ReleaseSharedContentTransformForOverpanReflex - contentType=%d.", this, contentType));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMCompositor != nullptr);

    // Again we can assume that there's only one entry in the m_mapViewportOverpanReflexes map
    // (see notes above in ShouldUseOverpanReflexesForContentType()).

    // First figure out which pieces of content we're going to remove the transforms for.
    ViewportOverpanReflexes* pReflexes = m_mapViewportOverpanReflexes.begin()->second;
    switch(contentType)
    {
        case XcpDMContentTypePrimary:
            ASSERT(m_isPrimarySharedTransformForOverpanReflexes);
            spPrimaryContent = pReflexes->m_spContentPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spContentSecondaryReflex.Get();
            break;
        case XcpDMContentTypeTopHeader:
            ASSERT(m_isTopHeaderSharedTransformForOverpanReflexes);
            spPrimaryContent = pReflexes->m_spTopHeaderPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spTopHeaderSecondaryReflex.Get();
            break;
        case XcpDMContentTypeLeftHeader:
            ASSERT(m_isLeftHeaderSharedTransformForOverpanReflexes);
            spPrimaryContent = pReflexes->m_spLeftHeaderPrimaryReflex.Get();
            spSecondaryContent = pReflexes->m_spLeftHeaderSecondaryReflex.Get();
            break;
    }

    // Now remove the DManip transforms.
    IFC(m_pDMCompositor->RemoveContent(spPrimaryContent));
    IFC(m_pDMCompositor->RemoveContent(spSecondaryContent));

Cleanup:
    return hr;
}

// Start an auto-scroll operation
_Check_return_ HRESULT
CDirectManipulationService::ActivateAutoScroll(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType,
    _In_ bool autoScrollForward)
{
    HRESULT hr = S_OK;

    ASSERT(m_pDMManager != nullptr);

    IDirectManipulationViewport* pDMViewport = nullptr;
    DIRECTMANIPULATION_MOTION_TYPES dmMotionType = GetDMMotionTypes(motionType);

    ASSERT(dmMotionType == DIRECTMANIPULATION_MOTION_TRANSLATEX || dmMotionType == DIRECTMANIPULATION_MOTION_TRANSLATEY);

    IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
    ASSERT(pDMViewport != nullptr);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        DIRECTMANIPULATION_STATUS dmStatusDbg;
        IGNOREHR(pDMViewport->GetStatus(&dmStatusDbg));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ActivateAutoScroll - entry. pViewport=0x%p, motionType=%d, autoScrollForward=%d, autoScrollStatus=%d, autoScrollActivations=%d, status=%d.",
            this, pViewport, motionType, autoScrollForward, m_autoScrollStatus, m_cAutoScrollActivations, dmStatusDbg));
    }
#endif // DM_DEBUG

    if (m_autoScrollBehaviorCookie == 0)
    {
        // First auto-scroll request for this DManip Service. Create an IDirectManipulationAutoScrollBehavior
        // for handling this operation and any future auto-scroll.
        ASSERT(m_spAutoScrollBehavior == nullptr);

        Microsoft::WRL::ComPtr<IDirectManipulationManager2> spManager2;
        Microsoft::WRL::ComPtr<IDirectManipulationViewport2> spDMViewport2;
        Microsoft::WRL::ComPtr<IDirectManipulationAutoScrollBehavior> spAutoScrollBehavior;

        IFC(m_pDMManager->QueryInterface(IID_PPV_ARGS(&spManager2)));
        IFC(pDMViewport->QueryInterface(IID_PPV_ARGS(&spDMViewport2)));

        IFC(spManager2->CreateBehavior(CLSID_Microsoft_AutoScrollBehavior, IID_PPV_ARGS(&spAutoScrollBehavior)));
        IFC(spDMViewport2->AddBehavior(spAutoScrollBehavior.Get(), &m_autoScrollBehaviorCookie));

        m_spAutoScrollBehavior = spAutoScrollBehavior;
    }

    ASSERT(m_spAutoScrollBehavior != nullptr);
    IFC(pDMViewport->Stop()); // Stop call is required when the operation is touch-driven and a SetContact was called.
    IFC(m_spAutoScrollBehavior->SetConfiguration(dmMotionType, autoScrollForward ? DIRECTMANIPULATION_AUTOSCROLL_CONFIGURATION_FORWARD : DIRECTMANIPULATION_AUTOSCROLL_CONFIGURATION_REVERSE));
    m_autoScrollStatus = AutoScrollActive;
    m_cAutoScrollActivations++;

Cleanup:
    return hr;
}

// Stops an ongoing auto-scroll operation
_Check_return_ HRESULT
CDirectManipulationService::StopAutoScroll(
    _In_ IObject* pViewport,
    _In_ XDMMotionTypes motionType)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IDirectManipulationViewport* pDMViewportDbg = nullptr;
        DIRECTMANIPULATION_STATUS dmStatusDbg;
        IGNOREHR(GetDMViewportFromHandle(pViewport, &pDMViewportDbg));
        ASSERT(pDMViewportDbg);
        IGNOREHR(pDMViewportDbg->GetStatus(&dmStatusDbg));
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   StopAutoScroll - entry. pViewport=0x%p, motionType=%d, autoScrollStatus=%d, autoScrollActivations=%d, status=%d.",
            this, pViewport, motionType, m_autoScrollStatus, m_cAutoScrollActivations, dmStatusDbg));
    }
#else
    UNREFERENCED_PARAMETER(pViewport);
#endif // DM_DEBUG

    if (m_autoScrollStatus == AutoScrollActive)
    {
        ASSERT(m_spAutoScrollBehavior != nullptr);
        hr = m_spAutoScrollBehavior->SetConfiguration(GetDMMotionTypes(motionType), DIRECTMANIPULATION_AUTOSCROLL_CONFIGURATION_STOP);
        switch (hr)
        {
        case S_OK:
            // Viewport was still in inertia phase and is about to reach the Ready status
            m_autoScrollStatus = AutoScrollStopping;
            break;
        case S_FALSE:
            // Viewport was already in the Ready status
            m_autoScrollStatus = AutoScrollStopped;
            break;
        default:
            IFC(hr);
        }
    }

Cleanup:
    return hr;
}

// Attach Drag Drop Behavior to the viewport. This should only be called once per manipulation.
_Check_return_ HRESULT
CDirectManipulationService::AttachDragDropBehavior(
    _In_ IDirectManipulationViewport* pDMViewport)
{
    Microsoft::WRL::ComPtr<IDirectManipulationViewport2> spDMViewport2;

    ASSERT(m_pDMManager && pDMViewport && !m_spDragDropViewport && (m_dragDropBehaviorCookie == 0));

    if (m_spDragDropBehavior == nullptr)
    {
        Microsoft::WRL::ComPtr<IDirectManipulationManager2> spManager2;
        Microsoft::WRL::ComPtr<IDirectManipulationDragDropBehavior> spDragDropBehavior;

        IFC_RETURN(m_pDMManager->QueryInterface(IID_PPV_ARGS(&spManager2)));
        IFC_RETURN(spManager2->CreateBehavior(CLSID_Microsoft_DragDropConfigurationBehavior, IID_PPV_ARGS(&spDragDropBehavior)));
        IFC_RETURN(spDragDropBehavior->SetConfiguration(DIRECTMANIPULATION_DRAG_DROP_CONFIGURATION_VERTICAL | DIRECTMANIPULATION_DRAG_DROP_CONFIGURATION_HORIZONTAL | DIRECTMANIPULATION_DRAG_DROP_CONFIGURATION_HOLD_DRAG));

        m_spDragDropBehavior = spDragDropBehavior;
    }

    // Add DragDrop behavior to the viewport.
    IFC_RETURN(pDMViewport->QueryInterface(IID_PPV_ARGS(&spDMViewport2)));
    IFC_RETURN(spDMViewport2->AddBehavior(m_spDragDropBehavior.Get(), &m_dragDropBehaviorCookie));

    m_spDragDropViewport = pDMViewport;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorContentTransform
//
//  Synopsis:
//    Returns the content's transform info given its PAL-friendly handle.
//    Used by the compositor.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorContentTransform(
    _In_ IObject* pCompositorContent,
    _In_ XDMContentType contentType,
    _Out_ bool& fIsInertial,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    HRESULT hr = S_OK;
    XDMViewportStatus status = XcpDMViewportBuilding;
    CCOMObjectWrapper* pCompositorContentWrapper = NULL;
    IDirectManipulationContent* pDMContent = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;
    IObject* pViewport = nullptr;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetCompositorContentTransform - entry. pCompositorContent=0x%p.", this, pCompositorContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pCompositorContent);

    //TODO: Remove the fIsInertial out parameter once DManip-on-DComp is always turned on. It won't be consumed anymore.
    fIsInertial = FALSE;
    translationX = 0.0f;
    translationY = 0.0f;
    uncompressedZoomFactor = 1.0f;
    zoomFactorX = 1.0f;
    zoomFactorY = 1.0f;

    pCompositorContentWrapper = static_cast<CCOMObjectWrapper*>(pCompositorContent);
    pDMContent = static_cast<IDirectManipulationContent*>(pCompositorContentWrapper->GetCOMObjectNoRef());

    // GetViewport returns hr == HRESULT_FROM_WIN32(ERROR_INVALID_STATE) when the viewport was unregistered by the
    // InputManager and the compositor is about to execute a COldCompositorDirectManipulationViewportCommand command.
    hr = pDMContent->GetViewport(IID_PPV_ARGS(&pDMViewport));
    if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
    {
        IFC_NOTRACE(hr);
    }
    else
    {
        IFC(hr);
    }
    ASSERT(pDMViewport);

    IFC(GetHandleFromDMViewport(pDMViewport, &pViewport));

    IFC(GetViewportStatus(pDMViewport, status));

    IFC(GetDMTransform(
        pDMContent,
        pViewport,
        contentType,
        translationX,
        translationY,
        uncompressedZoomFactor,
        zoomFactorX,
        zoomFactorY));

    fIsInertial = (status == XcpDMViewportInertia);

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  GetCompositorContentTransform - translationX=%4.6lf, translationY=%4.6lf, uncompressedZoomFactor=%4.8lf, zoomFactorX=%4.8lf, zoomFactorY=%4.8lf.", this, translationX, translationY, uncompressedZoomFactor, zoomFactorX, zoomFactorY));
    }
#endif // DM_DEBUG

Cleanup:
    ReleaseInterface(pDMViewport);
    RRETURN(hr);
}

_Check_return_ HRESULT
CDirectManipulationService::GetDMTransform(
    _In_ IDirectManipulationContent* pDMContent,
    _In_ IObject* pViewport,
    _In_ XDMContentType contentType,
    _Out_ XFLOAT& translationX,
    _Out_ XFLOAT& translationY,
    _Out_ XFLOAT& uncompressedZoomFactor,
    _Out_ XFLOAT& zoomFactorX,
    _Out_ XFLOAT& zoomFactorY)
{
    XFLOAT matrix[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

    translationX = 0.0f;
    translationY = 0.0f;
    uncompressedZoomFactor = 1.0f;
    zoomFactorX = 1.0f;
    zoomFactorY = 1.0f;

    IFC_RETURN(pDMContent->GetContentTransform(reinterpret_cast<XFLOAT*>(&matrix), 6));
    ASSERT(matrix[0] == matrix[3]);
    uncompressedZoomFactor = matrix[0];
    bool fUsingCustomOverpanReflexes = false;

    // Apply reflexes based on content type for viewports that override the default DM overpan behavior.
    // For TopLeftHeader content type, ignore overpan reflexes since it should never move.
    if (m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport)) &&
        XcpDMContentTypeTopLeftHeader != contentType &&
        XcpDMContentTypeDescendant != contentType)
    {
        XcpAutoLock lock(m_overpanReflexesLock);

        // After acquiring the lock, check a second time to make sure the overpan reflex data is still available.
        if (m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport)))
        {
            fUsingCustomOverpanReflexes = TRUE;

            ViewportOverpanReflexes* pReflexes = nullptr;
            IFC_RETURN(m_mapViewportOverpanReflexes.Get(static_cast<XHANDLE>(pViewport), pReflexes));
            IFC_RETURN(GetDMTransformFromOverpanReflexes(pDMContent, pReflexes, contentType, matrix));
        }
    }

    if (!fUsingCustomOverpanReflexes)
    {
        IFC_RETURN(pDMContent->GetOutputTransform(reinterpret_cast<XFLOAT*>(&matrix), 6));
    }

    zoomFactorX = matrix[0];
    zoomFactorY = matrix[3];
    translationX = matrix[4];
    translationY = matrix[5];

    return S_OK;
}

_Check_return_ HRESULT
CDirectManipulationService::GetDMTransformFromOverpanReflexes(
    _In_ IDirectManipulationContent* pDMContent,
    _In_ ViewportOverpanReflexes* pReflexes,
    _In_ XDMContentType contentType,
    _Out_writes_(6) XFLOAT* pMatrix)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IDirectManipulationContent> spPrimaryReflex;
    Microsoft::WRL::ComPtr<IDirectManipulationContent> spSecondaryReflex;
    XFLOAT primaryReflexTransform[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    XFLOAT secondaryReflexTransform[6] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    D2D1::Matrix3x2F primaryReflexD2DMatrix = D2D1::Matrix3x2F::Identity();
    D2D1::Matrix3x2F secondaryReflexD2DMatrix = D2D1::Matrix3x2F::Identity();
    D2D1::Matrix3x2F combinedD2DMatrix = D2D1::Matrix3x2F::Identity();

    switch (contentType)
    {
    case XcpDMContentTypePrimary:
        IFC(pReflexes->m_spContentPrimaryReflex.CopyTo(&spPrimaryReflex));
        IFC(pReflexes->m_spContentSecondaryReflex.CopyTo(&spSecondaryReflex));
        break;
    case XcpDMContentTypeTopHeader:
        IFC(pReflexes->m_spTopHeaderPrimaryReflex.CopyTo(&spPrimaryReflex));
        IFC(pReflexes->m_spTopHeaderSecondaryReflex.CopyTo(&spSecondaryReflex));
        break;
    case XcpDMContentTypeLeftHeader:
        IFC(pReflexes->m_spLeftHeaderPrimaryReflex.CopyTo(&spPrimaryReflex));
        IFC(pReflexes->m_spLeftHeaderSecondaryReflex.CopyTo(&spSecondaryReflex));
        break;
    case XcpDMContentTypeCustom:
        IFC(pReflexes->m_spContentPrimaryReflex.CopyTo(&spPrimaryReflex));
        spSecondaryReflex = pDMContent;
        break;
    default:
        ASSERT(FALSE);
    }

    IFC(spPrimaryReflex->GetOutputTransform(reinterpret_cast<XFLOAT*>(&primaryReflexTransform), 6));
    primaryReflexD2DMatrix._11 = primaryReflexTransform[0];
    primaryReflexD2DMatrix._12 = primaryReflexTransform[1];
    primaryReflexD2DMatrix._21 = primaryReflexTransform[2];
    primaryReflexD2DMatrix._22 = primaryReflexTransform[3];
    primaryReflexD2DMatrix._31 = primaryReflexTransform[4];
    primaryReflexD2DMatrix._32 = primaryReflexTransform[5];

    IFC(spSecondaryReflex->GetOutputTransform(reinterpret_cast<XFLOAT*>(&secondaryReflexTransform), 6));
    secondaryReflexD2DMatrix._11 = secondaryReflexTransform[0];
    secondaryReflexD2DMatrix._12 = secondaryReflexTransform[1];
    secondaryReflexD2DMatrix._21 = secondaryReflexTransform[2];
    secondaryReflexD2DMatrix._22 = secondaryReflexTransform[3];
    secondaryReflexD2DMatrix._31 = secondaryReflexTransform[4];
    secondaryReflexD2DMatrix._32 = secondaryReflexTransform[5];

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG),
            L"DMSv[0x%p]:  contentType=%d, primaryReflexD2DMatrix - _11=%4.6lf, _12=%4.6lf, _21=%4.8lf, _22=%4.8lf, _31=%4.8lf, _32=%4.8lf.",
            this, contentType, primaryReflexD2DMatrix._11, primaryReflexD2DMatrix._12, primaryReflexD2DMatrix._21, primaryReflexD2DMatrix._22, primaryReflexD2DMatrix._31, primaryReflexD2DMatrix._32));
    }

    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG),
            L"DMSv[0x%p]:  contentType=%d, secondaryReflexD2DMatrix - _11=%4.6lf, _12=%4.6lf, _21=%4.8lf, _22=%4.8lf, _31=%4.8lf, _32=%4.8lf.",
            this, contentType, secondaryReflexD2DMatrix._11, secondaryReflexD2DMatrix._12, secondaryReflexD2DMatrix._21, secondaryReflexD2DMatrix._22, secondaryReflexD2DMatrix._31, secondaryReflexD2DMatrix._32));
    }

    if (DMSv_TraceDbg())
    {
        IFC(pDMContent->GetOutputTransform(reinterpret_cast<XFLOAT*>(pMatrix), 6));
        D2D1::Matrix3x2F contentOutputTransformD2DMatrix = D2D1::Matrix3x2F::Identity();

        contentOutputTransformD2DMatrix._11 = pMatrix[0];
        contentOutputTransformD2DMatrix._12 = pMatrix[1];
        contentOutputTransformD2DMatrix._21 = pMatrix[2];
        contentOutputTransformD2DMatrix._22 = pMatrix[3];
        contentOutputTransformD2DMatrix._31 = pMatrix[4];
        contentOutputTransformD2DMatrix._32 = pMatrix[5];

        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG),
            L"DMSv[0x%p]:  contentType=%d, contentOutputTransformD2DMatrix - _11=%4.6lf, _12=%4.6lf, _21=%4.8lf, _22=%4.8lf, _31=%4.8lf, _32=%4.8lf.",
            this, contentType, contentOutputTransformD2DMatrix._11, contentOutputTransformD2DMatrix._12, contentOutputTransformD2DMatrix._21, contentOutputTransformD2DMatrix._22, contentOutputTransformD2DMatrix._31, contentOutputTransformD2DMatrix._32));
    }

    if (DMSv_TraceDbg())
    {
        D2D1::Matrix3x2F contentTransformD2DMatrix = D2D1::Matrix3x2F::Identity();
        IFC(pDMContent->GetContentTransform(reinterpret_cast<XFLOAT*>(pMatrix), 6));

        contentTransformD2DMatrix._11 = pMatrix[0];
        contentTransformD2DMatrix._12 = pMatrix[1];
        contentTransformD2DMatrix._21 = pMatrix[2];
        contentTransformD2DMatrix._22 = pMatrix[3];
        contentTransformD2DMatrix._31 = pMatrix[4];
        contentTransformD2DMatrix._32 = pMatrix[5];

        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG),
            L"DMSv[0x%p]:  contentType=%d, contentTransformD2DMatrix - _11=%4.6lf, _12=%4.6lf, _21=%4.8lf, _22=%4.8lf, _31=%4.8lf, _32=%4.8lf.",
            this, contentType, contentTransformD2DMatrix._11, contentTransformD2DMatrix._12, contentTransformD2DMatrix._21, contentTransformD2DMatrix._22, contentTransformD2DMatrix._31, contentTransformD2DMatrix._32));
    }
#endif // DM_DEBUG

    // Combine the reflex matrices to get the final transform.
    // The primary reflex implements the scaling effect and centerpoint correction effect.
    // The secondary reflex implements the overpan suppression effect.
    combinedD2DMatrix = secondaryReflexD2DMatrix * primaryReflexD2DMatrix;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_VERBOSE | DMSv_DBG),
            L"DMSv[0x%p]:  contentType=%d, combinedD2DMatrix - _11=%4.6lf, _12=%4.6lf, _21=%4.8lf, _22=%4.8lf, _31=%4.8lf, _32=%4.8lf.",
            this, contentType, combinedD2DMatrix._11, combinedD2DMatrix._12, combinedD2DMatrix._21, combinedD2DMatrix._22, combinedD2DMatrix._31, combinedD2DMatrix._32));
    }
#endif // DM_DEBUG

    pMatrix[0] = combinedD2DMatrix._11;
    pMatrix[1] = combinedD2DMatrix._12;
    pMatrix[2] = combinedD2DMatrix._21;
    pMatrix[3] = combinedD2DMatrix._22;
    pMatrix[4] = combinedD2DMatrix._31;
    pMatrix[5] = combinedD2DMatrix._32;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::UpdateCompositorContentTransform
//
//  Synopsis:
//    Used by the compositor. Even if no compositor node exists for a DM
//    content, DM needs to be ticked in inertia mode. This method is called
//    for that purpose.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::UpdateCompositorContentTransform(
    _In_ IObject* pCompositorContent,
    _In_ XUINT32 deltaCompositionTime)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorContentWrapper = NULL;
    IDirectManipulationContent* pDMContent = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMSv_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | XCP_TRACE_VERBOSE | DMSv_DBG) /*traceType*/,
            L"DMSv[0x%p]:  UpdateCompositorContentTransform - entry. pCompositorContent=0x%d.", this, pCompositorContent));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(pCompositorContent);
    ASSERT(m_pDMUpdateManager);

    pCompositorContentWrapper = static_cast<CCOMObjectWrapper*>(pCompositorContent);
    pDMContent = static_cast<IDirectManipulationContent*>(pCompositorContentWrapper->GetCOMObjectNoRef());

    // GetViewport returns hr == HRESULT_FROM_WIN32(ERROR_INVALID_STATE) when the viewport was unregistered by the
    // InputManager and the compositor is about to execute a COldCompositorDirectManipulationViewportCommand command.
    hr = pDMContent->GetViewport(IID_PPV_ARGS(&pDMViewport));
    if (hr == HRESULT_FROM_WIN32(ERROR_INVALID_STATE))
    {
        IFC_NOTRACE(hr);
    }
    else
    {
        IFC(hr);
    }
    ASSERT(pDMViewport);

    // This time lapse will be used in the CDirectManipulationCompositor::GetNextFrameInfo call caused by this Update call.
    m_deltaCompositionTime = deltaCompositionTime;
    IFC(m_pDMUpdateManager->Update(m_pDMFrameInfoProvider));

Cleanup:
    ReleaseInterface(pDMViewport);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorViewportKey
//
//  Synopsis:
//    Return a unique key value associated with the underlying DM viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorViewportKey(
    _In_ IObject* pCompositorViewport,
    _Out_ XHANDLE* pKey)
{
    CCOMObjectWrapper* pCompositorViewportWrapper = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;

    ASSERT(pKey);

    pCompositorViewportWrapper = static_cast<CCOMObjectWrapper*>(pCompositorViewport);
    pDMViewport = static_cast<IDirectManipulationViewport*>(pCompositorViewportWrapper->GetCOMObjectNoRef());

    // Use the actual DM viewport pointer as the key.
    *pKey = static_cast<XHANDLE>(pDMViewport);

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetCompositorViewportStatus
//
//  Synopsis:
//    Return the status of the underlying DM viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetCompositorViewportStatus(
    _In_ IObject* pCompositorViewport,
    _Out_ XDMViewportStatus* pStatus)
{
    HRESULT hr = S_OK;
    CCOMObjectWrapper* pCompositorViewportWrapper = NULL;
    IDirectManipulationViewport* pDMViewport = NULL;

    ASSERT(pStatus);

    pCompositorViewportWrapper = static_cast<CCOMObjectWrapper*>(pCompositorViewport);
    pDMViewport = static_cast<IDirectManipulationViewport*>(pCompositorViewportWrapper->GetCOMObjectNoRef());

    XDMViewportStatus status;

    // This call is designed to return an error code sometimes, as such as don't trace it in
    // the typical error handling tools to avoid allocating and capturing a stack.
    IFC_NOTRACE(GetViewportStatus(pDMViewport, status));

    *pStatus = status;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::NotifyViewportStatusUpdate
//
//  Synopsis:
//    Called by CDirectManipulationViewportEventHandler when a viewport's status changed.
//    Forwards the viewport status update to the
//    IXcpDirectManipulationViewportEventHandler implementation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::NotifyViewportStatusUpdate(
    _In_ IDirectManipulationViewport* pDMViewport,
    _In_ DIRECTMANIPULATION_STATUS oldStatus,
    _In_ DIRECTMANIPULATION_STATUS currentStatus)
{
    HRESULT hr = S_OK;
    IObject* pViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   NotifyViewportStatusUpdate - entry. oldStatus=%d, currentStatus=%d, autoScrollStatus=%d, autoScrollActivations=%d, pDMViewport=0x%p.",
            this, oldStatus, currentStatus, m_autoScrollStatus, m_cAutoScrollActivations, pDMViewport));
    }
#endif // DM_DEBUG

    ASSERT(pDMViewport);

    m_currentManipulationStatus = currentStatus;

    IFC(GetHandleFromDMViewport(pDMViewport, &pViewport));
    if (pViewport)
    {
        XDMViewportStatus oldXcpDMStatus = GetViewportStatus(oldStatus);
        XDMViewportStatus currentXcpDMStatus = GetViewportStatus(currentStatus);

        // m_cAutoScrollActivations can be 0, while m_autoScrollStatus==AutoScrollActive and currentXcpDMStatus==XcpDMViewportInertia
        // when the user initiates a mouse-wheel scroll during an auto-scroll.
        if (m_autoScrollStatus != AutoScrollStopped && currentXcpDMStatus == XcpDMViewportInertia && m_cAutoScrollActivations > 0)
        {
            m_cAutoScrollActivations--;
            currentXcpDMStatus = XcpDMViewportAutoRunning;
        }
        else if (m_autoScrollStatus != AutoScrollStopped && oldXcpDMStatus == XcpDMViewportInertia)
        {
            oldXcpDMStatus = XcpDMViewportAutoRunning;
            if (m_autoScrollStatus == AutoScrollStopping)
            {
                m_autoScrollStatus = (m_cAutoScrollActivations == 0) ? AutoScrollStopped : AutoScrollActive;
            }
        }

#ifdef DBG
        m_wasInInertiaStatusDbg = currentXcpDMStatus == XcpDMViewportInertia;
#endif

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                L"DMS[0x%p]:   NotifyViewportStatusUpdate - exit. oldStatus=%d, currentStatus=%d, autoScrollStatus=%d, autoScrollActivations=%d.",
                this, oldXcpDMStatus, currentXcpDMStatus, m_autoScrollStatus, m_cAutoScrollActivations));
        }
#endif // DM_DEBUG

        IFC(NotifyViewportStatusUpdate(pViewport, oldXcpDMStatus, currentXcpDMStatus));
    }
#ifdef DM_DEBUG
    else if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   NotifyViewportStatusUpdate - oldStatus=%d, currentStatus=%d. Unexpected unregistered viewport for pDMViewport=0x%p.", this, oldStatus, currentStatus, pDMViewport));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::NotifyViewportStatusUpdate
//
//  Synopsis:
//    Forwards the viewport status update to the
//    IXcpDirectManipulationViewportEventHandler implementation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::NotifyViewportStatusUpdate(
    _In_ IObject* pViewport,
    _In_ XDMViewportStatus oldStatus,
    _In_ XDMViewportStatus currentStatus)
{
    HRESULT hr = S_OK;

    if (m_pViewportEventHandler)
    {
        ASSERT(pViewport);

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                L"DMS[0x%p]:   NotifyViewportStatusUpdate oldStatus=%d, currentStatus=%d, pViewport=0x%p.",
                this, oldStatus, currentStatus, pViewport));
        }
#endif // DM_DEBUG

        IFC(m_pViewportEventHandler->ProcessDirectManipulationViewportStatusUpdate(
            pViewport, oldStatus, currentStatus));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::NotifyViewportInteractionTypeUpdate
//
//  Synopsis:
//    Called by CDirectManipulationViewportEventHandler when a viewport's interaction type changed.
//    Forwards the viewport interaction type update to the
//    IXcpDirectManipulationViewportEventHandler implementation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::NotifyViewportInteractionTypeUpdate(
    _In_ IDirectManipulationViewport* pDMViewport,
    _In_ DIRECTMANIPULATION_INTERACTION_TYPE newInteractionType)
{
    HRESULT hr = S_OK;
    IObject* pViewport = NULL;

    ASSERT(pDMViewport);

    IFC(GetHandleFromDMViewport(pDMViewport, &pViewport));
    if (pViewport)
    {
        IFC(NotifyViewportInteractionTypeUpdate(pViewport, GetViewportInteractionType(newInteractionType)));
    }
#ifdef DM_DEBUG
    else if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   NotifyViewportInteractionTypeUpdate - newInteractionType=%d. Unexpected unregistered viewport for pDMViewport=0x%p.", this, newInteractionType, pDMViewport));
    }
#endif // DM_DEBUG

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::NotifyViewportInteractionTypeUpdate
//
//  Synopsis:
//    Forwards the viewport interaction type update to the
//    IXcpDirectManipulationViewportEventHandler implementation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::NotifyViewportInteractionTypeUpdate(
    _In_ IObject* pViewport,
    _In_ XDMViewportInteractionType newInteractionType)
{
    HRESULT hr = S_OK;

    if (m_pViewportEventHandler)
    {
        ASSERT(pViewport);

#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                L"DMS[0x%p]:   NotifyViewportInteractionTypeUpdate newInteractionType=%d, pViewport=0x%p.",
                this, newInteractionType, pViewport));
        }
#endif // DM_DEBUG

        IFC(m_pViewportEventHandler->ProcessDirectManipulationViewportInteractionTypeUpdate(
            pViewport, newInteractionType));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::NotifyViewportDraggingStarted
//
//  Synopsis:
//    Forwards the viewport drag drop status change
//    IXcpDirectManipulationViewportEventHandler implementation.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::NotifyViewportDraggingStatusChange(
    _In_ IDirectManipulationViewport* pDMViewport,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous)
{
    IObject* pViewport = NULL;
    ASSERT(pDMViewport);

    IFC_RETURN(GetHandleFromDMViewport(pDMViewport, &pViewport));
    if (m_pViewportEventHandler && pViewport)
    {
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                L"DMS[0x%p]:   NotifyViewportDraggingStatusChange current=%d, previous=%d, pViewport=0x%p.",
                this, current, previous, pViewport));
        }
#endif // DM_DEBUG

        IFC_RETURN(m_pViewportEventHandler->ProcessDirectManipulationViewportDraggingStatusChange(pViewport, current, previous));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::CreateViewportEventHandler
//
//  Synopsis:
//    Creates an IDirectManipulationViewportEventHandler event handler
//    either dedicated for the UI thread IXcpDirectManipulationViewportEventHandler
//    listener or the compositor depending on the fUIThreadFeedback flag.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::CreateViewportEventHandler()
{
    HRESULT hr = S_OK;
    CComObject<CDirectManipulationViewportEventHandler>* pDMViewportEventHandler = nullptr;

    if (!m_pUIThreadViewportEventHandler)
    {
        IFC(CComObject<CDirectManipulationViewportEventHandler>::CreateInstance(&pDMViewportEventHandler));
        IFC(pDMViewportEventHandler->SetDMService(this));
        IFC(pDMViewportEventHandler->QueryInterface(IID_PPV_ARGS(&m_pUIThreadViewportEventHandler)));
        pDMViewportEventHandler = nullptr;
    }
    else if (m_mapUIThreadViewportEventHandlerCookies.Count() == 0)
    {
        CDirectManipulationViewportEventHandler* pDMViewportEventHandlerNoRef = static_cast<CDirectManipulationViewportEventHandler*>(m_pUIThreadViewportEventHandler);
        ASSERT(pDMViewportEventHandlerNoRef);
        IFC(pDMViewportEventHandlerNoRef->SetDMService(this));
    }

Cleanup:
    ReleaseInterface(pDMViewportEventHandler);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::EnsureFrameInfoProvider
//
//  Synopsis:
//    Retrieves the IDirectManipulationCompositor (which provides the
//    GetNextFrameInfo times to DirectManipulation).
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::EnsureFrameInfoProvider()
{
    HRESULT hr = S_OK;

    CComObject<CDirectManipulationFrameInfoProvider>* pFrameInfoProvider = NULL;

    if (!m_pDMFrameInfoProvider)
    {
        ASSERT(m_pDMCompositor);
        IFC(m_pDMCompositor->QueryInterface(IID_PPV_ARGS(&m_pDMFrameInfoProvider)));
    }

Cleanup:
    ReleaseInterface(pFrameInfoProvider);
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::AddViewport
//
//  Synopsis:
//    Adds a viewport to the internal m_mapViewports storage, addrefs the
//    key and value.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::AddViewport(
    _In_ IObject* pViewport,
    _In_ IDirectManipulationViewport* pDMViewport)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   AddViewport - entry. pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(pViewport);
    ASSERT(pDMViewport);

    IFC(m_mapViewports.Add(static_cast<XHANDLE>(pViewport), pDMViewport));
    AddRefInterface(pViewport);
    AddRefInterface(pDMViewport);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::CreateViewport
//
//  Synopsis:
//    Creates a IDirectManipulationViewport implementation, the event
//    handlers if needed, and attaches them to the new viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::CreateViewport(
    _Outptr_ IDirectManipulationViewport** ppDMViewport)
{
    HRESULT hr = S_OK;
    XDWORD viewportEventHandlerCookie = 0;
    wrl::ComPtr<IDirectManipulationViewport> dmViewport;
    
    #ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   CreateViewport - entry.", this));
    }
    #endif // DM_DEBUG
    
    ASSERT(m_pDMManager);
    ASSERT(nullptr != m_islandInputSite);
    
    ASSERT(ppDMViewport);
    *ppDMViewport = nullptr;
    
    HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
    IFC(static_cast<IDirectManipulationManager*>(m_pDMManager)->CreateViewport(m_pDMFrameInfoProvider, inputHwnd, IID_PPV_ARGS(dmViewport.ReleaseAndGetAddressOf())));
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   IDMViewport=0x%p.", dmViewport.Get()));
    }
#endif // DM_DEBUG

    // Use the delegate thread mechanism
    IFC(dmViewport->SetInputMode(DIRECTMANIPULATION_INPUT_MODE_AUTOMATIC));

    // Viewports that use the compositor must be in automatic input mode.
    IFC(dmViewport->SetUpdateMode(DIRECTMANIPULATION_INPUT_MODE_AUTOMATIC));

    // Disable DManip pixel snapping as we will perform all pixel snapping ourselves.
    IFC(dmViewport->SetViewportOptions(DIRECTMANIPULATION_VIEWPORT_OPTIONS_DISABLEPIXELSNAPPING));

    // Handler added for the UI thread IXcpDirectManipulationViewportEventHandler implementation
    IFC(CreateViewportEventHandler());
    ASSERT(m_pUIThreadViewportEventHandler);

    IFC(dmViewport->AddEventHandler(inputHwnd, m_pUIThreadViewportEventHandler, &viewportEventHandlerCookie));
    IFC(m_mapUIThreadViewportEventHandlerCookies.Add(static_cast<XHANDLE>(dmViewport.Get()), viewportEventHandlerCookie));

    *ppDMViewport = dmViewport.Detach();

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::CreateCrossSlideViewport
//
//  Synopsis:
//    Creates a new cross-slide viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::CreateCrossSlideViewport(
    _In_ bool fIsDragDrop,
    _Outptr_ IDirectManipulationViewport** ppDMViewport)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   CreateCrossSlideViewport - entry.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(nullptr != m_islandInputSite);

    ASSERT(ppDMViewport);
    *ppDMViewport = NULL;

    HWND inputHwnd = CInputServices::GetUnderlyingInputHwndFromIslandInputSite(m_islandInputSite.Get());
    IFC(static_cast<IDirectManipulationManager*>(m_pDMManager)->CreateViewport(NULL /*pFrameInfo*/, inputHwnd, IID_PPV_ARGS(&pDMViewport)));
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   IDMViewport=0x%p.", pDMViewport));
    }
#endif // DM_DEBUG
    if (fIsDragDrop)
    {
        // If it's a drag and drop viewport, we need to register the event handler to
        // get drag drop status change from DManip.
        XDWORD viewportEventHandlerCookie = 0;

        // Handler added for the UI thread IXcpDirectManipulationViewportEventHandler implementation
        IFC(CreateViewportEventHandler());
        ASSERT(m_pUIThreadViewportEventHandler);

        IFC(pDMViewport->AddEventHandler(inputHwnd, m_pUIThreadViewportEventHandler, &viewportEventHandlerCookie));
        IFC(m_mapUIThreadViewportEventHandlerCookies.Add(static_cast<XHANDLE>(pDMViewport), viewportEventHandlerCookie));
    }

    *ppDMViewport = pDMViewport;
    pDMViewport = NULL;

Cleanup:
    ReleaseInterface(pDMViewport);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationService::IsViewportHandleRegistered
//
//  Synopsis:
//    Determines if a viewport handle was previously registered.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::IsViewportHandleRegistered(
    _In_ IObject* pViewport,
    _Out_ bool* pfViewportRegistered)
{
    HRESULT hr = S_OK;

    ASSERT(pViewport);
    ASSERT(pfViewportRegistered);

    *pfViewportRegistered = m_mapViewports.ContainsKey(static_cast<XHANDLE>(pViewport));

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetDMViewportFromHandle
//
//  Synopsis:
//    Retrieves the IDirectManipulationViewport instance associated to the
//    provided viewport handle.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetDMViewportFromHandle(
    _In_ IObject* pViewport,
    _Out_ IDirectManipulationViewport** ppDMViewport)
{
    HRESULT hr = S_OK;
    IDirectManipulationViewport* pDMViewport = NULL;

    ASSERT(pViewport);
    ASSERT(ppDMViewport);
    *ppDMViewport = NULL;

    IFC(m_mapViewports.Get(static_cast<XHANDLE>(pViewport), pDMViewport));
    *ppDMViewport = pDMViewport;

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetHandleFromDMViewport
//
//  Synopsis:
//    Retrieves the viewport handle associated to the provided
//    IDirectManipulationViewport implementation, if it exists.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetHandleFromDMViewport(
    _In_ IDirectManipulationViewport* pDMViewport,
    _Out_ IObject** ppViewport)
{
    HRESULT hr = S_OK;
    IObject* pViewport = NULL;
    IDirectManipulationViewport* pDMViewportTmp = NULL;

    ASSERT(pDMViewport);
    ASSERT(ppViewport);
    *ppViewport = NULL;

    for (xchainedmap<XHANDLE, IDirectManipulationViewport*>::const_iterator it = m_mapViewports.begin();
        it != m_mapViewports.end();
        ++it)
    {
        pDMViewportTmp = (*it).second;
        ASSERT(pDMViewportTmp);
        if (pDMViewportTmp == pDMViewport)
        {
            pViewport = static_cast<IObject*>((*it).first);
            *ppViewport = pViewport;
            break;
        }
    }

    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetViewportStatus
//
//  Synopsis:
//    Returns the current status of the provided viewport.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetViewportStatus(
    _In_ IDirectManipulationViewport* pDMViewport,
    _Out_ XDMViewportStatus& status)
{
    HRESULT hr = S_OK;
    DIRECTMANIPULATION_STATUS dmStatus;

    ASSERT(pDMViewport);

    status = XcpDMViewportBuilding;

    // Errors don't generate callstack captures here because they can occur when a viewport is discarded.
    IFC_NOTRACE(pDMViewport->GetStatus(&dmStatus));
    status = GetViewportStatus(dmStatus);

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetupSecondaryContent
//
//  Synopsis:
//    Sets up secondary content according to its content type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetupSecondaryContent(
    _In_ IDirectManipulationContent* pDMSecondaryContent,
    _In_ XDMContentType contentType)
{
#ifdef DM_DEBUG
            if (DMS_TraceDbg())
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetupSecondaryContent - entry. pDMSecondaryContent=0x%p, contentType=%d.",
                    this, pDMSecondaryContent, contentType));
            }
#endif // DM_DEBUG

    ASSERT(pDMSecondaryContent);

    return m_pDMHelper->SetupSecondaryContent(
        pDMSecondaryContent,
        contentType);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::SetupSecondaryContent
//
//  Synopsis:
//    Sets up secondary content according to its content type.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::SetupSecondaryContent(
    _In_ IDirectManipulationContent* pDMSecondaryContent,
    _In_ XUINT32 cDefinitions,
    _In_reads_(cDefinitions) CParametricCurveDefinition *pDefinitions,
    _In_ XFLOAT offsetX,
    _In_ XFLOAT offsetY)
{
#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"DMS[0x%p]:   SetupSecondaryContent - entry. pDMSecondaryContent=0x%p, cDefinitions=%d.",
            this, pDMSecondaryContent, cDefinitions));
    }
#endif // DM_DEBUG

    ASSERT(pDMSecondaryContent);
    return m_pDMHelper->SetupSecondaryContent(
        pDMSecondaryContent,
        cDefinitions,
        pDefinitions,
        offsetX,
        offsetY,
        [this](XDMProperty prop) { return GetMotionTypeFromDMProperty(prop); },
        [](DIRECTMANIPULATION_MOTION_TYPES primaryMotionType, XUINT32 i, XUINT32 j, XFLOAT beginOffset, XFLOAT constantCoefficient, XFLOAT linearCoefficient, XFLOAT quadraticCoefficient, XFLOAT cubicCoefficient) {
#ifdef DM_DEBUG
            if (DMS_TraceDbg())
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | DMS_DBG) /*traceType*/, L"                   SetupSecondaryContent - Adding cubic #%d, segment #%d: BeginOffset=%f, ConstantCoefficient=%f, LinearCoefficient=%f, QuadraticCoefficient=%f, CubicCoefficient=%f.",
                    i, j, beginOffset, constantCoefficient, linearCoefficient, quadraticCoefficient, cubicCoefficient));
            }
#endif // DM_DEBUG
        },
        EventEnabledCurveSegmentInfo());
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetMotionTypeFromDMProperty
//
//  Synopsis:
//    Converts from an XDMProperty enum value to a DIRECTMANIPULATION_MOTION_TYPES enum value.
//
//-------------------------------------------------------------------------
DIRECTMANIPULATION_MOTION_TYPES
CDirectManipulationService::GetMotionTypeFromDMProperty(
    _In_ XDMProperty dmProperty)
{
    switch (dmProperty)
    {
    case XcpDMPropertyTranslationX:
        return DIRECTMANIPULATION_MOTION_TRANSLATEX;

    case XcpDMPropertyTranslationY:
        return DIRECTMANIPULATION_MOTION_TRANSLATEY;

    case XcpDMPropertyZoom:
        return DIRECTMANIPULATION_MOTION_ZOOM;

    default:
        return DIRECTMANIPULATION_MOTION_NONE;
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetDMConfigurations
//
//  Synopsis:
//    Converts from a PAL configuration enum value to a Windows DM
//    configuration enum value.
//
//-------------------------------------------------------------------------
DIRECTMANIPULATION_CONFIGURATION
CDirectManipulationService::GetDMConfigurations(
    _In_ XDMConfigurations configuration)
{
    DIRECTMANIPULATION_CONFIGURATION dmConfig = DIRECTMANIPULATION_CONFIGURATION_NONE;

    if ((configuration & XcpDMConfigurationPanX) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_X;
    }
    if ((configuration & XcpDMConfigurationPanY) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_Y;
    }
    if ((configuration & XcpDMConfigurationZoom) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_SCALING;
    }
    if ((configuration & XcpDMConfigurationPanInertia) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_TRANSLATION_INERTIA;
    }
    if ((configuration & XcpDMConfigurationZoomInertia) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_SCALING_INERTIA;
    }
    if ((configuration & XcpDMConfigurationRailsX) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_RAILS_X;
    }
    if ((configuration & XcpDMConfigurationRailsY) != 0)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_RAILS_Y;
    }

    if (dmConfig != DIRECTMANIPULATION_CONFIGURATION_NONE)
    {
        dmConfig |= DIRECTMANIPULATION_CONFIGURATION_INTERACTION;
    }

    return dmConfig;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetDMSnapCoordinate
//
//  Synopsis:
//    Converts from a PAL snap coordinate enum value to a Windows DM
//    snap coordinate enum value.
//
//-------------------------------------------------------------------------
DIRECTMANIPULATION_SNAPPOINT_COORDINATE
CDirectManipulationService::GetDMSnapCoordinate(
    _In_ XDMSnapCoordinate coordinate)
{
    switch (coordinate)
    {
    case XcpDMSnapCoordinateBoundary:
        return DIRECTMANIPULATION_COORDINATE_BOUNDARY;
    case XcpDMSnapCoordinateOrigin:
        return DIRECTMANIPULATION_COORDINATE_ORIGIN;
    case XcpDMSnapCoordinateMirrored:
        return DIRECTMANIPULATION_COORDINATE_MIRRORED;
    }
    ASSERT(FALSE);
    return DIRECTMANIPULATION_COORDINATE_BOUNDARY;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetWindowsMessageFromMessageMap
//
//  Synopsis:
//    Converts a PAL message ID to a Window message ID
//
//-------------------------------------------------------------------------
UINT
CDirectManipulationService::GetWindowsMessageFromMessageMap(
    _In_ MessageMap msgID,
    _In_ bool fIsSecondaryMessage,
    _Out_ bool& fIsKeyboardInput)
{
    fIsKeyboardInput = FALSE;

    switch (msgID)
    {
    case XCP_POINTERWHEELCHANGED:
        return fIsSecondaryMessage ? WM_POINTERHWHEEL : WM_POINTERWHEEL;
    case XCP_KEYDOWN:
        fIsKeyboardInput = TRUE;
        return fIsSecondaryMessage ? WM_SYSKEYDOWN : WM_KEYDOWN;
    }
    ASSERT(FALSE);
    return WM_USER;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::IsWindowsMessageForHorizontalPan
//
//  Synopsis:
//    Returns True when the message must only result in a horizontal pan.
//
//-------------------------------------------------------------------------
bool
CDirectManipulationService::IsWindowsMessageForHorizontalPan(
_In_ MessageMap msgID,
_In_ WPARAM wParam)
{
    return msgID == XCP_KEYDOWN &&
        (wParam == VK_LEFT ||
        wParam == VK_RIGHT);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::IsWindowsMessageForVerticalPan
//
//  Synopsis:
//    Returns True when the message must only result in a vertical pan.
//
//-------------------------------------------------------------------------
bool
CDirectManipulationService::IsWindowsMessageForVerticalPan(
_In_ MessageMap msgID,
_In_ WPARAM wParam)
{
    return msgID == XCP_KEYDOWN &&
        (wParam == VK_DOWN ||
        wParam == VK_UP);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::IsWindowsMessageForPan
//
//  Synopsis:
//    Returns True when the message must only result in a horizontal or
//    vertical pan depending on the DM config and Ctrl key.
//
//-------------------------------------------------------------------------
bool
CDirectManipulationService::IsWindowsMessageForPan(
    _In_ MessageMap msgID,
    _In_ WPARAM wParam)
{
    return msgID == XCP_KEYDOWN &&
        (wParam == VK_PRIOR ||
        wParam == VK_NEXT ||
        wParam == VK_HOME ||
        wParam == VK_END);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetWindowsMessageWParam
//
//  Synopsis:
//    Inverts horizontal scrolling for Right-To-Left layouts.
//
//-------------------------------------------------------------------------
WPARAM
CDirectManipulationService::GetWindowsMessageWParam(
    _In_ MessageMap msgID,
    _In_ WPARAM wParam,
    _In_ bool fInvertForRightToLeft)
{
    if (msgID == XCP_KEYDOWN)
    {
        if (fInvertForRightToLeft)
        {
            switch (wParam)
            {
            case VK_LEFT:
                return VK_RIGHT;
            case VK_RIGHT:
                return VK_LEFT;
            case VK_PRIOR:
                return VK_NEXT;
            case VK_NEXT:
                return VK_PRIOR;
            case VK_HOME:
                return VK_END;
            case VK_END:
                return VK_HOME;
            }
        }
    }
    return wParam;
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetRoundedLong
//
//  Synopsis:
//    Returns the closest LONG to the provided XFLOAT.
//
//-------------------------------------------------------------------------
LONG
CDirectManipulationService::GetRoundedLong(
    _In_ XFLOAT value)
{
    // Round to the closest LONG values.
    // -9.73f --> -10
    // +9.73f --> +10
    // -5.31f --> -5
    // +5.31f --> +5
    if (value < 0.0f)
    {
        return static_cast<LONG>(value - 0.5f);
    }
    else
    {
        return static_cast<LONG>(value + 0.5f);
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetRoundedDownLong
//
//  Synopsis:
//    Returns the rounded down LONG for the provided XFLOAT, or an exact
//    match if no rounding is necessary.
//
//-------------------------------------------------------------------------
LONG
CDirectManipulationService::GetRoundedDownLong(
    _In_ XFLOAT value)
{
    // Round to the closest lower LONG.
    // +3.00f --> +3
    // +9.73f --> +9
    // +5.31f --> +5
    ASSERT(value >= 0.0f);

    return static_cast<LONG>(value);
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetRoundedUpLong
//
//  Synopsis:
//    Returns the rounded up LONG for the provided XFLOAT, or an exact
//    match if no rounding is necessary.
//
//-------------------------------------------------------------------------
LONG
CDirectManipulationService::GetRoundedUpLong(
    _In_ XFLOAT value)
{
    // Round to the closest upper LONG.
    // +3.00f --> +3
    // +9.73f --> +10
    // +5.31f --> +6
    ASSERT(value >= 0.0f);

    LONG lValue = static_cast<LONG>(value);

    if (value == static_cast<XFLOAT>(lValue))
    {
        return lValue;
    }
    else
    {
        return lValue + 1;
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CDirectManipulationService::GetDragDropViewport
//
//  Synopsis:
//    Returns the viewport set up for Drag and Drop. Return nullptr if hit-testing
//    hasn't encountered a UIElement which has CanDrag=True
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDirectManipulationService::GetDragDropViewport(
    _Outptr_result_maybenull_ IObject** ppViewport)
{
    *ppViewport = nullptr;
    if (m_spDragDropViewport != nullptr)
    {
        IObject* pViewport = nullptr;
        IFC_RETURN(GetHandleFromDMViewport(m_spDragDropViewport.Get(), &pViewport));
        AddRefInterface(pViewport);
        *ppViewport = pViewport;
    }

    return S_OK;
}

// Sets up DM curves as necessary to achieve overpan suppression based on the horizontal/vertical overpan modes.
// Reflex curves are used for overpan suppression (Threshold Shell lock screen scenarios).
// pfAreOverpanModesChanged returns True when an overpan mode has changed and new DManip transforms need to be created.
_Check_return_ HRESULT
CDirectManipulationService::ApplyOverpanModes(
    _In_ IObject* pViewport,
    _In_ XDMOverpanMode horizontalOverpanMode,
    _In_ XDMOverpanMode verticalOverpanMode,
    _In_ XFLOAT zoomScale,
    _In_ bool fIsStartingNewManipulation,
    _Out_ bool* pfAreOverpanModesChanged)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IDirectManipulationViewport2> spDMViewport2;
    Microsoft::WRL::ComPtr<IDirectManipulationManager2> spManager2;
    IDirectManipulationViewport* pDMViewport = nullptr;
    ViewportOverpanReflexes* pReflexes = nullptr;
    bool fCleanupReflexDataForFailedHR = false;
    bool fReflexesExist = m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport));

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   ApplyOverpanModes - pViewport=0x%p, horizontalOverpanMode=%d, verticalOverpanMode=%d, zoomScale=%f, fIsStartingNewManipulation=%d.",
            this, pViewport, horizontalOverpanMode, verticalOverpanMode, zoomScale, fIsStartingNewManipulation));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMManager);
    ASSERT(zoomScale > 0);

    *pfAreOverpanModesChanged = FALSE;

    if (fReflexesExist || horizontalOverpanMode != XcpDMOverpanModeDefault || verticalOverpanMode != XcpDMOverpanModeDefault)
    {
        IFC(m_pDMManager->QueryInterface(IID_PPV_ARGS(&spManager2)));
        IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
        ASSERT(pDMViewport);
        IFC(pDMViewport->QueryInterface(IID_PPV_ARGS(&spDMViewport2)));

        // If horizontal overpan mode has changed, apply a viewport behaviors to turn on/off default DM overpan as necessary.
        if (fReflexesExist)
        {
            IFC(m_mapViewportOverpanReflexes.Get(static_cast<XHANDLE>(pViewport), pReflexes));

            if (pReflexes->m_horizontalOverpanMode != horizontalOverpanMode || pReflexes->m_verticalOverpanMode != verticalOverpanMode)
            {
                pReflexes->m_horizontalOverpanMode = horizontalOverpanMode;
                pReflexes->m_verticalOverpanMode = verticalOverpanMode;
                pReflexes->m_fIsBehaviorRefreshNeeded = TRUE;
            }

            // When beginning a new manipulation, update the viewport behavior so that we use built-in DM overpan if content
            // is smaller than viewport in one dimension.
            // We can't update the viewport behavior while in manipulation since this may hit a DM deadlock (WPB 275883).
            // The DM deadlock won't be fixed for WPB, but if fixed in vNext of Windows we may consider removing the
            // fIsStartingNewManipulation check here.  However, even if overpan mode changes while in manipulation because the user
            // zoomed out enough that the content became smaller than the viewport in one direction, this doesn't cause any
            // functional issues and it seems okay to wait until the zoom manipulation completes and a new manip is started to
            // fall back to default (i.e. built-in DM) overpan mode for the dimension where content is smaller than the viewport.
            if (fIsStartingNewManipulation && pReflexes->m_fIsBehaviorRefreshNeeded)
            {
                // Update to the new overpan modes and refresh the curves under lock so that
                // the compositor doesn't read the new curves until we're finished updating them.
                XcpAutoLock lock(m_overpanReflexesLock);

                if (pReflexes->m_viewportBehaviorCookie != 0)
                {
                    IFC(RemoveOverpanBehavior(spDMViewport2.Get(), pReflexes));
                }

                if (pReflexes->m_horizontalOverpanMode != XcpDMOverpanModeDefault || pReflexes->m_verticalOverpanMode != XcpDMOverpanModeDefault)
                {
                    IFC(CreateParametricBehavior(spManager2.Get(), spDMViewport2.Get(), pReflexes));
                }

                pReflexes->m_fIsBehaviorRefreshNeeded = FALSE;

                IFC(RefreshOverpanCurves(pViewport));
            }
        }
        else if (horizontalOverpanMode != XcpDMOverpanModeDefault || verticalOverpanMode != XcpDMOverpanModeDefault)
        {
            XcpAutoLock lock(m_overpanReflexesLock);

            // Going from default DM overpan to custom.  Add a behavior to turn off default DM overpan.
            // NOTE: Only the UI thread can add/remove entries to m_mapViewportOverpanReflexes, and the compositor
            // thread only reads from there.  So, we can safely acquire a lock here and we don't have to recheck
            // whether m_mapViewportOverpanReflexes still contains an entry for this viewport while locked.

            pReflexes = new ViewportOverpanReflexes();
            fCleanupReflexDataForFailedHR = TRUE;

            pReflexes->m_zoomScale = zoomScale;
            pReflexes->m_horizontalOverpanMode = horizontalOverpanMode;
            pReflexes->m_verticalOverpanMode = verticalOverpanMode;

            IFC(CreateParametricBehavior(spManager2.Get(), spDMViewport2.Get(), pReflexes));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spContentPrimaryReflex));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spContentSecondaryReflex));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spLeftHeaderPrimaryReflex));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spLeftHeaderSecondaryReflex));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spTopHeaderPrimaryReflex));
            IFC(CreateParametricReflex(spManager2.Get(), spDMViewport2.Get(), &pReflexes->m_spTopHeaderSecondaryReflex));

            // Add pReflexes to the map indicating it is ready for consumption.
#ifdef DM_DEBUG
            if (DMS_TraceDbg())
            {
                IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                    L"                   Adding overpan reflexes to map."));
            }
#endif // DM_DEBUG
            IFC(m_mapViewportOverpanReflexes.Add(static_cast<XHANDLE>(pViewport), pReflexes));
            AddRefInterface(pViewport);
            fCleanupReflexDataForFailedHR = FALSE;

            if (fIsStartingNewManipulation)
            {
                IFC(RefreshOverpanCurves(pViewport));
            }
        }
    }

Cleanup:
    if (FAILED(hr) && fCleanupReflexDataForFailedHR)
    {
        SAFE_DELETE(pReflexes);
    }

    // When the requirement for overpan reflexes changes, *pfAreOverpanModesChanged is set to True to indicate the
    // caller that the old transforms need to get released in CDirectManipulationService::ReleaseSharedContentTransform
    // and new ones created in CDirectManipulationService::EnsureSharedContentTransform.
    *pfAreOverpanModesChanged = m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport)) != fReflexesExist;

    RRETURN(hr);
}

_Check_return_ HRESULT
CDirectManipulationService::RefreshOverpanCurves(
    _In_ IObject* pViewport)
{
    HRESULT hr = S_OK;

    if (m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport)))
    {
        Microsoft::WRL::ComPtr<IDirectManipulationContent> spDMContent;
        IDirectManipulationViewport* pDMViewport = nullptr;
        ViewportOverpanReflexes* pReflexes = nullptr;
        RECT viewportBounds = {0, 0, 0, 0};
        RECT contentBounds = {0, 0, 0, 0};
        DEVMODE dm = { 0 };
        XFLOAT centerpointOffset = 0.0f;

        IFC(GetDMViewportFromHandle(pViewport, &pDMViewport));
        ASSERT(pDMViewport);

        IFC(m_mapViewportOverpanReflexes.Get(static_cast<XHANDLE>(pViewport), pReflexes));

        IFC(pDMViewport->GetViewportRect(&viewportBounds));
        ASSERT(viewportBounds.left != viewportBounds.right);
        ASSERT(viewportBounds.top != viewportBounds.bottom);

        // Note: GetContentRect() will fail unless SetContentRect() has been called
        // i.e. GetContentRect() will fail if we have not finished full setup of the manipluation
        IFC(pDMViewport->GetPrimaryContent(IID_PPV_ARGS(&spDMContent)));
        IFC((spDMContent->GetContentRect(&contentBounds)));

        dm.dmSize = sizeof(dm);
        if (::EnumDisplaySettings(nullptr /* CurrentDisplay */, ENUM_CURRENT_SETTINGS, &dm))
        {
            XFLOAT physicalDeviceHeight = static_cast<XFLOAT>(dm.dmPelsHeight);
            XFLOAT logicalDeviceHeight = physicalDeviceHeight / pReflexes->m_zoomScale;
            centerpointOffset = logicalDeviceHeight * s_centerPointScaleFactor;
        }
        ASSERT(centerpointOffset > 0.0f);

        IFC(m_pDMHelper->ApplyPrimaryReflexCurves(pReflexes->m_spContentPrimaryReflex.Get(), XcpDMContentTypePrimary,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, centerpointOffset, viewportBounds, contentBounds,
            s_curveSuppressionValueForZoom, s_curveSuppressionValueForTranslate));

        IFC(m_pDMHelper->ApplyPrimaryReflexCurves(pReflexes->m_spTopHeaderPrimaryReflex.Get(), XcpDMContentTypeTopHeader,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, centerpointOffset, viewportBounds, contentBounds,
            s_curveSuppressionValueForZoom, s_curveSuppressionValueForTranslate));

        IFC(m_pDMHelper->ApplyPrimaryReflexCurves(pReflexes->m_spLeftHeaderPrimaryReflex.Get(), XcpDMContentTypeLeftHeader,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, centerpointOffset, viewportBounds, contentBounds,
            s_curveSuppressionValueForZoom, s_curveSuppressionValueForTranslate));

        IFC(m_pDMHelper->ApplySecondaryReflexCurves(pReflexes->m_spContentSecondaryReflex.Get(), XcpDMContentTypePrimary,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, viewportBounds, contentBounds, 
            s_linearCurvePassThroughSlope, s_curveSuppressionValueForTranslate, s_range));

        IFC(m_pDMHelper->ApplySecondaryReflexCurves(pReflexes->m_spTopHeaderSecondaryReflex.Get(), XcpDMContentTypeTopHeader,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, viewportBounds, contentBounds, 
            s_linearCurvePassThroughSlope, s_curveSuppressionValueForTranslate, s_range));

        IFC(m_pDMHelper->ApplySecondaryReflexCurves(pReflexes->m_spLeftHeaderSecondaryReflex.Get(), XcpDMContentTypeLeftHeader,
            pReflexes->m_horizontalOverpanMode, pReflexes->m_verticalOverpanMode, viewportBounds, contentBounds, 
            s_linearCurvePassThroughSlope, s_curveSuppressionValueForTranslate, s_range));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CDirectManipulationService::CreateParametricBehavior(
    _In_ IDirectManipulationManager2* pManager2,
    _In_ IDirectManipulationViewport2* pDMViewport2,
    _In_ ViewportOverpanReflexes* pReflexes)
{
    HRESULT hr = S_OK;

    // Make sure one of the overpan modes is non-default.  There's no need to have a custom overpan behavior in that case,
    // since we can use the built-in DM overpan.
    ASSERT(pReflexes->m_horizontalOverpanMode != XcpDMOverpanModeDefault || pReflexes->m_verticalOverpanMode != XcpDMOverpanModeDefault);
    ASSERT(pReflexes->m_viewportBehaviorCookie == 0);

    IFC(m_pDMHelper->CreateParametricBehavior(
        pManager2,
        pDMViewport2,
        pReflexes->m_horizontalOverpanMode,
        pReflexes->m_verticalOverpanMode,
        &pReflexes->m_spViewportBehavior,
        &pReflexes->m_viewportBehaviorCookie));
    
    ASSERT(pReflexes->m_viewportBehaviorCookie != 0);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
CDirectManipulationService::CreateParametricReflex(
    _In_ IDirectManipulationManager2* pManager2,
    _In_ IDirectManipulationViewport2* pDMViewport2,
    _Outptr_result_nullonfailure_ IDirectManipulationContent** ppReflex)
{
    HRESULT hr = S_OK;
    Microsoft::WRL::ComPtr<IDirectManipulationContent> spReflex;

    *ppReflex = nullptr;

    IFC(pManager2->CreateContent(NULL /*pFrameInfoProvider*/, CLSID_Microsoft_ParametricMotionBehavior, IID_PPV_ARGS(&spReflex)));
    IFC(pDMViewport2->AddContent(spReflex.Get()));

    IFC(spReflex.CopyTo(ppReflex));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
CDirectManipulationService::CleanupOverpanReflexData(
    _In_ IObject* pViewport,
    _In_ IDirectManipulationViewport2* pDMViewport2)
{
    HRESULT hr = S_OK;
    ViewportOverpanReflexes* pOldReflexes = nullptr;
    XcpAutoLock lock(m_overpanReflexesLock);

#ifdef DM_DEBUG
    if (DMS_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
            L"DMS[0x%p]:   CleanupOverpanReflexData - pViewport=0x%p.", this, pViewport));
    }
#endif // DM_DEBUG

    ASSERT(pViewport);
    ASSERT(pDMViewport2);

    if (m_mapViewportOverpanReflexes.ContainsKey(static_cast<XHANDLE>(pViewport)))
    {
#ifdef DM_DEBUG
        if (DMS_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMS_DBG) /*traceType*/,
                L"                   Removing overpan reflexes from map."));
        }
#endif // DM_DEBUG
        IFC(m_mapViewportOverpanReflexes.Remove(static_cast<XHANDLE>(pViewport), pOldReflexes));
        ASSERT(pOldReflexes);
        pViewport->Release();
        if (pOldReflexes->m_viewportBehaviorCookie != 0)
        {
            IFC(RemoveOverpanBehavior(pDMViewport2, pOldReflexes));
        }
    }

Cleanup:
    SAFE_DELETE(pOldReflexes);
    RRETURN(hr);
}

_Check_return_ HRESULT
CDirectManipulationService::RemoveOverpanBehavior(
    _In_ IDirectManipulationViewport2* pDMViewport2,
    _In_ ViewportOverpanReflexes* pReflexes)
{
    HRESULT hr = S_OK;

    ASSERT(pReflexes->m_viewportBehaviorCookie != 0);
    IFC(pDMViewport2->RemoveBehavior(pReflexes->m_viewportBehaviorCookie));
    pReflexes->m_viewportBehaviorCookie = 0;

Cleanup:
    RRETURN(hr);
}


// Creates a viewport interaction
_Check_return_ HRESULT CDirectManipulationService::CreateViewportInteraction(
    _In_ IUnknown* compositor,
    _In_ IObject* viewport,
    _Out_ IUnknown** interaction)
{
    *interaction = nullptr;
    IDirectManipulationViewport* dmViewportNoRef = nullptr;
    IFC_RETURN(GetDMViewportFromHandle(viewport, &dmViewportNoRef));
    return m_pDMHelper->CreateSharedInteractionForViewport(dmViewportNoRef, compositor, interaction);
}


// Removes a viewport interaction
_Check_return_ HRESULT
CDirectManipulationService::RemoveViewportInteraction(
    _In_ IObject* viewport)
{
    IDirectManipulationViewport* dmViewportNoRef = nullptr;
    IFC_RETURN(GetDMViewportFromHandle(viewport, &dmViewportNoRef));
    return m_pDMHelper->RemoveViewportInteraction(dmViewportNoRef);
}

wrl::ComPtr<ixp::IPointerPoint> CDirectManipulationService::GetPointerPointFromPointerId(
    _In_ XUINT32 pointerId)
{
    if (m_pointerPointStatics == nullptr)
    {
        IFCFAILFAST(wf::GetActivationFactory(wrl_wrappers::HStringReference(STR_LEN_PAIR(RuntimeClass_Microsoft_UI_Input_PointerPoint)).Get(), &m_pointerPointStatics));
    }

    wrl::ComPtr<ixp::IPointerPoint> pointerPoint;
    IFCFAILFAST(m_pointerPointStatics->GetCurrentPoint(pointerId, pointerPoint.ReleaseAndGetAddressOf()));
    return pointerPoint;
}

wrl::ComPtr<IDirectManipulationManager3> CDirectManipulationService::CreateDirectManipulationManager()
{
    HMODULE hmodDManip = LoadLibraryExWAbs(L"Microsoft.DirectManipulation.dll", nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    ASSERT(hmodDManip != nullptr);

    wrl::ComPtr<IClassFactory> directManipulationFactory;
    IFCFAILFAST(DirectManipulationHelper::GetDirectManipulationManagerFactory(hmodDManip, &directManipulationFactory));

    wrl::ComPtr<IDirectManipulationManager> dmManager;
    IFCFAILFAST(directManipulationFactory->CreateInstance(nullptr, IID_PPV_ARGS(dmManager.ReleaseAndGetAddressOf())));

    wrl::ComPtr<IDirectManipulationManager3> expManager;
    IFCFAILFAST(dmManager.As(&expManager));

    return expManager;
}

DirectManipulationHelper* CDirectManipulationService::GetDirectManipulationHelper() const
{
    ASSERT(m_pDMCompositor != nullptr && m_pDMHelper.IsInitialized());
    return m_pDMHelper.Get();
}
