// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PropertyPath.g.h"

using namespace DirectUI;
using namespace xaml;

_Check_return_ 
HRESULT 
DirectUI::PropertyPath::CreateInstance(_In_ HSTRING hPath, _Outptr_ DirectUI::PropertyPath **ppPropertyPath)
{
    HRESULT hr = S_OK;
    ctl::ComPtr<DirectUI::PropertyPath> spPropertyPath;
    
    IFC(ctl::make(&spPropertyPath));
    IFC(spPropertyPath->SetValueByKnownIndex(KnownPropertyIndex::PropertyPath_Path, hPath));
    *ppPropertyPath = spPropertyPath.Detach();

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT PropertyPathFactory::CreateInstanceImpl(_In_ HSTRING path, _Outptr_ xaml::IPropertyPath** ppInstance)
{
    HRESULT hr = S_OK;
    DirectUI::PropertyPath *pNewInstance = NULL;

    IFC(DirectUI::PropertyPath::CreateInstance(path, &pNewInstance));
    *ppInstance = pNewInstance;
    pNewInstance = NULL;
   
Cleanup:
    ctl::release_interface(pNewInstance);
    RRETURN(hr);
}

