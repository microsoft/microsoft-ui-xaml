// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "IconSource.h"
#include "PathIconSource.g.h"
#include "PathIconSource.properties.h"

class PathIconSource :
    public ReferenceTracker<PathIconSource, winrt::implementation::PathIconSourceT, IconSource>,
    public PathIconSourceProperties
{
public:
    using PathIconSourceProperties::EnsureProperties;
    using PathIconSourceProperties::ClearProperties;

    winrt::DependencyProperty GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty);
    winrt::IconElement CreateIconElementCore();
};
