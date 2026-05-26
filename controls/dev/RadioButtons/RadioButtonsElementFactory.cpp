// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"
#include "ItemTemplateWrapper.h"
#include "ResourceAccessor.h"
#include "Utils.h"
#include "RadioButtonsElementFactory.h"

RadioButtonsElementFactory::RadioButtonsElementFactory()
{
}

void RadioButtonsElementFactory::UserElementFactory(const winrt::IInspectable& newValue)
{
    if (auto dataTemplate = newValue.try_as<winrt::DataTemplate>())
    {
        m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(dataTemplate);        
    }
    else if (auto selector = newValue.try_as<winrt::DataTemplateSelector>())
    {
        m_itemTemplateWrapper = winrt::make<ItemTemplateWrapper>(selector);
    }
    else if (auto customElementFactory = newValue.try_as<winrt::IElementFactory>())
    {
        m_itemTemplateWrapper = customElementFactory;
    }
}

winrt::UIElement RadioButtonsElementFactory::GetElementCore(const winrt::ElementFactoryGetArgs& args)
{
    auto const newContent = [itemTemplateWrapper = m_itemTemplateWrapper, args]() {
        if (itemTemplateWrapper)
        {
            return itemTemplateWrapper.GetElement(args).as<winrt::IInspectable>();
        }
        return args.Data();
    }();

    // Element is already a RadioButton, so we just return it.
    if (auto const radioButton = newContent.try_as<winrt::RadioButton>())
    {
        return radioButton;
    }

    // Element is not a RadioButton. We'll wrap it in a RadioButton now.
    auto const newRadioButton = winrt::RadioButton{};
    newRadioButton.Content(args.Data());

    // If a user provided item template exists, we pass the template down to the ContentPresenter of the RadioButton.
    if (auto const itemTemplateWrapper = m_itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        newRadioButton.ContentTemplate(itemTemplateWrapper->Template());
    }

    return newRadioButton;
}

void RadioButtonsElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{

}
