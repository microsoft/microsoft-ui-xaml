// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "LifetimeHandler.h"

/* static */
winrt::Rect CachedVisualTreeHelpers::GetLayoutSlot(winrt::FrameworkElement const& element)
{
    auto instance = LifetimeHandler::GetCachedVisualTreeHelpersInstance();
    MUX_ASSERT(instance);

    if (!instance->m_layoutInfo)
    {
        instance->m_layoutInfo = winrt::get_activation_factory<winrt::LayoutInformation, winrt::ILayoutInformationStatics>();
    }

    return instance->m_layoutInfo.GetLayoutSlot(element);
}

/* static */
winrt::DependencyObject CachedVisualTreeHelpers::GetParent(winrt::DependencyObject const& child)
{
    auto instance = LifetimeHandler::GetCachedVisualTreeHelpersInstance();
    MUX_ASSERT(instance);

    if (!instance->m_visualTreeHelper)
    {
        instance->m_visualTreeHelper = winrt::get_activation_factory<winrt::VisualTreeHelper, winrt::IVisualTreeHelperStatics>();
    }

    return instance->m_visualTreeHelper.GetParent(child);
}

/* static */ 
void CachedVisualTreeHelpers::SetPoolInstance(winrt::DataTemplate const& dataTemplate, winrt::RecyclePool const& recyclePool)
{
    auto instance = LifetimeHandler::GetCachedVisualTreeHelpersInstance();
    MUX_ASSERT(instance);

    if (!instance->m_recyclePoolStatics)
    {
        instance->m_recyclePoolStatics = winrt::get_activation_factory<winrt::RecyclePool, winrt::IRecyclePoolStatics>();
    }

    instance->m_recyclePoolStatics.SetPoolInstance(dataTemplate, recyclePool);
}

/* static */
winrt::RecyclePool CachedVisualTreeHelpers::GetPoolInstance(winrt::DataTemplate const& dataTemplate)
{
    auto instance = LifetimeHandler::GetCachedVisualTreeHelpersInstance();
    MUX_ASSERT(instance);

    if (!instance->m_recyclePoolStatics)
    {
        instance->m_recyclePoolStatics = winrt::get_activation_factory<winrt::RecyclePool, winrt::IRecyclePoolStatics>();
    }

    return instance->m_recyclePoolStatics.GetPoolInstance(dataTemplate);
}

/* static */
winrt::IDataTemplateComponent CachedVisualTreeHelpers::GetDataTemplateComponent(winrt::UIElement const& element)
{
    auto instance = LifetimeHandler::GetCachedVisualTreeHelpersInstance();
    MUX_ASSERT(instance);

    if (!instance->m_xamlBindingHelperStatics)
    {
        instance->m_xamlBindingHelperStatics = winrt::get_activation_factory<winrt::XamlBindingHelper, winrt::IXamlBindingHelperStatics>();
    }

    return instance->m_xamlBindingHelperStatics.GetDataTemplateComponent(element);
}

