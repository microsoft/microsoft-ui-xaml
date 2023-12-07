// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ScalarTransition.g.h"

using namespace DirectUI;

HRESULT ScalarTransition::QueryInterfaceImpl(_In_ REFIID riid, _Out_ void** ppObject)
{
    if (InlineIsEqualGUID(riid, __uuidof(xaml::IDependencyObject)))
    {
        // This type does not publicly derive from DO. Block all QIs to DO.
        return E_NOINTERFACE;
    }
    else
    {
        return __super::QueryInterfaceImpl(riid, ppObject);
    }
}
