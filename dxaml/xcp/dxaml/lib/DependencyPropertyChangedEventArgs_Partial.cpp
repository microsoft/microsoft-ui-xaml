// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DependencyPropertyChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

DependencyPropertyChangedEventArgs::DependencyPropertyChangedEventArgs()
{
    m_nPropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty;
}

KnownPropertyIndex 
DependencyPropertyChangedEventArgs::GetPropertyIndex()
{
    return m_nPropertyIndex;
}

_Check_return_ HRESULT
DependencyPropertyChangedEventArgs::Create(
    _In_ KnownPropertyIndex nPropertyIndex,
    _In_ IInspectable* pOldValue,
    _In_ IInspectable* pNewValue,
    _Outptr_ DependencyPropertyChangedEventArgs** ppArgs)
{
    HRESULT hr = S_OK;
    DependencyPropertyChangedEventArgs* pArgs = NULL;

    IFCPTR(ppArgs);

    IFC(ctl::ComObject<DependencyPropertyChangedEventArgs>::CreateInstance(&pArgs));
    pArgs->m_nPropertyIndex = nPropertyIndex;
    IFC(pArgs->put_OldValue(pOldValue));
    IFC(pArgs->put_NewValue(pNewValue));

    *ppArgs = pArgs;
    pArgs = NULL;

Cleanup:
    ctl::release_interface(pArgs);
    RRETURN(hr);
}


_Check_return_ HRESULT
DependencyPropertyChangedEventArgs::Initialize()
{
    HRESULT hr = S_OK;

    IFC(DependencyPropertyChangedEventArgsGenerated::Initialize());

    // Add this object to the list of those not in the peer table
    //DXamlCore::GetCurrent()->AddToReferenceTrackingList(this);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DependencyPropertyChangedEventArgs::get_PropertyImpl(_Outptr_ xaml::IDependencyProperty** pValue)
{
    HRESULT hr = S_OK;
    IFC(MetadataAPI::GetIDependencyProperty(m_nPropertyIndex, pValue));
Cleanup:
    RRETURN(hr);
}
