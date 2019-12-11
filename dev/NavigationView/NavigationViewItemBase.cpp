// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemBase.h"
#include "NavigationView.h"
#include "IndexPath.h"

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

NavigationViewRepeaterPosition NavigationViewItemBase::Position()
{
    return m_position;
}

void NavigationViewItemBase::Position(NavigationViewRepeaterPosition value)
{
    if (m_position != value)
    {
        m_position = value;
        OnNavigationViewRepeaterPositionChanged();
    }
}

winrt::NavigationView NavigationViewItemBase::GetNavigationView()
{
    return m_navigationView.get();
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

void NavigationViewItemBase::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}
