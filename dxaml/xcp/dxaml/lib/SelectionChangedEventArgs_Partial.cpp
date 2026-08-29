// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the SelectionChanged event that occurs when a user
//      changes selected item(s) on a Selector based control

#include "precomp.h"
#include "SelectionChangedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT SelectionChangedEventArgsFactory::CreateInstanceWithRemovedItemsAndAddedItemsImpl(
    _In_ wfc::IVector<IInspectable*>* pRemovedItems,
    _In_ wfc::IVector<IInspectable*>* pAddedItems,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ ISelectionChangedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    ISelectionChangedEventArgs* pArgsAsI = NULL;
    SelectionChangedEventArgs* pArgs = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(__super::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<SelectionChangedEventArgs*>(pArgsAsI);
    IFC(pArgs->put_RemovedItems(pRemovedItems));
    IFC(pArgs->put_AddedItems(pAddedItems));

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

