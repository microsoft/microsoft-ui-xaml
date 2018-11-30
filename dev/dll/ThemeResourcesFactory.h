// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ThemeResources.h"

class ThemeResourcesFactory
    : public winrt::factory_implementation::XamlControlsResourcesT<ThemeResourcesFactory, ThemeResources>
{
};