// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "NEWCONTROL.g.h"
#include "NEWCONTROL.properties.h"

class NEWCONTROL :
    public ReferenceTracker<NEWCONTROL, winrt::implementation::NEWCONTROLT>,
    public NEWCONTROLProperties
{

public:
    NEWCONTROL();
    ~NEWCONTROL() {}

    // IFrameworkElement
    void OnApplyTemplate();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
};
