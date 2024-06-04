// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ImageProviderDecodeHandlerTask.h"
#include <ImageTaskDispatcher.h>
#include <ImageDecodeRequest.h>


ImageProviderDecodeHandlerTask::ImageProviderDecodeHandlerTask(
    _In_ ImageTaskDispatcher* pDispatcher,
    _In_ xref_ptr<ImageDecodeRequest> spImageDecodeRequest
    )
    : m_pDispatcher(pDispatcher)
    , m_spImageDecodeRequest(std::move(spImageDecodeRequest))
{
    XCP_WEAK(&m_pDispatcher);
}

ImageProviderDecodeHandlerTask::~ImageProviderDecodeHandlerTask()
{
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handler for image decoding completing.
//
//  NOTE:
//      This runs on a background thread.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
ImageProviderDecodeHandlerTask::OnDecode(
    _In_ xref_ptr<IImageAvailableResponse> spResponse,
    _In_ uint64_t requestId
    )
{
    m_spDecodeResponse = std::move(spResponse);

    IFC_RETURN(m_pDispatcher->QueueTask(this));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Handle and dispatch decode response to callback.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
ImageProviderDecodeHandlerTask::Execute()
{
    if (m_spImageDecodeRequest != nullptr)
    {
        IFC_RETURN(m_spImageDecodeRequest->NotifyCallback(m_spDecodeResponse));
        m_spImageDecodeRequest.reset();
    }
    m_spDecodeResponse.reset();

    return S_OK;
}

