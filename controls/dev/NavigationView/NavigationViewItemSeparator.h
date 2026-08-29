// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.h"
#include "NavigationViewItemSeparator.g.h"

class NavigationViewItemSeparator :
    public winrt::implementation::NavigationViewItemSeparatorT<NavigationViewItemSeparator, NavigationViewItemBase>
{
public:
    ForwardRefToBaseReferenceTracker(NavigationViewItemBase)

    NavigationViewItemSeparator();
    void OnApplyTemplate() override;

private:
    void OnNavigationViewItemBasePositionChanged() override;
    void OnNavigationViewItemBaseDepthChanged() override;
    void UpdateVisualState(bool useTransitions);
    void UpdateItemIndentation();

    void OnSplitViewPropertyChanged(const winrt::DependencyObject& /*sender*/, const winrt::DependencyProperty& /*args*/);
    void UpdateIsClosedCompact(bool updateVisualState);

    bool m_appliedTemplate{ false };
    bool m_isClosedCompact{ false };

    tracker_ref<winrt::Grid> m_rootGrid{ this };

    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};
};
