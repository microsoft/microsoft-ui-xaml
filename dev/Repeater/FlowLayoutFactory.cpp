// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "FlowLayoutState.h"
#include "FlowLayout.h"
#include "FlowLayoutFactory.h"

GlobalDependencyProperty FlowLayout::s_orientationProperty{ nullptr };
GlobalDependencyProperty FlowLayout::s_minRowSpacingProperty{ nullptr };
GlobalDependencyProperty FlowLayout::s_minColumnSpacingProperty{ nullptr };
GlobalDependencyProperty FlowLayout::s_lineAlignmentProperty{ nullptr };

/* static */
void FlowLayout::EnsureProperties()
{
    if (!s_orientationProperty)
    {
        s_orientationProperty =
            InitializeDependencyProperty(
                L"Orientation",
                winrt::name_of<winrt::Orientation>(),
                winrt::name_of<winrt::FlowLayout>(),
                false /* isAttached */,
                box_value(winrt::Orientation::Horizontal), /* defaultValue */
                winrt::PropertyChangedCallback(&FlowLayout::OnPropertyChanged));
    }

    if (!s_minRowSpacingProperty)
    {
        s_minRowSpacingProperty =
            InitializeDependencyProperty(
                L"MinRowSpacing",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::FlowLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&FlowLayout::OnPropertyChanged));
    }

    if (!s_minColumnSpacingProperty)
    {
        s_minColumnSpacingProperty =
            InitializeDependencyProperty(
                L"MinColumnSpacing",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::FlowLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&FlowLayout::OnPropertyChanged));
    }

    if (!s_lineAlignmentProperty)
    {
        s_lineAlignmentProperty =
            InitializeDependencyProperty(
                L"LineAlignment",
                winrt::name_of<winrt::FlowLayoutLineAlignment>(),
                winrt::name_of<winrt::FlowLayout>(),
                false /* isAttached */,
                box_value(winrt::FlowLayoutLineAlignment::Start), /* defaultValue */
                winrt::PropertyChangedCallback(&FlowLayout::OnPropertyChanged));
    }
}

/*static*/
void FlowLayout::ClearProperties()
{
    s_orientationProperty = nullptr;
    s_minRowSpacingProperty = nullptr;
    s_minColumnSpacingProperty = nullptr;
    s_lineAlignmentProperty = nullptr;
}

void FlowLayout::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::get_self<FlowLayout>(sender.as<winrt::FlowLayout>())->OnPropertyChanged(args);
}