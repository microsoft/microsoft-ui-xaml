// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "NotifyCollectionChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT NotifyCollectionChangedEventArgsFactory::CreateInstanceWithAllParametersImpl(
    _In_ xaml_interop::NotifyCollectionChangedAction action, 
    _In_ xaml_interop::IBindableVector* newItems, 
    _In_ xaml_interop::IBindableVector* oldItems, 
    _In_ INT newIndex, 
    _In_ INT oldIndex, 
    _In_opt_ IInspectable* pOuter, 
    _Outptr_ IInspectable** ppInner, 
    _Outptr_ xaml_interop::INotifyCollectionChangedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable *pInner = nullptr;
    INotifyCollectionChangedEventArgs *pArgsAsI = nullptr;
    NotifyCollectionChangedEventArgs *pArgs = nullptr;

    IFCPTR(ppInstance);
    IFCEXPECTRC(pOuter == nullptr || ppInner != nullptr, E_POINTER);

    IFC(ctl::AggregableActivationFactory<NotifyCollectionChangedEventArgs>::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<NotifyCollectionChangedEventArgs *>(pArgsAsI);
    IFC(pArgs->put_Action(action));

    IFC(pArgs->put_NewStartingIndex(newIndex));
    IFC(pArgs->put_OldStartingIndex(oldIndex));

    IFC(pArgs->put_NewItems(newItems));
    IFC(pArgs->put_OldItems(oldItems));
    
    if (ppInner)
    {
        *ppInner = pInner;
        pInner = nullptr;
    }

    *ppInstance = pArgsAsI;
    pArgsAsI = nullptr;

Cleanup:
    
    ReleaseInterface(pArgsAsI);
    ReleaseInterface(pInner);

    RRETURN(hr);
}


