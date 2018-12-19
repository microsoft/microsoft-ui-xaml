// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "NavigationViewList.h"
#include "NavigationViewItem.h"
#include "NavigationView.h"

CppWinRTActivatableClassWithBasicFactory(NavigationViewList);

NavigationViewList::NavigationViewList()
{
}

// IItemsControlOverrides

winrt::DependencyObject NavigationViewList::GetContainerForItemOverride()
{
    return winrt::make<NavigationViewItem>();
}

bool NavigationViewList::IsItemItsOwnContainerOverride(winrt::IInspectable const& args)
{
    bool isItemItsOwnContainerOverride = false;
    if (args)
    {
        // This validation is only relevant outside of the Windows build where WUXC and MUXC have distinct types.
#if !BUILD_WINDOWS
        // Certain items are disallowed in a NavigationView's items list. Check for them.
        if (args.try_as<winrt::Windows::UI::Xaml::Controls::NavigationViewItemBase>())
        {
            throw winrt::hresult_invalid_argument(L"MenuItems contains a Windows.UI.Xaml.Controls.NavigationViewItem. This control requires that the NavigationViewItems be of type Microsoft.UI.Xaml.Controls.NavigationViewItem.");
        }
#endif

        auto nvib = args.try_as<winrt::NavigationViewItemBase>();
        if (nvib && nvib != m_lastItemCalledInIsItemItsOwnContainerOverride.get())
        {
            m_lastItemCalledInIsItemItsOwnContainerOverride.set(nvib);
        }
        isItemItsOwnContainerOverride = static_cast<bool>(nvib);
    }
    return isItemItsOwnContainerOverride;
}

void NavigationViewList::ClearContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item)
{
    if (auto itemContainer = element.try_as<winrt::NavigationViewItem>())
    {
        auto itemContainerImplementation = winrt::get_self<NavigationViewItem>(itemContainer);
        itemContainerImplementation->ClearIsContentChangeHandlingDelayedForTopNavFlag();
        itemContainerImplementation->SetDepth(0);
        itemContainerImplementation->SetParentItem(nullptr);
    }
    __super::PrepareContainerForItemOverride(element, item);
}

void NavigationViewList::PrepareContainerForItemOverride(winrt::DependencyObject const& element, winrt::IInspectable const& item)
{
    if (auto itemContainer = element.try_as<winrt::NavigationViewItemBase>())
    {
        winrt::get_self<NavigationViewItemBase>(itemContainer)->Position(m_navigationViewListPosition);
    }
    if (auto itemContainer = element.try_as<winrt::NavigationViewItem>())
    {
        itemContainer.UseSystemFocusVisuals(m_showFocusVisual);
        winrt::get_self<NavigationViewItem>(itemContainer)->ClearIsContentChangeHandlingDelayedForTopNavFlag();

        //Update Item depth and set item parent
        auto navigationView = GetNavigationViewParent();
        auto lastExpandedNavItem = winrt::get_self<NavigationView>(navigationView)->GetLastExpandedItem();
        if (lastExpandedNavItem)
        {
            winrt::get_self<NavigationViewItem>(itemContainer)->SetParentItem(lastExpandedNavItem);

            auto depth = winrt::get_self<NavigationViewItem>(lastExpandedNavItem)->GetDepth();
            winrt::get_self<NavigationViewItem>(itemContainer)->SetDepth(depth + 1);
        }

        if ((itemContainer.MenuItems() && itemContainer.MenuItems().Size() > 0) || itemContainer.MenuItemsSource() || itemContainer.HasUnrealizedChildren())
        {
            auto viewModel = winrt::get_self<NavigationView>(navigationView)->GetViewModel();
            viewModel->RegisterItemExpandEventToSelf(itemContainer, *this);
        }
    }

    __super::PrepareContainerForItemOverride(element, item);
}

void NavigationViewList::SetNavigationViewListPosition(NavigationViewListPosition navigationViewListPosition)
{
    m_navigationViewListPosition = navigationViewListPosition;
    PropagateChangeToAllContainers<winrt::NavigationViewItemBase>(
        [&navigationViewListPosition](const winrt::NavigationViewItemBase& container)
            {
                winrt::get_self<NavigationViewItemBase>(container)->Position(navigationViewListPosition);
            });
}

void NavigationViewList::SetShowFocusVisual(bool showFocus)
{
    m_showFocusVisual = showFocus;
    PropagateChangeToAllContainers<winrt::NavigationViewItem>(
        [showFocus](const winrt::NavigationViewItem& container)
    {
        container.UseSystemFocusVisuals(showFocus);
    });
}

void NavigationViewList::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}

// IControlOverrides
void NavigationViewList::OnKeyDown(winrt::KeyRoutedEventArgs const& eventArgs)
{
    auto key = eventArgs.Key();

    if (key == winrt::VirtualKey::GamepadLeftShoulder
        || key == winrt::VirtualKey::GamepadRightShoulder
        || key == winrt::VirtualKey::GamepadLeftTrigger
        || key == winrt::VirtualKey::GamepadRightTrigger)
    {
        // We need to return at this point to prevent ListView from handling page up / page down
        // when any of these four keys get triggered. The reason is that it would navigate to the
        // first or last item in the list and handle the event, preventing NavigationView
        // to do its KeyDown handling afterwards.
        return;
    }

    __super::OnKeyDown(eventArgs);
}

winrt::NavigationView NavigationViewList::GetNavigationViewParent()
{
    return m_navigationView.get();
}

winrt::NavigationViewItemBase NavigationViewList::GetLastItemCalledInIsItemItsOwnContainerOverride()
{
    return m_lastItemCalledInIsItemItsOwnContainerOverride.get();
}

template<typename T> 
void NavigationViewList::PropagateChangeToAllContainers(std::function<void(typename T& container)> function)
{
    if (auto items = Items())
    {
        auto size = static_cast<int>(items.Size());
        for (int i = 0; i < size; i++)
        {
            auto container = ContainerFromIndex(i);
            if (container)
            {
                auto itemContainer = container.try_as<typename T>();
                if (itemContainer)
                {
                    function(itemContainer);
                }
            }
        }
    }
}
