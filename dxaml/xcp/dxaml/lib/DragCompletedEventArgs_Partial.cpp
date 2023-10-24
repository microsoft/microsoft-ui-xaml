// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the DragCompleted event that occurs when a user
//      completes a drag operation with the mouse of a Thumb control

#include "precomp.h"
#include "DragCompletedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DragCompletedEventArgsFactory::CreateInstanceWithHorizontalChangeVerticalChangeAndCanceledImpl(
    _In_ DOUBLE dHorizontalChange,
    _In_ DOUBLE dVerticalChange,
    _In_ BOOLEAN bCanceled,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IDragCompletedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IDragCompletedEventArgs* pArgsAsI = NULL;
    DragCompletedEventArgs* pArgs = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(__super::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<DragCompletedEventArgs*>(pArgsAsI);
    IFC(pArgs->put_HorizontalChange(dHorizontalChange));
    IFC(pArgs->put_VerticalChange(dVerticalChange));
    IFC(pArgs->put_Canceled(bCanceled));

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
