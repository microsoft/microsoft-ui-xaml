// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "NavigationViewItemPresenterVisualStateManager.g.h"
//#include "NavigationViewItemPresenterVisualStateManager.properties.h"

class NavigationViewItemPresenterVisualStateManager :
    public winrt::implementation::NavigationViewItemPresenterVisualStateManagerT<NavigationViewItemPresenterVisualStateManager>
{
public:
    NavigationViewItemPresenterVisualStateManager();

    bool GoToStateCore(winrt::Control control, winrt::FrameworkElement templateRoot, winrt::hstring stateName, winrt::VisualStateGroup group, winrt::VisualState state, bool useTransitions);

};
