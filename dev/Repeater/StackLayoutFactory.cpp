// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include <pch.h>
#include <common.h>
#include "ItemsRepeater.common.h"
#include "FlowLayoutAlgorithm.h"
#include "StackLayoutState.h"
#include "StackLayout.h"
#include "StackLayoutFactory.h"

GlobalDependencyProperty StackLayout::s_orientationProperty{ nullptr };
GlobalDependencyProperty StackLayout::s_spacingProperty{ nullptr };

/* static */
void StackLayout::EnsureProperties()
{
    if (!s_orientationProperty)
    {
        s_orientationProperty =
            InitializeDependencyProperty(
                L"Orientation",
                winrt::name_of<winrt::Orientation>(),
                winrt::name_of<winrt::StackLayout>(),
                false /* isAttached */,
                box_value(winrt::Orientation::Vertical), /* defaultValue */
                winrt::PropertyChangedCallback(&StackLayout::OnPropertyChanged));
    }

    if (!s_spacingProperty)
    {
        s_spacingProperty =
            InitializeDependencyProperty(
                L"Spacing",
                winrt::name_of<winrt::IReference<double>>(),
                winrt::name_of<winrt::StackLayout>(),
                false /* isAttached */,
                box_value(0.0), /* defaultValue */
                winrt::PropertyChangedCallback(&StackLayout::OnPropertyChanged));
    }
}

/*static*/
void StackLayout::ClearProperties()
{
    s_orientationProperty = nullptr;
    s_spacingProperty = nullptr;
}

void StackLayout::OnPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    winrt::get_self<StackLayout>(sender.as<winrt::StackLayout>())->OnPropertyChanged(args);
}