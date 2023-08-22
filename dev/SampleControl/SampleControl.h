// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "SampleControl.g.h"
#include "SampleControl.properties.h"

class SampleControl :
    public ReferenceTracker<SampleControl, winrt::implementation::SampleControlT>,
    public SampleControlProperties
{

public:
    SampleControl();
    ~SampleControl() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
