// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MaterialHelper.h"

#include "SpectrumBrush.g.h"
#include "SpectrumBrush.properties.h"

#pragma warning(push)
#pragma warning(disable: 6101)  // Returning uninitialized memory '<value>'.  A successful path through the function does not set the named _Out_ parameter.
#include <Microsoft.UI.Private.Composition.Effects_impl.h>
#pragma warning(pop)


class SpectrumBrush :
    public ReferenceTracker<SpectrumBrush, winrt::implementation::SpectrumBrushT, winrt::composable>,
    public SpectrumBrushProperties
{
public:
    // IXamlCompositionBrushOverrides
    void OnConnected();
    void OnDisconnected();

    // Property changed handler
    void OnPropertyChanged(const winrt::DependencyPropertyChangedEventArgs& args);

private:

    void CreateSpectrumBrush();
    void UpdateSpectrumBrush();

    bool m_isConnected{ false };

    winrt::CompositionSurfaceBrush m_minSurfaceBrush{ nullptr };
    winrt::CompositionSurfaceBrush m_maxSurfaceBrush{ nullptr };

    com_ptr<Microsoft::UI::Private::Composition::Effects::CrossFadeEffect> m_brushEffect{ nullptr };
    winrt::CompositionEffectBrush m_brush{ nullptr };
};
