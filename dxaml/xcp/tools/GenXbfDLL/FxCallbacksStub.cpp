// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

namespace FxCallbacks
{
_Check_return_ HRESULT FrameworkCallbacks_CheckPeerType(
    _In_ CDependencyObject*,
    _In_ const xstring_ptr&,
    _In_ XINT32)
{
    return E_NOTIMPL;
}

void XamlRoot_RaiseChanged(_In_ IInspectable*)
{
}

void XamlRoot_RaiseInputActivationChanged(_In_ IInspectable*)
{
}
}
