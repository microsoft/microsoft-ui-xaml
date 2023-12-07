// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlMarkupHelper.g.h"

using namespace DirectUI;

_Check_return_ HRESULT 
XamlMarkupHelperFactory::UnloadObjectImpl(_In_ xaml::IDependencyObject* element)
{
    IFC_RETURN(CDeferredElement::TryDefer(
        static_cast<DependencyObject*>(element)->GetHandle()));

    return S_OK;
}