// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    DCompInteropCompositorPartnerCallback class which implements
//    IInteropCompositorPartnerCallback used for notifications of:
//    - WinRT visual dirtiness requiring a device commit.
//    - WinRT commit deferral start and end.

#include "precomp.h"
#include <DCompTreeHost.h>

DCompInteropCompositorPartnerCallback::DCompInteropCompositorPartnerCallback(
    _In_ DCompTreeHost *pDCompTreeHost)
    : m_pDCompTreeHostNoRef(pDCompTreeHost)
{
    XCP_WEAK(&m_pDCompTreeHostNoRef);
}

_Check_return_ HRESULT
DCompInteropCompositorPartnerCallback::Create(
    _In_ DCompTreeHost *pDCompTreeHost,
    _Outptr_ DCompInteropCompositorPartnerCallback **ppDCompInteropCompositorPartnerCallback)
{
    HRESULT hr = S_OK;
    xref_ptr<DCompInteropCompositorPartnerCallback> pDCompInteropCompositorPartnerCallback = nullptr;

    *ppDCompInteropCompositorPartnerCallback = nullptr;

    pDCompInteropCompositorPartnerCallback = make_xref<DCompInteropCompositorPartnerCallback>(pDCompTreeHost);

    *ppDCompInteropCompositorPartnerCallback = pDCompInteropCompositorPartnerCallback.detach();

    RRETURN(hr);//RRETURN_REMOVAL
}

// Called by the DComp Interop Compositor to mark the dirtiness of a visual.
// Only called on the UI thread.
IFACEMETHODIMP
DCompInteropCompositorPartnerCallback::NotifyDirty()
{
    // Note that DCompTreeHost does not commit the device immediately because we want to prevent tearing:
    // It is possible for the application to update the DComp Composition visuals at the same time as some UIElement properties,
    // in which case requesting another UI thread frame means both updates show up on screen at the same time.
    // Otherwise the DComp Composition updates show up immediately, and the UIElement updates need the next UI thread tick/render.

    if (m_pDCompTreeHostNoRef != nullptr)
    {
        return m_pDCompTreeHostNoRef->RequestMainDCompDeviceCommit();
    }
    return S_OK;
}

// Called by the DComp Interop Compositor to mark the beginning or end of a commit deferral.
// Can be called off the UI thread.
IFACEMETHODIMP DCompInteropCompositorPartnerCallback::NotifyDeferralState(_In_ bool deferRequested)
{
    // Ignored - Xaml no longer supports commit deferral via the interop compositor.
    return S_OK;
}
