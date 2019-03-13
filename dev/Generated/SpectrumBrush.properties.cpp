// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "SpectrumBrush.h"

CppWinRTActivatableClassWithDPFactory(SpectrumBrush)

GlobalDependencyProperty SpectrumBrushProperties::s_MaxSurfaceProperty{ nullptr };
GlobalDependencyProperty SpectrumBrushProperties::s_MaxSurfaceOpacityProperty{ nullptr };
GlobalDependencyProperty SpectrumBrushProperties::s_MinSurfaceProperty{ nullptr };

SpectrumBrushProperties::SpectrumBrushProperties()
{
    EnsureProperties();
}

void SpectrumBrushProperties::EnsureProperties()
{
    if (!s_MaxSurfaceProperty)
    {
        s_MaxSurfaceProperty =
            InitializeDependencyProperty(
                L"MaxSurface",
                winrt::name_of<winrt::LoadedImageSurface>(),
                winrt::name_of<winrt::SpectrumBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::LoadedImageSurface>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnMaxSurfacePropertyChanged));
    }
    if (!s_MaxSurfaceOpacityProperty)
    {
        s_MaxSurfaceOpacityProperty =
            InitializeDependencyProperty(
                L"MaxSurfaceOpacity",
                winrt::name_of<double>(),
                winrt::name_of<winrt::SpectrumBrush>(),
                false /* isAttached */,
                ValueHelper<double>::BoxValueIfNecessary(1.0),
                winrt::PropertyChangedCallback(&OnMaxSurfaceOpacityPropertyChanged));
    }
    if (!s_MinSurfaceProperty)
    {
        s_MinSurfaceProperty =
            InitializeDependencyProperty(
                L"MinSurface",
                winrt::name_of<winrt::LoadedImageSurface>(),
                winrt::name_of<winrt::SpectrumBrush>(),
                false /* isAttached */,
                ValueHelper<winrt::LoadedImageSurface>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnMinSurfacePropertyChanged));
    }
}

void SpectrumBrushProperties::ClearProperties()
{
    s_MaxSurfaceProperty = nullptr;
    s_MaxSurfaceOpacityProperty = nullptr;
    s_MinSurfaceProperty = nullptr;
}

void SpectrumBrushProperties::OnMaxSurfacePropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::SpectrumBrush>();
    winrt::get_self<SpectrumBrush>(owner)->OnPropertyChanged(args);
}

void SpectrumBrushProperties::OnMaxSurfaceOpacityPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::SpectrumBrush>();
    winrt::get_self<SpectrumBrush>(owner)->OnPropertyChanged(args);
}

void SpectrumBrushProperties::OnMinSurfacePropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::SpectrumBrush>();
    winrt::get_self<SpectrumBrush>(owner)->OnPropertyChanged(args);
}

void SpectrumBrushProperties::MaxSurface(winrt::LoadedImageSurface const& value)
{
    static_cast<SpectrumBrush*>(this)->SetValue(s_MaxSurfaceProperty, ValueHelper<winrt::LoadedImageSurface>::BoxValueIfNecessary(value));
}

winrt::LoadedImageSurface SpectrumBrushProperties::MaxSurface()
{
    return ValueHelper<winrt::LoadedImageSurface>::CastOrUnbox(static_cast<SpectrumBrush*>(this)->GetValue(s_MaxSurfaceProperty));
}

void SpectrumBrushProperties::MaxSurfaceOpacity(double value)
{
    static_cast<SpectrumBrush*>(this)->SetValue(s_MaxSurfaceOpacityProperty, ValueHelper<double>::BoxValueIfNecessary(value));
}

double SpectrumBrushProperties::MaxSurfaceOpacity()
{
    return ValueHelper<double>::CastOrUnbox(static_cast<SpectrumBrush*>(this)->GetValue(s_MaxSurfaceOpacityProperty));
}

void SpectrumBrushProperties::MinSurface(winrt::LoadedImageSurface const& value)
{
    static_cast<SpectrumBrush*>(this)->SetValue(s_MinSurfaceProperty, ValueHelper<winrt::LoadedImageSurface>::BoxValueIfNecessary(value));
}

winrt::LoadedImageSurface SpectrumBrushProperties::MinSurface()
{
    return ValueHelper<winrt::LoadedImageSurface>::CastOrUnbox(static_cast<SpectrumBrush*>(this)->GetValue(s_MinSurfaceProperty));
}
