// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DllInitializedHelpers.h>
#include <NamespaceAliases.h>

bool g_isDllInitialized = false;

namespace Private
{
    bool GetIsDllInitialized() { return g_isDllInitialized; }
    void SetIsDllInitialized(bool isDllInitialized) { g_isDllInitialized = isDllInitialized; }
}