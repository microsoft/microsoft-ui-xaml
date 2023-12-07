// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyFrame.h"
#include "KeyFrameCollection.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CCoreServices;

class CColorKeyFrame : public CKeyFrame
{
protected:
    CColorKeyFrame( _In_ CCoreServices *pCore )
        : CKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE_RETURN(CColorKeyFrame, E_UNEXPECTED);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColorKeyFrame>::Index;
    }

public:
    XUINT32 m_uValue = 0xFF000000;
};

class CLinearColorKeyFrame final : public CColorKeyFrame
{
protected:
    CLinearColorKeyFrame( _In_ CCoreServices *pCore )
        : CColorKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CLinearColorKeyFrame()  // !!! FOR UNIT TESTING ONLY !!!
        : CLinearColorKeyFrame(nullptr)
    {}
#endif

    DECLARE_CREATE(CLinearColorKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLinearColorKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(_In_ XFLOAT rCurrentProgress) override
    {
        return rCurrentProgress;
    }

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion
};

class CDiscreteColorKeyFrame final : public CColorKeyFrame
{
protected:
    CDiscreteColorKeyFrame( _In_ CCoreServices *pCore )
        : CColorKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CDiscreteColorKeyFrame()    // !!! FOR UNIT TESTING ONLY !!!
        : CDiscreteColorKeyFrame(nullptr)
    {}
#endif

    DECLARE_CREATE(CDiscreteColorKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDiscreteColorKeyFrame>::Index;
    }

    bool IsDiscrete() override { return true; }

    float GetEffectiveProgress(float rCurrentProgress) override
    {
        UNREFERENCED_PARAMETER(rCurrentProgress);
        // Discrete keyframes clamp to the starting value of the time segment
        return 0;
    }

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion
};

class CSplineColorKeyFrame : public CColorKeyFrame
{
protected:
    CSplineColorKeyFrame( _In_ CCoreServices *pCore )
        : CColorKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CSplineColorKeyFrame()  // !!! FOR UNIT TESTING ONLY !!!
        : CSplineColorKeyFrame(nullptr)
    {}
#endif

    ~CSplineColorKeyFrame() override;

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSplineColorKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override;

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion

public:
    CKeySpline* m_pKeySpline = nullptr;
};

class CEasingColorKeyFrame final : public CColorKeyFrame
{
protected:
    CEasingColorKeyFrame( _In_ CCoreServices *pCore )
        : CColorKeyFrame(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CEasingColorKeyFrame()  // !!! FOR UNIT TESTING ONLY !!!
        : CEasingColorKeyFrame(nullptr)
    {}
#endif

    ~CEasingColorKeyFrame() override;

    DECLARE_CREATE(CEasingColorKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEasingColorKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override;

#pragma region ::Windows::UI::Composition

    CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation) override;

#pragma endregion

public:
// Easing function for the animation.
// Native easing functions all derive from CEasingFunctionBase. Managed ones must implement IEasingFunction.
    CDependencyObject *m_pEasingFunction = nullptr;
};

class CColorKeyFrameCollection final : public CKeyFrameCollection
{
private:
    CColorKeyFrameCollection(_In_ CCoreServices *pCore)
        : CKeyFrameCollection(pCore)
    {}

public:
#if defined(__XAML_UNITTESTS__)
    CColorKeyFrameCollection()  // !!! FOR UNIT TESTING ONLY !!!
        : CColorKeyFrameCollection(nullptr)
    {}
#endif

    DECLARE_CREATE(CColorKeyFrameCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColorKeyFrameCollection>::Index;
    }
};
