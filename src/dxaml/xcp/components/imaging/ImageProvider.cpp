// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <wincodec.h>
#include "ImageProviderInterfaces.h"
#include "palgfx.h"
#include "corep.h"
#include "ImageProviderInterfaces.h"
#include "SurfaceDecodeParams.h"
#include "OfferableSoftwareBitmap.h"
#include "ImageCopyParams.h"
#include "ImageProviderDecodeHandlerTask.h"
#include "ImageProvider.h"
#include "ImageDecodeRequest.h"
#include "ImageDecodeParams.h"

_Check_return_ HRESULT
ImageProvider::CopyImage(
    _In_ xref_ptr<ImageCopyParams>& spCopyParams,
    _In_ const xref_ptr<IImageAvailableCallback>& spCallback,
    _In_ CCoreServices* pCore,
    _Outptr_ xref_ptr<IAbortableImageOperation>& spAbortableImageOperation
    )
{
    auto spDecodeRequest = make_xref<ImageDecodeRequest>(pCore);
    IFC_RETURN(spDecodeRequest->SetDecodeParams(spCallback, nullptr /* decodeParams */));

    auto spDecodeCallback = make_xref<ImageProviderDecodeHandlerTask>(m_pDispatcher, spDecodeRequest);

    IFC_RETURN(m_pImageFactory->CopyAsync(spCopyParams, spDecodeCallback, nullptr));

    spAbortableImageOperation = spDecodeRequest;

    return S_OK;
}