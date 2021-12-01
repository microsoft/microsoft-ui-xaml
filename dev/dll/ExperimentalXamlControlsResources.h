// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ExperimentalXamlControlsResources.g.h"

class ExperimentalXamlControlsResources :
    public ReferenceTracker<ExperimentalXamlControlsResources, winrt::implementation::ExperimentalXamlControlsResourcesT, winrt::composable>
{
public:
    ExperimentalXamlControlsResources();
private:
    void UpdateSource();
};
