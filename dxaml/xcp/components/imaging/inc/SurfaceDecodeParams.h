// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include <hwtexturemgr.h>

// This class is currently used as a minimal immutable entity to store a RECT and Surface representing
// a single surface tile that is part of a larger tiled entity.  It is currently used for
// Background Thread Image Loading feature.
// This should not to be confused with the TiledSurface class which subdivides a single larger surface
// into separate tiles.  This class is used in ImageSurfaceWrapper to minimally represent a single tile
// from TiledSurface.
class SurfaceDecodeParams
    : public CXcpObjectBase<>
{
public:

    SurfaceDecodeParams(
        _In_ const XRECT& rect,
        _In_ HWTexture* pSurface
        ) :
        m_rect(rect),
        m_spSurface(pSurface)
    {
    }

    XRECT GetRect() { return m_rect; }

    const xref_ptr<HWTexture>& GetSurface() { return m_spSurface; }

private:

    XRECT m_rect;

    xref_ptr<HWTexture> m_spSurface;
};
