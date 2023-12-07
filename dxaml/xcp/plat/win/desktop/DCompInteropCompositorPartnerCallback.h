// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//    DCompInteropCompositorPartnerCallback class which implements
//    IInteropCompositorPartnerCallback used for notifications of:
//    - WinRT visual dirtiness requiring a device commit.
//    - WinRT commit deferral start and end.

#pragma once

#include <ComTemplates.h>

#include <DCompExtras.h>
#include <dcompinternal.h>

class DCompTreeHost;

class DCompInteropCompositorPartnerCallback final : public ctl::implements<WUComp::IInteropCompositorPartnerCallback>
{
public:
    static _Check_return_ HRESULT Create(
        _In_ DCompTreeHost *pDCompTreeHost,
        _Outptr_ DCompInteropCompositorPartnerCallback **ppDCompInteropCompositorPartnerCallback);

    DCompInteropCompositorPartnerCallback(
        _In_ DCompTreeHost *pDCompTreeHost);

    void Disconnect()
    {
        m_pDCompTreeHostNoRef = nullptr;
    }

    IFACEMETHOD(NotifyDirty)();
    IFACEMETHOD(NotifyDeferralState)(_In_ bool deferRequested);

private:
    _Notnull_ DCompTreeHost *m_pDCompTreeHostNoRef;
};
