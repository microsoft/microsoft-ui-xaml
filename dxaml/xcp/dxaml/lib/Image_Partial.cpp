// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Image.g.h"
#include "ImageAutomationPeer.g.h"
#include "Image.h"

using namespace DirectUI;


Image::Image()
{
    m_pCastingSource = nullptr;
}

Image::~Image()
{
    ReleaseInterface(m_pCastingSource);
}

// Create ImageAutomationPeer to represent the Image.
IFACEMETHODIMP Image::OnCreateAutomationPeer(_Outptr_ xaml_automation_peers::IAutomationPeer** ppAutomationPeer)
{
    HRESULT hr = S_OK;
    xaml_automation_peers::IImageAutomationPeer* pImageAutomationPeer = NULL;
    xaml_automation_peers::IImageAutomationPeerFactory* pImageAPFactory = NULL;
    IActivationFactory* pActivationFactory = NULL;
    IInspectable* inner = NULL;

    pActivationFactory = ctl::ActivationFactoryCreator<DirectUI::ImageAutomationPeerFactory>::CreateActivationFactory();
    IFC(ctl::do_query_interface(pImageAPFactory, pActivationFactory));

    IFC(static_cast<ImageAutomationPeerFactory*>(pImageAPFactory)->CreateInstanceWithOwner(this,
        NULL,
        &inner,
        &pImageAutomationPeer));
    IFC(ctl::do_query_interface(*ppAutomationPeer, pImageAutomationPeer));

Cleanup:
    ReleaseInterface(pImageAutomationPeer);
    ReleaseInterface(pImageAPFactory);
    ReleaseInterface(pActivationFactory);
    ReleaseInterface(inner);
    RRETURN(hr);
}

// Returns the CastingSource instance to the App to be used in the Casting APIs
_Check_return_ HRESULT Image::GetAsCastingSourceImpl(_Outptr_ wm::Casting::ICastingSource** ppReturnValue)
{
    HRESULT hr = S_OK;
    IFCPTR(ppReturnValue);

    if (m_pCastingSource == nullptr)
    {
        CImage *pImage = static_cast<CImage*>(GetHandle());
        IFC(pImage->GetCastingSource(&m_pCastingSource));
    }

    IFC(m_pCastingSource->QueryInterface(IID_PPV_ARGS(ppReturnValue)));

Cleanup:
    return hr;
}

_Check_return_ HRESULT Image::GetAlphaMaskImpl(
    _Outptr_ WUComp::ICompositionBrush** ppResult)
{
    CImage* pImage = static_cast<CImage*>(GetHandle());
    IFC_RETURN(pImage->GetAlphaMask(ppResult));
    return S_OK;
}
