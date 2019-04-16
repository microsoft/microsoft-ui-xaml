// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "VirtualizingLayout.h"
#include "FlexboxLayout.g.h"

class FlexboxLayout :
    public ReferenceTracker<FlexboxLayout, winrt::implementation::FlexboxLayoutT, VirtualizingLayout>
{
public:
    FlexboxLayout();
};
