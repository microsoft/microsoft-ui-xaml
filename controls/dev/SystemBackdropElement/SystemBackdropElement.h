// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "SystemBackdropElement.g.h"
#include "SystemBackdropElement.properties.h"

class SystemBackdropElement :
    public ReferenceTracker<SystemBackdropElement, winrt::implementation::SystemBackdropElementT>,
    public SystemBackdropElementProperties
{
public:
    SystemBackdropElement();
    ~SystemBackdropElement();

    // Framework overrides
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    winrt::Size ArrangeOverride(winrt::Size const& finalSize);

private:
    void UpdatePlacementVisual();
    void ReleaseCompositionResources();
    winrt::Microsoft::UI::Xaml::Media::SystemBackdrop m_systemBackdrop{ nullptr };
    winrt::CornerRadius m_cornerRadius{};
    winrt::Size m_lastArrangedSize{};
};