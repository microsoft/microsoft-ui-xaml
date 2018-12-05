// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>

#include "NavigationView.h"
#include "NavigationViewItemGroup.h"
//#include "NavigationViewItemGroupAutomationPeer.h"
#include "Utils.h"
#include "Vector.h"
#include "NavigationViewList.h"

NavigationViewItemGroup::NavigationViewItemGroup()
{

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_MenuItemsProperty, items);

}

void NavigationViewItemGroup::OnApplyTemplate()
{
	__super::OnApplyTemplate();

    winrt::IControlProtected controlProtected = *this;

}

void NavigationViewItemGroup::SetNavigationViewItemGroupListPosition(winrt::ListView& listView, NavigationViewListPosition position)
{
    if (listView)
    {
        if (auto navigationViewList = listView.try_as<winrt::NavigationViewList>())
        {
            winrt::get_self<NavigationViewList>(navigationViewList)->SetNavigationViewListPosition(position);
        }
    }
}

//void NavigationViewItemGroup::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
//{
//	auto property = args.Property();
//}
