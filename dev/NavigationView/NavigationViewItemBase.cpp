// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemBase.h"
#include "NavigationViewList.h"
#include "NavigationView.h"

// NOTE: We need to manually define this factory because the IDL does not specify a create method which means that
// technically in the ABI this type is not activatable. However we might get asked for this factory so we need to provide it.
struct NavigationViewItemBaseFactory :
    public winrt::implements<NavigationViewItemBaseFactory, winrt::IActivationFactory, winrt::INavigationViewItemBaseFactory>
{
    hstring GetRuntimeClassName() const
    {
        return winrt::hstring_name_of<winrt::NavigationViewItemBase>();
    }

    winrt::IInspectable ActivateInstance() const
    {
        throw winrt::hresult_not_implemented();
    }
};

CppWinRTActivatableClassWithFactory(NavigationViewItemBase, NavigationViewItemBaseFactory);

NavigationViewItemBase::NavigationViewItemBase()
{
}

NavigationViewListPosition NavigationViewItemBase::Position()
{
    return m_position;
}

void NavigationViewItemBase::Position(NavigationViewListPosition value)
{
    if (m_position != value)
    {
        m_position = value;
        OnNavigationViewListPositionChanged();
    }
}

winrt::NavigationView NavigationViewItemBase::GetNavigationView()
{
    //Because of Overflow popup, we can't get NavigationView by SharedHelpers::GetAncestorOfType
    winrt::NavigationView navigationView{ nullptr };
    auto navigationViewList = GetNavigationViewList();
    if (navigationViewList)
    {
        navigationView = winrt::get_self<NavigationViewList>(navigationViewList)->GetNavigationViewParent();
    }
    else
    {
        // Like Settings, it's NavigationViewItem, but it's not in NavigationViewList. Give it a second chance
        navigationView = SharedHelpers::GetAncestorOfType<winrt::NavigationView>(winrt::VisualTreeHelper::GetParent(*this));
    }
    return navigationView;
}

winrt::SplitView NavigationViewItemBase::GetSplitView()
{
    winrt::SplitView splitView{ nullptr };
    auto navigationView = GetNavigationView();
    if (navigationView)
    {
        splitView = winrt::get_self<NavigationView>(navigationView)->GetSplitView();
    }
    return splitView;
}

winrt::NavigationViewList NavigationViewItemBase::GetNavigationViewList()
{
    // Find parent NavigationViewList
    return SharedHelpers::GetAncestorOfType<winrt::NavigationViewList>(winrt::VisualTreeHelper::GetParent(*this));
}

void NavigationViewItemBase::SetDepth(int depth)
{
    m_depth = depth;
}

int NavigationViewItemBase::GetDepth()
{
    return m_depth;
}
