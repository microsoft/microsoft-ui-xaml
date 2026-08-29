// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SurfaceDecodeParams.h"
#include "ImageCopyParams.h"
#include "SoftwareBitmapSource.h"
#include "SoftwareBitmapSource.g.h"
#include "ErrorHelper.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

ULONG SoftwareBitmapSource::z_ulUniqueAsyncActionId = 1;

IFACEMETHODIMP
SoftwareBitmapSource::Close()
{
    IFC_RETURN(CheckThread());
    CSoftwareBitmapSource* pSoftwareBitmapSource = reinterpret_cast<CSoftwareBitmapSource*>(GetHandle());
    IFC_RETURN(pSoftwareBitmapSource->Close());
    return S_OK;
}

IFACEMETHODIMP
SoftwareBitmapSource::SetBitmapAsyncImpl(
    _In_ wgri::ISoftwareBitmap* pSoftwareBitmap,
    _Outptr_ wf::IAsyncAction** ppReturnValue
    )
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::IDispatcher> spDispatcher;
    CSoftwareBitmapSource* pSoftwareBitmapSource = reinterpret_cast<CSoftwareBitmapSource*>(GetHandle());
    Microsoft::WRL::ComPtr<SoftwareBitmapSourceSetSourceAsyncAction> spSetSourceAsyncAction;
    wrl::ComPtr<wgri::ISoftwareBitmap> spSoftwareBitmap(pSoftwareBitmap);
    ULONG actionId = 0;

    // null value for spSoftwareBitmap is allowed, it will cause the core to release its current software bitmap reference.
    if (spSoftwareBitmap != nullptr)
    {
        // Do input data validation on the software bitmap
        // - Non-zero width/height
        // - Valid pixel format
        // - Only alpha ignore and pre-multiplied alpha are supported
        INT32 width = 0;
        INT32 height = 0;
        wgri::BitmapPixelFormat bitmapPixelFormat;
        wgri::BitmapAlphaMode bitmapAlphaMode;

        IFC(spSoftwareBitmap->get_PixelWidth(&width));
        IFC(spSoftwareBitmap->get_PixelHeight(&height));
        IFC(spSoftwareBitmap->get_BitmapPixelFormat(&bitmapPixelFormat));
        IFC(spSoftwareBitmap->get_BitmapAlphaMode(&bitmapAlphaMode));

        if ((width <= 0) ||
            (height <= 0) ||
            (bitmapPixelFormat != wgri::BitmapPixelFormat_Bgra8) ||
            (bitmapAlphaMode == wgri::BitmapAlphaMode_Straight))
        {
            IFC(ErrorHelper::OriginateErrorUsingResourceID(E_INVALIDARG, ERROR_SOFTWAREBITMAPSOURCE_INVALID_FORMAT));
        }
    }

    // If there are any current pending async actions, they should be cancelled first.
    if (m_spSetSourceAsyncAction != nullptr)
    {
        m_spSetSourceAsyncAction->Cancel();
        m_spSetSourceAsyncAction->CoreFireCompletion();
        m_spSetSourceAsyncAction = nullptr;
    }

    // Prepare a new async action
    IFC(GetXamlDispatcher(&spDispatcher));
    actionId = ::InterlockedIncrement(&SoftwareBitmapSource::z_ulUniqueAsyncActionId);
    IFC(Microsoft::WRL::MakeAndInitialize<SoftwareBitmapSourceSetSourceAsyncAction>(&m_spSetSourceAsyncAction, actionId, spDispatcher.Get()));

    IFC(m_spSetSourceAsyncAction->StartOperation());

    // Call into core to set the source to the new image.
    // CopyTo adds a ref to the action, which will be kept in the Core

    spSetSourceAsyncAction = m_spSetSourceAsyncAction;
    IFC(pSoftwareBitmapSource->SetBitmap(spSoftwareBitmap, m_spSetSourceAsyncAction.Get()));
    spSetSourceAsyncAction.Detach();  // If the call succeeded we handle off this reference to the Core

    IFC(m_spSetSourceAsyncAction.CopyTo(ppReturnValue));

Cleanup:
    // If we have an Async action and there was an error we need to FireCompletion now
    if ((spSetSourceAsyncAction != nullptr) &&
        FAILED(hr))
    {
        spSetSourceAsyncAction->CoreSetError(hr);  //CoreSetError does nothing if hr==S_OK
        spSetSourceAsyncAction->CoreFireCompletion();
        spSetSourceAsyncAction = nullptr;
    }

    return hr;
}
