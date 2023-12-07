// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Transition.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

_Check_return_ HRESULT TransitionFactory::CreateInstanceImpl(
    _In_ IInspectable* pOuter,
    _Outptr_ IInspectable** ppInner,
    _Outptr_ xaml_animation::ITransition** ppInstance)
{
    HRESULT hr = S_OK;
    xaml_animation::ITransition* pInstance = NULL;
    IInspectable* pInner = NULL;

    ARG_VALIDRETURNPOINTER(ppInstance);
    IFCEXPECT(pOuter == NULL || ppInner != NULL);

    IFC(ctl::BetterAggregableCoreObjectActivationFactory::ActivateInstance(pOuter, &pInner));
    IFC(ctl::do_query_interface(pInstance, pInner));

    if (ppInner)
    {
        *ppInner = pInner;
        pInner = NULL;
    }

    *ppInstance = pInstance;
    pInstance = NULL;

Cleanup:
    ReleaseInterface(pInstance);
    ReleaseInterface(pInner);
    RRETURN(hr);
}
