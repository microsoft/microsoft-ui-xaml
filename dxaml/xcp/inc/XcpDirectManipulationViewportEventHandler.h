// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Interface:  XcpDirectManipulationViewportEventHandler interface
//  Synopsis:
//    Interface used to forward DirectManipulation notifications from the
//    PAL to the InputManager. This interface is implemented by the
//    CInputManagerDMViewportEventHandler class.

#pragma once

#ifndef __XCP__DIRECTMANIPULATION__VIEWPORT__EVENTHANDLER
#define __XCP__DIRECTMANIPULATION__VIEWPORT__EVENTHANDLER
#include <Microsoft.DirectManipulation.h>

struct IXcpDirectManipulationViewportEventHandler : public IObject
{
    // Used to forward a viewport status change.
    virtual _Check_return_ HRESULT ProcessDirectManipulationViewportStatusUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportStatus oldStatus,
        _In_ XDMViewportStatus newStatus) = 0;

    // Used to forward a viewport interaction type change.
    virtual _Check_return_ HRESULT ProcessDirectManipulationViewportInteractionTypeUpdate(
        _In_ IObject* pViewport,
        _In_ XDMViewportInteractionType newInteractionType) = 0;

    // Forwards a viewport dragging status change to the InputManager
    virtual _Check_return_ HRESULT ProcessDirectManipulationViewportDraggingStatusChange(
        _In_ IObject* pViewport,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS current,
        _In_ DIRECTMANIPULATION_DRAG_DROP_STATUS previous) = 0;
};

#endif //__XCP__DIRECTMANIPULATION__VIEWPORT__EVENTHANDLER
