// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlProfilerTracing.h"
#include <atomic>

// Keep the atomic's definition in one translation unit; PCHs can give std::atomic
// different alignment annotations that conflict during link-time code generation.
static std::atomic<bool> g_profilerEnabled{false};

void XamlProfilerTracing::EnableAllocationProfiling(bool enabled) noexcept
{
    g_profilerEnabled.store(enabled, std::memory_order_relaxed);
}

bool XamlProfilerTracing::IsAllocationProfilingEnabled() noexcept
{
    return g_profilerEnabled.load(std::memory_order_relaxed);
}
