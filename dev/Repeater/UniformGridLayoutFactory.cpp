// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "UniformGridLayoutState.h"
#include "UniformGridLayout.h"
#include "UniformGridLayoutFactory.h"

GlobalDependencyProperty UniformGridLayout::s_orientationProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_minItemWidthProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_minItemHeightProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_minRowSpacingProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_minColumnSpacingProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_itemsJustificationProperty{ nullptr };
GlobalDependencyProperty UniformGridLayout::s_itemsStretchProperty{ nullptr };

/* static */
void UniformGridLayout::EnsureProperties()
{
    if (!s_orientationProperty)
    {
        s_orientationProperty =
            InitializeDependencyProperty(
                L"Orientation",
                winrt::name_of<winrt::Orientation>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(winrt::Orientation::Horizontal), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if (!s_minItemWidthProperty)
    {
        s_minItemWidthProperty =
            InitializeDependencyProperty(
                L"MinItemWidth",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if (!s_minItemHeightProperty)
    {
        s_minItemHeightProperty =
            InitializeDependencyProperty(
                L"MinItemHeight",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if (!s_minRowSpacingProperty)
    {
        s_minRowSpacingProperty =
            InitializeDependencyProperty(
                L"MinRowSpacing",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if (!s_minColumnSpacingProperty)
    {
        s_minColumnSpacingProperty =
            InitializeDependencyProperty(
                L"MinColumnSpacing",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if (!s_itemsJustificationProperty)
    {
        s_itemsJustificationProperty =
            InitializeDependencyProperty(
                L"ItemsJustification",
                winrt::name_of<winrt::UniformGridLayoutItemsJustification>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(winrt::UniformGridLayoutItemsJustification::Start), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }

    if(!s_itemsStretchProperty)
    {
        s_itemsStretchProperty =
            InitializeDependencyProperty(
                L"ItemsStretch",
                winrt::name_of<winrt::UniformGridLayoutItemsStretch>(),
                winrt::name_of<winrt::UniformGridLayout>(),
                false /* isAttached */,
                box_value(winrt::UniformGridLayoutItemsStretch::None), /* defaultValue */
                winrt::PropertyChangedCallback(&UniformGridLayout::OnPropertyChanged));
    }
}

/*static*/
void UniformGridLayout::ClearProperties()
{
    s_orientationProperty = nullptr;
    s_minItemWidthProperty = nullptr;
    s_minItemHeightProperty = nullptr;
    s_minRowSpacingProperty = nullptr;
    s_minColumnSpacingProperty = nullptr;
    s_itemsJustificationProperty = nullptr;
    s_itemsStretchProperty = nullptr;
}

void UniformGridLayout::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::get_self<UniformGridLayout>(sender.as<winrt::UniformGridLayout>())->OnPropertyChanged(args);
}
