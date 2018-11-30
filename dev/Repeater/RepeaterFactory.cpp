// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "RepeaterFactory.h"
#include "ItemsRepeater.h"

GlobalDependencyProperty ItemsRepeater::s_VirtualizationInfoProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_itemsSourceProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_itemTemplateProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_layoutProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_animatorProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_HorizontalCacheLengthProperty{ nullptr };
GlobalDependencyProperty ItemsRepeater::s_VerticalCacheLengthProperty{ nullptr };

/* static */
void ItemsRepeater::EnsureProperties()
{
    if (!s_VirtualizationInfoProperty)
    {
        s_VirtualizationInfoProperty =
            InitializeDependencyProperty(
                L"VirtualizationInfo",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                true /* isAttached */,
                nullptr /* defaultValue */,
                nullptr /* propertyChangedCallback */);
    }

    if (!s_itemsSourceProperty)
    {
        s_itemsSourceProperty =
            InitializeDependencyProperty(
                L"ItemsSource",
                winrt::name_of<winrt::IInspectable>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }

    if (!s_itemTemplateProperty)
    {
        s_itemTemplateProperty =
            InitializeDependencyProperty(
                L"ElementFactory",
                winrt::name_of<winrt::IElementFactory>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }

    if (!s_layoutProperty)
    {
        s_layoutProperty =
            InitializeDependencyProperty(
                L"Layout",
                winrt::name_of<winrt::VirtualizingLayout>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                winrt::StackLayout() /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }

    if (!s_animatorProperty)
    {
        s_animatorProperty =
            InitializeDependencyProperty(
                L"Animator",
                winrt::name_of<winrt::ElementAnimator>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }

    if (!s_HorizontalCacheLengthProperty)
    {
        s_HorizontalCacheLengthProperty =
            InitializeDependencyProperty(
                L"HorizontalCacheLength",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }

    if (!s_VerticalCacheLengthProperty)
    {
        s_VerticalCacheLengthProperty =
            InitializeDependencyProperty(
                L"VerticalCacheLength",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::ItemsRepeater>(),
                false /* isAttached */,
                nullptr /* defaultValue */,
                winrt::PropertyChangedCallback(&ItemsRepeater::OnPropertyChanged));
    }
}

/*static*/
void ItemsRepeater::ClearProperties()
{
    s_itemsSourceProperty = nullptr;
    s_itemTemplateProperty = nullptr;
    s_layoutProperty = nullptr;
    s_animatorProperty = nullptr;
    s_HorizontalCacheLengthProperty = nullptr;
    s_VerticalCacheLengthProperty = nullptr;
}

void ItemsRepeater::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::get_self<ItemsRepeater>(sender.as<winrt::ItemsRepeater>())->OnPropertyChanged(args);
}