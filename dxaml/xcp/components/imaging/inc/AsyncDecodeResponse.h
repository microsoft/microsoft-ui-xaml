// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ImageProviderInterfaces.h"
#include "SurfaceDecodeParams.h"

class OfferableSoftwareBitmap;

class AsyncDecodeResponse : public CXcpObjectBase< IImageAvailableResponse >
{
public:
    AsyncDecodeResponse(const AsyncDecodeResponse&) = delete;

    // Used for returning errors and aborted operations
    explicit AsyncDecodeResponse(HRESULT result);

    // Used for image copy operations which don't have encoded image data or decode parameters.
    AsyncDecodeResponse(
        HRESULT result,
        _In_ xref_ptr<OfferableSoftwareBitmap> softwareBitmap
        );

    // Used for passing back a full decode with all the requisite information
    AsyncDecodeResponse(
        HRESULT result,
        _In_ xref_ptr<OfferableSoftwareBitmap> softwareBitmap,
        SurfaceUpdateList surfaceUpdateListToReleaseOnUIThread
        );

    ~AsyncDecodeResponse() override;

    _Check_return_ HRESULT GetDecodingResult() const override { return m_result; }

    const xref_ptr<OfferableSoftwareBitmap>& GetSurface() const override { return m_softwareBitmap; }

protected:
    HRESULT m_result = S_OK;
    xref_ptr<OfferableSoftwareBitmap> m_softwareBitmap;
    // TODO: make it safe to release surfaces from the background thread
    SurfaceUpdateList m_surfaceUpdateListToReleaseOnUIThread;
};
