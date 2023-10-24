// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define HNS_FROM_SECOND(x) ((x)*10 * 1000 * 1000)

namespace RevealFocus
{
    enum class DefaultValue
    {
        AlphaMask,
        AmbientLightDuration,
        GlowSizeModifier,
        ThicknessToGlowSizeScale,
        MinimumGlowSize,
        DefaultLightIntensityMin,
        DefaultLightIntensityMax,
        DefaultLightIntensity,
        UseLightingEffect,
        PulsingLightIntensityLow,
        PulsingLightIntensityMid,
        PulsingLightIntensityHigh,
        PulsingAnimationDuration,
        PulsingAnimationDelay,
        UseTravelingFocus,
        ConeIntensityMin,
        ConeIntensityMax,
        SpotLightIntensityDurationFactor,
        SpotLightSpeed,
        SpotLightStartOffsetFactor,
        // Elements that are too small don't look good using traveling focus
        SmallTravelingSpotLightThreshold,
        InnerConeAngle,
        OuterConeAngle,
        PressLightDuration,
        UsePressAnimations,

        Last
    };
    
    float GetDefaultValue(DefaultValue paramType);
}