// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "PipsControl.g.h"
#include "PipsControl.properties.h"

class PipsControl :
    public ReferenceTracker<PipsControl, winrt::implementation::PipsControlT>,
    public PipsControlProperties
{

public:
    PipsControl();
    ~PipsControl() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
