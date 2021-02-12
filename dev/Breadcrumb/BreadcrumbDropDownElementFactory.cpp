// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemTemplateWrapper.h"
#include "RuntimeProfiler.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "BreadcrumbDropDownElementFactory.h"
#include "BreadcrumbDropDownItem.h"
#include "ElementFactoryRecycleArgs.h"

BreadcrumbDropDownElementFactory::BreadcrumbDropDownElementFactory()
{
}

void BreadcrumbDropDownElementFactory::UserElementFactory(const winrt::IInspectable& newValue)
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

winrt::UIElement BreadcrumbDropDownElementFactory::GetElementCore(const winrt::ElementFactoryGetArgs& args)
{
    auto const newContent = [itemTemplateWrapper = m_itemTemplateWrapper, args]() {

        if (args.Data().try_as<winrt::BreadcrumbDropDownItem>())
        {
            return args.Data();
        }

        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();
  
    // Element is already a BreadcrumbItem, so we just return it.
    if (auto const breadcrumbItem = newContent.try_as<winrt::BreadcrumbDropDownItem>())
    {
        return breadcrumbItem;
    }

    auto const newBreadcrumbItem = winrt::BreadcrumbDropDownItem{};
    newBreadcrumbItem.Content(args.Data());

    // If a user provided item template exists, we pass the template down
    // to the ContentPresenter of the BreadcrumbItem.
    if (auto const itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        newBreadcrumbItem.ContentTemplate(itemTemplateWrapper->Template());
    }
    
    return newBreadcrumbItem;
}

void BreadcrumbDropDownElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{
    if (auto element = args.Element())
    {
        if (auto breadcrumbItem = element.try_as<winrt::BreadcrumbDropDownItem>())
        {
            auto breadcrumbItemImpl = winrt::get_self<BreadcrumbDropDownItem>(breadcrumbItem);
            // breadcrumbItemImpl->ResetVisualProperties();
        }
    }

    m_itemTemplateWrapper.RecycleElement(args);
}
