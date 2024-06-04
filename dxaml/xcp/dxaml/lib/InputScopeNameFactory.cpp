// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "InputScopeName.g.h"

using namespace DirectUI;

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates a new InputScopeName.
//  
//---------------------------------------------------------------------------
_Check_return_ HRESULT InputScopeNameFactory::CreateInstanceImpl(
    _In_ xaml_input::InputScopeNameValue nameValue,
    _Outptr_ xaml_input::IInputScopeName** ppInstance)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<InputScopeName> spInstance;

    IFC(ctl::make(&spInstance));
    IFC(spInstance->put_NameValue(nameValue));

    *ppInstance = spInstance.Detach();

Cleanup:
    RRETURN(hr);
}
