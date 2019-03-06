// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "NavigationViewHelper.h"
#include "NavigationViewList.g.h"
#include "ViewModel.h"
#include "MultiLevelListViewBase.h"

class NavigationViewList :
    public ReferenceTracker<NavigationViewList, winrt::implementation::NavigationViewListT>, public MultiLevelListViewBase
{
public:
    NavigationViewList();

    // IItemsControlOverrides
    winrt::DependencyObject GetContainerForItemOverride();
    bool IsItemItsOwnContainerOverride(winrt::IInspectable const& item);
    void ClearContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item);
    void PrepareContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item);

    void SetNavigationViewListPosition(NavigationViewListPosition navigationViewListPosition);
    void SetShowFocusVisual(bool showFocus);

    // In overflow, NavigationViewItem can't reach to NavigationView from visual tree by iterating all parents since it's a popup.
    // As a workaround, we make NavigationViewList keep a weakref of NavigationView.
    void SetNavigationViewParent(winrt::NavigationView const& navigationView);
    winrt::NavigationView GetNavigationViewParent();

    winrt::NavigationViewItemBase GetLastItemCalledInIsItemItsOwnContainerOverride();

    // IControlOverrides / IControlOverridesHelper
    void OnKeyDown(winrt::KeyRoutedEventArgs const& e);

private:
    NavigationViewListPosition m_navigationViewListPosition{ NavigationViewListPosition::LeftNav };
    bool m_showFocusVisual{ true };
    template<typename T> void PropagateChangeToAllContainers(std::function<void(typename T& container)> function);
    winrt::weak_ref<winrt::NavigationView> m_navigationView{ nullptr };

    // For topnav, like alarm application, we may only need icon and no content for NavigationViewItem. 
    // ListView raise ItemClicked event, but it only provides the content and not the container.
    // It's impossible for customer to identify which NavigationViewItem is associated with the clicked event.
    // So we need to provide a container to help with this. Below solution is fragile. We expect Task 17546992 will finally resolved from os
    // Before ListView raises OnItemClick, it checks if IsItemItsOwnContainerOverride in ListViewBase::OnItemClick
    // We assume this is the container of the clicked item.
    tracker_ref<winrt::NavigationViewItemBase> m_lastItemCalledInIsItemItsOwnContainerOverride{ this };
};

