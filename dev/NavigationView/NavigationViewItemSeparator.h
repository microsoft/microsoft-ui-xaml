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
    void OnNavigationViewRepeaterPositionChanged() override;
    void UpdateVisualState(bool useTransitions);

    bool m_appliedTemplate{ false };
};
