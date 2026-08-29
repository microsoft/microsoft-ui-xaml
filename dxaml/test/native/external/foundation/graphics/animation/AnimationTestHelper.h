// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Foundation { namespace Graphics {

class AnimationTestHelper
{
public:
    static xaml_animation::DoubleAnimation^ MakeDoubleAnimation(
        xaml_controls::Canvas^ rootCanvas,
        Platform::String^ targetName,
        Platform::String^ targetProperty,
        ULONGLONG duration,
        double from,
        double to
        );

    static xaml_animation::DoubleAnimation^ MakeDoubleAnimation(
        xaml_controls::Canvas^ rootCanvas,
        Platform::String^ targetName,
        ULONGLONG duration,
        double from = 0.0,
        double to = 10.0
        );

    static xaml_animation::DoubleAnimation^ MakeDoubleAnimation(
        xaml::DependencyObject^ targetObject,
        Platform::String^ targetProperty);

    static xaml_animation::ColorAnimation^ MakeColorAnimation(
        xaml_controls::Canvas^ rootCanvas,
        Platform::String^ targetName,
        ULONGLONG duration,
        unsigned int fromArgb,
        unsigned int toArgb
        );

    static xaml_animation::Storyboard^ MakeStoryboard(
        xaml_animation::Timeline^ timeline
        );

    static xaml_animation::Storyboard^ MakeStoryboard(
        xaml_animation::Timeline^ timeline,
        ULONGLONG duration
        );

    static xaml_animation::Storyboard^ MakeBlankStoryboard(
        ULONGLONG duration
        );

    static xaml_animation::DoubleAnimation^ MakeOpacityAnimation(Microsoft::UI::Xaml::Controls::Canvas^ target);
};

} } } } } }
