// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"
#include "RadialGradientBrush.g.h"
#include "RadialGradientBrush.properties.h"

class RadialGradientBrush :
    public ReferenceTracker<RadialGradientBrush, winrt::implementation::RadialGradientBrushT>,
    public RadialGradientBrushProperties
{

public:
    RadialGradientBrush();
    ~RadialGradientBrush() {};

    winrt::IObservableVector<winrt::GradientStop> GradientStops();

    // IXamlCompositionBrushBase overrides
    void OnConnected();
    void OnDisconnected();

    void OnEllipseCenterPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnEllipseRadiusPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnGradientOffsetPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnMappingModePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnInterpolationSpacePropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);
    void OnSpreadMethodPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:
    bool m_isConnected{};
    winrt::CompositionBrush m_brush{ nullptr };
    winrt::IObservableVector<winrt::GradientStop> m_gradientStops{ nullptr };

    winrt::Collections::IObservableVector<winrt::GradientStop>::VectorChanged_revoker m_gradientStopsVectorChangedRevoker{};
    PropertyChanged_revoker m_fallbackColorChangedRevoker{};

    void OnGradientStopsVectorChanged(winrt::Collections::IObservableVector<winrt::GradientStop> const& sender, winrt::Collections::IVectorChangedEventArgs const& e);
    void OnFallbackColorChanged(const winrt::DependencyObject& sender, const winrt::DependencyProperty& args);

    void EnsureCompositionBrush();

    void UpdateCompositionGradientEllipseCenter();
    void UpdateCompositionGradientEllipseRadius();
    void UpdateCompositionGradientMappingMode();
    void UpdateCompositionGradientOffset();
    void UpdateCompositionGradientStops();
    void UpdateCompositionInterpolationSpace();
    void UpdateCompositionExtendMode();

    void UpdateFallbackBrush();
};
