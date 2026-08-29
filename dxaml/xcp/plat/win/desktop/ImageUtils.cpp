// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace ImageUtils
{

//+----------------------------------------------------------------------------
//
//  Synopsis:   Saves a given WIC bitmap to the given filename as a PNG.
//
//-----------------------------------------------------------------------------
HRESULT SaveWICBitmapAsPNG(_In_ IWICBitmapSource *pWICBitmapSource, _In_ LPCWSTR filename)
{
    HRESULT hr = S_OK;
    IWICStream *pWICStream = NULL;
    IWICImagingFactory *pWICFactory = NULL;
    IWICBitmapEncoder *pWICBitmapEncoder = NULL;
    IWICBitmapFrameEncode *pWICBitmapFrameEncode = NULL;
    WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;

    IFC(CoCreateInstance(
        CLSID_WICImagingFactory,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&pWICFactory)
        ));

    IFC(pWICFactory->CreateStream(&pWICStream));
    IFC(pWICStream->InitializeFromFilename(filename, GENERIC_WRITE));

    IFC(pWICFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &pWICBitmapEncoder));
    IFC(pWICBitmapEncoder->Initialize(pWICStream, WICBitmapEncoderNoCache));

    IFC(pWICBitmapEncoder->CreateNewFrame(&pWICBitmapFrameEncode, NULL));
    IFC(pWICBitmapFrameEncode->Initialize(NULL));

    UINT width;
    UINT height;
    IFC(pWICBitmapSource->GetSize(&width, &height));
    IFC(pWICBitmapFrameEncode->SetSize(width, height));

    IFC(pWICBitmapFrameEncode->SetPixelFormat(&format));
    IFC(pWICBitmapFrameEncode->WriteSource(pWICBitmapSource, NULL));
    IFC(pWICBitmapFrameEncode->Commit());
    IFC(pWICBitmapEncoder->Commit());

Cleanup:
    ReleaseInterfaceNoNULL(pWICBitmapFrameEncode);
    ReleaseInterfaceNoNULL(pWICBitmapEncoder);
    ReleaseInterfaceNoNULL(pWICFactory);
    ReleaseInterfaceNoNULL(pWICStream);

    RRETURN(hr);
}

} // namespace ImageUtils

