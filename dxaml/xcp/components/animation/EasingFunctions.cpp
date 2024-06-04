// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "EasingFunctions.h"
#include "DCompAnimationConversionContext.h"

#ifndef FLOAT_PI
#define FLOAT_PI    (3.1415926535897932384626433832795f)
#endif

#ifndef FLOAT_E
#define FLOAT_E     (2.7182818284590452353602874713527f)
#endif

#ifndef FLOAT_EPSILON
#define FLOAT_EPSILON   (0.0001)
#endif

float CEasingFunctionBase::EaseValue(_In_ CDependencyObject* pEasingFunction, float value)
{
    if (pEasingFunction != nullptr)
    {
        CEasingFunctionBase *pEasing = static_cast<CEasingFunctionBase*>(pEasingFunction);
        return pEasing->Ease(value);
    }

    return value;
}

void CEasingFunctionBase::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    *easingFunction = nullptr;
}

ixp::CompositionEasingFunctionMode CEasingFunctionBase::GetWUCEasingMode() const
{
    switch (m_eEasingMode)
    {
        case DirectUI::EasingMode::EaseOut:
            return ixp::CompositionEasingFunctionMode::CompositionEasingFunctionMode_Out;

        case DirectUI::EasingMode::EaseIn:
            return ixp::CompositionEasingFunctionMode::CompositionEasingFunctionMode_In;

        case DirectUI::EasingMode::EaseInOut:
            return ixp::CompositionEasingFunctionMode::CompositionEasingFunctionMode_InOut;
    }

    IFCFAILFAST(E_FAIL);
    return ixp::CompositionEasingFunctionMode::CompositionEasingFunctionMode_InOut;
}

float CEasingFunctionImpl::Ease(float normalizedTime)
{
    switch (m_eEasingMode)
    {
        case DirectUI::EasingMode::EaseIn:
        {
            return EaseInCore(normalizedTime);
        }

        case DirectUI::EasingMode::EaseOut:
        {
            return 1.0f - EaseInCore(1.0f - normalizedTime);
        }

        case DirectUI::EasingMode::EaseInOut:
        {
            if (normalizedTime < 0.5f)
            {
                return EaseInCore(normalizedTime * 2.0f) / 2.0f;
            }
            else
            {
                return (1.0f - EaseInCore(2.0f - normalizedTime * 2.0f)) / 2.0f + 0.5f;
            }
        }
    }

    ASSERT(false);    // There is no other type of easing mode supported
    return 0;
}

XFLOAT CCircInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    if(normalizedTime < -1.0f)
    {
        normalizedTime = -1.0f;
    }

    if(normalizedTime > 1.0f)
    {
        normalizedTime = 1.0f;
    }

    return -1.0f * (sqrtf(1.0f - (normalizedTime * normalizedTime)) - 1.0f);
}

void CCircInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::ICircleEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreateCircleEasingFunction(compositor, wucEasingMode, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CBackInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    if (normalizedTime < 0.0f)
    {
        normalizedTime = 0.0f;
    }

    return (normalizedTime * normalizedTime * normalizedTime) - normalizedTime * m_fAmplitude * sinf(normalizedTime * FLOAT_PI);
}

void CBackInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IBackEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    // Although Xaml allows negative amplitudes, WUC's BackEasingFunction/XamlBackInterpolation do not. They have no other
    // interpolator that uses the same curve (t^3 - t*a*sin(t); see EaseInCore and WUC's XamlBackInterpolation::EvaluateCurve),
    // so we clamp the amplitude to something positive.
    float clampedAmplitude = m_fAmplitude > 0.0f ? m_fAmplitude : 1.0f;
    IFCFAILFAST(easingFunctionStatics->CreateBackEasingFunction(compositor, wucEasingMode, clampedAmplitude, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CExponentialInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    if(m_fExponent >= -FLOAT_EPSILON && m_fExponent <= FLOAT_EPSILON)
    {
        return normalizedTime;
    }

    return (powf(FLOAT_E, m_fExponent * normalizedTime) - 1.0f) / (powf(FLOAT_E, m_fExponent) - 1.0f);
}

void CExponentialInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IExponentialEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreateExponentialEasingFunction(compositor, wucEasingMode, m_fExponent, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CPowerInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    XFLOAT EffectivePower = MAX(m_fPower, 0.0f);

    return powf(normalizedTime, EffectivePower);
}

void CPowerInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IPowerEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreatePowerEasingFunction(compositor, wucEasingMode, m_fPower, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CQuadraticInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    return normalizedTime * normalizedTime;
}

void CQuadraticInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IPowerEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreatePowerEasingFunction(compositor, wucEasingMode, 2.0f, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CCubicInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    return normalizedTime * normalizedTime * normalizedTime;
}

void CCubicInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IPowerEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreatePowerEasingFunction(compositor, wucEasingMode, 3.0f, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CQuarticInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    return normalizedTime * normalizedTime * normalizedTime * normalizedTime;
}

void CQuarticInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IPowerEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreatePowerEasingFunction(compositor, wucEasingMode, 4.0f, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CQuinticInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    return normalizedTime * normalizedTime * normalizedTime * normalizedTime * normalizedTime;
}

void CQuinticInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IPowerEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreatePowerEasingFunction(compositor, wucEasingMode, 5.0f, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CElasticInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    XFLOAT Oscillations = static_cast<XFLOAT>(GetClampedOscillations());

    XFLOAT ExponentialModifier;

    if(m_fSpringiness >= -FLOAT_EPSILON && m_fSpringiness <= FLOAT_EPSILON)
    {
        ExponentialModifier = normalizedTime;
    }
    else
    {
        ExponentialModifier = ((powf(FLOAT_E, m_fSpringiness * normalizedTime) - 1.0f) / (powf(FLOAT_E, m_fSpringiness) - 1.0f));
    }

    return ExponentialModifier * (sinf(normalizedTime * ((2.0f * FLOAT_PI * Oscillations) + (FLOAT_PI / 2.0f))));
}

void CElasticInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IElasticEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreateElasticEasingFunction(
        compositor,
        wucEasingMode,
        GetClampedOscillations(),
        m_fSpringiness,
        derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

float CBounceInterpolator::GetClampedBounciness() const
{
    if (m_fBounciness <= 1.0f + FLOAT_EPSILON)
    {
        return 1.01f;
    }
    else
    {
        return m_fBounciness;
    }
}

XFLOAT CBounceInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    // constants
    XFLOAT bounces = (m_iBounces >= 0) ? (XFLOAT)m_iBounces : 0.0f;

    // Clamp the bounciness so we don't hit a divide by zero
    XFLOAT bounciness = GetClampedBounciness();

    XFLOAT pow = powf(bounciness, bounces);
    XFLOAT oneMinusBounciness = 1.0f - bounciness;

    // 'unit' space calculations, geometric series with only half the last sum
    XFLOAT sumOfUnits = (1.0f - pow) / oneMinusBounciness + pow / 2.0f;
    XFLOAT unitAtT = normalizedTime * sumOfUnits;

    // 'bounce' space calculations
    XFLOAT bounceAtT = (XFLOAT)(log((XDOUBLE)(-unitAtT * (1.0f - bounciness) + 1.0f)) / log((XDOUBLE)bounciness));
    XFLOAT start = (XFLOAT)floorf(bounceAtT);
    XFLOAT end = start + 1.0f;

    // 'time' space calculations
    XFLOAT startTime = (1.0f - powf(bounciness, start)) /
        (oneMinusBounciness * sumOfUnits);
    XFLOAT endTime = (1.0f - powf(bounciness, end)) /
        (oneMinusBounciness * sumOfUnits);

    // Curve fitting for bounce.
    XFLOAT midTime = (startTime + endTime) / 2.0f;
    XFLOAT timeRelativeToPeak = normalizedTime - midTime;
    XFLOAT radius = midTime - startTime;
    XFLOAT amplitude = powf(1.0f / bounciness, (bounces - start));

    // Evaluate a quadratic that hits (startTime, 0), (endTime, 0), and peaks at amplitude
    return (-amplitude / (radius * radius)) * (timeRelativeToPeak - radius) * (timeRelativeToPeak + radius);
}

void CBounceInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::IBounceEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();

    // WUC requires a minimum bounciness of 1. Clamp it like we do when Xaml does the math in EaseInCore.
    float bounciness = GetClampedBounciness();

    IFCFAILFAST(easingFunctionStatics->CreateBounceEasingFunction(compositor, wucEasingMode, m_iBounces, bounciness, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}

XFLOAT CSineInterpolator::EaseInCore(_In_ XFLOAT normalizedTime)
{
    return 1.0f - sinf((1.0f - normalizedTime) * (FLOAT_PI / 2.0f));
}

void CSineInterpolator::GetWUCEasingFunction(
    _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
    _In_ ixp::ICompositor* compositor,
    _Outptr_ ixp::ICompositionEasingFunction** easingFunction)
{
    wrl::ComPtr<ixp::ISineEasingFunction> derivedEasingFunction;
    const auto& wucEasingMode = GetWUCEasingMode();
    IFCFAILFAST(easingFunctionStatics->CreateSineEasingFunction(compositor, wucEasingMode, derivedEasingFunction.ReleaseAndGetAddressOf()));
    IFCFAILFAST(derivedEasingFunction->QueryInterface(IID_PPV_ARGS(easingFunction)));
}
