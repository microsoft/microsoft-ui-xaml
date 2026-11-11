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

void ItemTemplateWrapper::EnableTracking(const ITrackerHandleManager* owner)
{
    if (!owner || m_isTrackingReset || m_trackedDataTemplate)
    {
        return;
    }

    m_trackedDataTemplate.emplace(owner, m_dataTemplate);
    m_trackedDataTemplateSelector.emplace(owner, m_dataTemplateSelector);
    m_dataTemplate = nullptr;
    m_dataTemplateSelector = nullptr;
}

void ItemTemplateWrapper::ResetTracking() noexcept
{
    m_isTrackingReset = true;
    m_trackedDataTemplateSelector.reset();
    m_trackedDataTemplate.reset();
    m_dataTemplateSelector = nullptr;
    m_dataTemplate = nullptr;
}

winrt::DataTemplate ItemTemplateWrapper::GetDataTemplate() const
{
    return m_trackedDataTemplate ? m_trackedDataTemplate->safe_get() : m_dataTemplate;
}

winrt::DataTemplateSelector ItemTemplateWrapper::GetDataTemplateSelector() const
{
    return m_trackedDataTemplateSelector ? m_trackedDataTemplateSelector->safe_get() : m_dataTemplateSelector;
}

winrt::DataTemplate ItemTemplateWrapper::Template()
{
    return GetDataTemplate();
}

void ItemTemplateWrapper::Template(winrt::DataTemplate const& value)
{
    if (m_isTrackingReset)
    {
        return;
    }

    if (m_trackedDataTemplate)
    {
        m_trackedDataTemplate->set(value);
    }
    else
    {
        m_dataTemplate = value;
    }
}

winrt::DataTemplateSelector ItemTemplateWrapper::TemplateSelector()
{
    return GetDataTemplateSelector();
}

void ItemTemplateWrapper::TemplateSelector(winrt::DataTemplateSelector const& value)
{
    if (m_isTrackingReset)
    {
        return;
    }

    if (m_trackedDataTemplateSelector)
    {
        m_trackedDataTemplateSelector->set(value);
    }
    else
    {
        m_dataTemplateSelector = value;
    }
}

#pragma region IElementFactory

winrt::UIElement ItemTemplateWrapper::GetElement(winrt::ElementFactoryGetArgs const& args)
{
    auto const dataTemplate = GetDataTemplate();
    auto const dataTemplateSelector = GetDataTemplateSelector();
    auto selectedTemplate =
        dataTemplate ? dataTemplate :
        dataTemplateSelector ? dataTemplateSelector.SelectTemplate(args.Data()) :
        nullptr;

    // Check if selected template we got is valid
    if (!selectedTemplate && dataTemplateSelector)
    {
        // Null template, use other SelectTemplate method
        try
        {
            selectedTemplate = dataTemplateSelector.SelectTemplate(args.Data(), nullptr);
        }
        catch (winrt::hresult_error const& e)
        {
            // The default implementation of SelectTemplate(IInspectable item, ILayout container) throws invalid arg for null container
            // To not force everbody to provide an implementation of that, catch that here
            if (e.code().value != E_INVALIDARG)
            {
                throw e;
            }
        }

    }

    if (!selectedTemplate)
    {
        // Still nullptr, fail with a reasonable message now.
        throw winrt::hresult_invalid_argument(L"Null encountered as data template. That is not a valid value for a data template, and can not be used.");
    }

    winrt::RecyclePool recyclePool = RecyclePool::GetPoolInstance(selectedTemplate);
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
        RecyclePool::SetOriginTemplate(element, selectedTemplate);
    }

    return element;
}

void ItemTemplateWrapper::RecycleElement(winrt::ElementFactoryRecycleArgs const& args)
{
    auto element = args.Element();
    auto const dataTemplate = GetDataTemplate();
    winrt::DataTemplate selectedTemplate = dataTemplate ?
        dataTemplate :
        RecyclePool::GetOriginTemplate(element);
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
