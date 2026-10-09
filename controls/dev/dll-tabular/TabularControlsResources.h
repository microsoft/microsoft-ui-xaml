// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TabularControlsResources.g.h"
#include "TabularControlsResources.properties.h"

class TabularControlsResources :
    public ReferenceTracker<TabularControlsResources, winrt::implementation::TabularControlsResourcesT, winrt::composable>,
    public TabularControlsResourcesProperties
{
public:
    TabularControlsResources();

    // Not MUXControlsFactory::IsInitialized(): XamlControlsTabularXamlMetaDataProvider.Initialize() also sets that.
    static bool HasBeenCreated() noexcept { return s_hasBeenCreated; }
private:
    void UpdateSource();

    static inline bool s_hasBeenCreated{ false };
};
