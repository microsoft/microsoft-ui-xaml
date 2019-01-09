// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "Forms.g.h"
#include "Forms.properties.h"

class Forms :
    public ReferenceTracker<Forms, winrt::implementation::FormsT>,
    public FormsProperties
{

public:
    Forms();
    ~Forms() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
