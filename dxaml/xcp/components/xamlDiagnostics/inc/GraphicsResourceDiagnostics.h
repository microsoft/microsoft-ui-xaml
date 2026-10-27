// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlOM.WinUI.Private.h"

namespace GraphicsResourceDiagnostics
{
    HRESULT GetCounterCount(_Out_ UINT32* count);
    HRESULT GetCounterSnapshot(UINT32 index, _Out_ XamlGraphicsResourceCounterSnapshot* snapshot);
}
