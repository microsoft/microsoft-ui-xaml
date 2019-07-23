// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "RadioButtons.h"
#include "ResourceAccessor.h"
#include "RuntimeProfiler.h"
#include "Vector.h"
#include "common.h"

RadioButtons::RadioButtons()
{
    __RP_Marker_ClassById(RuntimeProfiler::ProfId_RadioButtons);

    auto items = winrt::make<Vector<winrt::IInspectable>>();
    SetValue(s_ItemsProperty, items);

    SetDefaultStyleKey(this);
}

void RadioButtons::OnApplyTemplate()
{
    winrt::IControlProtected controlProtected{ *this };
    
    m_listView.set(GetTemplateChildT<winrt::ListView>(L"InnerListView", controlProtected));
    if (auto listView = m_listView.get())
    {
        m_listViewLoadedRevoker = listView.Loaded(winrt::auto_revoke, { this, &RadioButtons::OnListViewLoaded });
        m_listViewSelectionChangedRevoker = listView.SelectionChanged(winrt::auto_revoke, { this, &RadioButtons::OnListViewSelectionChanged });
    }

    UpdateItemsSource();
}

void RadioButtons::OnListViewLoaded(const winrt::IInspectable& /*sender*/, const winrt::RoutedEventArgs&  /*args*/)
{
    if (auto listView = m_listView.get())
    {
        m_itemsWrapGrid.set(SharedHelpers::FindInVisualTreeByType<winrt::ItemsWrapGrid>(listView));
        if (auto itemsWrapGrid = m_itemsWrapGrid.get())
        {
            // Override normal up/down behavior -- down should always go to the next item and up to the previous.
            m_listViewKeyDownRevoker = itemsWrapGrid.KeyDown(winrt::auto_revoke, { this, &RadioButtons::OnListViewKeyDown });
            m_listViewKeyUpRevoker = itemsWrapGrid.KeyUp(winrt::auto_revoke, { this, &RadioButtons::OnListViewKeyUp });

            UpdateSelectedIndex();
            UpdateSelectedItem();
            UpdateMaximumColumns();
        }
    }
}

void RadioButtons::OnListViewSelectionChanged(const winrt::IInspectable& sender, const winrt::SelectionChangedEventArgs& args)
{
    if (auto listView = m_listView.get())
    {
        SelectedIndex(listView.SelectedIndex());
        SelectedItem(listView.SelectedItem());
    }

    m_selectionChangedEventSource(sender, args);
}

void RadioButtons::OnListViewKeyDown(const winrt::IInspectable&  /*sender*/, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Control)
    {
        m_isControlDown = true;
    }
    else if (args.Key() == winrt::VirtualKey::Down)
    {
        args.Handled(MoveSelection(1));
    }
    else if (args.Key() == winrt::VirtualKey::Up)
    {
        args.Handled(MoveSelection(-1));
    }
}

void RadioButtons::OnListViewKeyUp(const winrt::IInspectable&  /*sender*/, const winrt::KeyRoutedEventArgs& args)
{
    if (args.Key() == winrt::VirtualKey::Control)
    {
        m_isControlDown = false;
    }
}

bool RadioButtons::MoveSelection(int direction)
{
    bool found = false;

    if (auto listView = m_listView.get())
    {
        if (auto focusedElement = winrt::FocusManager::GetFocusedElement())
        {
            auto dpElement = focusedElement.as<winrt::DependencyObject>();
            int focusedIndex = listView.IndexFromContainer(dpElement);
            
            if (focusedIndex >= 0)
            {
                focusedIndex += direction;

                int itemCount = listView.Items().Size();

                while (focusedIndex >= 0 && focusedIndex < itemCount)
                {
                    if (auto item = listView.Items().GetAt(focusedIndex))
                    {
                        if (auto itemContainer = listView.ContainerFromItem(item))
                        {
                            if (auto itemContainerAsControl = itemContainer.as<winrt::Control>())
                            {
                                if (itemContainerAsControl.IsEnabled())
                                {
                                    if (m_isControlDown)
                                    {
                                        // only move focus
                                        itemContainerAsControl.Focus(winrt::Windows::UI::Xaml::FocusState::Keyboard);
                                        found = true;
                                    }
                                    else
                                    {
                                        // change selection
                                        listView.SelectedIndex(focusedIndex);
                                        found = true;
                                    }
                                    break;
                                }
                            }
                        }
                    }

                    focusedIndex += direction;
                }
            }
        }
    }

    return found;
}

void RadioButtons::OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::IDependencyProperty property = args.Property();

    if (property == s_ItemsProperty || property == s_ItemsSourceProperty)
    {
        UpdateItemsSource();
    }
    else if (property == s_MaximumColumnsProperty)
    {
        UpdateMaximumColumns();
    }
    else if (property == s_SelectedIndexProperty)
    {
        UpdateSelectedIndex();
    }
    else if (property == s_SelectedItemProperty)
    {
        UpdateSelectedItem();
    }
}

void RadioButtons::UpdateItemsSource()
{
    if (auto listView = m_listView.get())
    {
        if (ItemsSource())
        {
            listView.ItemsSource(ItemsSource());
        }
        else
        {
            listView.ItemsSource(Items());
        }
    }
}

void RadioButtons::UpdateMaximumColumns()
{
    if (auto itemsWrapGrid = m_itemsWrapGrid.get())
    {
        itemsWrapGrid.MaximumRowsOrColumns(MaximumColumns());
    }
}

void RadioButtons::UpdateSelectedItem()
{
    if (auto listView = m_listView.get())
    {
        // Setting ListView.SelectedItem will not work here in all cases.
        // The reason why that doesn't work but this does is unknown.
        auto container = listView.ContainerFromItem(SelectedItem());
        if (auto lvi = container.as<winrt::ListViewItem>())
        {
            lvi.IsSelected(true);
        }
    }
}

void RadioButtons::UpdateSelectedIndex()
{
    if (auto listView = m_listView.get())
    {
        listView.SelectedIndex(SelectedIndex());
    }
}

winrt::DependencyObject RadioButtons::ContainerFromItem(winrt::IInspectable const& item)
{
    if (auto listView = m_listView.get())
    {
        return listView.ContainerFromItem(item);
    }
    return nullptr;
}

winrt::DependencyObject RadioButtons::ContainerFromIndex(int index)
{
    if (auto listView = m_listView.get())
    {
        return listView.ContainerFromIndex(index);
    }
    return nullptr;
}
