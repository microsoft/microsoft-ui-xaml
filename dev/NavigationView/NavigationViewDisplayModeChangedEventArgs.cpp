// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewDisplayModeChangedEventArgs.h"
#include "NavigationView.h"

NavigationViewDisplayModeChangedEventArgs::NavigationViewDisplayModeChangedEventArgs()
= default;

#pragma region INavigationViewDisplayModeChangedEventArgs
winrt::NavigationViewDisplayMode NavigationViewDisplayModeChangedEventArgs::DisplayMode()
{
    return m_DisplayMode;
}
#pragma endregion

void NavigationViewDisplayModeChangedEventArgs::DisplayMode(
    winrt::NavigationViewDisplayMode value)
{
    m_DisplayMode = value;
}