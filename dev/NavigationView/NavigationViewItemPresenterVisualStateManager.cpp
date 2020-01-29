// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationViewItemPresenterVisualStateManager.h"
#include "NavigationViewItemBase.h"
#include "NavigationViewItem.h"
#include "ItemTemplateWrapper.h"


NavigationViewItemPresenterVisualStateManager::NavigationViewItemPresenterVisualStateManager()
{
}

bool NavigationViewItemPresenterVisualStateManager::GoToStateCore(winrt::Control control, winrt::FrameworkElement templateRoot, winrt::hstring stateName, winrt::VisualStateGroup group, winrt::VisualState state, bool useTransitions)
{
    auto rootGridDO = control.GetTemplateChild(L"NVIRootGrid");
    if (auto rootGrid = rootGridDO.try_as<winrt::Grid>())
    {
        auto nvipDO = rootGrid.Children().GetAt(0);
        if (auto nvip = nvipDO.try_as<winrt::Control>())
        {
            return winrt::VisualStateManager::GoToState(nvip, stateName, useTransitions);
        }
    }
    return false;
    //return winrt::VisualStateManager::GoToState(control, stateName, useTransitions);
}
