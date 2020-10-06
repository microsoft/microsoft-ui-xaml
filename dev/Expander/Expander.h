// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "Expander.g.h"
#include "Expander.properties.h"

class Expander :
    public ReferenceTracker<Expander, winrt::implementation::ExpanderT>,
    public ExpanderProperties
{

public:
    Expander();
    ~Expander() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnIsExpandedPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
