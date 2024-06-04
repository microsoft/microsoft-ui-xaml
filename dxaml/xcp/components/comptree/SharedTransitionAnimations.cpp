// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "SharedTransitionAnimations.h"
#include "Animation.h"

void SharedTransitionAnimations::EnsureEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor)
{
    if (!m_brushEasingFunction)
    {
        wrl::ComPtr<WUComp::ILinearEasingFunction> linear;
        IFCFAILFAST(easingFunctionStatics->CreateLinearEasingFunction(compositor, &linear));
        IFCFAILFAST(linear.As(&m_brushEasingFunction));
    }

    if (!m_scalarEasingFunction)
    {
        wrl::ComPtr<WUComp::ICubicBezierEasingFunction> bezier;
        IFCFAILFAST(easingFunctionStatics->CreateCubicBezierEasingFunction(compositor, { 0.8f, 0.0f }, { 0.2f, 1.0f }, &bezier));
        IFCFAILFAST(bezier.As(&m_scalarEasingFunction));
        IFCFAILFAST(bezier.As(&m_vector3EasingFunction));
    }
}

WUComp::ICompositionAnimationBase* SharedTransitionAnimations::GetScalarAnimationNoRef(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor,
    const wrl::Wrappers::HStringReference& propertyName,
    float scalar,
    const wf::TimeSpan& duration)
{
    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarKFA;
    wrl::ComPtr<WUComp::IKeyFrameAnimation> kfa;
    if (!m_scalarAnimation)
    {
        IFCFAILFAST(compositor->CreateScalarKeyFrameAnimation(&scalarKFA));
        IFCFAILFAST(scalarKFA.As(&m_scalarAnimation));

        // With ScalarTransitions, the first key frame is always "this.StartingValue" at time 0.
        IFCFAILFAST(m_scalarAnimation.As(&kfa));
        IFCFAILFAST(kfa->InsertExpressionKeyFrame(0.0f, wrl::Wrappers::HStringReference(L"this.StartingValue").Get()));
    }
    else
    {
        IFCFAILFAST(m_scalarAnimation.As(&scalarKFA));
        IFCFAILFAST(m_scalarAnimation.As(&kfa));
    }

    EnsureEasingFunction(easingFunctionStatics, compositor);

    SetAnimationDuration(kfa, duration);

    IFCFAILFAST(scalarKFA->InsertKeyFrameWithEasingFunction(1.0f, scalar, m_scalarEasingFunction.Get()));

    wrl::ComPtr<WUComp::ICompositionAnimation2> animation2;
    IFCFAILFAST(m_scalarAnimation.As(&animation2));
    IFCFAILFAST(animation2->put_Target(propertyName.Get()));

    return m_scalarAnimation.Get();
}

WUComp::ICompositionAnimationBase* SharedTransitionAnimations::GetVector3AnimationNoRef(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor,
    const wrl::Wrappers::HStringReference& propertyName,
    const wfn::Vector3& vector3,
    const wrl::Wrappers::HStringReference& initialFrame,
    const wf::TimeSpan& duration)
{
    wrl::ComPtr<WUComp::IVector3KeyFrameAnimation> vector3KFA;
    if (!m_vector3Animation)
    {
        IFCFAILFAST(compositor->CreateVector3KeyFrameAnimation(&vector3KFA));
        IFCFAILFAST(vector3KFA.As(&m_vector3Animation));
    }
    else
    {
        IFCFAILFAST(m_vector3Animation.As(&vector3KFA));
    }

    EnsureEasingFunction(easingFunctionStatics, compositor);

    wrl::ComPtr<WUComp::IKeyFrameAnimation> kfa;
    IFCFAILFAST(m_vector3Animation.As(&kfa));
    SetAnimationDuration(kfa, duration);

    IFCFAILFAST(kfa->InsertExpressionKeyFrame(0.0f, initialFrame.Get()));
    IFCFAILFAST(vector3KFA->InsertKeyFrameWithEasingFunction(1.0f, vector3, m_vector3EasingFunction.Get()));

    wrl::ComPtr<WUComp::ICompositionAnimation2> animation2;
    IFCFAILFAST(m_vector3Animation.As(&animation2));
    IFCFAILFAST(animation2->put_Target(propertyName.Get()));

    return m_vector3Animation.Get();
}

WUComp::ICompositionAnimation* SharedTransitionAnimations::GetBrushAnimationNoRef(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ WUComp::ICompositor* compositor,
    const std::optional<wu::Color>& fromColor,
    const wu::Color& toColor,
    const wf::TimeSpan& duration)
{
    wrl::ComPtr<WUComp::IColorKeyFrameAnimation> colorKFA;
    if (!m_brushAnimation)
    {
        IFCFAILFAST(compositor->CreateColorKeyFrameAnimation(&colorKFA));
        IFCFAILFAST(colorKFA->put_InterpolationColorSpace(WUComp::CompositionColorSpace_Rgb));
        IFCFAILFAST(colorKFA.As(&m_brushAnimation));
    }
    else
    {
        IFCFAILFAST(m_brushAnimation.As(&colorKFA));
    }

    EnsureEasingFunction(easingFunctionStatics, compositor);

    wrl::ComPtr<WUComp::IKeyFrameAnimation> kfa;
    IFCFAILFAST(m_brushAnimation.As(&kfa));
    SetAnimationDuration(kfa, duration);

    // There won't be an explicit start color if this is a hand off scenario.
    if (fromColor)
    {
        IFCFAILFAST(colorKFA->InsertKeyFrame(0.0f, fromColor.value()));
    }
    else
    {
        // Insert an expression key frame at time 0 for the starting value.
        // We get this behavior automatically if there's no key frame at time 0, but since we're reusing a shared KFA,
        // a previous animation might have put in an explicit key frame at time 0. There's no WUC API to remove key
        // frames, so we put in an expression key frame to overwrite any existing ones.
        IFCFAILFAST(kfa->InsertExpressionKeyFrame(0.0f, wrl::Wrappers::HStringReference(L"this.StartingValue").Get()));
    }

    IFCFAILFAST(colorKFA->InsertKeyFrameWithEasingFunction(1.0f, toColor, m_brushEasingFunction.Get()));

    return m_brushAnimation.Get();
}

void SharedTransitionAnimations::SetAnimationDuration(
    const wrl::ComPtr<WUComp::IKeyFrameAnimation>& kfa,
    const wf::TimeSpan& duration)
{
    // WUC has limits of [1ms, 24 days] for its animation durations.
    if (duration.Duration > static_cast<long long>(CAnimation::s_maxWUCKeyFrameAnimationDurationInTicks))
    {
        IFCFAILFAST(kfa->put_Duration({ static_cast<long long>(CAnimation::s_maxWUCKeyFrameAnimationDurationInTicks) }));
    }
    else if (duration.Duration < CAnimation::s_minWUCKeyFrameAnimationDurationInTicks)
    {
        IFCFAILFAST(kfa->put_Duration({ CAnimation::s_minWUCKeyFrameAnimationDurationInTicks }));
    }
    else
    {
        IFCFAILFAST(kfa->put_Duration(duration));
    }
}
