// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "AcrylicTestApi.g.h"

class AcrylicTestApi : 
    public ReferenceTracker<AcrylicTestApi, winrt::implementation::AcrylicTestApiT, winrt::composable>
{
public:
    winrt::AcrylicBrush AcrylicBrush();
    void AcrylicBrush(winrt::AcrylicBrush const& value);
    bool IsUsingAcrylicBrush();
    winrt::CompositionBrush CompositionBrush();
    winrt::CompositionBrush NoiseBrush();

    // AcrylicBrush has three kinds of effect: fallback color, crossfading acrylic(for animation) and non crossfading acrylic
    // This function will ignore the internal state and create a new crossfading acrylic or non crossfading acrylic effect brush.
    void ForceCreateAcrylicBrush(bool useCrossFadeEffect);

private:
    winrt::AcrylicBrush m_acrylicBrush{ nullptr };
};
