// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NavigateFocusResult.h>

using namespace xaml_hosting;

NavigateFocusResult::NavigateFocusResult(_In_ bool focusMoved)
    : m_focusMoved(focusMoved)
{
}

_Check_return_ HRESULT
NavigateFocusResult::get_WasFocusMoved(_Out_ boolean* result)
{
    *result = m_focusMoved;
    return S_OK;
}

