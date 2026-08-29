// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RevealFocusDefaultValue.h"
#include <string>

namespace RevealFocus
{
    struct DefaultValueKey
    {
        float Value;
        const wchar_t* const RegKey;
    };
    
    float GetDefaultValue(DefaultValue paramType)
    {
        static const DefaultValueKey s_defaultValues[] = {
            {.75f,                L"AlphaMask"},
            {HNS_FROM_SECOND(1),  L"AmbientLightDuration"},
            {2.0f,                L"GlowSizeModifier" },
            {3.5f,                L"ThicknessToGlowSizeScale"},
            {12.0f,               L"MinimumGlowSize"},
            {0.4f,                L"DefaultLightIntensityMin" },
            {1.0f,                L"DefaultLightIntensityMax" },
            {0.5f,                L"DefaultLightIntensity" },
            {1.0f,                L"UseLightingEffect" },
            {0.0f,                L"PulsingLightIntensityLow"},
            {0.1f,                L"PulsingLightIntensityMid"},
            {0.4f,                L"PulsingLightIntensityHigh"},
            {HNS_FROM_SECOND(6),  L"PulsingAnimationDuration"},
            {HNS_FROM_SECOND(2),  L"PulsingAnimationDelay"},
            {1.0f,                L"UseTravelingFocus"},
            {0.3f,                L"ConeIntensityMin"},
            {0.6f,                L"ConeIntensityMax"},
            {0.6225f,             L"SpotLightIntensityDurationFactor"},
            {700.0f,              L"SpotLightSpeed"},
            {2.0f,                L"SpotLightStartOffsetFactor"},
            {70.0f,               L"SmallTravelingSpotLightThreshold"},
            {30.0f,               L"InnerConeAngle"},
            {45.0f,               L"OuterConeAngle"},
            {HNS_FROM_SECOND(.8), L"PressLightDuration"},
            {1.0f,                L"UsePressAnimations"}
        };
        static_assert(ARRAY_SIZE(s_defaultValues) == static_cast<size_t>(DefaultValue::Last), "mismatched array size");

        return s_defaultValues[static_cast<int>(paramType)].Value;
    }
}