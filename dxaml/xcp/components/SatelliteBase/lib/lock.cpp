// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NamespaceAliases.h>
#include <ReferenceTrackerExtension.h>
#include <TrackerPtrFamily.h>
#include <ReferenceTrackerRuntimeClass.h>

namespace Private
{
#if DBG
    ULONG g_wuxeDOInstancesCount = 0;
#endif
}
