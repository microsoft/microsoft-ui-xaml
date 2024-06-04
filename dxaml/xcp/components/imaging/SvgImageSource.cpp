// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// TODO: Refactor and remove the need to include the marked header files below
//       marked with "Shouldn't be necessary".  Since imagesource.h is fairly monolithic and
//       requires the world, this requires a fair bit of refactoring of imagesource.h in order
//       to remove these include dependencies.

#include "precomp.h"
#include <limits>
#include <MUX-ETWEvents.h>
#include <Windows.Foundation.h>
#include <GraphicsUtility.h>
#include "ImageProviderInterfaces.h"
#include "ImageCache.h"
#include "palgfx.h"
#include "corep.h" // Shouldn't be necessary
#include "depends.h" // Shouldn't be necessary
#include "framework.h" // Shouldn't be necessary
#include "brush.h" // Shouldn't be necessary
#include "tilebrush.h" // Shouldn't be necessary
#include "imagebrush.h" // Shouldn't be necessary
#include "mediabase.h" // Shouldn't be necessary
#include "imagebase.h" // Shouldn't be necessary
#include "ImageMetadata.h"
#include "TiledSurface.h"
#include "ImageSurfaceWrapper.h"
#include "imagesource.h"
#include "ImageProvider.h"
#include "SurfaceDecodeParams.h"
#include "ImageCopyParams.h"
#include "ImageAvailableCallback.h"
#include "CoreAsyncAction.h"
#include "SvgImageSource.h"
#include "host.h"
#include <real.h>
#include <d2d1_3.h>
#include <d2d1svg.h>

XUINT32
CSvgImageSource::GetDecodeWidth(
    )
{
    // Treat infinity (property was unset) as 0 which is reliable to test against for auto-sizing
    auto rasterizeWidth = (m_rasterizeWidth == std::numeric_limits<float>::infinity()) ? 0 : m_rasterizeWidth;

    return std::max(XcpRound(rasterizeWidth), 0);
}

XUINT32
CSvgImageSource::GetDecodeHeight(
    )
{
    // Treat infinity (property was unset) as 0 which is reliable to test against for auto-sizing
    auto rasterizeHeight = (m_rasterizeHeight == std::numeric_limits<float>::infinity()) ? 0 : m_rasterizeHeight;

    return std::max(XcpRound(rasterizeHeight), 0);
}

_Check_return_ HRESULT
CSvgImageSource::EnsureAndUpdateHardwareResources(
    _In_ HWTextureManager *pTextureManager,
    _In_ CWindowRenderTarget *pRenderTarget,
    _In_ SurfaceCache *pSurfaceCache
    )
{
    HRESULT hr = S_OK;

    TraceImageEnsureAndUpdateHardwareResourcesBegin(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    // Since this CBitmapImage is being render walked it's not a candidate to be suspended.
    // Also make sure the animation is resumed if it has been suspended in the past.
    // Whether we had device lost or not, check and see if we have lost
    // hardware and software surfaces but can restore them from the encoded image
    if (!m_pImageSurfaceWrapper->CheckForHardwareResources() &&
        (GetSoftwareSurface() == NULL) &&
        IsMetadataAvailable() &&
        (m_bitmapState == BitmapImageState::Decoded))
    {
        SetBitmapState(BitmapImageState::HasEncodedImageOnly);
        IFC(RedecodeEncodedImage(TRUE));
    }

    // If we don't have hardware resources, but do have a software surface
    // we're in the decoded state
    if (!m_pImageSurfaceWrapper->CheckForHardwareResources() &&
        GetSoftwareSurface() != NULL)
    {
        ASSERT(IsMetadataAvailable()); // Can't get a software surface without an encoded image
        SetBitmapState(BitmapImageState::Decoded);
    }

    IFC(__super::EnsureAndUpdateHardwareResources(
            pTextureManager,
            pRenderTarget,
            pSurfaceCache));
Cleanup:

    TraceImageEnsureAndUpdateHardwareResourcesEnd(reinterpret_cast<XUINT64>(this), m_strSource.GetBuffer());

    RRETURN(hr);
}

_Check_return_ HRESULT
CSvgImageSource::PrepareDecode(bool retainPlaybackState)
{
    IFC_RETURN(SetImageCache(nullptr));
    SetBitmapState(BitmapImageState::Initial);

    // If we should not use decode to render size, disable it for this object
    // A case that we would want to do this is when the API specifies the
    // DecodePixelWidth or DecodePixelHeight.  If it is later modified, it should only
    // re-enable decode to render size on the next source set.
    m_fDecodeToRenderSize = ShouldDecodeToRenderSize();
    return S_OK;
}

// This function is for accessibility, it is called from ImageAutomationPeer::GetNameCore
_Check_return_ HRESULT CSvgImageSource::GetTitle(_Outptr_ HSTRING* output)
{
    return GetSvgDocumentString(L"title", output);
}

// This function is for accessibility, it is called from ImageAutomationPeer::GetFullDescriptionCore
_Check_return_ HRESULT CSvgImageSource::GetDescription(_Outptr_ HSTRING* output)
{
    return GetSvgDocumentString(L"desc", output);
}

