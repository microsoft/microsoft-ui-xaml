// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemTemplateWrapper.h"
#include "BreadcrumbBarElementFactory.h"
#include "BreadcrumbBarItem.h"
#include "ElementFactoryRecycleArgs.h"

BreadcrumbElementFactory::BreadcrumbElementFactory()
{
}

void BreadcrumbElementFactory::UserElementFactory(const winrt::IInspectable& newValue)
{
    m_itemTemplateWrapper = newValue.try_as<winrt::IElementFactoryShim>();
    if (!m_itemTemplateWrapper)
    {
        // ItemTemplate set does not implement IElementFactoryShim. We also want to support DataTemplate.
        if (auto const dataTemplate = newValue.try_as<winrt::DataTemplate>())
        {
            m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(dataTemplate);
        }
    }
}

winrt::UIElement BreadcrumbElementFactory::GetElementCore(const winrt::ElementFactoryGetArgs& args)
{
    auto const newContent = [itemTemplateWrapper = m_itemTemplateWrapper, args]()
    {
        if (args.Data().try_as<winrt::BreadcrumbBarItem>())
        {
            return args.Data();
        }

        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();
  
    // Element is already a BreadcrumbBarItem, so we just return it.
    if (auto const breadcrumbItem = newContent.try_as<winrt::BreadcrumbBarItem>())
    {
        // When the list has not changed the returned item is still a BreadcrumbBarItem but the
        // item is not reset, so we set the content here
        breadcrumbItem.Content(args.Data());
        return breadcrumbItem;
    }

    auto const newBreadcrumbBarItem = winrt::BreadcrumbBarItem{};
    newBreadcrumbBarItem.Content(args.Data());

    // If a user provided item template exists, we pass the template down
    // to the ContentPresenter of the BreadcrumbBarItem.
    if (auto const itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        newBreadcrumbBarItem.ContentTemplate(itemTemplateWrapper->Template());
    }
    
    return newBreadcrumbBarItem;
}

void BreadcrumbElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{
    if (auto element = args.Element())
    {
        bool isEllipsisDropDownItem = false; // Use of isEllipsisDropDownItem is workaround for
        // crashing bug when attempting to show ellipsis dropdown after clicking one of its items.

        if (auto breadcrumbItem = element.try_as<winrt::BreadcrumbBarItem>())
        {
            auto breadcrumbItemImpl = winrt::get_self<BreadcrumbBarItem>(breadcrumbItem);
            breadcrumbItemImpl->ResetVisualProperties();

            isEllipsisDropDownItem = breadcrumbItemImpl->IsEllipsisDropDownItem();
        }

        if (m_itemTemplateWrapper && isEllipsisDropDownItem)
        {
            m_itemTemplateWrapper.RecycleElement(args);
        }
    }
}
