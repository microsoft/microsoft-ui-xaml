// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include <HashMap.h>
#include "SelectTemplateEventArgs.h"
#include "RecyclingElementFactory.h"
#include "ItemsRepeater.h"
#include "RecyclePool.h"

#include "RecyclingElementFactory.properties.cpp"

RecyclingElementFactory::RecyclingElementFactory()
{
    m_templates.set(winrt::make<HashMap<winrt::hstring, winrt::DataTemplate>>());
}

#pragma region IRecyclingElementFactory

winrt::RecyclePool RecyclingElementFactory::RecyclePool()
{
    return m_recyclePool.get();
}

void RecyclingElementFactory::RecyclePool(winrt::RecyclePool const& value)
{
    m_recyclePool.set(value);
}

winrt::IMap<winrt::hstring, winrt::DataTemplate> RecyclingElementFactory::Templates()
{
    return m_templates.get();
}

void RecyclingElementFactory::Templates(winrt::IMap<winrt::hstring, winrt::DataTemplate> const& value)
{
    m_templates.set(value);
}

#pragma endregion

#pragma region IRecyclingElementFactoryOverrides

winrt::hstring RecyclingElementFactory::OnSelectTemplateKeyCore(
    winrt::IInspectable const& dataContext, 
    winrt::UIElement const& owner)
{
    if (!m_args)
    {
        m_args.set(winrt::make<SelectTemplateEventArgs>());
    }

    auto args = winrt::get_self<SelectTemplateEventArgs>(m_args.get());
    args->TemplateKey({});
    args->DataContext(dataContext);
    args->Owner(owner);
    
    m_selectTemplateKeyEventSource(*this, *args);

    auto templateKey = args->TemplateKey();
    if (templateKey.empty())
    {
        throw winrt::hresult_error(E_FAIL, L"Please provide a valid template identifier in the handler for the SelectTemplateKey event.");
    }

    return templateKey;
}

#pragma endregion

#pragma region IElementFactoryOverrides

winrt::UIElement RecyclingElementFactory::GetElementCore(winrt::ElementFactoryGetArgs const& args)
{
    if (!m_templates || m_templates.get().Size() == 0)
    {
        throw winrt::hresult_error(E_FAIL, L"Templates property cannot be null or empty.");
    }

    const auto winrtOwner = args.Parent();
    const auto templateKey =
        m_templates.get().Size() == 1 ?
        m_templates.get().First().Current().Key() :
        OnSelectTemplateKeyCore(args.Data(), winrtOwner);

    if (templateKey.empty())
    {
        // Note: We could allow null/whitespace, which would work as long as
        // the recycle pool is not shared. in order to make this work in all cases
        // currently we validate that a valid template key is provided.
        throw winrt::hresult_error(E_FAIL, L"Template key cannot be empty or null.");
    }

    // Get an element from the Recycle Pool or create one
    auto element = m_recyclePool.get().TryGetElement(templateKey, winrtOwner).as<winrt::FrameworkElement>();

    if (!element)
    {
        // No need to call HasKey if there is only one template.
        if (m_templates.get().Size() > 1 && !m_templates.get().HasKey(templateKey))
        {
            std::wstring message = L"No templates of key " + std::wstring(templateKey.data()) + L" were found in the templates collection.";
            throw winrt::hresult_error(E_FAIL, message.c_str());
        }

        auto dataTemplate = m_templates.get().Lookup(templateKey);
        element = dataTemplate.LoadContent().as<winrt::FrameworkElement>();

        // Associate ReuseKey with element
        RecyclePool::SetReuseKey(element, templateKey);
    }

    return element;
}

void RecyclingElementFactory::RecycleElementCore(winrt::ElementFactoryRecycleArgs const& args)
{
    auto element = args.Element();
    auto key = RecyclePool::GetReuseKey(element);
    m_recyclePool.get().PutElement(element, key, args.Parent());
}

#pragma endregion
