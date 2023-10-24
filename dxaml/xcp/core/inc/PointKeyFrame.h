// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "KeyFrame.h"
#include "KeyFrameCollection.h"

class CCoreServices;
class CKeySpline;

class CPointKeyFrame : public CKeyFrame
{
protected:
    CPointKeyFrame( _In_ CCoreServices *pCore )
        : CKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE_RETURN(CPointKeyFrame, E_UNEXPECTED);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointKeyFrame>::Index;
    }

public:
    XPOINTF m_ptValue = {};
};

class CLinearPointKeyFrame final : public CPointKeyFrame
{
protected:
    CLinearPointKeyFrame( _In_ CCoreServices *pCore )
        : CPointKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE(CLinearPointKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CLinearPointKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override
    {
        return rCurrentProgress;
    }
};

class CDiscretePointKeyFrame final : public CPointKeyFrame
{
protected:
    CDiscretePointKeyFrame( _In_ CCoreServices *pCore )
        : CPointKeyFrame(pCore)
    {}

public:
    DECLARE_CREATE(CDiscretePointKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDiscretePointKeyFrame>::Index;
    }

    bool IsDiscrete() override { return true; }

    float GetEffectiveProgress(float rCurrentProgress) override
    {
        // Discrete keyframes clamp to the starting value of the time segment
        return 0;
    }
};

class CSplinePointKeyFrame : public CPointKeyFrame
{
protected:
    CSplinePointKeyFrame( _In_ CCoreServices *pCore )
        : CPointKeyFrame(pCore)
    {}

    ~CSplinePointKeyFrame() override;

public:
    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSplinePointKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override;

public:
    CKeySpline *m_pKeySpline = nullptr;
};

class CEasingPointKeyFrame final : public CPointKeyFrame
{
protected:
    CEasingPointKeyFrame( _In_ CCoreServices *pCore )
        : CPointKeyFrame(pCore)
    {}

    ~CEasingPointKeyFrame() override;

public:
    DECLARE_CREATE(CEasingPointKeyFrame);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CEasingPointKeyFrame>::Index;
    }

    bool IsDiscrete() override { return false; }

    float GetEffectiveProgress(float rCurrentProgress) override;

// Easing function for the animation.
// Native easing functions all derive from CEasingFunctionBase. Managed ones must implement IEasingFunction.
    CDependencyObject *m_pEasingFunction = nullptr;
};

class CPointKeyFrameCollection final : public CKeyFrameCollection
{
private:
    CPointKeyFrameCollection(_In_ CCoreServices *pCore)
        : CKeyFrameCollection(pCore)
    {}

public:
    DECLARE_CREATE(CPointKeyFrameCollection);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPointKeyFrameCollection>::Index;
    }
};
