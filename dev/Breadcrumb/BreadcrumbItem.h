// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include "BreadcrumbItem.g.h"

class BreadcrumbItem :
    public ReferenceTracker<BreadcrumbItem, winrt::implementation::BreadcrumbItemT>
{
public:
    BreadcrumbItem();
    ~BreadcrumbItem();

    // IFrameworkElement
    void OnApplyTemplate();

    void RevokeListeners();
    void ResetVisualProperties();
    void SetPropertiesForLastNode();
    void SetPropertiesForEllipsisNode();
    void SetItemsRepeater(const winrt::Breadcrumb& parent);

private:
    void OnLoadedEvent(const winrt::IInspectable& sender, const winrt::RoutedEventArgs& args);
    void OnBreadcrumbItemClick(const winrt::IInspectable& sender, const winrt::SplitButtonClickEventArgs& args);

    void SetPrimaryButtonFontWeight(bool mustBeBold);
    void SetSecondaryButtonVisibility(bool isVisible);
    void SetSecondaryButtonText(bool isCollapsed);
    
    tracker_ref<winrt::SplitButton> m_splitButton{ this };
    tracker_ref<winrt::Grid> m_rootGrid{ this };
    tracker_ref<winrt::Grid> m_secondaryButtonGrid{ this };
    tracker_ref<winrt::Grid> m_splitButtonBorder{ this };
    tracker_ref<winrt::Button> m_primaryButton{ this };
    tracker_ref<winrt::Button> m_secondaryButton{ this };

    winrt::SplitButton::Loaded_revoker m_splitButtonLoadedRevoker{};
    winrt::SplitButton::Click_revoker m_splitButtonClickRevoker{};

    bool m_isEllipsisNode{};
    bool m_isLastNode{};
    bool m_isChevronVisible{};
    winrt::GridLength m_chevronOriginalWidth;

    tracker_ref<winrt::Breadcrumb> m_parent { this };

    
};
