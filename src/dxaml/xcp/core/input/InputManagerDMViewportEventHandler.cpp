// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// CInputManagerDMViewportEventHandler class implementation. It is an
// IXcpDirectManipulationViewportEventHandler implementation used to
// forward DirectManipulation notifications to the InputManager on the
// UI thread.

#include "precomp.h"
#include "InputServices.h"
//------------------------------------------------------------------------
//
//  Method:   CInputManagerDMViewportEventHandler::Create   (static)
//
//  Synopsis:
//      Creates an instance of the CInputManagerDMViewportEventHandler class
//
//------------------------------------------------------------------------
_Check_return_
HRESULT
CInputManagerDMViewportEventHandler::Create(
    _Outptr_ CInputManagerDMViewportEventHandler** ppInputManagerDMViewportEventHandler,
    _In_ CInputServices* inputServices)
{
    HRESULT hr = S_OK;
    CInputManagerDMViewportEventHandler* pInputManagerDMViewportEventHandler = NULL;

    IFCPTR(ppInputManagerDMViewportEventHandler);
    *ppInputManagerDMViewportEventHandler = NULL;

    IFCPTR(inputServices);

    pInputManagerDMViewportEventHandler = new CInputManagerDMViewportEventHandler(inputServices);

    *ppInputManagerDMViewportEventHandler = pInputManagerDMViewportEventHandler;
    pInputManagerDMViewportEventHandler = NULL;

Cleanup:
    delete pInputManagerDMViewportEventHandler;
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Function:   CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportStatusUpdate
//
//  Synopsis:
//    Forwards a viewport status change to the InputManager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportStatusUpdate(
    _In_ IObject* pViewport,
    _In_ XDMViewportStatus oldStatus,
    _In_ XDMViewportStatus newStatus)
{
    IFCPTR_RETURN(pViewport);
    IFCEXPECT_ASSERT_RETURN(m_inputServicesNoRef);

    IFC_RETURN(m_inputServicesNoRef->ProcessDirectManipulationViewportStatusUpdate(static_cast<CDMViewport*>(pViewport), oldStatus, newStatus));

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportInteractionTypeUpdate
//
//  Synopsis:
//    Forwards a viewport interaction type change to the InputManager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportInteractionTypeUpdate(
    _In_ IObject* pViewport,
    _In_ XDMViewportInteractionType newInteractionType)
{
    IFCPTR_RETURN(pViewport);
    IFCEXPECT_ASSERT_RETURN(m_inputServicesNoRef);

    // pViewport is a CDMViewportBase, i.e. the base class for CDMViewport and CDMCrossSlideViewport. Only CDMViewport
    // instances are processing the interaction type change. Their GetIsCrossSlideViewport() implementation returns False.
    auto dmViewportBase = static_cast<CDMViewportBase*>(pViewport);
    if (!dmViewportBase->GetIsCrossSlideViewport())
    {
        IFC_RETURN(m_inputServicesNoRef->ProcessDirectManipulationViewportInteractionTypeUpdate(static_cast<CDMViewport*>(dmViewportBase), newInteractionType));
    }

    return S_OK;
}


//-------------------------------------------------------------------------
//
//  Function:   CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportDraggingStatusChange
//
//  Synopsis:
//    Forwards a viewport drag drop status change to the InputManager
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT
CInputManagerDMViewportEventHandler::ProcessDirectManipulationViewportDraggingStatusChange(
    _In_ IObject* pViewport,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
    _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous)
{
    ASSERT(pViewport);
    IFCEXPECT_ASSERT_RETURN(m_inputServicesNoRef);

    return m_inputServicesNoRef->ProcessDirectManipulationViewportDraggingStatusChange(
        static_cast<CDMCrossSlideViewport*>(pViewport),
        current,
        previous);
}
