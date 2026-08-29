// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <OptionalChangeState.h>

namespace OptionalChangeState
{
    uint64_t g_enabledChanges = 0;
    bool     g_locked = false;
    SRWLOCK  g_srwLock = SRWLOCK_INIT;
}
