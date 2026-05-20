// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <InheritedProperties.h>
#include <TypeTableStructs.h>


_Check_return_ bool InheritedProperties::IsPropertyFlagSet(
    _In_ const CDependencyProperty* pDP,
    InheritedPropertyFlag flag
    )
{
    return false;
}

void InheritedProperties::SetPropertyFlag(_In_ const CDependencyProperty* pDP, InheritedPropertyFlag flag, bool fState)
{
}

_Check_return_ HRESULT InheritedProperties::GetDefaultValue(_In_ const KnownPropertyIndex index, _Out_ CValue* defaultValue)
{
    return E_NOTIMPL;
}
