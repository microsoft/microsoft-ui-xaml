// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "PullToRefreshHelperTestApi.h"
#include "PullToRefreshHelperTestApiFactory.h"
#include "RefreshInteractionRatioChangedEventArgs.h"
#include "RefreshVisualizerEventArgs.h"
#include "common.h"

CppWinRTActivatableClassWithBasicFactory(PullToRefreshHelperTestApi)

winrt::RefreshInteractionRatioChangedEventArgs PullToRefreshHelperTestApi::CreateRefreshInteractionRatioChangedEventArgsInstance(double value)
{
    return winrt::make<RefreshInteractionRatioChangedEventArgs>(value);
}

winrt::RefreshStateChangedEventArgs PullToRefreshHelperTestApi::CreateRefreshStateChangedEventArgsInstance(winrt::RefreshVisualizerState const& oldValue, winrt::RefreshVisualizerState const& newValue)
{
    return winrt::make<RefreshStateChangedEventArgs>(oldValue, newValue);
}

winrt::RefreshRequestedEventArgs PullToRefreshHelperTestApi::CreateRefreshRequestedEventArgsInstance(winrt::Deferral const& handler)
{
    return winrt::make<RefreshRequestedEventArgs>(handler);
}
