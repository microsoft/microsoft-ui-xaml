// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <ActivationAPI.h>

using namespace DirectUI;

_Check_return_ HRESULT ActivationAPI::ActivateCoreInstance(_In_ const CClassInfo* pType, _Outptr_ CDependencyObject** ppInstance)
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT ActivationAPI::ActivateInstance(_In_ CClassInfo const *, _In_opt_ IInspectable *, _Outptr_ IInspectable **)
{
    return E_NOTIMPL;
}
