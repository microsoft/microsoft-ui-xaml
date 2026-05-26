// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SurfaceDecodeParams.h"

class ImageCopyParams
    : public CXcpObjectBase<IObject>
{
public:
    ImageCopyParams(
        _In_ wrl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap
        )
        : m_spSoftwareBitmap(spSoftwareBitmap)
    {
    }

    ImageCopyParams(
        _In_ wrl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap,
        _In_ SurfaceUpdateList& surfaceUpdateList
        )
        : m_spSoftwareBitmap(spSoftwareBitmap)
        , m_surfaceUpdateList(std::move(surfaceUpdateList))
    {
    }

    const wrl::ComPtr<wgri::ISoftwareBitmap>& GetBitmap() const { return m_spSoftwareBitmap; }

    const SurfaceUpdateList& GetSurfaceUpdateList() const { return m_surfaceUpdateList; }

private:

    wrl::ComPtr<wgri::ISoftwareBitmap> m_spSoftwareBitmap;
    SurfaceUpdateList m_surfaceUpdateList;
};
