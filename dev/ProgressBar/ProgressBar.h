// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "ProgressBar.g.h"
#include "ProgressBar.properties.h"

class ProgressBar :
    public ReferenceTracker<ProgressBar, winrt::implementation::ProgressBarT>,
    public ProgressBarProperties
{

public:
    ProgressBar();
    ~ProgressBar() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
