// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ManipulationPivot.g.h"

using namespace DirectUI;
using namespace DirectUISynonyms;

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a new ManipulationPivot from the specified Center and Radius.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT ManipulationPivotFactory::CreateInstanceWithCenterAndRadiusImpl(
    _In_ wf::Point center, 
    _In_ DOUBLE radius,
    _Outptr_ IManipulationPivot** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<ManipulationPivot> spInstance;

    IFC(ctl::make(&spInstance));
    IFC(spInstance->put_Center(center));
    IFC(spInstance->put_Radius(radius));

    *ppInstance = spInstance.Detach();

Cleanup:
    RRETURN(hr);
}


