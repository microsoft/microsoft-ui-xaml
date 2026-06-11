// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <DoubleAnimation.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CCoreServices;

class CDoubleAnimationUsingKeyFrames : public CDoubleAnimation
{
public:
    CDoubleAnimationUsingKeyFrames()
        : CDoubleAnimation()
        // The CDoubleAnimation(CCoreServices) ctor asserts that metadata lookups return non-null. Unit tests violate that assert.
    {
        m_fUsesKeyFrames = true;
    }

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CDoubleAnimationUsingKeyFrames>::Index;
    }

protected:
    CDoubleAnimationUsingKeyFrames(_In_ CCoreServices *pCore)
        : CDoubleAnimation(pCore)
    {
        m_fUsesKeyFrames = true;
    }

#pragma region ::Windows::UI::Composition

    _Check_return_ CompositionAnimationConversionResult MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

#pragma endregion

};
