// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <AsyncDecodeResponse.h>
#include <OfferableSoftwareBitmap.h>

AsyncDecodeResponse::AsyncDecodeResponse(HRESULT result)
    : m_result(result)
{
}

AsyncDecodeResponse::AsyncDecodeResponse(
    HRESULT result,
    _In_ xref_ptr<OfferableSoftwareBitmap> softwareBitmap
    )
    : m_result(result)
    , m_softwareBitmap(std::move(softwareBitmap))
{
}

// Used for passing back a full decode with all the requisite information
AsyncDecodeResponse::AsyncDecodeResponse(
    HRESULT result,
    _In_ xref_ptr<OfferableSoftwareBitmap> softwareBitmap,
    SurfaceUpdateList surfaceUpdateListToReleaseOnUIThread
    )
    : m_result(result)
    , m_softwareBitmap(std::move(softwareBitmap))
    , m_surfaceUpdateListToReleaseOnUIThread(std::move(surfaceUpdateListToReleaseOnUIThread))
{
}

AsyncDecodeResponse::~AsyncDecodeResponse()
{
}
