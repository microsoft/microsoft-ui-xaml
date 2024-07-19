// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "corep.h"
#include "perf.h"

// static member
EncodedPtr<IPlatformUtilities> CControlBase::s_pUtilities;

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Create the control basic infrastructure
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CControlBase::Init()
{
    HRESULT hr = S_OK;

    IFCEXPECT(m_pDispatcher);
    if (IsInit())
    {
        goto Cleanup;
    }

    if (!GetPALCoreServices()->IsSupportedPlatform())
    {
        IFC(E_FAIL);
    }

    m_bFirstLoad = true;

    // This needs to be done before the call to UpdateSource
    // because the security token needs to be generated before
    // starting any downloads
    m_objIdentity = gps->GenerateSecurityToken();

    // Create the BrowserHost
    if (m_pBH)
    {
        m_pBH->Deinit();
    }
    ReleaseInterface(m_pBH);
    IFC(gps->BrowserHostCreate(static_cast<IXcpHostSite *>(this), m_pDispatcher, &m_pBH));
    IFC(m_pDispatcher->SetBrowserHost(m_pBH));

    // Cannot load source until load event since the DOM is not
    // in place yet.
    if (m_bFirstLoad)
    {
        UpdateSource();
    }

Cleanup:
    if (FAILED(hr))
    {
        Deinit();
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function:   CControlBase::GetClientURI
//
//  Synopsis:
//      Get the URIs
//
//------------------------------------------------------------------------
HRESULT
CControlBase::GetClientURI()
{
    if (!m_pBaseUri || !m_pDocUri)
    {
        IFC_RETURN(m_pXcpHostCallback->GetBASEUrlFromDocument(&m_pBaseUri, &m_pDocUri));
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Cleanup
//
//-------------------------------------------------------------------------
void
CControlBase::Deinit()
{
    //
    // CORE DEPENDENT CLEANUP
    //
    if (m_pDispatcher)
    {
        m_pDispatcher->Deinit();
    }

    //
    // IMPORTANT: After the browser host Deinit, core will be released. Perform any
    // core-dependent clean-up above.
    //
    if (m_pBH)
    {
        m_pBH->Deinit();
    }
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//     Redraw
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CControlBase::OnPaint()
{
    if (IsInit())
    {
        if (m_bFirstLoad)
        {
            UpdateSource();
        }

        if (m_pDispatcher && m_pBH && m_pBH->HasRenderTarget())
        {
            IFC_RETURN(m_pDispatcher->OnPaint());
        }
    }
    return S_OK;
}

//-----------------------------------------------------------------------------
//
//  Function: CControlBase::UpdateSource
//
//  Synopsis:
//     Load the source Xaml
//
//-----------------------------------------------------------------------------
HRESULT
CControlBase::UpdateSource(_In_ EVENTPFN pFn)
{
    HRESULT hr = S_OK;

    bool firstLoad = m_bFirstLoad;

    if (!m_pDispatcher || !m_pBH)
    {
        goto Cleanup;
    }

    IFC(m_pDispatcher->SetControl(this, pFn));

    m_bFirstLoad = false;

    IFC(m_pBH->put_EmptySource(
        firstLoad));

 Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CControlBase::ScriptCallback
//
//  Synopsis:
//     This code calls back into a handler that is defined in the DOM.
//      If flags set with HandledEventsToo flag, the indicating event handler
//      should be invoked even though the routed event is marked as handled.
//
//-------------------------------------------------------------------------
HRESULT
CControlBase::ScriptCallback(
    _In_reads_bytes_(sizeof(CControlBase)) void* pControl,
    _In_ CDependencyObject *pListener,
    _In_ EventHandle hEvent,
    _In_opt_ CDependencyObject* pSender,
    _In_opt_ CEventArgs* pArgs,
    _In_ XINT32 flags,
    _In_opt_ IScriptObject* pScriptObject,
    _In_opt_ INTERNAL_EVENT_HANDLER pInternalHandler)
{
    // look in cache
    // walk the IE DOM looking for function to call
    HRESULT hr = S_OK;
    XINT32 pfShouldFire = FALSE;
    CControlBase* pControlInstance = NULL;

    IFCPTR(pControl);

    pControlInstance = static_cast<CControlBase*> (pControl);

    // If flags set with HandledEventsToo, ShouldFireEvent() will return true
    if ( pArgs && pControlInstance && pControlInstance->m_pBH )
    {
        IFC(pControlInstance->m_pBH->ShouldFireEvent(pListener, hEvent, pSender, pArgs, flags, &pfShouldFire));
        if (!pfShouldFire)
        {
            // Always use the cleanup code path! - it frees memory and sets the security
            // state for fullscreen.
            goto Cleanup;
        }
    }

    if (pInternalHandler == NULL)
    {
        // Looks like a valid CLR event
        if (pControlInstance->m_pBH)
        {
            XCP_PERF_LOG(XCP_PMK_CLRCALLBACK_BEGIN);
            IFC(pControlInstance->m_pBH->CLR_FireEvent(pListener, hEvent, pSender, pArgs, flags));
            XCP_PERF_LOG(XCP_PMK_CLRCALLBACK_END);
        }
        // Always use the cleanup code path! - it frees memory and sets the security
        // state for fullscreen.
        goto Cleanup;
    }
    else
    {
        if (pSender)
        {
            IFC(pControlInstance->IsDependencyObjectValid(pSender));
            IFC(pInternalHandler(pSender, pArgs));
        }
    }

 Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CControlBase::IsDependencyObjectValid
//
//  Synopsis:
//     Check DO whether it point the dead core or not by reset Core
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CControlBase::IsDependencyObjectValid(_In_ CDependencyObject* pDO)
{
    HRESULT hr = S_OK;

    CCoreServices *pCoreFromHost = nullptr;
    CCoreServices *pCoreFromDO = nullptr;

    IFCPTR(m_pBH);
    IFCPTR(pDO);

    pCoreFromHost = m_pBH->GetContextInterface();
    IFCPTR(pCoreFromHost);

    pCoreFromDO = pDO->GetContextInterface();
    IFCPTR(pCoreFromDO);

    if (pCoreFromHost != pCoreFromDO || pCoreFromHost->GetIdentity() != pCoreFromDO->GetIdentity())
    {
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Function:   CControlBase::CheckReentrancy
//
//  Synopsis:
//      Is reentrancy?
//
//------------------------------------------------------------------------

_Check_return_
HRESULT CControlBase::CheckReentrancy()
{
    HRESULT hr = S_OK;
    XINT32 bReentrancyAllowed;

    if (!m_pBH)
    {
        goto Cleanup;
    }

    // Reentrancy is not allowed in a synchronous callout

    IFC(m_pBH->IsReentrancyAllowed(&bReentrancyAllowed));

    // Report Error if reentrancy is not allowed
    if (!bReentrancyAllowed)
    {
        IErrorService *pErrorService;
        IFC(m_pBH->GetErrorService(&pErrorService));
        HRESULT hrToOriginate = E_FAIL;
        IGNOREHR(pErrorService->ReportGenericError(hrToOriginate, RuntimeError,
                        AG_E_RUNTIME_INVALID_CALL, TRUE, 0,0, NULL, 0));

        IFC(hrToOriginate);
    }

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the frame counter debug setting.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CControlBase::SetEnableFrameRateCounter(bool isEnabled)
{
    HRESULT hr = S_OK;

    if (isEnabled != m_isFrameCounterEnabled)
    {
        m_isFrameCounterEnabled = isEnabled;

        IFC(OnDebugSettingsChanged());
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify the core that a debug setting has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CControlBase::OnDebugSettingsChanged()
{
    HRESULT hr = S_OK;

    if (m_pBH != NULL)
    {
        CCoreServices *pCoreNoRef = m_pBH->GetContextInterface();
        if (pCoreNoRef != NULL)
        {
            IFC(pCoreNoRef->OnDebugSettingsChanged());
        }
    }

Cleanup:

    RRETURN(hr);
}
