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
#include <FrameworkUdk/Containment.h>

// Telemetry: Image Decoding Activity is skipped in WinAppSDK 1.5.5+ Servicing releases to avoid crashing
// Bug 44612834: [1.5 servicing] [Watson Failure] caused by FAIL_FAST_FATAL_APP_EXIT_c0000409_Microsoft.UI.Xaml.dll!ImagingTelemetry::ImageDecodeActivity::Split
#define WINAPPSDK_CHANGEID_44612834 44612834

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

std::shared_ptr<ImagingTelemetry::ImageDecodeActivity>& ImageProvider::GetDecodeActivity()
{
    if (!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_44612834>())
    {
        EnsureDecodeActivity();
    }

    return m_decodeActivity;
}

void ImageProvider::EnsureDecodeActivity()
{
    // Telemetry: Image Decoding Activity is skipped in WinAppSDK 1.5.5+ Servicing releases to avoid crashing bug 44612834.
    ASSERT(!WinAppSdk::Containment::IsChangeEnabled<WINAPPSDK_CHANGEID_44612834>());

    if (!m_decodeActivity)
    {
        m_decodeActivity = std::make_shared<ImagingTelemetry::ImageDecodeActivity>(ImagingTelemetry::ImageDecodeActivity::Start());
    }
}