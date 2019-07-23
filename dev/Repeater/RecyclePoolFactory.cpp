// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "ItemsRepeater.common.h"
#include "RecyclePool.h"
#include "RecyclePoolFactory.h"
#include <common.h>
#include <pch.h>

GlobalDependencyProperty RecyclePool::s_reuseKeyProperty = nullptr;
GlobalDependencyProperty RecyclePool::s_poolInstanceProperty = nullptr;
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

winrt::RecyclePool RecyclePool::GetPoolInstance(winrt::DataTemplate const& dataTemplate)
{
    if (!s_poolInstanceProperty)
    {
        EnsureProperties();
    }

    return dataTemplate.GetValue(s_poolInstanceProperty).try_as<winrt::RecyclePool>();
}

void RecyclePool::SetPoolInstance(winrt::DataTemplate const& dataTemplate, winrt::RecyclePool  /*unused*/const& value)
{
    if (!s_poolInstanceProperty)
    {
        EnsureProperties();
    }

    dataTemplate.SetValue(s_poolInstanceProperty, value);
}

#pragma endregion

/* static */
void RecyclePool::EnsureProperties()
{
    if (s_reuseKeyProperty == nullptr)
    {
        s_reuseKeyProperty =
            InitializeDependencyProperty(
                L"ReuseKey",
                winrt::name_of<hstring>(),
                winrt::name_of<winrt::RecyclePool>(),
                true /* isAttached */,
                box_value(wstring_view(L"")) /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }

    if (s_poolInstanceProperty == nullptr)
    {
        s_poolInstanceProperty =
            InitializeDependencyProperty(
                L"PoolInstance",
                winrt::name_of<winrt::RecyclePool>(),
                winrt::name_of<winrt::RecyclePool>(),
                true /* isAttached */,
                nullptr /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }

    if (s_originTemplateProperty == nullptr)
    {
        s_originTemplateProperty =
            InitializeDependencyProperty(
                L"OriginTemplate",
                winrt::name_of<winrt::DataTemplate>(),
                winrt::name_of<winrt::RecyclePool>(),
                true /* isAttached */,
                nullptr /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }
}

/* static */
void RecyclePool::ClearProperties()
{
    s_reuseKeyProperty = nullptr;
    s_poolInstanceProperty = nullptr;
    s_originTemplateProperty = nullptr;
}
