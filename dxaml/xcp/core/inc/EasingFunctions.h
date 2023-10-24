// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "NoParentShareableDependencyObject.h"
#include "EnumDefs.g.h"
#include <fwd/windows.ui.composition.h>

//
// Base interface class for core easing functions.
//
class CEasingFunctionBase : public CNoParentShareableDependencyObject
{
protected:
    CEasingFunctionBase(_In_ CCoreServices *pCore)
        : CNoParentShareableDependencyObject(pCore)
    {}

public:
// Creation function
    DECLARE_CREATE(CEasingFunctionBase);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEasingFunctionBase>::Index;
    }

    virtual float Ease(float normalizedTime)
    {
        UNREFERENCED_PARAMETER(normalizedTime);
        XAML_FAIL_FAST();
        return 0;
    }

    virtual float EaseInCore(float normalizedTime)
    {
        return normalizedTime;
    }

    // Should the managed peer tree be updated during SetParent?
    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    static float EaseValue(_In_ CDependencyObject* pEasingFunction, float value);

    ixp::CompositionEasingFunctionMode GetWUCEasingMode() const;

    virtual void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction);

    DirectUI::EasingMode m_eEasingMode = DirectUI::EasingMode::EaseOut;
};

//
// Common easing function implementation wrapper class.
//
class CEasingFunctionImpl : public CEasingFunctionBase
{
protected:
    CEasingFunctionImpl(_In_ CCoreServices *pCore)
        : CEasingFunctionBase(pCore)
    {}

public:
    float Ease(float normalizedTime) final;
};

//
// Circle
//
class CCircInterpolator final : public CEasingFunctionImpl
{
public:
    CCircInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CCircInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCircInterpolator>::Index;
    }

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;
};

//
// Back
//
class CBackInterpolator final : public CEasingFunctionImpl
{
public:
    CBackInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CBackInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBackInterpolator>::Index;
    }

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float m_fAmplitude = 1.0f;
};

//
// Exponential
//
class CExponentialInterpolator final : public CEasingFunctionImpl
{
public:
    CExponentialInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CExponentialInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CExponentialInterpolator>::Index;
    }

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float m_fExponent = 2.0f;
};

//
// Power
//
class CPowerInterpolator final : public CEasingFunctionImpl
{
public:
    CPowerInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CPowerInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPowerInterpolator>::Index;
    }

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float m_fPower = 2.0f;
};

//
// Quadratic
//
class CQuadraticInterpolator final : public CEasingFunctionImpl
{
public:
    CQuadraticInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CQuadraticInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CQuadraticInterpolator>::Index;
    }

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float EaseInCore(float normalizedTime) override;
};

//
// Cubic
//
class CCubicInterpolator final : public CEasingFunctionImpl
{
public:
    CCubicInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CCubicInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CCubicInterpolator>::Index;
    }

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float EaseInCore(float normalizedTime) override;
};

//
// Quartic
//
class CQuarticInterpolator final : public CEasingFunctionImpl
{
public:
    CQuarticInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CQuarticInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CQuarticInterpolator>::Index;
    }

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float EaseInCore(float normalizedTime) override;
};

//
// Quintic
//
class CQuinticInterpolator final : public CEasingFunctionImpl
{
public:
    CQuinticInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CQuinticInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CQuinticInterpolator>::Index;
    }

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float EaseInCore(float normalizedTime) override;
};

//
// Elastic
//
class CElasticInterpolator final : public CEasingFunctionImpl
{
public:
    CElasticInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CElasticInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CElasticInterpolator>::Index;
    }

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    int32_t GetClampedOscillations() const
    {
        return (m_iOscillations >= 0) ? m_iOscillations : 0;
    }

    int32_t m_iOscillations = 3;
    float m_fSpringiness = 3.0f;
};

//
// Bounce
//
class CBounceInterpolator final : public CEasingFunctionImpl
{
public:
    CBounceInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CBounceInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CBounceInterpolator>::Index;
    };

    float EaseInCore(float normalizedTime) override;

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    XINT32  m_iBounces = 3;
    float  m_fBounciness = 2.0f;

private:
    float GetClampedBounciness() const;
};

//
// Sine
//
class CSineInterpolator final : public CEasingFunctionImpl
{
public:
    CSineInterpolator(_In_ CCoreServices* pCore)
        : CEasingFunctionImpl(pCore)
    {}

    DECLARE_CREATE(CSineInterpolator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSineInterpolator>::Index;
    };

    void GetWUCEasingFunction(
        _In_ ixp::ICompositionEasingFunctionStatics* easingFunctionStatics,
        _In_ ixp::ICompositor* compositor,
        _Outptr_ ixp::ICompositionEasingFunction** easingFunction) override;

    float EaseInCore(float normalizedTime) override;
};
