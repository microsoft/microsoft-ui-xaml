// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <limits.h>
#include "NodeStreamCache.h"
#include <ParserAPI.h>
#include <FrameworkTheming.h>
#include <FocusMgr.h>

#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <DesignMode.h>

//------------------------------------------------------------------------
//
//  Method:   CApplication::~CApplication
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------

CApplication::~CApplication()
{
    if (m_pEventList)
    {
        m_pEventList->Clean();
        delete m_pEventList;
    }

    ReleaseInterface(m_pRootVisual);
    ReleaseInterface(m_pRootScrollViewer);
    ReleaseInterface(m_pRootContentPresenter);
    if (m_pResources)
    {
        // Clear the ResourceDictionary's owner weak ref to avoid a dangling pointer
        m_pResources->SetResourceOwner(nullptr);
        ReleaseInterface(m_pResources);
    }
}

//------------------------------------------------------------------------
//
//  Method: AddEventListener
//
//  Synopsis:
//      Override to base AddEventListener
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CApplication::AddEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue,
    _In_ XINT32 iListenerType,
    _Out_opt_ CValue *pResult,
    _In_ bool fHandledEventsToo)
{
    return CEventManager::AddEventListener(this, &m_pEventList, hEvent, pValue, iListenerType, pResult, fHandledEventsToo);
}

//------------------------------------------------------------------------
//
//  Method: RemoveEventListener
//
//  Synopsis:
//      Override to base RemoveEventListener
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CApplication::RemoveEventListener(
    _In_ EventHandle hEvent,
    _In_ CValue *pValue)
{
    return CEventManager::RemoveEventListener(this, m_pEventList, hEvent, pValue);
}


//------------------------------------------------------------------------
//
//  Method: EnterImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CApplication::EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params)
{
    CEventManager * pEventManager = NULL;

    params.fSkipNameRegistration = TRUE;
    IFC_RETURN(CDependencyObject::EnterImpl(pNamescopeOwner, params));

    // If there are events registered on this element, ask the
    // EventManager to extract them and a request for every event.
    if (params.fIsLive && m_pEventList)
    {
        auto core = GetContext();
        // Get the event manager.
        IFCPTR_RETURN(core);
        pEventManager = core->GetEventManager();
        IFCPTR_RETURN(pEventManager);
        IFC_RETURN(pEventManager->AddRequestsInOrder(this, m_pEventList));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: LeaveImpl
//
//  Synopsis:
//
//------------------------------------------------------------------------

_Check_return_
HRESULT
CApplication::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CDependencyObject::LeaveImpl(pNamescopeOwner, params));

    // If we are leaving the Live tree and there are events.
    if (params.fIsLive && m_pEventList)
    {
        // Add the events in...
        CXcpList<REQUEST>::XCPListNode *pTemp = m_pEventList->GetHead();
        while (pTemp)
        {
            REQUEST * pRequest = (REQUEST *)pTemp->m_pData;
            IFC_RETURN( GetContext()->GetEventManager()->RemoveRequest(this, pRequest));
            pTemp = pTemp->m_pNext;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireStartupEvent
//
//  Synopsis:
//      Call the event manager to fire this event.
//
//------------------------------------------------------------------------
void
CApplication::FireStartupEvent()
{
    // See if we have an event handler for "Startup"
    if (m_pEventList)
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            CStartupEventArgs* pArgs = new CStartupEventArgs();

            pEventManager->Raise(EventHandle(KnownEventIndex::Application_Startup), TRUE, this, pArgs);
            ReleaseInterface(pArgs);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireExitEvent
//
//  Synopsis:
//      Call the event manager to fire this event.  These events are fired
//      directly (rather than async like the Starting, Startup, Started
//      events), so it is safe to fire Exiting and Exited without needing
//      to wait on a request p-invoke.
//
//------------------------------------------------------------------------
void
CApplication::FireExitEvent()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    REQUEST * pRequest = NULL;
    CXcpList<REQUEST>* pTempEventList = NULL;

    // Fire Exiting event to notify application services that
    // the application is exiting
    FireExitingEvent();

    if (m_pEventList)
    {
        EventHandle hExitEvent = EventHandle(KnownEventIndex::Application_Exit);
        CXcpList<REQUEST>::XCPListNode *pTemp;

        pTemp = m_pEventList->GetHead();
        IFCEXPECT(pTemp);

        pTempEventList = m_pEventList;
        m_pEventList = NULL;

        while (pTemp != NULL)
        {
            pRequest = (REQUEST *)pTemp->m_pData;

            // Is this an exit event?
            if (hExitEvent == pRequest->m_hEvent && pRequest->m_pListener)
            {
                // Fire exit event. This is a synchronous call, so
                // can reenter CApplication.
                IFC(GetContext()->CLR_FireEvent(
                    pRequest->m_pListener,
                    hExitEvent,
                    this,
                    NULL));
            }

            pTemp = pTemp->m_pNext;
        }

        m_pEventList = pTempEventList;
        pTempEventList = NULL;
    }

    // Fire exited event to notify application services that the
    // Exit event has been fired and to stop any running services
    FireExitedEvent();

Cleanup:
    if (pTempEventList)
    {
        // Make sure that the event list doesn't leak
        m_pEventList = pTempEventList;
    }

}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireStartingEvent
//
//  Synopsis:
//      Call the event manager to fire this event.
//
//------------------------------------------------------------------------
void
CApplication::FireStartingEvent()
{
    // See if we have an event handler for "Started"

    if (m_pEventList)
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            CStartupEventArgs* pArgs = new CStartupEventArgs();

            pEventManager->Raise(EventHandle(KnownEventIndex::Application_Starting), TRUE, this, pArgs);
            ReleaseInterface(pArgs);
        }
    }
}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireStartedEvent
//
//  Synopsis:
//      Call the event manager to fire this event.
//
//------------------------------------------------------------------------
void
CApplication::FireStartedEvent()
{
    // See if we have an event handler for "Started"

    if (m_pEventList)
    {
        CEventManager *pEventManager = GetContext()->GetEventManager();
        if (pEventManager)
        {
            pEventManager->Raise(EventHandle(KnownEventIndex::Application_Started), TRUE, this, NULL);
        }
    }

    // set a flag to keep track of whether
    // the application startup event has
    // finished executing
    m_bApplicationStartupCompleted = true;

    TraceApplicationStartedInfo();
}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireExitingEvent
//
//  Synopsis:
//      Call the event manager to fire this event.
//
//------------------------------------------------------------------------
void
CApplication::FireExitingEvent()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    REQUEST * pRequest = NULL;
    CXcpList<REQUEST>* pTempEventList = NULL;

    if (m_pEventList)
    {
        EventHandle hExitEvent(KnownEventIndex::Application_Exiting);
        CXcpList<REQUEST>::XCPListNode *pTemp;

        pTemp = m_pEventList->GetHead();
        IFCEXPECT(pTemp);

        pTempEventList = m_pEventList;
        m_pEventList = NULL;

        while (pTemp != NULL)
        {
            pRequest = (REQUEST *)pTemp->m_pData;

            // Is this an exit event?
            if (hExitEvent == pRequest->m_hEvent && pRequest->m_pListener)
            {
                // Fire exiting event. This is a synchronous call, so
                // can reenter CApplication.
                IFC(GetContext()->CLR_FireEvent(
                    pRequest->m_pListener,
                    hExitEvent,
                    this,
                    NULL));
            }

            pTemp = pTemp->m_pNext;
        }

        m_pEventList = pTempEventList;
        pTempEventList = NULL;
    }

Cleanup:
    if (pTempEventList)
    {
        // Make sure that the event list doesn't leak
        m_pEventList = pTempEventList;
    }

}

//------------------------------------------------------------------------
//
//  Method:   CApplication::FireExitedEvent
//
//  Synopsis:
//      Call the event manager to fire this event.
//
//------------------------------------------------------------------------
void
CApplication::FireExitedEvent()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    REQUEST * pRequest = NULL;
    CXcpList<REQUEST>* pTempEventList = NULL;

    if (m_pEventList)
    {
        EventHandle hExitEvent(KnownEventIndex::Application_Exited);
        CXcpList<REQUEST>::XCPListNode *pTemp;

        pTemp = m_pEventList->GetHead();
        IFCEXPECT(pTemp);

        pTempEventList = m_pEventList;
        m_pEventList = NULL;

        while (pTemp != NULL)
        {
            pRequest = (REQUEST *)pTemp->m_pData;

            // Is this an exit event?
            if (hExitEvent == pRequest->m_hEvent && pRequest->m_pListener)
            {
                // Fire exited event. This is a synchronous call, so
                // can reenter CApplication.
                IFC(GetContext()->CLR_FireEvent(
                    pRequest->m_pListener,
                    hExitEvent,
                    this,
                    NULL));
            }

            pTemp = pTemp->m_pNext;
        }

        m_pEventList = pTempEventList;
        pTempEventList = NULL;
    }

Cleanup:
    if (pTempEventList)
    {
        // Make sure that the event list doesn't leak
        m_pEventList = pTempEventList;
    }

}

//------------------------------------------------------------------------
//
//  Method:   CApplication::LoadComponent
//
//  Synopsis: OM Method
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CApplication::LoadComponent(
    _In_ CCoreServices     *pCore,
    _In_ CDependencyObject *pComponent,
    _In_ IPALUri *pUri
    )
{
    xref_ptr<CDependencyObject> obj;
    Parser::XamlBuffer buffer;
    xref_ptr<IPALResourceManager> resourceManager;
    bool isLocalResource = false;
    xstring_ptr strCanonicalUri;
    xstring_ptr strPhysicalResourceUri;
    std::shared_ptr<XamlNodeStreamCacheManager> spXamlNodeStreamCacheManager;
    bool isXamlCached = false;
    xref_ptr<IPALUri> physicalResourceUri;
    xref_ptr<IPALMemory> memory;

    IFC_RETURN(pCore->GetResourceManager(resourceManager.ReleaseAndGetAddressOf()));
    IFC_RETURN(resourceManager->IsLocalResourceUri(pUri, &isLocalResource));
    if (!isLocalResource)
    {
        // TODO: Set error message
        return E_INVALIDARG;
    }

    IFC_RETURN(pUri->GetCanonical(&strCanonicalUri));

    if (gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE)))
    {
        const WCHAR* wszComponentResourceLocation = pUri->GetComponentResourceLocation() == ::ComponentResourceLocation::Application ? L"Application" : L"Nested";
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE), L"LoadComponent: %s, %s", strCanonicalUri.GetBuffer(), wszComponentResourceLocation));
    }

    IFC_RETURN(pCore->GetXamlNodeStreamCacheManager(spXamlNodeStreamCacheManager));
    IFC_RETURN(spXamlNodeStreamCacheManager->HasCacheForUri(strCanonicalUri, &isXamlCached));

    if (isXamlCached)
    {
        // The XAML is cached, so we don't have to pass in an actual XAML string. However, we cannot pass
        // an empty string to ParseXamlWithExistingFrameworkRoot below, because it detects that and
        // returns a NULL DO. So we simply pass in a string with one space.
        buffer.m_count = 2;
        buffer.m_buffer = reinterpret_cast<const BYTE*>(L" ");
        buffer.m_bufferType = Parser::XamlBufferType::Text;
        if (gps->IsDebugTraceTypeActive(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE)))
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_RESOURCELOADING | XCP_TRACE_VERBOSE), L"LoadComponent: Retrieving XAML for %s from cache", strCanonicalUri.GetBuffer()));
        }
    }
    else
    {

        bool isBinaryXaml = false;

        IFC_RETURN(pCore->LoadXamlResource(pUri, &isBinaryXaml, memory.ReleaseAndGetAddressOf(), &buffer, physicalResourceUri.ReleaseAndGetAddressOf()));

        if (pUri != physicalResourceUri)
        {
            IFC_RETURN(physicalResourceUri->GetCanonical(&strPhysicalResourceUri));
        }
    }

    pComponent->SetBaseUri(pUri);
    pComponent->SetCanParserOverwriteBaseUri(false);
    auto parsingGuard = wil::scope_exit([&pComponent]()
    {
        pComponent->SetBaseUri(nullptr);
        pComponent->SetCanParserOverwriteBaseUri(true);
        TraceApplicationLoadComponentEnd();
    });

    // Allow the designer to perform template validation when calling Application::LoadComponent
    auto expandTemplatesDuringParse = DesignerInterop::GetDesignerMode(DesignerMode::V2Only);

    TraceApplicationLoadComponentBegin(strCanonicalUri.GetBuffer());
    IFC_RETURN(pCore->ParseXamlWithExistingFrameworkRoot(buffer, pComponent, xstring_ptr(), strPhysicalResourceUri, expandTemplatesDuringParse, obj.ReleaseAndGetAddressOf()));
    parsingGuard.release();
    TraceApplicationLoadComponentEnd();

    return S_OK;
}

// Accesses the xaml::Application::FocusVisualKind property value.
DirectUI::FocusVisualKind
CApplication::GetFocusVisualKind()
{
    return FxCallbacks::FrameworkApplication_GetFocusVisualKind();
}

// Accesses the xaml::Application::ApplicationHighContrastAdjustment property value.
_Check_return_ HRESULT
CApplication::GetApplicationHighContrastAdjustment(
    _Out_ DirectUI::ApplicationHighContrastAdjustment* pApplicationHighContrastAdjustment)
{
    IFC_RETURN(FxCallbacks::FrameworkApplication_GetApplicationHighContrastAdjustment(pApplicationHighContrastAdjustment));
    return S_OK;
}

// Invoked when the xaml::Application::FocusVisualKind property changed.
// Ensures the current focused element's focus rect gets re-rendered according to the new property value.
void
CApplication::OnFocusVisualKindChanged()
{
    ContentRootCoordinator* contentRootCoordinator = GetContext()->GetContentRootCoordinator();
    const auto& contentRoots = contentRootCoordinator->GetContentRoots();

    for (const auto& contentRoot: contentRoots)
    {
        contentRoot->GetFocusManagerNoRef()->SetFocusVisualDirty();
    }
}

_Check_return_ HRESULT
CApplication::OnHighContrastAdjustmentChanged()
{
    IFC_RETURN(GetContext()->GetMainRootVisual()->NotifyApplicationHighContrastAdjustmentChangedCore());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CreateBaseUri
//
//  Synopsis:
//
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CApplication::CreateBaseUri(
                _In_ XUINT32 cUriString,
                _In_reads_(cUriString) const WCHAR *pUriString,
                _Outptr_ IPALUri** ppUri)
{
    IPALUri *pPalUri = NULL;
    HRESULT hr = S_OK;
    XUINT32 cBaseUristr = APPLICATION_BASE_URI_LEN;
    WCHAR *pTempUristr = NULL;

    IFCPTR(ppUri);
    *ppUri = NULL;

    if (!pUriString || cUriString == 0)
    {
        IFC(E_FAIL);
    }

    if (pUriString && pUriString[0] == '/')
    {
        pUriString++;
        cUriString--;
    }

    if (cUriString == 0)
    {
        IFC(E_FAIL);
    }

    {
        xephemeral_string_ptr strTemp(pUriString, cUriString);

        if (MsUriHelpers::IsMsResourceUri(strTemp) ||
            strTemp.StartsWith(MsUriHelpers::GetAppxScheme(), xstrCompareCaseInsensitive))
        {
            IFC(gps->UriCreate(strTemp.GetCount(), strTemp.GetBuffer(), &pPalUri));
        }
        else
        {
            xstrconcat(&pTempUristr, &cBaseUristr, APPLICATION_BASE_URI, cBaseUristr, pUriString, cUriString);
            IFCPTR(pTempUristr);
            IFC(gps->UriCreate(cBaseUristr, pTempUristr, &pPalUri));
        }
    }

   *ppUri = pPalUri;
   pPalUri = NULL;

Cleanup:
    delete[] pTempUristr;
    ReleaseInterface(pPalUri);

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   SetValue
//
//  Synopsis: Override for DO::SetValue
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CApplication::SetValue(_In_ const SetValueParams& args)
{
    xref_ptr<CResourceDictionary> oldResources;
    auto core = GetContext();

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Application_Resources:
        {
            // check for a special case: application resources being set from App.xaml
            // (this happens before the native application object is fully configured)
            if (!core->GetDeployment()->m_pApplication)
            {
                IFC_RETURN(core->GetDeployment()->SetTempApplicationObjectForResourceLookup(this));
            }

            if (m_pResources)
            {
                if (m_pResources->HasImplicitStyle())
                {
                    // Store old ResourceDictionary
                    oldResources = m_pResources;
                }
                else
                {
                    m_pResources->SetResourceOwner(nullptr);
                }
            }
            break;
        }

        case KnownPropertyIndex::Application_RequestedTheme:
        {
            xaml::ApplicationTheme requestedTheme;

            if (args.m_value.IsEnum())
            {
                requestedTheme = static_cast<xaml::ApplicationTheme>(args.m_value.AsEnum());
            }
            else
            {
                auto pEnum = static_cast<CEnumerated*>(args.m_value.AsObject());
                requestedTheme = static_cast<xaml::ApplicationTheme>(pEnum->m_nValue);
            }

            // Return early here so we mimick FrameworkApplication::put_RequestedThemeImpl and don't actually
            // store the value in sparse storage
            return core->GetFrameworkTheming()->SetRequestedTheme(requestedTheme);
        }
    }

    IFC_RETURN(CDependencyObject::SetValue(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::Application_Resources:
        {
            if (oldResources != m_pResources)
            {
                bool stylesInvalidated = false;
                // if we have implicit style in new ResourceDictionary
                // then we want to invalidate all implicit styles from root
                // to be sure that correct style will picked up by the element
                if (m_pResources)
                {
                    m_pResources->SetResourceOwner(this);

                    if (m_pResources->HasImplicitStyle())
                    {
                        // if we got new ResourceDictionary then invalidate all implicit styles
                        IFC_RETURN(core->InvalidateImplicitStylesOnRoots(nullptr));

                        stylesInvalidated = true;
                    }

                    m_pResources->InvalidateNotFoundCache(true);
                }

                if (oldResources)
                {
                    if (!stylesInvalidated)
                    {
                        // otherwise invalidate only existed.
                        IFC_RETURN(core->InvalidateImplicitStylesOnRoots(oldResources.get()));
                    }
                    oldResources->SetResourceOwner(nullptr);
                }
            }
            break;
        }
    }

    return S_OK;
}

void CApplication::CleanupDeviceRelatedResourcesRecursive(bool cleanupDComp)
{
    if (m_pResources)
    {
        m_pResources->CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

        auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
        if (runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::EnableCoreShutdown))
        {
            // Clear the ResourceDictionary's owner weak ref to avoid a dangling pointer
            m_pResources->SetResourceOwner(nullptr);
            m_pResources->RemoveParent(this);
            ReleaseInterface(m_pResources);
        }
    }
}
