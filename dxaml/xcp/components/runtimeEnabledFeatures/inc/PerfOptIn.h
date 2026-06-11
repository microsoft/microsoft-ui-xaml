// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Convenience wrapper for checking the ForcePerfOptIn runtime feature flag.
// Instead of the verbose:
//   GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeEnabledFeature::ForcePerfOptIn)
// Just use:
//   IsPerfOptInEnabled()

#pragma once


// IMPORTANT CONVENTION WHEN CALLING THIS FUNCTION:
// The check should be on its own line to make it easy to read and remove later if needed.
// So, canonical usage should look like:
//  if (IsPerfOptInEnabled())    // <-- check should be on its own line, not part of other logic
//  {
//      // new, optimized code
//      // OK if there's some code duplication with old path
//  }
//  else
//  {
//      // old, compatible code
//  }
//
// IMPORTANT: Even though this is called "opt-in", it's _currently_ on by default while
// we get things setup and stabilized.  It will eventually be opt-in only.
//
// PERF NOTE: The result is cached in a process-wide static for speed.  On the first call,
// we go through RuntimeEnabledFeatureDetector (which may read the registry).  After that,
// it's just a branch on a static bool.
//
// The cache is automatically invalidated when:
//   - SetFeatureOverride/ClearFeatureOverride is called for ForcePerfOptIn (test overrides)
//   - ClearAllFeatureOverrides or ClearCache is called
//   - InitializeXamlCore is called (between tests, for a clean slate)
bool IsPerfOptInEnabled();

// Clears the static cache so the next call to IsPerfOptInEnabled() re-evaluates.
void InvalidatePerfOptInCache();
