// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#include "pch.h"
#include "common.h"
#include "MapIcon.h"

namespace winrt::Microsoft::UI::Xaml::Controls
{
    CppWinRTActivatableClassWithDPFactory(MapIcon)
}

#include "MapIcon.g.cpp"

GlobalDependencyProperty MapIconProperties::s_LocationProperty{ nullptr };

MapIconProperties::MapIconProperties()
{
    EnsureProperties();
}

void MapIconProperties::EnsureProperties()
{
    if (!s_LocationProperty)
    {
        s_LocationProperty =
            InitializeDependencyProperty(
                L"Location",
                winrt::name_of<winrt::Geopoint>(),
                winrt::name_of<winrt::MapIcon>(),
                false /* isAttached */,
                ValueHelper<winrt::Geopoint>::BoxedDefaultValue(),
                winrt::PropertyChangedCallback(&OnLocationPropertyChanged));
    }
}

void MapIconProperties::ClearProperties()
{
    s_LocationProperty = nullptr;
}

void MapIconProperties::OnLocationPropertyChanged(
    winrt::DependencyObject const& sender,
    winrt::DependencyPropertyChangedEventArgs const& args)
{
    auto owner = sender.as<winrt::MapIcon>();
    winrt::get_self<MapIcon>(owner)->OnPropertyChanged(args);
}

void MapIconProperties::Location(winrt::Geopoint const& value)
{
    [[gsl::suppress(con)]]
    {
    static_cast<MapIcon*>(this)->SetValue(s_LocationProperty, ValueHelper<winrt::Geopoint>::BoxValueIfNecessary(value));
    }
}

winrt::Geopoint MapIconProperties::Location()
{
    return ValueHelper<winrt::Geopoint>::CastOrUnbox(static_cast<MapIcon*>(this)->GetValue(s_LocationProperty));
}
