// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemTemplateWrapper.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "BreadcrumbElementFactory.h"

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
    auto const newContent = [itemTemplateWrapper = m_itemTemplateWrapper, args]() {
        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();

    // Element is already a BreadcrumbItem, so we just return it.
    if (auto const breadcrumbItem = newContent.try_as<winrt::BreadcrumbItem>())
    {
        return breadcrumbItem;
    }

    // Element is not a BreadcrumbItem. We'll wrap it in a BreadcrumbItem now.
    auto const newBreadcrumbItem = winrt::BreadcrumbItem{};
    newBreadcrumbItem.Content(args.Data());

    // If a user provided item template exists, we pass the template down to the ContentPresenter of the RadioButton.
    if (auto const itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        newBreadcrumbItem.ContentTemplate(itemTemplateWrapper->Template());
    }

    return newBreadcrumbItem;

    /*
    if (auto sentinel = args.Data().try_as<winrt::hstring>())
    {
        if (winrt::to_string(*sentinel) == "Sentinel")
        {
            auto ellipsisButton = winrt::make<BreadcrumbItem>();
            ellipsisButton.Content(winrt::box_value(L"..."));

            return ellipsisButton;
        }
    }

    if (auto innerTemplate = m_innerTemplate.get())
    {
        if (auto elementFactory = innerTemplate.try_as<winrt::IElementFactoryShim>())
        {
            auto getArgs = winrt::make<winrt::ElementFactoryGetArgs>();
            getArgs->Data(args.Data());

            elementFactory.GetElement(getArgs);
        }
    }

    return nullptr;
    */
}

void BreadcrumbElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{

}
