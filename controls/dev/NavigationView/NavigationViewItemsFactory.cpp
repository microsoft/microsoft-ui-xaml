// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "NavigationViewItemsFactory.h"
#include "NavigationViewItemBase.h"
#include "NavigationViewItem.h"
#include "ItemTemplateWrapper.h"

void NavigationViewItemsFactory::UserElementFactory(
    winrt::IInspectable const& newValue,
    const ITrackerHandleManager* owner)
{
    if (m_isTerminated)
    {
        return;
    }

    winrt::IElementFactory newItemTemplateWrapper{ nullptr };
    if (auto dataTemplate = newValue.try_as<winrt::DataTemplate>())
    {
        auto itemTemplateWrapper = winrt::make_self<ItemTemplateWrapper>(dataTemplate);
        itemTemplateWrapper->EnableTracking(owner);
        newItemTemplateWrapper = itemTemplateWrapper.as<winrt::IElementFactory>();
    }
    else if (auto selector = newValue.try_as<winrt::DataTemplateSelector>())
    {
        auto itemTemplateWrapper = winrt::make_self<ItemTemplateWrapper>(selector);
        itemTemplateWrapper->EnableTracking(owner);
        newItemTemplateWrapper = itemTemplateWrapper.as<winrt::IElementFactory>();
    }
    else if (auto customElementFactory = newValue.try_as<winrt::IElementFactory>())
    {
        newItemTemplateWrapper = customElementFactory;
    }

    ResetItemTemplateWrapper();
    m_itemTemplateWrapper = newItemTemplateWrapper;
    navigationViewItemPool = std::vector<winrt::NavigationViewItem>();
}

void NavigationViewItemsFactory::SettingsItem(winrt::NavigationViewItemBase const& settingsItem)
{
    if (!m_isTerminated)
    {
        m_settingsItem = settingsItem;
    }
}

void NavigationViewItemsFactory::ResetForOwnerDestruction() noexcept
{
    m_isTerminated = true;
    ResetItemTemplateWrapper();
    m_settingsItem = nullptr;
    navigationViewItemPool.clear();
}

void NavigationViewItemsFactory::ResetItemTemplateWrapper() noexcept
{
    if (auto const itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        itemTemplateWrapper->ResetTracking();
    }

    m_itemTemplateWrapper = nullptr;
}


// Retrieve the element that will be displayed for a specific data item.
// If the resolved element is not derived from NavigationViewItemBase, wrap in a NavigationViewItem before returning.
winrt::UIElement NavigationViewItemsFactory::GetElementCore(winrt::ElementFactoryGetArgs const& args)
{
    auto const itemTemplateWrapper = m_itemTemplateWrapper;
    auto const settingsItem = m_settingsItem;
    auto const wrapper = itemTemplateWrapper.try_as<ItemTemplateWrapper>();
    auto const contentTemplate = wrapper ? wrapper->Template() : nullptr;
    auto const contentTemplateSelector = wrapper ? wrapper->TemplateSelector() : nullptr;

    auto const newContent = [itemTemplateWrapper, settingsItem, args]() {
        // Do not template SettingsItem
        if (settingsItem && settingsItem == args.Data())
        {
            return args.Data();
        }

        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();

    // Element is already of expected type, just return it
    if (auto const newItem = newContent.try_as<winrt::NavigationViewItemBase>())
    {
        return newItem;
    }

    // Get or create a wrapping container for the data
    auto const nvi = [this]() {
        if (navigationViewItemPool.size() > 0)
        {
            auto nvi = navigationViewItemPool.back();
            navigationViewItemPool.pop_back();
            return nvi;
        }
        return winrt::make<NavigationViewItem>();
    }();
    auto const nviImpl = winrt::get_self<NavigationViewItem>(nvi);
    nviImpl->CreatedByNavigationViewItemsFactory(true);

    // If a user provided item template exists, just pass the template and data down to the ContentPresenter of the NavigationViewItem
    if (itemTemplateWrapper)
    {
        if (wrapper)
        {
            // Recycle newContent
            auto const tempArgs = winrt::ElementFactoryRecycleArgs();
            tempArgs.Element(newContent.try_as<winrt::UIElement>());
            itemTemplateWrapper.RecycleElement(tempArgs);


            nviImpl->Content(args.Data());
            nviImpl->ContentTemplate(contentTemplate);
            nviImpl->ContentTemplateSelector(contentTemplateSelector);
            return *nviImpl;
        }
    }

    nviImpl->Content(newContent);
    return *nviImpl;
}

void NavigationViewItemsFactory::RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args)
{
    auto const itemTemplateWrapper = m_itemTemplateWrapper;
    auto const settingsItem = m_settingsItem;

    if (auto element = args.Element())
    {
        if (auto nvi = element.try_as<winrt::NavigationViewItem>())
        {
            auto const nviImpl = winrt::get_self<NavigationViewItem>(nvi);
            // Check whether we wrapped the element in a NavigationViewItem ourselves.
            // If yes, we are responsible for recycling it.
            if (nviImpl->CreatedByNavigationViewItemsFactory())
            {
                nviImpl->CreatedByNavigationViewItemsFactory(false);
                UnlinkElementFromParent(args);
                args.Element(nullptr);

                if (!m_isTerminated)
                {
                    // Retain the NVI that we created for future re-use
                    navigationViewItemPool.push_back(nvi);
                }

                // Retrieve the proper element that requires recycling for a user defined item template
                // and update the args correspondingly
                if (itemTemplateWrapper)
                {
                    // TODO: Retrieve the element and add to the args
                }
            }
        }

        // Do not recycle SettingsItem
        bool isSettingsItem = settingsItem && settingsItem == args.Element();
        
        UnlinkElementFromParent(args);
        if (itemTemplateWrapper && !isSettingsItem && !m_isTerminated)
        {
            itemTemplateWrapper.RecycleElement(args);
        }
    }
}

void NavigationViewItemsFactory::UnlinkElementFromParent(winrt::ElementFactoryRecycleArgs const& args)
{
    // We want to unlink the containers from the parent repeater
    // in case we are required to move it to a different repeater.
    if (auto panel = args.Parent().try_as<winrt::Panel>())
    {
        auto children = panel.Children();
        unsigned int childIndex = 0;
        if (children.IndexOf(args.Element(), childIndex))
        {
            children.RemoveAt(childIndex);
            args.Parent(nullptr);
        }
    }
}
