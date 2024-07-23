// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "DCompAnimationConversionContext.h"
#include "KeyFrameCollection.h"
#include "DoubleKeyFrame.h"
#include "KeyTime.h"
#include <DependencyObjectDCompRegistry.h>
#include <Duration.h>
#include <FloatUtil.h>
#include <TimeMgr.h>

_Check_return_ CompositionAnimationConversionResult CDoubleAnimationUsingKeyFrames::MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    wrl::ComPtr<WUComp::IScalarKeyFrameAnimation> scalarAnimation = myContext->CreateEmptyScalarAnimation();
    scalarAnimation.As(&m_spWUCAnimation);
    IFC_ANIMATION(myContext->ApplyProperties(m_spWUCAnimation.Get()));
    UpdateWUCAnimationTelemetryString(myContext);

    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
    }

    // Add an initial key frame if there's no handoff. Otherwise, ::Windows::UI::Composition will automatically add a key frame
    // for this.StartingValue.
    if (!m_hasHandoff)
    {
        // m_vBaseValue was updated from the animation target in CDoubleAnimation::GetAnimationBaseValue. It represents
        // the unanimated value of the property.
        scalarAnimation->InsertKeyFrame(0.0f, m_vBaseValue);
    }

    if (m_pKeyFrames != nullptr && m_pKeyFrames->GetCount() > 0)
    {
        IFC_ANIMATION(m_pKeyFrames->AddCompositionKeyFrames(myContext, m_spWUCAnimation.Get()));
    }

    return CompositionAnimationConversionResult::Success;
}
