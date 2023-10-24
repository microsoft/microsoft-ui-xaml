// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ResourceAccessor.h"
#include "ItemContainer.h"
#include "ItemContainerAutomationPeer.h"
#include "ItemContainerAutomationPeer.properties.cpp"
#include "../ResourceHelper/ResourceAccessor.h"

ItemContainerAutomationPeer::ItemContainerAutomationPeer(winrt::ItemContainer const& owner)
    : ReferenceTracker(owner)
{
}

// IAutomationPeerOverrides
winrt::hstring ItemContainerAutomationPeer::GetNameCore()
{
    winrt::hstring returnHString = __super::GetNameCore();

    // If a name hasn't been provided by AutomationProperties.Name in markup:
    if (returnHString.empty())
    {
        if (auto itemContainer = Owner().try_as<winrt::ItemContainer>())
        {
            returnHString = SharedHelpers::TryGetStringRepresentationFromObject(itemContainer.Child());
        }
    }

    if (returnHString.empty())
    {
        returnHString = ResourceAccessor::GetLocalizedStringResource(SR_ItemContainerDefaultControlName);
    }

    return returnHString;
}

winrt::IInspectable ItemContainerAutomationPeer::GetPatternCore(winrt::PatternInterface const& patternInterface)
{
    if (auto itemContainer = Owner().try_as<winrt::ItemContainer>())
    {           
        if (patternInterface == winrt::PatternInterface::SelectionItem)            
        {
#ifdef MUX_PRERELEASE
            const bool canUserSelect = static_cast<int>(itemContainer.CanUserSelect() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#else
            const bool canUserSelect = static_cast<int>(GetImpl()->CanUserSelectInternal() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#endif

            if (canUserSelect)
            {
                return *this;
            }
            
        }
        else if (patternInterface == winrt::PatternInterface::Invoke)
        {
#ifdef MUX_PRERELEASE
            const bool canUserInvoke = static_cast<int>(itemContainer.CanUserInvoke() & winrt::ItemContainerUserInvokeMode::UserCanInvoke);
#else
            const bool canUserInvoke = static_cast<int>(GetImpl()->CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::UserCanInvoke);
#endif

            if (canUserInvoke)
            {
                return *this;
            }
        }
    }

    return __super::GetPatternCore(patternInterface);
}

winrt::AutomationControlType ItemContainerAutomationPeer::GetAutomationControlTypeCore()
{
    return winrt::AutomationControlType::ListItem;
}

winrt::hstring ItemContainerAutomationPeer::GetLocalizedControlTypeCore()
{
    return ResourceAccessor::GetLocalizedStringResource(SR_ItemContainerDefaultControlName);
}

winrt::hstring ItemContainerAutomationPeer::GetClassNameCore()
{
    return winrt::hstring_name_of<winrt::ItemContainer>();
}

// IInvokeProvider
void ItemContainerAutomationPeer::Invoke()
{
    if (auto itemContainer = GetImpl())
    {
#ifdef MUX_PRERELEASE
        const bool canUserInvoke = static_cast<int>(itemContainer->CanUserInvoke() & winrt::ItemContainerUserInvokeMode::UserCanInvoke);
#else
        const bool canUserInvoke = static_cast<int>(itemContainer->CanUserInvokeInternal() & winrt::ItemContainerUserInvokeMode::UserCanInvoke);
#endif

        if (canUserInvoke)
        {
            itemContainer->RaiseItemInvoked(winrt::ItemContainerInteractionTrigger::AutomationInvoke, nullptr);
        }
    }
}

//ISelectionItemProvider
bool ItemContainerAutomationPeer::IsSelected()
{
    if (auto itemContainer = GetImpl())
    {
        return itemContainer->IsSelected();
    }
    return false;
}

winrt::IRawElementProviderSimple ItemContainerAutomationPeer::SelectionContainer()
{
    if (auto itemContainer = GetImpl())
    {
        auto const selectionContainer = [itemContainer]()
        {
            auto parent = winrt::VisualTreeHelper::GetParent(*itemContainer);

            while (parent && !parent.try_as<winrt::ItemsView>())
            {
                parent = winrt::VisualTreeHelper::GetParent(parent);
            }

            return parent.as<winrt::UIElement>();
        }();

        if (selectionContainer)
        {
            if (auto const peer = winrt::FrameworkElementAutomationPeer::CreatePeerForElement(selectionContainer))
            {
                return ProviderFromPeer(peer);
            }
        }
    }

    return nullptr;
}

void ItemContainerAutomationPeer::AddToSelection()
{
    UpdateSelection(true);
}

void ItemContainerAutomationPeer::RemoveFromSelection()
{
    UpdateSelection(false);
}

void ItemContainerAutomationPeer::Select()
{
    UpdateSelection(true);
}

com_ptr<ItemContainer> ItemContainerAutomationPeer::GetImpl()
{
    com_ptr<ItemContainer> impl = nullptr;

    if (auto itemContainer = Owner().try_as<winrt::ItemContainer>())
    {
        impl = winrt::get_self<ItemContainer>(itemContainer)->get_strong();
    }

    return impl;
}

void ItemContainerAutomationPeer::UpdateSelection(bool isSelected)
{
    if (auto itemContainer = GetImpl())
    {
        if (isSelected)
        {
#ifdef MUX_PRERELEASE
            const bool canUserSelect = static_cast<int>(itemContainer->CanUserSelect() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#else
            const bool canUserSelect = static_cast<int>(itemContainer->CanUserSelectInternal() & (winrt::ItemContainerUserSelectMode::Auto | winrt::ItemContainerUserSelectMode::UserCanSelect));
#endif

            if (canUserSelect)
            {
                itemContainer->IsSelected(true);
            }
        }
        else
        {
            itemContainer->IsSelected(false);
        }
    }
}
