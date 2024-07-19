// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"
#include "KeyTime.h"
#include "DCompAnimationConversionContext.h"

// Abstract base class for keyframe types. Derived types will add
// actual value, the base class deals only with time values.

class CKeyFrame : public CDependencyObject
{
protected:
    CKeyFrame(_In_ CCoreServices* core)
        : CDependencyObject(core)
    {}

    _Check_return_ HRESULT InitInstance() override;

public:
    DECLARE_CREATE_RETURN(CKeyFrame, E_UNEXPECTED);

    KnownTypeIndex GetTypeIndex() const override;

    virtual bool IsDiscrete() = 0;

    virtual float GetEffectiveProgress(float progress) = 0;

#pragma region ::Windows::UI::Composition

    virtual CompositionAnimationConversionResult AddCompositionKeyFrame(
        _In_ CompositionAnimationConversionContext* context,
        _Inout_ WUComp::IKeyFrameAnimation* animation)
    {
        return CompositionAnimationConversionResult::Success;
    }

#pragma endregion

#if defined(__XAML_UNITTESTS__)
    // !!! FOR UNIT TESTING ONLY !!!

    void SetKeyTime(CKeyTime* keyTime)
    {
        m_keyTime = keyTime->ValueWrapper();
    }

#endif

public:
    xref_ptr<KeyTimeVO::Wrapper> m_keyTime;
};