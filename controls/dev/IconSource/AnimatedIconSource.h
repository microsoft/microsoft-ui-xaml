// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AnimatedIconSource.g.h"
#include "AnimatedIconSource.properties.h"

class AnimatedIconSource :
    public ReferenceTracker<AnimatedIconSource, DeriveFromPathIconSourceHelper_base, winrt::AnimatedIconSource>,
    public AnimatedIconSourceProperties
{
public:
    using AnimatedIconSourceProperties::EnsureProperties;
    using AnimatedIconSourceProperties::ClearProperties;

    winrt::DependencyProperty GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty);
    winrt::IconElement CreateIconElementCore();
};
