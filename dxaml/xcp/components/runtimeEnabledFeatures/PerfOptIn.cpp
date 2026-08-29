// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <RuntimeEnabledFeatures.h>
#include <PerfOptIn.h>

// Process-wide static cache for IsPerfOptInEnabled().
// This avoids repeated calls through GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled()
// on every check (which involves shared_ptr, state lookup, etc.).
static bool s_perfOptInCached = false;
static bool s_perfOptInValue = false;

bool IsPerfOptInEnabled()
{
    if (!s_perfOptInCached)
    {
        s_perfOptInValue = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()
            ->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForcePerfOptIn);
        s_perfOptInCached = true;
    }
    return s_perfOptInValue;
}

void InvalidatePerfOptInCache()
{
    s_perfOptInCached = false;
}
