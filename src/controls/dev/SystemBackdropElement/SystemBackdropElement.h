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
    void UpdatePlacementVisualSize();
    void UpdatePlacementVisualClip();
    void UpdatePlacementVisual();
    void EnsureCompositionResources();
    void ReleaseCompositionResources();
    void TryConnectSystemBackdrop();

    // Member variables
    winrt::Microsoft::UI::Content::ContentExternalBackdropLink m_backdropLink{ nullptr };
    winrt::Microsoft::UI::Composition::Compositor m_compositor{ nullptr };
    winrt::Microsoft::UI::Xaml::Media::SystemBackdrop m_systemBackdrop{ nullptr };
    winrt::Microsoft::UI::Composition::RectangleClip m_clip{ nullptr };
    winrt::CornerRadius m_cornerRadius{};
    
    bool m_registeredWithSystemBackdrop{ false };
    winrt::Size m_lastArrangedSize{};
};