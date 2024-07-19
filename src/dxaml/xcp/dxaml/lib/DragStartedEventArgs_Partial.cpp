// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides data for the DragStarted event that occurs when a user drags a
//      Thumb control with the mouse.

#include "precomp.h"
#include "DragStartedEventArgs.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT DragStartedEventArgsFactory::CreateInstanceWithHorizontalOffsetAndVerticalOffsetImpl(
    _In_ DOUBLE dHorizontalOffset,
    _In_ DOUBLE dVerticalOffset,
    _In_opt_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ IDragStartedEventArgs** ppInstance)
{
    HRESULT hr = S_OK;
    IInspectable* pInner = NULL;
    IDragStartedEventArgs* pArgsAsI = NULL;
    DragStartedEventArgs* pArgs = NULL;

    IFCPTR(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(__super::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pArgsAsI, pInner));

    pArgs = static_cast<DragStartedEventArgs*>(pArgsAsI);
    IFC(pArgs->put_HorizontalOffset(dHorizontalOffset));
    IFC(pArgs->put_VerticalOffset(dVerticalOffset));

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
