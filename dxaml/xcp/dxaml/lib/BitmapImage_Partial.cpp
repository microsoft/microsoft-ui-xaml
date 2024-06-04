// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BitmapImage.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new BitmapImage from the specified URI.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT BitmapImageFactory::CreateInstanceWithUriSourceImpl(
    _In_ wf::IUriRuntimeClass* pUri,
    _Outptr_ IBitmapImage** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<BitmapImage> spInstance;

    IFCPTR(ppInstance);

    IFC(ctl::make(&spInstance));
    IFC(spInstance->put_UriSource(pUri));

    *ppInstance = spInstance.Detach();

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT BitmapImage::PlayImpl()
{
    IFC_RETURN(static_cast<CBitmapImage*>(GetHandle())->PlayAnimation());
    return S_OK;
}

_Check_return_ HRESULT BitmapImage::StopImpl()
{
    IFC_RETURN(static_cast<CBitmapImage*>(GetHandle())->StopAnimation());
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the source of this BitmapImage from a stream.
//
//------------------------------------------------------------------------

IFACEMETHODIMP
BitmapImage::SetSource(_In_ wsts::IRandomAccessStream* pStreamSource)
{
    HRESULT hr = S_OK;

    IFCPTR(pStreamSource);
    IFC(CheckThread());

    IFC(this->put_UriSource(NULL));
    IFC(BitmapSource::SetSource(pStreamSource));

Cleanup:
    RRETURN(hr);
}