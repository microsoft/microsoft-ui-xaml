// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "RecyclePoolFactory.h"
#include "RecyclePool.h"

#ifndef MUX_PRERELEASE
GlobalDependencyProperty RecyclePool::s_PoolInstanceProperty = nullptr;
#endif
GlobalDependencyProperty RecyclePool::s_reuseKeyProperty = nullptr;
GlobalDependencyProperty RecyclePool::s_originTemplateProperty = nullptr;

#pragma region IRecyclePoolStatics 

winrt::hstring RecyclePool::GetReuseKey(winrt::UIElement const& element)
{
    return auto_unbox(element.GetValue(s_reuseKeyProperty));
}

void RecyclePool::SetReuseKey(winrt::UIElement const& element, winrt::hstring const& value)
{
    element.SetValue(s_reuseKeyProperty, box_value(value));
}

#ifdef MUX_PRERELEASE
winrt::RecyclePool RecyclePool::GetPoolInstance(winrt::DataTemplate const& dataTemplate)
{
    if (!s_PoolInstanceProperty)
    {
        EnsureProperties();
    }

    return RecyclePoolProperties::GetPoolInstance(dataTemplate);
}

void RecyclePool::SetPoolInstance(winrt::DataTemplate const& dataTemplate, winrt::RecyclePool const& value)
{
    if (!s_PoolInstanceProperty)
    {
        EnsureProperties();
    }

    RecyclePoolProperties::SetPoolInstance(dataTemplate, value);
}
#else
winrt::RecyclePool RecyclePool::GetPoolInstance(winrt::DataTemplate const& dataTemplate)
{
    if (!s_PoolInstanceProperty)
    {
        EnsureProperties();
    }

    return ValueHelper<winrt::RecyclePool>::CastOrUnbox(dataTemplate.GetValue(s_PoolInstanceProperty));
}

void RecyclePool::SetPoolInstance(winrt::DataTemplate const& dataTemplate, winrt::RecyclePool const& value)
{
    if (!s_PoolInstanceProperty)
    {
        EnsureProperties();
    }

    dataTemplate.SetValue(s_PoolInstanceProperty, ValueHelper<winrt::RecyclePool>::BoxValueIfNecessary(value));
}
#endif

winrt::DataTemplate RecyclePool::GetOriginTemplate(winrt::UIElement const& element)
{
    return auto_unbox(element.GetValue(s_originTemplateProperty));
}

void RecyclePool::SetOriginTemplate(winrt::UIElement const& element, winrt::DataTemplate const& value)
{
    element.SetValue(s_originTemplateProperty, box_value(value));
}

#pragma endregion

RecyclePool::RecyclePool()
{
    RecyclePool::EnsureProperties();
}

/* static */
void RecyclePool::EnsureProperties()
{
#ifdef MUX_PRERELEASE
    RecyclePoolProperties::EnsureProperties();
#else
    if (!s_PoolInstanceProperty)
    {
        s_PoolInstanceProperty =
            InitializeDependencyProperty(
                L"PoolInstance",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                true /* isAttached */,
                ValueHelper<winrt::RecyclePool>::BoxedDefaultValue(),
                nullptr);
    }
#endif
    
    if (s_reuseKeyProperty == nullptr)
    {
        s_reuseKeyProperty =
            InitializeDependencyProperty(
                L"ReuseKey",
                winrt::name_of<hstring>(),
#ifdef MUX_PRERELEASE
                winrt::name_of<winrt::RecyclePool>(),
#else
                winrt::name_of<winrt::ItemsRepeater>(),
#endif
                true /* isAttached */,
                box_value(wstring_view(L"")) /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }

    if (s_originTemplateProperty == nullptr)
    {
        s_originTemplateProperty =
            InitializeDependencyProperty(
                L"OriginTemplate",
                winrt::name_of<winrt::DataTemplate>(),
#ifdef MUX_PRERELEASE
                winrt::name_of<winrt::RecyclePool>(),
#else
                winrt::name_of<winrt::IInspectable>(),
#endif
                true /* isAttached */,
                nullptr /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }
}

/* static */
void RecyclePool::ClearProperties()
{
    s_reuseKeyProperty = nullptr;
    s_originTemplateProperty = nullptr;
#ifndef MUX_PRERELEASE
    s_PoolInstanceProperty = nullptr;
#else
    RecyclePoolProperties::ClearProperties();
#endif
}
