// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NavigationViewItemBase.h"
#include "NavigationViewItemHeader.g.h"

class NavigationViewItemHeader :
    public winrt::implementation::NavigationViewItemHeaderT<NavigationViewItemHeader, NavigationViewItemBase>
{
public:
    ForwardRefToBaseReferenceTracker(NavigationViewItemBase)

    NavigationViewItemHeader();

    // IFrameworkElementOverrides
    void OnApplyTemplate() override;

private:
    void OnSplitViewPropertyChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);
    void UpdateIsClosedCompact();
    void UpdateItemIndentation();
    void OnNavigationViewItemBaseDepthChanged() override;

    void UpdateVisualState(bool useTransitions);

    bool m_isClosedCompact{ false };

    PropertyChanged_revoker m_splitViewIsPaneOpenChangedRevoker{};
    PropertyChanged_revoker m_splitViewDisplayModeChangedRevoker{};

    tracker_ref<winrt::Grid> m_rootGrid{ this };

};
