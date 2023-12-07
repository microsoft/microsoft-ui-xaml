// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Interface:  IPALDirectManipulationCompositorService
//  Synopsis:
//    Interface implemented by CDirectManipulationService and consumed by
//    the compositor thread via the CCompositorDirectManipulationViewport 
//    class.

#ifndef __PAL__DIRECTMANIPULATION__COMPOSITOR__SERVICE
#define __PAL__DIRECTMANIPULATION__COMPOSITOR__SERVICE

struct IPALDirectManipulationCompositorService : public IObject
{
    // Returns the content's transform info given its PAL-friendly handle.
    // Also retrieves whether the content is in inertia phase or not.
    virtual _Check_return_ HRESULT GetCompositorContentTransform(_In_ IObject* pCompositorContent, _In_ XDMContentType contentType, _Out_ bool& fIsInertial, _Out_ XFLOAT& translationX, _Out_ XFLOAT& translationY, _Out_ XFLOAT& uncompressedZoomFactor, _Out_ XFLOAT& zoomFactorX, _Out_ XFLOAT& zoomFactorY) = 0;

    // Even if no compositor node exists for a DM content, DM needs to be ticked in inertia mode. UpdateCompositorContentTransform is called for that purpose
    // deltaCompositionTime is the lapse of time in milliseconds between the time this call is made and the time the resulting transform is shown on screen
    virtual _Check_return_ HRESULT UpdateCompositorContentTransform(_In_ IObject* pCompositorContent, _In_ XUINT32 deltaCompositionTime) = 0;

    // Return a unique key value associated with the underlying DM viewport.
    virtual _Check_return_ HRESULT GetCompositorViewportKey(_In_ IObject* pCompositorViewport, _Out_ XHANDLE* pKey) = 0;

    // Return the status of the underlying DM viewport.
    virtual _Check_return_ HRESULT GetCompositorViewportStatus(_In_ IObject* pCompositorViewport, _Out_ XDMViewportStatus* pStatus) = 0;
};

#endif //__PAL__DIRECTMANIPULATION__COMPOSITOR__SERVICE
