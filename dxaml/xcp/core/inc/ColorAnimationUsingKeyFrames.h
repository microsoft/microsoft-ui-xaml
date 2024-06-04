// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ColorAnimation.h"

class CCoreServices;

// Object created for <ColorAnimationUsingKeyFrames> tag
class CColorAnimationUsingKeyFrames final : public CColorAnimation
{
public:
    CColorAnimationUsingKeyFrames()
        : CColorAnimationUsingKeyFrames(nullptr)
    {
    }

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CColorAnimationUsingKeyFrames>::Index;
    }

protected:
#pragma region ::Windows::UI::Composition

    _Check_return_ CompositionAnimationConversionResult MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext) override;

#pragma endregion

private:
    CColorAnimationUsingKeyFrames(_In_ CCoreServices *pCore)
        : CColorAnimation(pCore)
    {
        m_fUsesKeyFrames = true;
    }
};
