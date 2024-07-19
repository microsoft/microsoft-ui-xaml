// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class DCompSurface;

//-------------------------------------------------------------------------
//
//  Synopsis:
//      DCompSurfaceMonitor is a debugging aid.  It keeps a collection of
//      DComp surfaces for each UI thread (stored in TLS).  This collection
//      dynamically changes each UI thread frame to track only the surfaces
//      which were drawn.  At the end of each frame, all tracked surfaces
//      are dumped to disk.
// 
//-------------------------------------------------------------------------
class DCompSurfaceMonitor
{
public:

    static HRESULT Initialize();
    static HRESULT DeInitialize();
    static HRESULT CleanupDCompSurfaceCollection();
    static HRESULT TrackSurfaceDrawn(_In_ DCompSurface *pSurface);
    static HRESULT RemoveSurface(_In_ DCompSurface* pSurface);
    static HRESULT DumpSurfacesDrawn(XUINT32 frameNumber);

private:
    typedef xchainedmap<DCompSurface*, bool> DCompSurfaceCollection;
    static DCompSurfaceCollection *GetDCompSurfaceCollection(_In_ bool createIfNull = true);

private:
    static XUINT32 g_monitorTlsIndex;
};

