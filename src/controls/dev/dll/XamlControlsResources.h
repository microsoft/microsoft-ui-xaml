﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlControlsResources.g.h"
#include "XamlControlsResources.properties.h"

class XamlControlsResources :
    public ReferenceTracker<XamlControlsResources, winrt::implementation::XamlControlsResourcesT, winrt::composable>,
    public XamlControlsResourcesProperties
{
public:
    XamlControlsResources();

    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs&  args);
    void UpdateAcrylicBrushesLightTheme(const winrt::IInspectable themeDictionary);
    void UpdateAcrylicBrushesDarkTheme(const winrt::IInspectable themeDictionary);
    static void EnsureRevealLights(winrt::UIElement const& element);
private:
    void UpdateSource();
    void UpdateTintLuminosityOpacity(winrt::Microsoft::UI::Xaml::Media::AcrylicBrush brush, double luminosityValue);
};
