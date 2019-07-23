// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CppWinRTHelpers.h"

enum class EasingTypes
{
    Default,
    CubicBezier,
    Linear,
    Step
};

// needed so that AnimateTo can set a non-animated value directly without the overhead of a 0 duration animation
enum class SpotlightProperty
{
    None,
    Offset,
    Direction,
    ConstantAttenuation,
    LinearAttenuation,
    QuadraticAttenuation,
    InnerConeAngleInDegrees,
    InnerConeAngle,
    InnerConeColor,
    InnerConeIntensity,
    OuterConeAngleInDegrees,
    OuterConeAngle,
    OuterConeColor,
    OuterConeIntensity,
    Height,
    OuterAngleScale
};

// RevealHoverLight-related definitions

enum class RevealHoverSpotlightFlags
{
    None = 0x0,
    Ignore = 0x1,
    Animate = 0x2
};

struct EasingInfo
{
    EasingTypes EasingType{};
    winrt::float2 EasingControlPoint1{};
    winrt::float2 EasingControlPoint2{};
};

template <typename ValueType>
struct RevealHoverSpotlightAnimationInfo
{
    RevealHoverSpotlightFlags Flags{};
    SpotlightProperty SpotlightProperty{};
    PCWSTR PropertyName{};
    winrt::TimeSpan Duration{};
    ValueType Value{};
    EasingInfo EasingInfo{};
};

struct RevealHoverSpotlightStateDesc
{
    RevealHoverSpotlightAnimationInfo<winrt::Color> InnerConeColor;
    RevealHoverSpotlightAnimationInfo<winrt::Color> OuterConeColor;
    RevealHoverSpotlightAnimationInfo<float> InnerConeIntensity;
    RevealHoverSpotlightAnimationInfo<float> OuterConeIntensity;
    RevealHoverSpotlightAnimationInfo<float> OuterAngleScale;
};

enum RevealHoverSpotlightState
{
    RevealHoverSpotlightState_AnimToOff,
    RevealHoverSpotlightState_AnimToHover,
    RevealHoverSpotlightState_Pressing,
    RevealHoverSpotlightState_FastRelease,
    RevealHoverSpotlightState_SlowRelease,

    RevealHoverSpotlightState_StateCount
};

// RevealBorderLight-related definitions

template <typename ValueType>
struct RevealBorderSpotlightInfo
{
    SpotlightProperty SpotlightProperty{};
    PCWSTR PropertyName{};
    ValueType Value{};
};

struct RevealBorderSpotlightStateDesc
{
    RevealBorderSpotlightInfo<float> ConstantAttenuation;
    RevealBorderSpotlightInfo<float> LinearAttenuation;
    RevealBorderSpotlightInfo<float> InnerConeAngleInDegrees;
    RevealBorderSpotlightInfo<winrt::Color> InnerConeColor;
    RevealBorderSpotlightInfo<float> OuterConeAngleInDegrees;
    RevealBorderSpotlightInfo<winrt::Color> OuterConeColor;
    RevealBorderSpotlightInfo<float> Height;
    RevealBorderSpotlightInfo<float> OuterAngleScale;
};

template <typename ValueType>
bool IsAnimated(const RevealHoverSpotlightAnimationInfo<ValueType>& info)
{
    return (int)RevealHoverSpotlightFlags::Animate & (int)info.Flags;
}

template <typename ValueType>
bool IsIgnored(const RevealHoverSpotlightAnimationInfo<ValueType>& info)
{
    return (int)RevealHoverSpotlightFlags::Ignore & (int)info.Flags;
}

bool inline IsStateAnimated(const RevealHoverSpotlightStateDesc& state);
bool inline IsStateIgnored(const RevealHoverSpotlightStateDesc& state);

void PlaySpotLightStateAnimation(
    const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealHoverSpotlightStateDesc& targetState,
    std::function<void()>* cancelationFunction,
    std::function<void()> onComplete = nullptr);

void SetSpotLightStateImmediate(const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealHoverSpotlightStateDesc& targetState);

void SetSpotLightStateImmediate(const winrt::SpotLight& compositionSpotLight,
    const winrt::CompositionPropertySet& colorsProxy,
    const winrt::CompositionPropertySet& offsetProps,
    const RevealBorderSpotlightStateDesc& targetState);

winrt::CompositionPropertySet CreateSpotLightColorsProxy(const winrt::SpotLight& compositionSpotLight);