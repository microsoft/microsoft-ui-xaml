// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

template<typename T>
inline void OnAttachedIsTargetPropertyChanged(
    const winrt::DependencyObject& sender,
    const winrt::DependencyPropertyChangedEventArgs& args)
{
    if (auto element = sender.try_as<winrt::UIElement>())
    {
        if (unbox_value<bool>(args.NewValue()))
        {
            winrt::XamlLight::AddTargetElement(T::GetLightIdStatic(), element);
        }
        else
        {
            winrt::XamlLight::RemoveTargetElement(T::GetLightIdStatic(), element);
        }
    }
    else if (auto brush = sender.try_as<winrt::Brush>())
    {
        if (unbox_value<bool>(args.NewValue()))
        {
            winrt::XamlLight::AddTargetBrush(T::GetLightIdStatic(), brush);
        }
        else
        {
            winrt::XamlLight::RemoveTargetBrush(T::GetLightIdStatic(), brush);
        }
    }
}

template<typename T>
inline auto InitializeIsTargetDependencyProperty()
{
    return InitializeDependencyProperty(
        L"IsTarget",
        winrt::name_of<bool>(),
        winrt::name_of<winrt::DependencyObject>(),
        true /* isAttached */,
        box_value(false),
        &OnAttachedIsTargetPropertyChanged<T>);
}
