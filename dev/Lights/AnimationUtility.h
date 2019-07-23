// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "SpotLightStateHelper.h"

template <typename ValueType>
inline auto MakeAnimation(const winrt::Compositor &compositor);

template <>
inline auto MakeAnimation<float>(const winrt::Compositor &compositor)
{
    return compositor.CreateScalarKeyFrameAnimation();
}

template <>
inline auto MakeAnimation<winrt::Color>(const winrt::Compositor &compositor)
{
    return compositor.CreateColorKeyFrameAnimation();
}

void SetValueDirect(const winrt::CompositionPropertySet& target, std::wstring_view const& propName, float value);
void SetValueDirect(const winrt::CompositionPropertySet& target, std::wstring_view const& propName, const winrt::Color& value);
void SetValueDirect(const winrt::SpotLight& target, SpotlightProperty prop, const winrt::float3& value);
void SetValueDirect(const winrt::SpotLight& target, SpotlightProperty prop, float value);
void SetValueDirect(const winrt::SpotLight& target, SpotlightProperty prop, const winrt::Color& value);
void SetValueDirect(const winrt::CompositionPropertySet& target, SpotlightProperty prop, const winrt::Color& value);

template <typename ValueType>
inline void AnimateTo(
    const winrt::SpotLight& target,
    const RevealHoverSpotlightAnimationInfo<ValueType>& targetFrame)
{
    if (IsIgnored(targetFrame))
    {
        return;
    }

    target.StopAnimation(targetFrame.PropertyName);
    if (!IsAnimated(targetFrame))
    {
        SetValueDirect(target, targetFrame.SpotlightProperty, targetFrame.Value);
    }
    else
    {
        SetupAnimationAndStart(target, targetFrame);
    }
}

template <typename ValueType>
inline void AnimateTo(
    const winrt::CompositionPropertySet& target,
    const RevealHoverSpotlightAnimationInfo<ValueType>& targetFrame)
{
    if (IsIgnored(targetFrame))
    {
        return;
    }

    target.StopAnimation(targetFrame.PropertyName);
    if (!IsAnimated(targetFrame))
    {
        SetValueDirect(target, targetFrame.PropertyName, targetFrame.Value);
    }
    else
    {
        SetupAnimationAndStart(target, targetFrame);
    }
}

template <typename ValueType>
inline void SetupAnimationAndStart(
    const winrt::CompositionObject& target,
    const RevealHoverSpotlightAnimationInfo<ValueType>& targetFrame)
{
    auto compositor = target.Compositor();
    auto animation = MakeAnimation<ValueType>(compositor);

    animation.Duration(targetFrame.Duration);
    animation.StopBehavior(winrt::Composition::AnimationStopBehavior::LeaveCurrentValue);

    if (targetFrame.EasingInfo.EasingType == EasingTypes::CubicBezier)
    {
        animation.InsertKeyFrame(1, targetFrame.Value, compositor.CreateCubicBezierEasingFunction(targetFrame.EasingInfo.EasingControlPoint1, targetFrame.EasingInfo.EasingControlPoint2));
    }
    else if (targetFrame.EasingInfo.EasingType == EasingTypes::Linear)
    {
        animation.InsertKeyFrame(1, targetFrame.Value, compositor.CreateLinearEasingFunction());
    }
    else
    {
        animation.InsertKeyFrame(1, targetFrame.Value);
    }

    target.StartAnimation(targetFrame.PropertyName, animation);
}
