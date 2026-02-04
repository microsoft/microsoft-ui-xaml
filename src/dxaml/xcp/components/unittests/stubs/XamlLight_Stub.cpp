// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "Indexes.g.h"
#include "DependencyObject.h"
#include "XamlLight.g.h"


_Check_return_ HRESULT DirectUI::XamlLight::GetIdProtected(_Out_ HSTRING* pReturnValue)
{
    *pReturnValue = nullptr;
    return S_OK;
}

_Check_return_ HRESULT DirectUI::XamlLight::OnConnectedProtected(_In_ xaml::IUIElement* pNewElement)
{
    return S_OK;
}

_Check_return_ HRESULT DirectUI::XamlLight::OnDisconnectedProtected(_In_ xaml::IUIElement* pOldElement)
{
    return S_OK;
}

