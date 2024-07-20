// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT
PropertyChangedEventArgsFactory::CreateInstanceImpl(
    _In_ HSTRING name, 
    _In_opt_ IInspectable* pOuter, 
    _Outptr_result_maybenull_ IInspectable** ppInner, 
    _Outptr_ xaml_data::IPropertyChangedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IPropertyChangedEventArgs* pArgsAsI = NULL;
    PropertyChangedEventArgs* pArgs = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(ctl::AggregableActivationFactory<PropertyChangedEventArgs>::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<PropertyChangedEventArgs*>(pArgsAsI);
    IFC(pArgs->put_PropertyName(name));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pArgsAsI;
    pArgsAsI = NULL;

Cleanup:
    ReleaseInterface(pInner);
    ReleaseInterface(pArgsAsI);
    RRETURN(hr);
}
