// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CInputManagerDMViewportEventHandler class declaration. It is an
//    IXcpDirectManipulationViewportEventHandler implementation used to
//    forward DirectManipulation notifications to the InputManager.

#pragma once

// Uncomment to get DirectManipulation debug traces
// #define DMIMEV_DBG

#include "XcpDirectManipulationViewportEventHandler.h"

class CInputManagerDMViewportEventHandler : public CXcpObjectBase<IXcpDirectManipulationViewportEventHandler>
{
    // ------------------------------------------------------------------------
    // CInputManagerDMViewportEventHandler Public Methods
    // ------------------------------------------------------------------------
public:
    FORWARD_ADDREF_RELEASE(CXcpObjectBase<IXcpDirectManipulationViewportEventHandler>);

    static _Check_return_ HRESULT Create(
        _Outptr_ CInputManagerDMViewportEventHandler** ppInputManagerDMViewportEventHandler,
        _In_ CInputServices* inputServices);

    // IXcpDirectManipulationViewportEventHandler interface

    // Forwards a viewport status change to the InputManager
    _Check_return_ HRESULT ProcessDirectManipulationViewportStatusUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportStatus oldStatus,
        _In_ XDMViewportStatus newStatus) override;

    // Forwards a viewport interaction type change to the InputManager
    _Check_return_ HRESULT ProcessDirectManipulationViewportInteractionTypeUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportInteractionType newInteractionType) override;

    // Forwards a viewport dragging status change to the InputManager
    _Check_return_ HRESULT ProcessDirectManipulationViewportDraggingStatusChange(
        _In_ IObject* pViewport,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous) override;

    // ------------------------------------------------------------------------
    // CInputManagerDMViewportEventHandler Private Constructor/Destructor
    // ------------------------------------------------------------------------
private:
    CInputManagerDMViewportEventHandler(_In_ CInputServices* inputServices)
        : m_inputServicesNoRef(inputServices)
    {
#ifdef DMIMEV_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DMIMEV[0x%p]:CInputManagerDMViewportEventHandler - constructor.\r\n", this));
#endif // DMIMEV_DBG
        XCP_WEAK(&m_inputServicesNoRef);
        ASSERT(inputServices);
    }

    ~CInputManagerDMViewportEventHandler() override
    {
#ifdef DMIMEV_DBG
        IGNOREHR(gps->DebugOutputSzNoEndl(L"DMIMEV[0x%p]:~CInputManagerDMViewportEventHandler - destructor.\r\n", this));
#endif // DMIMEV_DBG
    }

private:
    // InputManager instance interested in the DirectManipulation notifications
    CInputServices* m_inputServicesNoRef;
};
