// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "PullToRefreshHelperTestApi.g.h"

class PullToRefreshHelperTestApi :
    public winrt::implementation::PullToRefreshHelperTestApiT<PullToRefreshHelperTestApi>
{
public:
    static winrt::RefreshInteractionRatioChangedEventArgs CreateRefreshInteractionRatioChangedEventArgsInstance(double value);
    static winrt::RefreshStateChangedEventArgs CreateRefreshStateChangedEventArgsInstance(winrt::RefreshVisualizerState const& oldValue, winrt::RefreshVisualizerState const& newValue);
    static winrt::RefreshRequestedEventArgs CreateRefreshRequestedEventArgsInstance(winrt::Deferral const& handler);
};