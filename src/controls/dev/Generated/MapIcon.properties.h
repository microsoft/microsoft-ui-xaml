// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// DO NOT EDIT! This file was generated by CustomTasks.DependencyPropertyCodeGen
#pragma once

class MapIconProperties
{
public:
    MapIconProperties();

    void Location(winrt::Geopoint const& value);
    winrt::Geopoint Location();

    static winrt::DependencyProperty LocationProperty() { return s_LocationProperty; }

    static GlobalDependencyProperty s_LocationProperty;

    static void EnsureProperties();
    static void ClearProperties();

    static void OnLocationPropertyChanged(
        winrt::DependencyObject const& sender,
        winrt::DependencyPropertyChangedEventArgs const& args);
};
