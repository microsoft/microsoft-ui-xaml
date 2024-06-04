// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XUINT32 DCompSurfaceMonitor::g_monitorTlsIndex = TLS_OUT_OF_INDEXES;

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Helper function to retrieve the collection of DComp surfaces from TLS
//
//-------------------------------------------------------------------------
DCompSurfaceMonitor::DCompSurfaceCollection *DCompSurfaceMonitor::GetDCompSurfaceCollection(_In_ bool createIfNull)
{
    DCompSurfaceCollection *pDCompSurfaceCollection = NULL;

    if (g_monitorTlsIndex != TLS_OUT_OF_INDEXES)
    {
        pDCompSurfaceCollection = reinterpret_cast<DCompSurfaceCollection*>(TlsGetValue(g_monitorTlsIndex));

        if (pDCompSurfaceCollection == NULL && createIfNull)
        {
            // Create the collection on demand and store in TLS
            pDCompSurfaceCollection = new DCompSurfaceCollection;
            TlsSetValue(g_monitorTlsIndex, pDCompSurfaceCollection);
        }
    }

    return pDCompSurfaceCollection;
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Initialization method, called on DLL load
//
//-------------------------------------------------------------------------
HRESULT
DCompSurfaceMonitor::Initialize()
{
    HRESULT hr = S_OK;

    // Don't reinitialize if already done so.
    if (g_monitorTlsIndex != TLS_OUT_OF_INDEXES)
    {
        return S_OK;
    }

    g_monitorTlsIndex = TlsAlloc();
    if (g_monitorTlsIndex == TLS_OUT_OF_INDEXES)
    {
        IFC(E_FAIL);
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      De-initialization method, called on DLL unload
//
//-------------------------------------------------------------------------
HRESULT
DCompSurfaceMonitor::DeInitialize()
{
    if (g_monitorTlsIndex != TLS_OUT_OF_INDEXES)
    {
        TlsFree(g_monitorTlsIndex);
        g_monitorTlsIndex = TLS_OUT_OF_INDEXES;
    }

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Cleanup method, called when UI thread exits
//
//-------------------------------------------------------------------------
HRESULT
DCompSurfaceMonitor::CleanupDCompSurfaceCollection()
{
    // We don't need to create an instance just to delete it.
    DCompSurfaceCollection *pDCompSurfaceCollection = GetDCompSurfaceCollection(false /*createIfNull*/);
    if (pDCompSurfaceCollection != NULL)
    {
        delete pDCompSurfaceCollection;
        TlsSetValue(g_monitorTlsIndex, NULL);
    }

    RRETURN(S_OK);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Mark this surface as having been drawn to this frame
//
//-------------------------------------------------------------------------
HRESULT
DCompSurfaceMonitor::TrackSurfaceDrawn(_In_ DCompSurface* pSurface)
{
    HRESULT hr = S_OK;

    DCompSurfaceCollection *pDCompSurfaceCollection = GetDCompSurfaceCollection();
    if (pDCompSurfaceCollection != NULL)
    {
        bool dummy = false;
        IFC(pDCompSurfaceCollection->Add(pSurface, dummy));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Remove a surface from the collection, useful when a surface is destroyed.
//
//-------------------------------------------------------------------------
HRESULT DCompSurfaceMonitor::RemoveSurface(_In_ DCompSurface *pSurface)
{
    HRESULT hr = S_OK;

    // If the surface collection doesn't exist, then it can't be tracking this surface so there is no point
    // in creating it.
    DCompSurfaceCollection *pDCompSurfaceCollection = GetDCompSurfaceCollection(false /*createIfNull*/);
    if (pDCompSurfaceCollection != NULL)
    {
        bool dummy = false;
        IFC(pDCompSurfaceCollection->Remove(pSurface, dummy));
    }

Cleanup:
    RRETURN(hr);
}

//-------------------------------------------------------------------------
//
//  Synopsis:
//      Dump the collection of surfaces we've marked as drawn this frame to disk
//
//-------------------------------------------------------------------------
HRESULT
DCompSurfaceMonitor::DumpSurfacesDrawn(XUINT32 frameNumber)
{
    HRESULT hr = S_OK;

    // If the surface collection doesn't exist, then there isn't anything to dump and we'll just no-op
    DCompSurfaceCollection *pDCompSurfaceCollection = GetDCompSurfaceCollection(false /*createIfNull*/);
    if (pDCompSurfaceCollection != NULL)
    {
        for (DCompSurfaceCollection::iterator it = pDCompSurfaceCollection->begin();
             it != pDCompSurfaceCollection->end();
             it++)
        {
            IFC(it->first->TraceDCompSurface(frameNumber));
        }

        // Clear out the collection.  The next time a surface is drawn
        // it will be added back to the collection.
        pDCompSurfaceCollection->Clear();
    }

Cleanup:
    RRETURN(hr);
}

