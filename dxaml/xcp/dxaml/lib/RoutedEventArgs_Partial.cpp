// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RoutedEventArgs.g.h"
#include "RoutedEventArgs.h"

CEventArgs* DirectUI::RoutedEventArgs::CreateCorePeer()
{
    return new CRoutedEventArgs();
}

_Check_return_ HRESULT 
DirectUI::RoutedEventArgs::IsHandled(_Out_ bool* pHandled)
{
    xref_ptr<CEventArgs> corePeer;
    corePeer.attach(GetCorePeer());
    IFCEXPECT_ASSERT_RETURN(corePeer);
    IFCEXPECT_ASSERT_RETURN(pHandled);
    *pHandled = static_sp_cast<CRoutedEventArgs>(corePeer)->m_bHandled;
    return S_OK;
}

_Check_return_ HRESULT 
DirectUI::RoutedEventArgs::SetHandled(_In_ bool bHandled)
{
    xref_ptr<CEventArgs> corePeer;
    corePeer.attach(GetCorePeer());
    IFCEXPECT_ASSERT_RETURN(corePeer);
    static_sp_cast<CRoutedEventArgs>(corePeer)->m_bHandled = bHandled;
    return S_OK;
}

_Check_return_ HRESULT
DirectUI::RoutedEventArgs::get_OriginalSourceImpl(_Outptr_ IInspectable** ppValue)
{
    HRESULT hr = S_OK;
    CDependencyObject* pSource = NULL;
    ctl::ComPtr<DependencyObject> spDO;

    *ppValue = nullptr;

    CEventArgs* const pCorePeer = GetCorePeer();
    IFC(static_cast<CRoutedEventArgs*>(pCorePeer)->get_Source(&pSource));
    if (pSource != nullptr)
    {
        IFC(DXamlCore::GetCurrent()->GetPeer(static_cast<CDependencyObject*>(pSource), &spDO));
        *ppValue = ctl::as_iinspectable(spDO.Detach());
    }

Cleanup:
    ReleaseInterfaceNoNULL(pCorePeer);
    ReleaseInterface(pSource);
    RRETURN(hr);
}

_Check_return_ HRESULT
DirectUI::RoutedEventArgs::put_OriginalSourceImpl(_In_ IInspectable* pValue)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DependencyObject> spDO;

    CEventArgs* const pCorePeer = GetCorePeer();
    spDO.Attach(static_cast<DependencyObject*>(ctl::query_interface<xaml::IDependencyObject>(pValue)));
    if (spDO != nullptr)
    {
        IFC(static_cast<CRoutedEventArgs*>(pCorePeer)->put_Source(spDO->GetHandle()));
    }
    else
    {
        IFC(static_cast<CRoutedEventArgs*>(pCorePeer)->put_Source(nullptr));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pCorePeer);
    RRETURN(hr);
}
