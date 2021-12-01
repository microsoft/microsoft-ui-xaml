// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "ItemTemplateWrapper.h"
#include "RecyclePool.h"
#include "ItemsRepeater.common.h"

ItemTemplateWrapper::ItemTemplateWrapper(winrt::DataTemplate const& dataTemplate)
{
    m_dataTemplate = dataTemplate;
}

ItemTemplateWrapper::ItemTemplateWrapper(winrt::DataTemplateSelector const& dataTemplateSelector)
{
    m_dataTemplateSelector = dataTemplateSelector;
}

winrt::DataTemplate ItemTemplateWrapper::Template()
{
    return m_dataTemplate;
}

void ItemTemplateWrapper::Template(winrt::DataTemplate const& value)
{
    m_dataTemplate = value;
}

winrt::DataTemplateSelector ItemTemplateWrapper::TemplateSelector()
{
    return m_dataTemplateSelector;
}

void ItemTemplateWrapper::TemplateSelector(winrt::DataTemplateSelector const& value)
{
    m_dataTemplateSelector = value;
}

#pragma region IElementFactory

winrt::UIElement ItemTemplateWrapper::GetElement(winrt::ElementFactoryGetArgs const& args)
{
    auto selectedTemplate = m_dataTemplate ? m_dataTemplate : m_dataTemplateSelector.SelectTemplate(args.Data());
    // Check if selected template we got is valid
    if (selectedTemplate == nullptr)
    {
        // Null template, use other SelectTemplate method
        try
        {
            selectedTemplate = m_dataTemplateSelector.SelectTemplate(args.Data(), nullptr);
        }
        catch (winrt::hresult_error e)
        {
            // The default implementation of SelectTemplate(IInspectable item, ILayout container) throws invalid arg for null container
            // To not force everbody to provide an implementation of that, catch that here
            if (e.code().value != E_INVALIDARG)
            {
                throw e;
            }
        }

        if (selectedTemplate == nullptr)
        {
            // Still nullptr, fail with a reasonable message now.
            throw winrt::hresult_invalid_argument(L"Null encountered as data template. That is not a valid value for a data template, and can not be used.");
        }
    }
    auto recyclePool = RecyclePool::GetPoolInstance(selectedTemplate);
    winrt::UIElement element = nullptr;

    if (recyclePool)
    {
        // try to get an element from the recycle pool.
        element = recyclePool.TryGetElement(L"" /* key */, args.Parent().as<winrt::FrameworkElement>());
    }

    if (!element)
    {
        // no element was found in recycle pool, create a new element
        element = selectedTemplate.LoadContent().as<winrt::FrameworkElement>();

        // Template returned null, so insert empty element to render nothing
        if (!element) {
            auto rectangle = winrt::Rectangle();
            rectangle.Width(0);
            rectangle.Height(0);
            element = rectangle;
        }

        // Associate template with element
        element.SetValue(RecyclePool::GetOriginTemplateProperty(), selectedTemplate);
    }

    return element;
}

void ItemTemplateWrapper::RecycleElement(winrt::ElementFactoryRecycleArgs const& args)
{
    auto element = args.Element();
    winrt::DataTemplate selectedTemplate = m_dataTemplate? 
        m_dataTemplate:
        element.GetValue(RecyclePool::GetOriginTemplateProperty()).as<winrt::DataTemplate>();
    auto recyclePool = RecyclePool::GetPoolInstance(selectedTemplate);
    if (!recyclePool)
    {
        // No Recycle pool in the template, create one.
        recyclePool = winrt::make<RecyclePool>();
        RecyclePool::SetPoolInstance(selectedTemplate, recyclePool);
    }

    recyclePool.PutElement(args.Element(), L"" /* key */, args.Parent());
}

#pragma endregion
