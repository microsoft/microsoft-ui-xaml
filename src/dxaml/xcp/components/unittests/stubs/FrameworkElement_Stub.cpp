// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "theming\inc\Theme.h"
#include <Framework.h>

_Check_return_ HRESULT CFrameworkElement::GetValueFromStyle(
        _In_ const CDependencyProperty* dp,
        _Out_ CValue* pValue,
        _Out_ bool* gotValue)
{
    *gotValue = false;
    *pValue = CValue();
    return S_OK;
}

Theming::Theme CFrameworkElement::GetRequestedThemeOverride(
        _In_ Theming::Theme theme)
{
    return theme;
}

void CFrameworkElement::UpdateRequiresCompNodeForRoundedCorners()
{
}
