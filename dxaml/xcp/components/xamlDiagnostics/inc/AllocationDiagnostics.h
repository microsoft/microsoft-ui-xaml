// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlOM.WinUI.Private.h>

namespace AllocationDiagnostics
{
    HRESULT GetMemorySnapshot(_Out_ XamlMemorySnapshot* snapshot);
}
