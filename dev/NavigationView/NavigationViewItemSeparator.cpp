// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemSeparator.h"
#include "Utils.h"

#include "NavigationViewItemSeparator.properties.cpp"

static constexpr auto c_rootGrid = L"NavigationViewItemSeparatorRootGrid"sv;

NavigationViewItemSeparator::NavigationViewItemSeparator()
{
    SetDefaultStyleKey(this);
}

void NavigationViewItemSeparator::UpdateVisualState(bool useTransitions)
{
    if (m_appliedTemplate)
    {
        static auto groupName = L"NavigationSeparatorLineStates"sv;
        auto stateName = (Position() != NavigationViewRepeaterPosition::TopPrimary) ? L"HorizontalLine"sv : L"VerticalLine"sv;

        VisualStateUtil::GotToStateIfGroupExists(*this, groupName, stateName, false /*useTransitions*/);
    }
}

void NavigationViewItemSeparator::OnApplyTemplate()
{
    // Stop UpdateVisualState before template is applied. Otherwise the visual may not the same as we expect
    m_appliedTemplate = false;
    NavigationViewItemBase::OnApplyTemplate();

    if (auto rootGrid = GetTemplateChildT<winrt::Grid>(c_rootGrid, *this))
    {
        m_rootGrid.set(rootGrid);
    }

    m_appliedTemplate = true;
    UpdateVisualState(false /*useTransition*/);
    UpdateItemIndentation();
}

void NavigationViewItemSeparator::OnNavigationViewRepeaterPositionChanged()
{
    UpdateVisualState(false /*useTransition*/);
}

void NavigationViewItemSeparator::OnNavigationViewItemBaseDepthChanged()
{
    UpdateItemIndentation();
}

void NavigationViewItemSeparator::UpdateItemIndentation()
{
    // Update item indentation based on its depth
    if (auto const rootGrid = m_rootGrid.get())
    {
        auto const oldMargin = rootGrid.Margin();
        auto newLeftMargin = Depth() * c_itemIndentation;
        rootGrid.Margin({ static_cast<double>(newLeftMargin), oldMargin.Top, oldMargin.Right, oldMargin.Bottom });
    }
}
