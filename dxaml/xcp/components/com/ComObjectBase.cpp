// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ComObjectBase.h"

IFACEMETHODIMP ctl::ComObjectBase::NonDelegatingQueryInterfaceBase(REFIID iid, void **ppValue)
{
    IUnknown* pInterface = nullptr;

    if (InlineIsEqualGUID(iid, IID_IUnknown))
    {
        pInterface = reinterpret_cast<IUnknown *>(static_cast<INonDelegatingUnknown *>(this));
    }
    else if (InlineIsEqualGUID(iid, IID_IInspectable))
    {
        pInterface = reinterpret_cast<IInspectable *>(static_cast<INonDelegatingInspectable *>(this));
    }
    else
    {
        return QueryInterfaceImplBase(iid, ppValue);
    }

    *ppValue = pInterface;
    pInterface->AddRef();
    return S_OK;
}

IFACEMETHODIMP ctl::ComObjectBase::QueryInterfaceBase(REFIID iid, void **ppValue)
{
    if (m_pControllingUnknown)
    {
        return m_pControllingUnknown->QueryInterface(iid, ppValue);
    }
    else
    {
        return QueryInterfaceImplBase(iid, ppValue);
    }
}


HRESULT ctl::ComObjectBase::CreateInstanceBase(_In_ ComBase* pNewInstance, bool fNoInit)
{
    if (!fNoInit)
    {
        IFC_RETURN(pNewInstance->Initialize());
    }

    return S_OK;
}
