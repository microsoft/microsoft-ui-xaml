// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemSeparator.h"
#include "Utils.h"

#include "NavigationViewItemSeparator.properties.cpp"

NavigationViewItemSeparator::NavigationViewItemSeparator()
{
    SetDefaultStyleKey(this);
}

void NavigationViewItemSeparator::UpdateVisualState(bool useTransitions)
{
    if (m_appliedTemplate)
    {
        static auto groupName = L"NavigationSeparatorLineStates"sv;
        auto stateName = (Position() != NavigationViewListPosition::TopPrimary) ? L"HorizontalLine"sv : L"VerticalLine"sv;

        VisualStateUtil::GotToStateIfGroupExists(*this, groupName, stateName, false /*useTransitions*/);
    }
}

void NavigationViewItemSeparator::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visual may not the same as we expect
    m_appliedTemplate = false;
    NavigationViewItemBase::OnApplyTemplate();

    m_appliedTemplate = true;
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItemSeparator::OnNavigationViewListPositionChanged()
{
    UpdateVisualState(false /*useTransition*/);
}
