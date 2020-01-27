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
    if (stateName == L"PointerOver")
    {
        return true;
    }
    return true;
}
