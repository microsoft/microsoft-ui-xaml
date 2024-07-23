// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Define as 1 (i.e. XCP_TRACE_OUTPUT_MSG) to get debug outputs, and 0 otherwise
#define DMVEH_DBG 0

#ifdef DM_DEBUG
bool CDirectManipulationViewportEventHandler::DMVEH_TraceDbg() const
{
    bool result = gps->IsDebugTraceTypeActive(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER);

    if constexpr (DMVEH_DBG)
    {
        result = true;
    }

    return result;
}
#endif // DM_DEBUG

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::CDirectManipulationViewportEventHandler
//
//  Synopsis:
//      Constructor for the CDirectManipulationViewportEventHandler class.
//
//------------------------------------------------------------------------
CDirectManipulationViewportEventHandler::CDirectManipulationViewportEventHandler()
{
#ifdef DM_DEBUG
    if (DMVEH_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMVEH_DBG) /*traceType*/,
            L"DMVEH[0x%p]: CDirectManipulationViewportEventHandler constructor.", this));
    }
#endif // DM_DEBUG

    m_pDMService = NULL;
}

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::~CDirectManipulationViewportEventHandler
//
//  Synopsis:
//      Destructor for the CDirectManipulationViewportEventHandler class.
//
//------------------------------------------------------------------------
CDirectManipulationViewportEventHandler::~CDirectManipulationViewportEventHandler()
{
#ifdef DM_DEBUG
    if (DMVEH_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMVEH_DBG) /*traceType*/,
            L"DMVEH[0x%p]: ~CDirectManipulationViewportEventHandler destructor.", this));
    }
#endif // DM_DEBUG

    ASSERT(m_pDMService == NULL);
}

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::SetDMService
//
//  Synopsis:
//    Sets the owning CDirectManipulationService instance that is interested
//    in handling the DirectManipulation feedback.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CDirectManipulationViewportEventHandler::SetDMService(
    _In_opt_ CDirectManipulationService* pDMService)
{
    m_pDMService = pDMService;
    RRETURN(S_OK);
}


// IDirectManipulationViewportEventHandler implementation

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::OnViewportStatusChanged
//
//  Synopsis:
//    Called when the status of a viewport changed.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationViewportEventHandler::OnViewportStatusChanged(
    _In_ IDirectManipulationViewport* pDMViewport,
    _In_ DIRECTMANIPULATION_STATUS current,
    _In_ DIRECTMANIPULATION_STATUS previous)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMVEH_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMVEH_DBG) /*traceType*/,
            L"DMVEH[0x%p]: OnViewportStatusChanged entry for UI thread consumption. pDMViewport=0x%p, previous=%d, current=%d.", this, pDMViewport, previous, current));
    }
#endif // DM_DEBUG

    if (m_pDMService)
    {
        IFCPTR(pDMViewport);
        IFC(m_pDMService->NotifyViewportStatusUpdate(pDMViewport, previous, current));
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::OnViewportUpdated
//
//  Synopsis:
//    Called once all contents in a viewport raised the OnContentUpdated
//    notification below. This notification is not consumed at this point.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationViewportEventHandler::OnViewportUpdated(
    _In_ IDirectManipulationViewport* pDMViewport)
{
    RRETURN(S_FALSE);
}
        
//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::OnContentUpdated
//
//  Synopsis:
//    Called once the transformation matrix for a particular content is
//    updated and ready for consumption.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationViewportEventHandler::OnContentUpdated(
    _In_ IDirectManipulationViewport* pDMViewport,
    _In_ IDirectManipulationContent* pDMContent)
{
    RRETURN(S_FALSE);
}


// IDirectManipulationIntereactionEventHandler implementation

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::OnInteraction
//
//  Synopsis:
//    Called when the interaction type of a viewport changed.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationViewportEventHandler::OnInteraction(
    _In_ IDirectManipulationViewport2* pDMViewport,
    _In_ DIRECTMANIPULATION_INTERACTION_TYPE newInteractionType)
{
    HRESULT hr = S_OK;

#ifdef DM_DEBUG
    if (DMVEH_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMVEH_DBG) /*traceType*/,
            L"DMVEH[0x%p]: OnInteraction entry for UI thread consumption. pDMViewport=0x%p, newInteractionType=%d.", this, pDMViewport, newInteractionType));
    }
#endif // DM_DEBUG

    if (m_pDMService)
    {
        IFCPTR(pDMViewport);
        IFC(m_pDMService->NotifyViewportInteractionTypeUpdate(pDMViewport, newInteractionType));
    }

Cleanup:
    RRETURN(hr);
}

// IDirectManipulationDragDropEventHandler implementation

//------------------------------------------------------------------------
//
//  Method:   CDirectManipulationViewportEventHandler::OnDragDropStatusChange
//
//  Synopsis:
//    Called when the drag drop status changed.
//
//------------------------------------------------------------------------
IFACEMETHODIMP
CDirectManipulationViewportEventHandler::OnDragDropStatusChange(
    _In_ IDirectManipulationViewport2* pDMViewport,           // same as _spDirectManipulationViewport
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,         // the current status of the viewport
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous         // the previous status of the viewport
    )
{
#ifdef DM_DEBUG
    if (DMVEH_TraceDbg())
    {
        IGNOREHR(gps->DebugTrace(static_cast<XDebugTraceType>(XCP_TRACE_DM_PAL_SERVICE_VIEWPORT_EVENTHANDLER | XCP_TRACE_APPEND_OUTPUT_WITH_THREAD_ID | DMVEH_DBG) /*traceType*/,
            L"DMVEH[0x%p]: OnDragDropStatusChange entry for UI thread consumption. pDMViewport=0x%p, current=%d, previous=%d.", this, pDMViewport, current, previous));
    }
#endif // DM_DEBUG

    IFCPTR_RETURN(pDMViewport);
    if (m_pDMService)
    {
        IFC_RETURN(m_pDMService->NotifyViewportDraggingStatusChange(pDMViewport, current, previous));
    }

    return S_OK;
}
