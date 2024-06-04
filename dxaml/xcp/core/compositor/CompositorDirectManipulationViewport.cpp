// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// CCompositorDirectManipulationViewport class implementation.
// Each DirectManipulation viewport is associated with a 
// CCompositorDirectManipulationViewport instance by CCoreServices and 
// handed off to the compositor thread for handling DM notifications.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Method:   CCompositorDirectManipulationViewport::Create (static)
//
//  Synopsis:
//      Creates an instance of the CCompositorDirectManipulationViewport class
//
//------------------------------------------------------------------------
_Check_return_
HRESULT 
CCompositorDirectManipulationViewport::Create(
    _Outptr_ CCompositorDirectManipulationViewport **ppCDMViewport, 
    _In_ IObject* pViewport,
    _In_ IObject* pPrimaryContent,
    _In_ IPALDirectManipulationCompositorService* pDirectManipulationCompositorService)
{
    HRESULT hr = S_OK;
    CCompositorDirectManipulationViewport* pCDMViewport = NULL;

    *ppCDMViewport = NULL;

    pCDMViewport = new CCompositorDirectManipulationViewport(
        pViewport,
        pPrimaryContent,
        pDirectManipulationCompositorService);

    // Query and save the key for the underlying DM viewport.
    IGNOREHR(pDirectManipulationCompositorService->GetCompositorViewportKey(pViewport, &pCDMViewport->m_dmViewportKey));

    *ppCDMViewport = pCDMViewport;
    RRETURN(hr);//RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the transforms associated with the DirectManipulation viewport
//
//------------------------------------------------------------------------
void
CCompositorDirectManipulationViewport::UpdateTransform()
{
    IObject* pCompositorContent = GetCompositorPrimaryContentNoRef();
    ASSERT(pCompositorContent);

    IPALDirectManipulationCompositorService* pCompositorService = GetDMCompositorServiceNoRef();
    ASSERT(pCompositorService);

    // TODO - Jupiter (Windows) bug 847117. Replace 16 with the actually milliseconds until the transform is shown on screen
    // Errors are ignored here because they can occur when a viewport is discarded.
    IGNOREHR(pCompositorService->UpdateCompositorContentTransform(
        pCompositorContent,
        16 /*deltaCompositionTime*/
        ));
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Queries the underlying DM viewport for the latest status.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
CCompositorDirectManipulationViewport::QueryDMStatus(_Out_ XDMViewportStatus& status)
{
    IObject* pCompositorViewport = GetCompositorViewportNoRef();
    ASSERT(pCompositorViewport);

    IPALDirectManipulationCompositorService* pCompositorService = GetDMCompositorServiceNoRef();
    ASSERT(pCompositorService);

    // This call is designed to return an error code sometimes, as such as don't trace it in
    // the typical error handling tools to avoid allocating and capturing a stack.
    IFC_NOTRACE_RETURN(pCompositorService->GetCompositorViewportStatus(pCompositorViewport, &status));

    // We only expect internal DM viewport status codes.
    ASSERT(status <= XcpDMViewportSuspended);

    return S_OK;
}
