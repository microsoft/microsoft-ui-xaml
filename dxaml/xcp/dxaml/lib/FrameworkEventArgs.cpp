// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FrameworkEventArgs.h"

using namespace DirectUI;

// Constructors/destructors.
DirectUI::EventArgs::EventArgs() : m_pUnderlyingObject(nullptr)
{
}

DirectUI::EventArgs::~EventArgs()
{
}

HRESULT DirectUI::EventArgs::QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject)
{
    if (InlineIsEqualGUID(iid, __uuidof(DirectUI::EventArgs)))
    {
        *ppObject = static_cast<DirectUI::EventArgs*>(this);
    }
    else
    {
        RRETURN(__super::QueryInterfaceImpl(iid, ppObject));
    }

    AddRefOuter();
    RRETURN(S_OK);
}

_Check_return_ HRESULT DirectUI::EventArgs::GetCorePeerNoRefWithValidation(_Outptr_ CEventArgs** ppPeer)
{
    HRESULT hr = S_OK;

    IFC(CheckThread());

    *ppPeer = GetCorePeer();
    (*ppPeer)->Release(); // The caller does not expect a ref

Cleanup:
    return hr;
}

CEventArgs* DirectUI::EventArgs::GetCorePeer()
{
    if (!m_pUnderlyingObject)
    {
        m_pUnderlyingObject.attach(CreateCorePeer());
    }

    CEventArgs* pPeer = nullptr;
    m_pUnderlyingObject.CopyTo(&pPeer);

    return pPeer;
}

CEventArgs* DirectUI::EventArgs::CreateCorePeer()
{
    return new CEventArgs();
}

_Check_return_ HRESULT DirectUI::EventArgs::EndShutdown()
{
    m_pUnderlyingObject.reset();
    return S_OK;
}

namespace DirectUI
{
    _Check_return_ HRESULT OnFrameworkCreateEventArgs(_In_ CEventArgs* pCoreObject, _Out_ IInspectable** ppNewInstance)
    {
        HRESULT hr = S_OK;
        ctl::ComPtr<DirectUI::EventArgs> spInstance;
        *ppNewInstance = nullptr;
        IFC(ctl::make(pCoreObject, &spInstance));
        *ppNewInstance = ctl::iinspectable_cast(spInstance.Detach());
    Cleanup:
        RRETURN(hr);
    }
}
