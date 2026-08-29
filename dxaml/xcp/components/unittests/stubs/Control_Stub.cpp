// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <CControl.h>
#include <Template.h>

_Check_return_ HRESULT CControl::GetValueFromBuiltInStyle(
        _In_ const CDependencyProperty* dp,
        _Out_ CValue* pValue,
        _Out_ bool* gotValue)
{
    *gotValue = false;
    *pValue = CValue();
    return S_OK;
}


xref_ptr<CControlTemplate> CControl::GetTemplate() const
{
    return nullptr;
}