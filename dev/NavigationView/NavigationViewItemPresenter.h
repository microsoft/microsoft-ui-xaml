// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "NavigationViewItemPresenter.g.h"
#include "NavigationViewHelper.h"
#include "NavigationViewItemPresenter.properties.h"

class NavigationViewItem;

class NavigationViewItemPresenter:
    public ReferenceTracker<NavigationViewItemPresenter, winrt::implementation::NavigationViewItemPresenterT>,
    public NavigationViewItemPresenterProperties
{
public:
    NavigationViewItemPresenter();

    // IFrameworkElementOverrides
    void OnApplyTemplate();

    // IFrameworkElementOverrides
    bool GoToElementStateCore(winrt::hstring const& stateName, bool useTransitions);

    winrt::UIElement GetSelectionIndicator();

    void UpdateCompactPaneLength(double compactPaneLength,bool shouldUpdate);

private:
    NavigationViewItem * GetNavigationViewItem();

    double m_compactPaneLengthValue = 40;

    NavigationViewItemHelper<NavigationViewItemPresenter> m_helper{ this };
};
