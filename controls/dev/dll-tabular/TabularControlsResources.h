// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <atomic>

#include "TabularControlsResources.g.h"
#include "TabularControlsResources.properties.h"

class TabularControlsResources :
    public ReferenceTracker<TabularControlsResources, winrt::implementation::TabularControlsResourcesT, winrt::composable>,
    public TabularControlsResourcesProperties
{
public:
    TabularControlsResources();

    // Successful construction does not imply that the dictionary is merged or still available.
    static bool HasBeenCreated() noexcept { return s_hasBeenCreated.load(std::memory_order_relaxed); }
private:
    void UpdateSource();

    static inline std::atomic<bool> s_hasBeenCreated{ false };
};
