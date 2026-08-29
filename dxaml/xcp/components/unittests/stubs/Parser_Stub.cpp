// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlSchemaContext.h>

_Check_return_ HRESULT XamlSchemaContext::GetRuntime(
    _In_ XamlFactoryKind,
    _Out_ std::shared_ptr<XamlRuntime>& runtime
    )
{
    runtime = nullptr;
    return S_OK;
}
