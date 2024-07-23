// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// CDMViewport class implementation.
// Each DirectManipulation viewport is associated with a CDMViewport
// instance by the InputManager.
// It corresponds to IObject pViewport passed to IPALDirectManipulationService
// in the PAL.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  CDMCViewport class.
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Method:   CDMCViewport::Create    (static)
//
//  Synopsis:
//      Creates an instance of the CDMCViewport class
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDMCViewport::Create(
_Outptr_ CDMCViewport** ppDMCViewport,
_In_ IPALDirectManipulationCompositorService* pDMCompositorService,
_In_ IObject* pCompositorViewport,
_In_ IObject* pCompositorPrimaryContent)
{
    HRESULT hr = S_OK;
    CDMCViewport* pDMCViewport = NULL;

    IFCPTR(ppDMCViewport);
    *ppDMCViewport = NULL;

    IFCPTR(pDMCompositorService);
    IFCPTR(pCompositorViewport);
    IFCPTR(pCompositorPrimaryContent);

    pDMCViewport = new CDMCViewport(
        pDMCompositorService,
        pCompositorViewport,
        pCompositorPrimaryContent);

    *ppDMCViewport = pDMCViewport;
    pDMCViewport = NULL;

Cleanup:
    delete pDMCViewport;
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  CDMViewportBase class.
//
//------------------------------------------------------------------------

//-------------------------------------------------------------------------
//
//  Function:   CDMViewportBase::GetContactIdCount
//
//  Synopsis:
//    Returns the number of contact Ids (pointerId) associated with this viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewportBase::GetContactIdCount(_Out_ XUINT32* pcContactIds)
{
    IFCPTR_RETURN(pcContactIds);

    *pcContactIds = m_contactIds.size();

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDMViewportBase::AddContactId
//
//  Synopsis:
//    Associates the provided contact Id (pointer Id) with this viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewportBase::AddContactId(_In_ XUINT32 contactId)
{
#ifdef DEBUG
    bool fContainsContactId;
    IGNOREHR(ContainsContactId(contactId, &fContainsContactId));
    ASSERT(!fContainsContactId);
#endif // DEBUG

    IFC_RETURN(m_contactIds.push_back(contactId));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDMViewportBase::GetContactId
//
//  Synopsis:
//    Gets the existing contact Id at the provided 0-based index
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewportBase::GetContactId(_In_ XUINT32 index, _Out_ XUINT32* pContactId)
{
    XUINT32 contactId = 0;

    IFCPTR_RETURN(pContactId);
    *pContactId = 0;

    IFC_RETURN(m_contactIds.get_item(index, contactId));
    *pContactId = contactId;

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDMViewportBase::ContainsContactId
//
//  Synopsis:
//    Returns True if the provided contact Id is associated with this viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewportBase::ContainsContactId(_In_ XUINT32 contactId, _Out_ bool* pfContainsContactId)
{
    XUINT32 contactIdTmp = 0;
    XUINT32 cContactIds = m_contactIds.size();

    IFCPTR_RETURN(pfContainsContactId);

    *pfContainsContactId = FALSE;

    for (XUINT32 iContactId = 0; iContactId < cContactIds; iContactId++)
    {
        IFC_RETURN(m_contactIds.get_item(iContactId, contactIdTmp));
        if (contactIdTmp == contactId)
        {
            *pfContainsContactId = TRUE;
            break;
        }
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CDMViewportBase::RemoveContactId
//
//  Synopsis:
//    Detach the provided contact Id from this viewport
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewportBase::RemoveContactId(_In_ XUINT32 contactId)
{
    XUINT32 iContactId = 0;
    XUINT32 contactIdTmp = 0;
    XUINT32 cContactIds = m_contactIds.size();

    for (iContactId = 0; iContactId < cContactIds; iContactId++)
    {
        IFC_RETURN(m_contactIds.get_item(iContactId, contactIdTmp));
        if (contactIdTmp == contactId)
        {
            IFC_RETURN(m_contactIds.erase(iContactId));
            break;
        }
    }
    if (iContactId == cContactIds)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  CDMViewport class.
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::Create    (static)
//
//  Synopsis:
//      Creates an instance of the CDMViewport class
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::Create(
_Outptr_result_nullonfailure_ CDMViewport** ppDMViewport,
_In_ CUIElement* pDMContainer,
_In_ CUIElement* pManipulatedElement)
{
    HRESULT hr = S_OK;
    CDMViewport* pDMViewport = NULL;

    IFCPTR(ppDMViewport);
    *ppDMViewport = NULL;

    IFCPTR(pDMContainer);
    IFCPTR(pManipulatedElement);

    pDMViewport = new CDMViewport(pDMContainer, pManipulatedElement);

    IFC(pDMViewport->SetCurrentStatus(XcpDMViewportBuilding));

    *ppDMViewport = pDMViewport;
    pDMViewport = NULL;

Cleanup:
    delete pDMViewport;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetOldStatus
//
//  Synopsis:
//    Returns the first status in the statuses vector.  
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetOldStatus(_Out_ XDMViewportStatus& oldStatus)
{
    oldStatus = XcpDMViewportBuilding;

    if (m_statuses.size() > 0)
    {
        IFC_RETURN(m_statuses.get_item(0, oldStatus));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::SetOldStatus
//
//  Synopsis:
//    Clears the statuses vector, and adds one status with the provided value.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::SetOldStatus(_In_ XDMViewportStatus oldStatus)
{
#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetOldStatus - oldStatus=%d.", this, oldStatus));
    }
#endif // DM_DEBUG

    m_statuses.clear();
    RRETURN(m_statuses.push_back(oldStatus));
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetCurrentStatus
//
//  Synopsis:
//    Returns the last status in the statuses vector.  
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetCurrentStatus(_Out_ XDMViewportStatus& currentStatus)
{
    currentStatus = XcpDMViewportBuilding;

    if (m_statuses.size() > 0)
    {
        IFC_RETURN(m_statuses.get_item(m_statuses.size() - 1, currentStatus));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::SetCurrentStatus
//
//  Synopsis:
//    Adds the provided status to the end of the statuses vector.  
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::SetCurrentStatus(_In_ XDMViewportStatus currentStatus)
{
#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   SetCurrentStatus - currentStatus=%d.", this, currentStatus));
    }
#endif // DM_DEBUG

    RRETURN(m_statuses.push_back(currentStatus));
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetStatus
//
//  Synopsis:
//    Returns the status in the statuses vector with the provided index,
//    or XcpDMViewportBuilding if the vector is empty.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetStatus(_In_ XUINT32 index, _Out_ XDMViewportStatus& status)
{
    status = XcpDMViewportBuilding;

    if (m_statuses.size() > 0)
    {
        ASSERT(index < m_statuses.size());
        IFC_RETURN(m_statuses.get_item(index, status));
    }

    return S_OK;
}

// Adds the provided interaction type to the end of the interaction types queue.
void
CDMViewport::PushInteractionType(_In_ XDMViewportInteractionType interactionType)
{
#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   PushInteractionType - interactionType=%d.", this, interactionType));
    }
#endif // DM_DEBUG

    m_queuedInteractionTypes.push(interactionType);
}

// Returns the front of the interaction type queue.
XDMViewportInteractionType
CDMViewport::GetFrontInteractionType()
{
    ASSERT(!m_queuedInteractionTypes.empty());
    return m_queuedInteractionTypes.front();
}

// Returns the back of the interaction type queue.
XDMViewportInteractionType
CDMViewport::GetBackInteractionType()
{
    ASSERT(!m_queuedInteractionTypes.empty());
    return m_queuedInteractionTypes.back();
}

// Deletes the front of the interaction type queue.
void
CDMViewport::PopInteractionType()
{
#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   PopInteractionType - interactionType=%d.", this, m_queuedInteractionTypes.front()));
    }
#endif // DM_DEBUG

    ASSERT(!m_queuedInteractionTypes.empty());
    m_queuedInteractionTypes.pop();
}

XUINT32
CDMViewport::GetConfigurationCount()
{
    return m_configurations.size();
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetConfiguration
//
//  Synopsis:
//    Returns the configuration at the provided index.  
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetConfiguration(_In_ XUINT32 index, _Out_ XDMConfigurations& configuration)
{
    configuration = XcpDMConfigurationNone;

    if (index < m_configurations.size())
    {
        IFC_RETURN(m_configurations.get_item(index, configuration));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::HasActiveStatus
//
//  Synopsis:
//    Sets fHasActiveStatus to True if the statuses vector contains an
//    active status.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::HasActiveStatus(_Out_ bool& fHasActiveStatus)
{
    XDMViewportStatus status = XcpDMViewportBuilding;

    fHasActiveStatus = FALSE;

    for (XUINT32 index = 0; index < m_statuses.size(); index++)
    {
        IFC_RETURN(m_statuses.get_item(index, status));
        if (status == XcpDMViewportRunning ||
            status == XcpDMViewportInertia ||
            status == XcpDMViewportSuspended)
        {
            fHasActiveStatus = TRUE;
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveStatus
//
//  Synopsis:
//    Removes the status in the statuses vector at the provided index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveStatus(_In_ XUINT32 index)
{
    ASSERT(index < m_statuses.size());
    RRETURN(m_statuses.erase(index));
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveOldStatus
//
//  Synopsis:
//    Removes the first status in the statuses vector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveOldStatus()
{
    RRETURN(RemoveStatus(0));
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveCurrentStatus
//
//  Synopsis:
//    Removes the last status in the statuses vector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveCurrentStatus()
{
    ASSERT(m_statuses.size() > 0);
    RRETURN(RemoveStatus(m_statuses.size() - 1));
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::HasConfiguration
//
//  Synopsis:
//    Sets fHasConfiguration to True when the provided configuration is
//    part of the added configurations.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::HasConfiguration(_In_ XDMConfigurations configuration, _Out_ bool& fHasConfiguration)
{
    XINT32 configurationIndex = -1;

    fHasConfiguration = FALSE;

    IFC_RETURN(GetConfigurationIndex(configuration, configurationIndex));
    fHasConfiguration = (configurationIndex != -1);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::AddConfiguration
//
//  Synopsis:
//    Adds a new configuration to the configurations array.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::AddConfiguration(_In_ XDMConfigurations configuration)
{
    XINT32 configurationIndex = -1;

    IFC_RETURN(GetConfigurationIndex(configuration, configurationIndex));
    if (configurationIndex == -1)
    {
#ifdef DM_DEBUG
        if (DM_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   AddConfiguration - Adding configuration=%d.", this, configuration));
        }
#endif // DM_DEBUG

        IFC_RETURN(m_configurations.push_back(configuration));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveConfiguration
//
//  Synopsis:
//    Removes a configuration from the configuration array if it exists.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveConfiguration(_In_ XDMConfigurations configuration)
{
    XINT32 configurationIndex = -1;

    IFC_RETURN(GetConfigurationIndex(configuration, configurationIndex));
    if (configurationIndex != -1)
    {
#ifdef DM_DEBUG
        if (DM_TraceDbg())
        {
            IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   RemoveConfiguration - Removing configuration=%d.", this, configuration));
        }
#endif // DM_DEBUG
        IFC_RETURN(m_configurations.erase(configurationIndex));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetConfigurationIndex
//
//  Synopsis:
//    Returns the index of the provided configuration in the array.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetConfigurationIndex(_In_ XDMConfigurations configuration, _Out_ XINT32& configurationIndex)
{
    XUINT32 cConfigurations = 0;
    XDMConfigurations configurationTmp;

    configurationIndex = -1;

    cConfigurations = m_configurations.size();
    for (XUINT32 iConfiguration = 0; iConfiguration < cConfigurations; iConfiguration++)
    {
        IFC_RETURN(m_configurations.get_item(iConfiguration, configurationTmp));
        if (configurationTmp == configuration)
        {
            configurationIndex = (XINT32)iConfiguration;
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::SetManipulatedElement
//
//  Synopsis:
//    Replaces the manipulated element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::SetManipulatedElement(_In_ CUIElement* pManipulatedElement)
{
    IFCPTR_RETURN(pManipulatedElement);

    ASSERT(m_pManipulatedElement);
    ReplaceInterface(m_pManipulatedElement, pManipulatedElement);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::AddContent
//
//  Synopsis:
//    Adds a new CDMContent instance to the m_pContents vector. Returns
//    it if ppContent is set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::AddContent(_In_ CUIElement* pContentElement, _In_ XDMContentType contentType, _Outptr_result_maybenull_ CDMContent** ppContent)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   AddContent - pContentElement=0x%p, contentType=%d.",
            this, pContentElement, contentType));
    }
#endif // DM_DEBUG

    IFCPTR(pContentElement);
    if (ppContent)
    {
        *ppContent = NULL;
    }

    if (!m_pContents)
    {
        m_pContents = new xvector<CDMContent*>();
    }

#ifdef DBG
    CDMContent* pContentDbg = NULL;
    IGNOREHR(GetContentNoRef(pContentElement, &pContentDbg, NULL /*pContentIndex*/));
    ASSERT(!pContentDbg);
#endif // DBG

    IFC(CDMContent::Create(pContentElement, contentType, &pContent));
    IFC(m_pContents->push_back(pContent));
    if (ppContent)
    {
        AddRefInterface(pContent);
        *ppContent = pContent;
    }
    pContent = NULL;

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveContent
//
//  Synopsis:
//    Removes the CDMContent instance from the m_pContents vector for the
//    provided element. Releases the instance or returns it depending on
//    the optional ppContent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveContent(_In_ CUIElement* pContentElement, _Outptr_result_maybenull_ CDMContent** ppContent)
{
    XUINT32 contentIndex = 0;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   RemoveContent - pContentElement=0x%p.",
            this, pContentElement));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pContentElement);
    if (ppContent)
    {
        *ppContent = NULL;
    }

    IFC_RETURN(GetContentNoRef(pContentElement, &pContent, &contentIndex));
    if (pContent)
    {
        IFC_RETURN(m_pContents->erase(contentIndex));
        if (ppContent)
        {
            *ppContent = pContent;
        }
        else
        {
            ReleaseInterface(pContent);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetContent
//
//  Synopsis:
//    Returns the CDMContent instance at the provided index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetContent(_In_ XUINT32 contentIndex, _Outptr_ CDMContent** ppContent)
{
    CDMContent* pContent = NULL;
    ASSERT(ppContent != nullptr);
    *ppContent = NULL;

    IFC_RETURN(m_pContents->get_item(contentIndex, pContent));
    ASSERT(pContent);
    AddRefInterface(pContent);
    *ppContent = pContent;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetContentNoRef
//
//  Synopsis:
//    Returns the CDMContent instance for the provided element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetContentNoRef(_In_ CUIElement* pContentElement, _Out_opt_ CDMContent** ppContent, _Out_opt_ XUINT32* pContentIndex)
{
    XUINT32 cContents = 0;
    CDMContent* pContent = NULL;

    if (ppContent)
    {
        *ppContent = NULL;
    }
    if (pContentIndex)
    {
        *pContentIndex = 0;
    }

    if (m_pContents)
    {
        cContents = m_pContents->size();
        for (XUINT32 iContent = 0; iContent < cContents; iContent++)
        {
            IFC_RETURN(m_pContents->get_item(iContent, pContent));
            ASSERT(pContent);
            if (pContent->GetContentElementNoRef() == pContentElement)
            {
                if (ppContent)
                {
                    *ppContent = pContent;
                }
                if (pContentIndex)
                {
                    *pContentIndex = iContent;
                }
                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetContentElementNoRef
//
//  Synopsis:
//    Returns the content element at the provided index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetContentElementNoRef(_In_ XUINT32 contentIndex, _Out_ CUIElement** ppContentElement)
{
    CDMContent* pContent = NULL;

    IFCPTR_RETURN(ppContentElement);
    *ppContentElement = NULL;

    if (!m_pContents || m_pContents->size() <= contentIndex)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(m_pContents->get_item(contentIndex, pContent));
    ASSERT(pContent);
    *ppContentElement = pContent->GetContentElementNoRef();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::ClearContents
//
//  Synopsis:
//    Releases all the CDMContent instances, clears and deletes the 
//    m_pContents vector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::ClearContents()
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   ClearContents.", this));
    }
#endif // DM_DEBUG

    if (m_pContents)
    {
        while (m_pContents->size() > 0)
        {
            IFC(m_pContents->get_item(0, pContent));
            ASSERT(pContent);
            IFC(m_pContents->erase(0));
            ReleaseInterface(pContent);
        }
    }

Cleanup:
    delete m_pContents;
    m_pContents = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::AddClipContent
//
//  Synopsis:
//    Adds a new CDMContent instance to the m_pClipContents vector. Returns
//    it if ppContent is set.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::AddClipContent(_In_ CUIElement* pContentElement, _In_ XDMContentType contentType, _Outptr_result_maybenull_ CDMContent** ppContent)
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   AddClipContent - pContentElement=0x%p, contentType=%d.",
            this, pContentElement, contentType));
    }
#endif // DM_DEBUG

    IFCPTR(pContentElement);
    if (ppContent)
    {
        *ppContent = NULL;
    }

    if (!m_pClipContents)
    {
        m_pClipContents = new xvector<CDMContent*>();
    }

#ifdef DBG
    CDMContent* pClipContentDbg = NULL;
    IGNOREHR(GetClipContentNoRef(pContentElement, &pClipContentDbg, NULL /*pContentIndex*/));
    ASSERT(!pClipContentDbg);
#endif // DBG

    IFC(CDMContent::Create(pContentElement, contentType, &pContent));
    IFC(m_pClipContents->push_back(pContent));
    if (ppContent)
    {
        AddRefInterface(pContent);
        *ppContent = pContent;
    }
    pContent = NULL;

Cleanup:
    ReleaseInterface(pContent);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::RemoveClipContent
//
//  Synopsis:
//    Removes the CDMContent instance from the m_pClipContents vector for the
//    provided element. Releases the instance or returns it depending on
//    the optional ppContent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::RemoveClipContent(_In_ CUIElement* pContentElement, _Outptr_result_maybenull_ CDMContent** ppContent)
{
    XUINT32 contentIndex = 0;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   RemoveClipContent - pContentElement=0x%p.",
            this, pContentElement));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pContentElement);
    if (ppContent)
    {
        *ppContent = NULL;
    }

    IFC_RETURN(GetClipContentNoRef(pContentElement, &pContent, &contentIndex));
    if (pContent)
    {
        IFC_RETURN(m_pClipContents->erase(contentIndex));
        if (ppContent)
        {
            *ppContent = pContent;
        }
        else
        {
            ReleaseInterface(pContent);
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetClipContent
//
//  Synopsis:
//    Returns the CDMContent instance at the provided index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetClipContent(_In_ XUINT32 contentIndex, _Outptr_ CDMContent** ppContent)
{
    CDMContent* pContent = NULL;

    IFCPTR_RETURN(ppContent);
    *ppContent = NULL;

    if (!m_pClipContents)
    {
        IFC_RETURN(E_FAIL);
    }

    if (m_pClipContents->size() <= contentIndex)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    IFC_RETURN(m_pClipContents->get_item(contentIndex, pContent));
    ASSERT(pContent);
    AddRefInterface(pContent);
    *ppContent = pContent;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetClipContentNoRef
//
//  Synopsis:
//    Returns the CDMContent instance for the provided element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetClipContentNoRef(_In_ CUIElement* pContentElement, _Outptr_opt_result_maybenull_ CDMContent** ppContent, _Out_opt_ XUINT32* pContentIndex)
{
    XUINT32 cContents = 0;
    CDMContent* pContent = NULL;

    if (ppContent)
    {
        *ppContent = NULL;
    }
    if (pContentIndex)
    {
        *pContentIndex = 0;
    }

    if (m_pClipContents)
    {
        cContents = m_pClipContents->size();
        for (XUINT32 iContent = 0; iContent < cContents; iContent++)
        {
            IFC_RETURN(m_pClipContents->get_item(iContent, pContent));
            ASSERT(pContent);
            if (pContent->GetContentElementNoRef() == pContentElement)
            {
                if (ppContent)
                {
                    *ppContent = pContent;
                }
                if (pContentIndex)
                {
                    *pContentIndex = iContent;
                }
                break;
            }
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::GetClipContentElementNoRef
//
//  Synopsis:
//    Returns the clip content element at the provided index.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::GetClipContentElementNoRef(_In_ XUINT32 contentIndex, _Outptr_ CUIElement** ppContentElement)
{
    CDMContent* pContent = NULL;

    IFCPTR_RETURN(ppContentElement);
    *ppContentElement = NULL;

    if (!m_pClipContents || m_pClipContents->size() <= contentIndex)
    {
        IFC_RETURN(E_FAIL);
    }

    IFC_RETURN(m_pClipContents->get_item(contentIndex, pContent));
    ASSERT(pContent);
    *ppContentElement = pContent->GetContentElementNoRef();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CDMViewport::ClearClipContents
//
//  Synopsis:
//    Releases all the CDMContent instances, clears and deletes the 
//    m_pClipContents vector.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CDMViewport::ClearClipContents()
{
    HRESULT hr = S_OK;
    CDMContent* pContent = NULL;

#ifdef DM_DEBUG
    if (DM_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_INPUT_MANAGER_VIEWPORT | DMV_DBG) /*traceType*/, L"DMV[0x%p]:   ClearClipContents.", this));
    }
#endif // DM_DEBUG

    if (m_pClipContents)
    {
        while (m_pClipContents->size() > 0)
        {
            IFC(m_pClipContents->get_item(0, pContent));
            ASSERT(pContent);
            IFC(m_pClipContents->erase(0));
            ReleaseInterface(pContent);
        }
    }

Cleanup:
    delete m_pClipContents;
    m_pClipContents = NULL;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  CDMCrossSlideViewport class.
//
//------------------------------------------------------------------------

//------------------------------------------------------------------------
//
//  Method:   CDMCrossSlideViewport::Create    (static)
//
//  Synopsis:
//      Creates an instance of the CDMCrossSlideViewport class
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CDMCrossSlideViewport::Create(
_Outptr_ CDMCrossSlideViewport** ppDMCrossSlideViewport,
_In_ CUIElement* pDMContainer)
{
    HRESULT hr = S_OK;
    CDMCrossSlideViewport* pDMCrossSlideViewport = NULL;

    IFCPTR(ppDMCrossSlideViewport);
    *ppDMCrossSlideViewport = NULL;

    IFCPTR(pDMContainer);

    pDMCrossSlideViewport = new CDMCrossSlideViewport(pDMContainer);

    *ppDMCrossSlideViewport = pDMCrossSlideViewport;
    pDMCrossSlideViewport = NULL;

Cleanup:
    delete pDMCrossSlideViewport;
    RRETURN(hr);
}
