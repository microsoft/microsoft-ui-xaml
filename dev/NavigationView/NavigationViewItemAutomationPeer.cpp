// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "NavigationViewItemAutomationPeer.h"
#include "NavigationView.h"
#include "NavigationViewItemBase.h"
#include "SharedHelpers.h"
#include "NavigationViewHelper.h"


#include "NavigationViewItemAutomationPeer.properties.cpp"

NavigationViewItemAutomationPeer::NavigationViewItemAutomationPeer(winrt::NavigationViewItem const& owner) :
    ReferenceTracker(owner)
{
}

//IAutomationPeerOverrides

winrt::hstring NavigationViewItemAutomationPeer::GetNameCore()
{
    winrt::hstring returnHString = __super::GetNameCore();

    // If a name hasn't been provided by AutomationProperties.Name in markup:
    if (returnHString.empty())
    {
        if (auto lvi = Owner().try_as<winrt::NavigationViewItem>())
        {
            returnHString = SharedHelpers::TryGetStringRepresentationFromObject(lvi.Content());
        }
    }

    if (returnHString.empty())
    {
        // NB: It'll be up to the app to determine the automation label for
        // when they're using a PlaceholderValue vs. Value.

        returnHString = ResourceAccessor::GetLocalizedStringResource(SR_NavigationViewItemDefaultControlName);
    }

    return returnHString;
}

winrt::IInspectable NavigationViewItemAutomationPeer::GetPatternCore(winrt::PatternInterface const& pattern)
{
    // Note: We are intentionally not supporting Invoke Pattern, since supporting both SelectionItem and Invoke was
    // causing problems. 
    // See this Issue for more details: https://github.com/microsoft/microsoft-ui-xaml/issues/2702
    if (pattern == winrt::PatternInterface::SelectionItem ||
        // Only provide expand collapse pattern if we have children!
        (pattern == winrt::PatternInterface::ExpandCollapse && HasChildren()))
    {
        return *this;
    }

    return __super::GetPatternCore(pattern);
}

winrt::hstring  NavigationViewItemAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::NavigationViewItem>();
}

winrt::AutomationControlType NavigationViewItemAutomationPeer::GetAutomationControlTypeCore()
{
    // To be compliant with MAS 4.1.2, in DisplayMode 'Top',
    //  a NavigationViewItem should report itsself as TabItem
    if (IsOnTopNavigation())
    {
        return winrt::AutomationControlType::TabItem;
    }
    else
    {
        // TODO: Should this be ListItem in minimal mode and
        // TreeItem otherwise.
        return winrt::AutomationControlType::ListItem;
    }
}


int32_t NavigationViewItemAutomationPeer::GetPositionInSetCore()
{
    return GetPositionOrSetCountHelper(AutomationOutput::Position);
}

int32_t NavigationViewItemAutomationPeer::GetSizeOfSetCore()
{
    return GetPositionOrSetCountHelper(AutomationOutput::Size);
}

int32_t NavigationViewItemAutomationPeer::GetLevelCore()
{
    if (winrt::NavigationViewItemBase nvib = Owner().try_as<winrt::NavigationViewItemBase>())
    {
        auto const nvibImpl = winrt::get_self<NavigationViewItemBase>(nvib);
        if (nvibImpl->IsTopLevelItem())
        {
            return 1;
        }
        else
        {
            if (auto const navView = GetParentNavigationView())
            {
                if (auto const indexPath = winrt::get_self<NavigationView>(navView)->GetIndexPathForContainer(nvib))
                {
                    // first index in path stands for main or footer menu
                    return indexPath.GetSize() - 1;
                }
            }
        }
    }

    return 0;
}

void NavigationViewItemAutomationPeer::Invoke()
{
    if (auto const navView = GetParentNavigationView())
    {
        if (auto const navigationViewItem = Owner().try_as<winrt::NavigationViewItem>())
        {
            if (navigationViewItem == navView.SettingsItem())
            {
                winrt::get_self<NavigationView>(navView)->OnSettingsInvoked();
            }
            else
            {
                winrt::get_self<NavigationView>(navView)->OnNavigationViewItemInvoked(navigationViewItem);
            }
        }
    }
}

// IExpandCollapseProvider 
winrt::ExpandCollapseState NavigationViewItemAutomationPeer::ExpandCollapseState()
{
    auto state = winrt::ExpandCollapseState::LeafNode;
    if (winrt::NavigationViewItem navigationViewItem = Owner().try_as<winrt::NavigationViewItem>())
    {
        state = winrt::get_self<NavigationViewItem>(navigationViewItem)->IsExpanded() ?
            winrt::ExpandCollapseState::Expanded :
            winrt::ExpandCollapseState::Collapsed;
    }

    return state;
}

void NavigationViewItemAutomationPeer::Collapse()
{
    if (auto const navView = GetParentNavigationView())
    {
        if (winrt::NavigationViewItem navigationViewItem = Owner().try_as<winrt::NavigationViewItem>())
        {
            navView.Collapse(navigationViewItem);
            RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Collapsed);
        }
    }
}

void NavigationViewItemAutomationPeer::Expand()
{
    if (auto const navView = GetParentNavigationView())
    {
        if (winrt::NavigationViewItem navigationViewItem = Owner().try_as<winrt::NavigationViewItem>())
        {
            navView.Expand(navigationViewItem);
            RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState::Expanded);
        }
    }
}

void NavigationViewItemAutomationPeer::RaiseExpandCollapseAutomationEvent(winrt::ExpandCollapseState newState)
{
    if (winrt::AutomationPeer::ListenerExists(winrt::AutomationEvents::PropertyChanged))
    {
        const winrt::ExpandCollapseState oldState = (newState == winrt::ExpandCollapseState::Expanded) ?
            winrt::ExpandCollapseState::Collapsed :
            winrt::ExpandCollapseState::Expanded;

        // box_value(oldState) doesn't work here, use ReferenceWithABIRuntimeClassName to make Narrator can unbox it.
        RaisePropertyChangedEvent(winrt::ExpandCollapsePatternIdentifiers::ExpandCollapseStateProperty(),
            box_value(oldState),
            box_value(newState));
    }
}



winrt::NavigationView NavigationViewItemAutomationPeer::GetParentNavigationView()
{
    winrt::NavigationView navigationView{ nullptr };

    winrt::NavigationViewItemBase navigationViewItem = Owner().try_as<winrt::NavigationViewItemBase>();
    if (navigationViewItem)
    {
        navigationView = winrt::get_self<NavigationViewItemBase>(navigationViewItem)->GetNavigationView();
    }
    return navigationView;
}

int32_t NavigationViewItemAutomationPeer::GetNavigationViewItemCountInPrimaryList()
{
    int32_t count = 0;
    if (auto navigationView = GetParentNavigationView())
    {
        count = winrt::get_self<NavigationView>(navigationView)->GetNavigationViewItemCountInPrimaryList();
    }
    return count;
}

int32_t NavigationViewItemAutomationPeer::GetNavigationViewItemCountInTopNav()
{
    int32_t count = 0;
    if (auto navigationView = GetParentNavigationView())
    {
        count = winrt::get_self<NavigationView>(navigationView)->GetNavigationViewItemCountInTopNav();
    }
    return count;
}

bool NavigationViewItemAutomationPeer::IsSettingsItem()
{
    if (auto navView = GetParentNavigationView())
    {
        winrt::NavigationViewItem item = Owner().try_as<winrt::NavigationViewItem>();
        auto settingsItem = navView.SettingsItem();
        if (item && settingsItem && (item == settingsItem || item.Content() == settingsItem))
        {
            return true;
        }
    }
    return false;
}

bool NavigationViewItemAutomationPeer::IsOnTopNavigation()
{
    const auto position = GetNavigationViewRepeaterPosition();
    return position != NavigationViewRepeaterPosition::LeftNav && position != NavigationViewRepeaterPosition::LeftFooter;
}

bool NavigationViewItemAutomationPeer::IsOnTopNavigationOverflow()
{
    return GetNavigationViewRepeaterPosition() == NavigationViewRepeaterPosition::TopOverflow;
}

bool NavigationViewItemAutomationPeer::IsOnFooterNavigation()
{
    const auto position = GetNavigationViewRepeaterPosition();
    return position == NavigationViewRepeaterPosition::LeftFooter || position == NavigationViewRepeaterPosition::TopFooter;
}

NavigationViewRepeaterPosition NavigationViewItemAutomationPeer::GetNavigationViewRepeaterPosition()
{
    if (winrt::NavigationViewItemBase navigationViewItem = Owner().try_as<winrt::NavigationViewItemBase>())
    {
        return winrt::get_self<NavigationViewItemBase>(navigationViewItem)->Position();
    }
    return NavigationViewRepeaterPosition::LeftNav;
}

winrt::ItemsRepeater NavigationViewItemAutomationPeer::GetParentItemsRepeater()
{
    if (auto const navview = GetParentNavigationView())
    {
        if (winrt::NavigationViewItemBase navigationViewItem = Owner().try_as<winrt::NavigationViewItemBase>())
        {
            return winrt::get_self<NavigationView>(navview)->GetParentItemsRepeaterForContainer(navigationViewItem);
        }
    }
    return nullptr;
}

// Get either the position or the size of the set for this particular item by iterating through the children of the
// parent items repeater and comparing the value of the FrameworkElementAutomationPeer we can get from the item
// we're iterating through to this object.
int32_t NavigationViewItemAutomationPeer::GetPositionOrSetCountHelper(AutomationOutput automationOutput)
{
    int32_t returnValue = 0;
    bool itemFound = false;

    if (auto const parentRepeater = GetParentItemsRepeater())
    {
        if (auto const itemsSourceView = parentRepeater.ItemsSourceView())
        {
            auto const numberOfElements = itemsSourceView.Count();

            for (int32_t i = 0; i < numberOfElements; i++)
            {
                if (auto child = parentRepeater.TryGetElement(i))
                {
                    if (child.try_as<winrt::NavigationViewItemHeader>())
                    {
                        if (automationOutput == AutomationOutput::Size && itemFound)
                        {
                            break;
                        }
                        else
                        {
                            returnValue = 0;
                        }
                    }
                    else if (auto const navviewitem = child.try_as<winrt::NavigationViewItem>())
                    {
                        if (navviewitem.Visibility() == winrt::Visibility::Visible)
                        {
                            returnValue++;

                            if (winrt::FrameworkElementAutomationPeer::FromElement(navviewitem) == static_cast<winrt::NavigationViewItemAutomationPeer>(*this))
                            {
                                if (automationOutput == AutomationOutput::Position)
                                {
                                    break;
                                }
                                else
                                {
                                    itemFound = true;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    return returnValue;
}

bool NavigationViewItemAutomationPeer::IsSelected()
{
    if (auto const nvi = Owner().try_as<winrt::NavigationViewItem>())
    {
        return nvi.IsSelected();
    }
    return false;
}

winrt::IRawElementProviderSimple NavigationViewItemAutomationPeer::SelectionContainer()
{
    if (auto const navview = GetParentNavigationView())
    {
        if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(navview))
        {
            return ProviderFromPeer(peer);
        }
    }

    return nullptr;
}

void NavigationViewItemAutomationPeer::AddToSelection()
{
    ChangeSelection(true);
}

void NavigationViewItemAutomationPeer::Select()
{
    ChangeSelection(true);
}

void NavigationViewItemAutomationPeer::RemoveFromSelection()
{
    ChangeSelection(false);
}

void NavigationViewItemAutomationPeer::ChangeSelection(bool isSelected)
{
    // If the item is being selected, we trigger an invoke as if the user had clicked on the item:
    if(isSelected)
    {
        Invoke();
    }
    if (auto nvi = Owner().try_as<winrt::NavigationViewItem>())
    {
        nvi.IsSelected(isSelected);
    }
}

bool NavigationViewItemAutomationPeer::HasChildren()
{
    if (const auto& navigationViewItem = Owner().try_as<NavigationViewItem>())
    {
        return navigationViewItem->HasChildren();
    }
    return false;
}
