// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//-----------------------------------------------------------------------------
//
// Returns TRUE if the current process has package identity.
//
//-----------------------------------------------------------------------------
bool CWindowsServices::IsProcessPackaged()
{
    UINT32 n = 0;
    return GetCurrentPackageId(&n, NULL) == ERROR_INSUFFICIENT_BUFFER;
}

