// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    CDirectManipulationViewportEventHandler class used for listening to
//    DirectManipulation feedback on the UI thread.

#pragma once

#include <Microsoft.DirectManipulation.h>

// Uncomment for DManip debug outputs.
//#define DM_DEBUG

class CDirectManipulationService;

class CDirectManipulationViewportEventHandler : public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>,
                                                public IDirectManipulationViewportEventHandler,
                                                public IDirectManipulationInteractionEventHandler,
                                                public IDirectManipulationDragDropEventHandler
{
private:
    // CDirectManipulationService implementation that handles the notifications.
    CDirectManipulationService* m_pDMService;

#ifdef DM_DEBUG
    bool DMVEH_TraceDbg() const;
#endif // DM_DEBUG

protected:
    CDirectManipulationViewportEventHandler();
    ~CDirectManipulationViewportEventHandler();

public:
    _Check_return_ HRESULT SetDMService(_In_opt_ CDirectManipulationService* pDMService);

public:
    DECLARE_NOT_AGGREGATABLE(CDirectManipulationViewportEventHandler)
    DECLARE_PROTECT_FINAL_CONSTRUCT()

    BEGIN_COM_MAP(CDirectManipulationViewportEventHandler)
        COM_INTERFACE_ENTRY(IDirectManipulationViewportEventHandler)
        COM_INTERFACE_ENTRY(IDirectManipulationInteractionEventHandler)
        COM_INTERFACE_ENTRY(IDirectManipulationDragDropEventHandler)
    END_COM_MAP()

    // IDirectManipulationViewportEventHandler
    IFACEMETHOD(OnViewportStatusChanged)(
        _In_ IDirectManipulationViewport* pDMViewport,
        _In_ DIRECTMANIPULATION_STATUS current,
        _In_ DIRECTMANIPULATION_STATUS previous);
        
    IFACEMETHOD(OnViewportUpdated)(
        _In_ IDirectManipulationViewport* pDMViewport);
        
    IFACEMETHOD(OnContentUpdated)(
        _In_ IDirectManipulationViewport* pDMViewport,
        _In_ IDirectManipulationContent* pDMContent);

    // IDirectManipulationInteractionEventHandler
    IFACEMETHOD(OnInteraction)(
        _In_ IDirectManipulationViewport2* pDMViewport,
        _In_ DIRECTMANIPULATION_INTERACTION_TYPE newInteractionType);

    // IDirectManipulationDragDropEventHandler 
    IFACEMETHOD(OnDragDropStatusChange)(
        _In_  IDirectManipulationViewport2 *viewport,
        _In_  DIRECTMANIPULATION_DRAG_DROP_STATUS current,
        _In_  DIRECTMANIPULATION_DRAG_DROP_STATUS previous);
};
