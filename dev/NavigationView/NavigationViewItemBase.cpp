// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "NavigationViewItemBase.h"
#include "NavigationViewList.h"
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

void NavigationViewItemBase::OnRepeatedIndexPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    auto indexPath = GetIndexPath();
    if (auto selectionModel = SelectionModel())
    {
        bool isSelectedNullable = false;
        if (IsRealized(indexPath))
        {
            isSelectedNullable = selectionModel.IsSelectedAt(indexPath).Value();
        }
        IsSelected(isSelectedNullable);
    }
}

void NavigationViewItemBase::OnSelectionModelPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (args.OldValue())
    {
        //(args.OldValue().try_as<winrt::SelectionModel>()).SelectionChanged(m_selectionChangedEventToken);
        m_selectionChangedEventToken.revoke();
    }

    if (args.NewValue())
    {
        //m_selectionChangedEventToken = (args.NewValue().try_as<winrt::SelectionModel>()).SelectionChanged(&OnSelectionChanged);
        m_selectionChangedEventToken = (args.NewValue().try_as<winrt::SelectionModel>()).SelectionChanged(winrt::auto_revoke, { this, &NavigationViewItemBase::OnSelectionChanged });
    }
}
void NavigationViewItemBase::OnSelectionChanged(winrt::SelectionModel selectionModel, winrt::SelectionModelSelectionChangedEventArgs e)
{
    bool oldValue = IsSelected();
    auto indexPath = GetIndexPath();

    bool newValue = false;
    if (IsRealized(indexPath))
    {
        newValue = SelectionModel().IsSelectedAt(indexPath).Value();
    }

    if (oldValue != newValue)
    {
        IsSelected(newValue);

        // Updated selected item in NavigationView 
        if (newValue)
        {
            auto item = SelectionModel().SelectedItem();
            m_navigationView.get().SelectedItem(item);
        }

        //// AutomationEvents.PropertyChanged is used as a value that means dont raise anything 
        //AutomationEvents eventToRaise =
        //    oldValue ?
        //        (SelectionModel.SingleSelect ? AutomationEvents.PropertyChanged : AutomationEvents.SelectionItemPatternOnElementRemovedFromSelection) :
        //        (SelectionModel.SingleSelect ? AutomationEvents.SelectionItemPatternOnElementSelected : AutomationEvents.SelectionItemPatternOnElementAddedToSelection);

        //if (eventToRaise != AutomationEvents.PropertyChanged && AutomationPeer.ListenerExists(eventToRaise))
        //{
        //    var peer = FrameworkElementAutomationPeer.CreatePeerForElement(this);
        //    peer.RaiseAutomationEvent(eventToRaise);
        //}
    }
}

winrt::IndexPath NavigationViewItemBase::GetIndexPath()
{
    auto child = (*this).try_as<winrt::FrameworkElement>();
    auto parent = child.Parent().try_as<winrt::FrameworkElement>();
    if (!parent)
    {
        parent = (winrt::VisualTreeHelper::GetParent(child)).try_as<winrt::FrameworkElement>();
    }

    auto path = std::vector<int>();
    if (parent == nullptr)
    {
        return IndexPath::CreateFromIndices(path);
    }

    bool  test = !IsRootItemsRepeater((parent.try_as<winrt::ItemsRepeater>()).Name());
    auto testName = (parent.try_as<winrt::ItemsRepeater>()).Name();
    bool nameComparison = (testName == c_leftRepeater);
    bool  test1 = !(parent.try_as<winrt::ItemsRepeater>());
    // TOOD: Hack to know when to stop
    while (!(parent.try_as<winrt::ItemsRepeater>()) || !IsRootItemsRepeater((parent.try_as<winrt::ItemsRepeater>()).Name()))
    {
        if (auto parentIR = parent.try_as<winrt::ItemsRepeater>())
        {
            path.insert(path.begin(), parentIR.GetElementIndex(child));
        }

        child = parent;
        auto name = parent.Name();
        parent = parent.Parent().try_as<winrt::FrameworkElement>();
        if (!parent)
        {
            parent = (winrt::VisualTreeHelper::GetParent(child)).try_as<winrt::FrameworkElement>();
        }
    }

    if (auto parentIR = parent.try_as<winrt::ItemsRepeater>())
    {
        path.insert(path.begin(), parentIR.GetElementIndex(child));
    }

    // If item is in one of the disconnected ItemRepeaters, account for that in IndexPath calculations
    //if ((parent as ItemsRepeater).Name == RepNavigationView.c_flyoutItemsRepeater)
    //{

    //}
    //else if ((parent as ItemsRepeater).Name == RepNavigationView.c_overflowRepeater)
    //{

    //}

    return IndexPath::CreateFromIndices(path);
}

bool NavigationViewItemBase::IsRootItemsRepeater(winrt::hstring name)
{
    return (name == c_topNavRepeater ||
        name == c_leftRepeater ||
        name == c_overflowRepeater ||
        name == c_flyoutItemsRepeater);
}

bool NavigationViewItemBase::IsRealized(winrt::IndexPath indexPath)
{
    bool isRealized = true;
    for (int i = 0; i < indexPath.GetSize(); i++)
    {
        if (indexPath.GetAt(i) < 0)
        {
            isRealized = false;
            break;
        }
    }

    return isRealized;
}

void NavigationViewItemBase::OnPointerReleased(winrt::PointerRoutedEventArgs const& args)
{
    __super::OnPointerReleased(args);
    if (auto selectionModel = SelectionModel())
    {
        winrt::IndexPath ip = GetIndexPath();
        SelectionModel().SelectAt(ip);
    }
}

void NavigationViewItemBase::SetNavigationViewParent(winrt::NavigationView const& navigationView)
{
    m_navigationView = winrt::make_weak(navigationView);
}
