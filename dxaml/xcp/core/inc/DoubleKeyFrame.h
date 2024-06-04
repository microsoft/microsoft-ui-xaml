// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyFrame.h"
#include "KeyFrameCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CCoreServices;
class CKeySpline;

class CDoubleKeyFrame : public CKeyFrame
{
protected:
    CDoubleKeyFrame(_In_ CCoreServices *pCore)
        : CKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE_RETURN(CDoubleKeyFrame, E_UNEXPECTED);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDoubleKeyFrame>::Index;
    }

public:
    XFLOAT m_rValue = 0.0f;
};

//
class CLinearDoubleKeyFrame final : public CDoubleKeyFrame
{
protected:
    CLinearDoubleKeyFrame(_In_ CCoreServices *pCore)
        : CDoubleKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CLinearDoubleKeyFrame() // !!! FOR UNIT TESTING ONLY !!!
        : CLinearDoubleKeyFrame(nullptr)
    {}
#endif

    DECLARE_CREATE(CLinearDoubleKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLinearDoubleKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float progress) override
    {
        return progress;
    }

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion
};

//
class CDiscreteDoubleKeyFrame final : public CDoubleKeyFrame
{
protected:
    CDiscreteDoubleKeyFrame(_In_ CCoreServices *pCore)
        : CDoubleKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CDiscreteDoubleKeyFrame()   // !!! FOR UNIT TESTING ONLY !!!
        : CDiscreteDoubleKeyFrame(nullptr)
    {}
#endif

    DECLARE_CREATE(CDiscreteDoubleKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDiscreteDoubleKeyFrame>::Index;
    }

    bool IsDiscrete() override { return true; }

    float GetEffectiveProgress(float progress) override
    {
        UNREFERENCED_PARAMETER(progress);
        // Discrete keyframes clamp to the starting value of the time segment
        return 0;
    }

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion
};

//
class CSplineDoubleKeyFrame : public CDoubleKeyFrame
{
protected:
    CSplineDoubleKeyFrame(_In_ CCoreServices *pCore)
        : CDoubleKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CSplineDoubleKeyFrame() // !!! FOR UNIT TESTING ONLY !!!
        : CSplineDoubleKeyFrame(nullptr)
    {}
#endif

    ~CSplineDoubleKeyFrame() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSplineDoubleKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float progress) override;

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion

public:
    CKeySpline *m_pKeySpline = nullptr;
};

//
class CEasingDoubleKeyFrame final : public CDoubleKeyFrame
{
protected:
    CEasingDoubleKeyFrame(_In_ CCoreServices *pCore)
        : CDoubleKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CEasingDoubleKeyFrame() // !!! FOR UNIT TESTING ONLY !!!
        : CEasingDoubleKeyFrame(nullptr)
    {}
#endif

    ~CEasingDoubleKeyFrame() override;

    DECLARE_CREATE(CEasingDoubleKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEasingDoubleKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float progress) override;

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion

// Easing function for the animation.
// Native easing functions all derive from CEasingFunctionBase. Managed ones must implement IEasingFunction.
    CDependencyObject *m_pEasingFunction = nullptr;
};

//
class CDoubleKeyFrameCollection final : public CKeyFrameCollection
{
private:
    CDoubleKeyFrameCollection(_In_ CCoreServices *pCore)
        : CKeyFrameCollection(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CDoubleKeyFrameCollection() // !!! FOR UNIT TESTING ONLY !!!
        : CDoubleKeyFrameCollection(nullptr)
    {}
#endif

    DECLARE_CREATE(CDoubleKeyFrameCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDoubleKeyFrameCollection>::Index;
    }
};
