// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "Pager.g.h"
#include "Pager.properties.h"

class Pager :
    public ReferenceTracker<Pager, winrt::implementation::PagerT>,
    public PagerProperties
{

public:
    Pager();
    ~Pager() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
