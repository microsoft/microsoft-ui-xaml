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
    m_isFactoryCreatedProperty =
        InitializeDependencyProperty(
            s_isFactoryCreatedPropertyName,
            winrt::name_of<bool>(),
            winrt::name_of<winrt::RadioButtons>(),
            true /* isAttached */,
            box_value(false));
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
    auto const itemTemplateWrapper = m_itemTemplateWrapper;
    auto const newContent = [itemTemplateWrapper, args]() {
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

    // Element is not a RadioButton. Reuse or create a RadioButton wrapper.
    auto const newRadioButton = [this, parent = args.Parent()]()
    {
        if (!m_radioButtonPool.empty())
        {
            auto radioButton = m_radioButtonPool.back();
            if (winrt::VisualTreeHelper::GetParent(radioButton) == parent)
            {
                m_radioButtonPool.pop_back();
                return radioButton;
            }

            m_radioButtonPool.clear();
        }

        auto radioButton = winrt::RadioButton{};
        radioButton.SetValue(m_isFactoryCreatedProperty, box_value(true));
        return radioButton;
    }();

    newRadioButton.Content(args.Data());

    // If a user provided item template exists, we pass the template down to the ContentPresenter of the RadioButton.
    if (auto const itemTemplateWrapperImpl = itemTemplateWrapper.try_as<ItemTemplateWrapper>())
    {
        newRadioButton.ContentTemplate(itemTemplateWrapperImpl->Template());
    }
    else
    {
        newRadioButton.ContentTemplate(nullptr);
    }

    return newRadioButton;
}

void RadioButtonsElementFactory::RecycleElementCore(const winrt::ElementFactoryRecycleArgs& args)
{
    if (auto const element = args.Element())
    {
        auto const isFactoryCreated = unbox_value<bool>(element.GetValue(m_isFactoryCreatedProperty));

        if (isFactoryCreated)
        {
            if (auto const radioButton = element.try_as<winrt::RadioButton>())
            {
                radioButton.IsChecked(false);
                radioButton.Content(nullptr);
                radioButton.ContentTemplate(nullptr);
                m_radioButtonPool.push_back(radioButton);
            }
        }
        else if (m_itemTemplateWrapper)
        {
            m_itemTemplateWrapper.RecycleElement(args);
        }
    }
}
