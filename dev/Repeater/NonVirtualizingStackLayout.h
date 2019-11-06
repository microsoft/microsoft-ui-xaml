// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NonVirtualizingLayout.h"
#include "NonVirtualizingStackLayout.g.h"
#include "NonVirtualizingStackLayout.properties.h"

class NonVirtualizingStackLayout :
    public ReferenceTracker<NonVirtualizingStackLayout, winrt::implementation::NonVirtualizingStackLayoutT, NonVirtualizingLayout>,
    public NonVirtualizingStackLayoutProperties
{
public:
    NonVirtualizingStackLayout();

    winrt::Size MeasureOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& availableSize);
    winrt::Size ArrangeOverride(winrt::NonVirtualizingLayoutContext const& context, winrt::Size const& finalSize);

    void OnOrientationPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

};
