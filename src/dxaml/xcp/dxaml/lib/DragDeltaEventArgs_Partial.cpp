// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the DragDelta event that occurs one or more times when
//      a user drags a Thumb control with the mouse.

#include "precomp.h"
#include "DragDeltaEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DragDeltaEventArgsFactory::CreateInstanceWithHorizontalChangeAndVerticalChangeImpl(
    _In_ DOUBLE dHorizontalChange,
    _In_ DOUBLE dVerticalChange,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IDragDeltaEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IDragDeltaEventArgs* pArgsAsI = NULL;
    DragDeltaEventArgs* pArgs = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(__super::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<DragDeltaEventArgs*>(pArgsAsI);
    IFC(pArgs->put_HorizontalChange(dHorizontalChange));
    IFC(pArgs->put_VerticalChange(dVerticalChange));

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
