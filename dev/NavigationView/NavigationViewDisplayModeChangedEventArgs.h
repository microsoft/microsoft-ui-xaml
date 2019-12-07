// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewDisplayModeChangedEventArgs.g.h"

class NavigationViewDisplayModeChangedEventArgs :
    public winrt::implementation::NavigationViewDisplayModeChangedEventArgsT<NavigationViewDisplayModeChangedEventArgs>
{
public:
#pragma region INavigationViewDisplayModeChangedEventArgs
    winrt::NavigationViewDisplayMode DisplayMode();
#pragma endregion

    void DisplayMode(winrt::NavigationViewDisplayMode value);

private:
    winrt::NavigationViewDisplayMode m_DisplayMode{};
};
