// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "AnimatedIcon.g.h"
#include "AnimatedIcon.properties.h"

class AnimatedIcon :
    public ReferenceTracker<AnimatedIcon, winrt::implementation::AnimatedIconT>,
    public AnimatedIconProperties
{

public:
    AnimatedIcon();
    ~AnimatedIcon() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
