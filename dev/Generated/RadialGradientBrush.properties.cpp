// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "RadialGradientBrush.h"

namespace winrt::Microsoft::UI::Xaml::Media
{
    CppWinRTActivatableClassWithDPFactory(RadialGradientBrush)
}

#include "RadialGradientBrush.g.cpp"

GlobalDependencyProperty RadialGradientBrushProperties::s_EllipseCenterProperty{ nullptr };
GlobalDependencyProperty RadialGradientBrushProperties::s_EllipseRadiusProperty{ nullptr };
GlobalDependencyProperty RadialGradientBrushProperties::s_GradientOffsetProperty{ nullptr };
GlobalDependencyProperty RadialGradientBrushProperties::s_InterpolationSpaceProperty{ nullptr };
GlobalDependencyProperty RadialGradientBrushProperties::s_MappingModeProperty{ nullptr };
GlobalDependencyProperty RadialGradientBrushProperties::s_SpreadMethodProperty{ nullptr };

RadialGradientBrushProperties::RadialGradientBrushProperties()
{
    EnsureProperties();
}

void RadialGradientBrushProperties::EnsureProperties()
{
    if (!s_EllipseCenterProperty)
    {
        s_EllipseCenterProperty =
            InitializeDependencyProperty(
                L"EllipseCenter",
                winrt::name_of<winrt::Point>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::Point>::BoxValueIfNecessary(winrt::Point(0.5,0.5)),
                winrt::PropertyChangedCallback(&OnEllipseCenterPropertyChanged));
    }
    if (!s_EllipseRadiusProperty)
    {
        s_EllipseRadiusProperty =
            InitializeDependencyProperty(
                L"EllipseRadius",
                winrt::name_of<winrt::Point>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::Point>::BoxValueIfNecessary(winrt::Point(0.5,0.5)),
                winrt::PropertyChangedCallback(&OnEllipseRadiusPropertyChanged));
    }
    if (!s_GradientOffsetProperty)
    {
        s_GradientOffsetProperty =
            InitializeDependencyProperty(
                L"GradientOffset",
                winrt::name_of<winrt::Point>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::Point>::BoxValueIfNecessary(winrt::Point(0.5,0.5)),
                winrt::PropertyChangedCallback(&OnGradientOffsetPropertyChanged));
    }
    if (!s_InterpolationSpaceProperty)
    {
        s_InterpolationSpaceProperty =
            InitializeDependencyProperty(
                L"InterpolationSpace",
                winrt::name_of<winrt::CompositionColorSpace>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::CompositionColorSpace>::BoxValueIfNecessary(winrt::Windows::UI::Composition::CompositionColorSpace::Auto),
                winrt::PropertyChangedCallback(&OnInterpolationSpacePropertyChanged));
    }
    if (!s_MappingModeProperty)
    {
        s_MappingModeProperty =
            InitializeDependencyProperty(
                L"MappingMode",
                winrt::name_of<winrt::BrushMappingMode>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::BrushMappingMode>::BoxValueIfNecessary(winrt::BrushMappingMode::RelativeToBoundingBox),
                winrt::PropertyChangedCallback(&OnMappingModePropertyChanged));
    }
    if (!s_SpreadMethodProperty)
    {
        s_SpreadMethodProperty =
            InitializeDependencyProperty(
                L"SpreadMethod",
                winrt::name_of<winrt::GradientSpreadMethod>(),
                winrt::name_of<winrt::RadialGradientBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::GradientSpreadMethod>::BoxValueIfNecessary(winrt::GradientSpreadMethod::Pad),
                winrt::PropertyChangedCallback(&OnSpreadMethodPropertyChanged));
    }
}

void RadialGradientBrushProperties::ClearProperties()
{
    s_EllipseCenterProperty = nullptr;
    s_EllipseRadiusProperty = nullptr;
    s_GradientOffsetProperty = nullptr;
    s_InterpolationSpaceProperty = nullptr;
    s_MappingModeProperty = nullptr;
    s_SpreadMethodProperty = nullptr;
}

void RadialGradientBrushProperties::OnEllipseCenterPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnEllipseCenterPropertyChanged(args);
}

void RadialGradientBrushProperties::OnEllipseRadiusPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnEllipseRadiusPropertyChanged(args);
}

void RadialGradientBrushProperties::OnGradientOffsetPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnGradientOffsetPropertyChanged(args);
}

void RadialGradientBrushProperties::OnInterpolationSpacePropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnInterpolationSpacePropertyChanged(args);
}

void RadialGradientBrushProperties::OnMappingModePropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnMappingModePropertyChanged(args);
}

void RadialGradientBrushProperties::OnSpreadMethodPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::RadialGradientBrush>();
    winrt::get_self<RadialGradientBrush>(owner)->OnSpreadMethodPropertyChanged(args);
}

void RadialGradientBrushProperties::EllipseCenter(winrt::Point const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_EllipseCenterProperty, ValueHelper<winrt::Point>::BoxValueIfNecessary(value));
}

winrt::Point RadialGradientBrushProperties::EllipseCenter()
{
    return ValueHelper<winrt::Point>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_EllipseCenterProperty));
}

void RadialGradientBrushProperties::EllipseRadius(winrt::Point const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_EllipseRadiusProperty, ValueHelper<winrt::Point>::BoxValueIfNecessary(value));
}

winrt::Point RadialGradientBrushProperties::EllipseRadius()
{
    return ValueHelper<winrt::Point>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_EllipseRadiusProperty));
}

void RadialGradientBrushProperties::GradientOffset(winrt::Point const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_GradientOffsetProperty, ValueHelper<winrt::Point>::BoxValueIfNecessary(value));
}

winrt::Point RadialGradientBrushProperties::GradientOffset()
{
    return ValueHelper<winrt::Point>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_GradientOffsetProperty));
}

void RadialGradientBrushProperties::InterpolationSpace(winrt::CompositionColorSpace const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_InterpolationSpaceProperty, ValueHelper<winrt::CompositionColorSpace>::BoxValueIfNecessary(value));
}

winrt::CompositionColorSpace RadialGradientBrushProperties::InterpolationSpace()
{
    return ValueHelper<winrt::CompositionColorSpace>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_InterpolationSpaceProperty));
}

void RadialGradientBrushProperties::MappingMode(winrt::BrushMappingMode const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_MappingModeProperty, ValueHelper<winrt::BrushMappingMode>::BoxValueIfNecessary(value));
}

winrt::BrushMappingMode RadialGradientBrushProperties::MappingMode()
{
    return ValueHelper<winrt::BrushMappingMode>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_MappingModeProperty));
}

void RadialGradientBrushProperties::SpreadMethod(winrt::GradientSpreadMethod const& value)
{
    static_cast<RadialGradientBrush*>(this)->SetValue(s_SpreadMethodProperty, ValueHelper<winrt::GradientSpreadMethod>::BoxValueIfNecessary(value));
}

winrt::GradientSpreadMethod RadialGradientBrushProperties::SpreadMethod()
{
    return ValueHelper<winrt::GradientSpreadMethod>::CastOrUnbox(static_cast<RadialGradientBrush*>(this)->GetValue(s_SpreadMethodProperty));
}
