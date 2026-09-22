// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "SortIndicator.h"
#include "SortIndicatorAutomationPeer.h"
#include "RuntimeProfiler.h"

static constexpr std::wstring_view s_NoSortStateName{ L"NoSort"sv };
static constexpr std::wstring_view s_AscendingStateName{ L"Ascending"sv };
static constexpr std::wstring_view s_DescendingStateName{ L"Descending"sv };

SortIndicator::SortIndicator()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_SortIndicator);

    SetDefaultStyleKey(this);
}

void SortIndicator::OnApplyTemplate()
{
    __super::OnApplyTemplate();

    UpdateVisualState(false /* useTransitions */);
}

winrt::AutomationPeer SortIndicator::OnCreateAutomationPeer()
{
    return winrt::make<SortIndicatorAutomationPeer>(*this);
}

void SortIndicator::OnDirectionPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& /*args*/)
{
    UpdateVisualState(true /* useTransitions */);
}

void SortIndicator::UpdateVisualState(bool useTransitions)
{
    const auto direction = Direction();

    std::wstring_view directionStateName{};
    switch (direction)
    {
        case winrt::SortIndicatorDirection::Ascending:
            directionStateName = s_AscendingStateName;
            break;
        case winrt::SortIndicatorDirection::Descending:
            directionStateName = s_DescendingStateName;
            break;
        case winrt::SortIndicatorDirection::None:
        default:
            directionStateName = s_NoSortStateName;
            break;
    }

    winrt::VisualStateManager::GoToState(*this, directionStateName, useTransitions);
}
