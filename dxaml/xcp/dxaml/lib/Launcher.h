// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NamespaceAliases.h"

namespace Launcher
{
    _Check_return_ HRESULT TryInvokeLauncher(_In_ wf::IUriRuntimeClass* pUri);
}