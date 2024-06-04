// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ItemsView.h"
#include "ItemsViewTestHooks.h"
#include "Utils.h"
#include "ItemsViewAutomationPeer.h"
#include "ItemsViewAutomationPeer.properties.cpp"

ItemsViewAutomationPeer::ItemsViewAutomationPeer(winrt::ItemsView const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::IInspectable ItemsViewAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (const auto itemsView = Owner().try_as<winrt::ItemsView>())
    {
        if (patternInterface == winrt::PatternInterface::Selection && itemsView.SelectionMode() != winrt::ItemsViewSelectionMode::None)
        {
            return *this;
        }
    }

    return __super::GetPatternCore(patternInterface);
}

hstring ItemsViewAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ItemsView>();
}

winrt::AutomationControlType ItemsViewAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::List;
}

// ISelectionProvider
bool ItemsViewAutomationPeer::CanSelectMultiple()
{
    if (const auto itemsView = GetImpl())
    {
        const auto selectionMode = itemsView->SelectionMode();
        if (selectionMode == winrt::ItemsViewSelectionMode::Multiple ||
            selectionMode == winrt::ItemsViewSelectionMode::Extended)
        {
            return true;
        }
    }

    return false;
}

winrt::com_array<winrt::IRawElementProviderSimple> ItemsViewAutomationPeer::GetSelection()
{
    std::vector<winrt::IRawElementProviderSimple> selectionList;

    if (const auto itemsView = GetImpl())
    {
        if (const auto selectionModel = itemsView->GetSelectionModel())
        {
            if (const auto selectedIndices = selectionModel.SelectedIndices())
            {
                if (selectedIndices.Size() > 0)
                {
                    if (const auto repeater = ItemsViewTestHooks::GetItemsRepeaterPart(*itemsView))
                    {
                        for (const auto indexPath : selectedIndices)
                        {
                            // TODO: Update once ItemsView has grouping.
                            const auto index = indexPath.GetAt(0);

                            if (const auto itemElement = repeater.TryGetElement(index))
                            {
                                if (const auto peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(itemElement))
                                {
                                    selectionList.push_back(ProviderFromPeer(peer));
                                }
                            }
                        }
                    } 
                }
            }
        }
    }

    return winrt::com_array(selectionList);
}

void ItemsViewAutomationPeer::RaiseSelectionChanged(double oldIndex, double newIndex)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::SelectionPatternOnInvalidated))
    {
        if (const auto itemsView = Owner().try_as<winrt::ItemsView>())
        {
            if (const auto peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(itemsView))
            {
                peer.RaiseAutomationEvent(winrt::AutomationEvents::SelectionPatternOnInvalidated);
            }
        }
        
    }
}

com_ptr<ItemsView> ItemsViewAutomationPeer::GetImpl()
{
    com_ptr<ItemsView> impl = nullptr;

    if (const auto itemsView = Owner().try_as<winrt::ItemsView>())
    {
        impl = winrt::get_self<ItemsView>(itemsView)->get_strong();
    }

    return impl;
}
