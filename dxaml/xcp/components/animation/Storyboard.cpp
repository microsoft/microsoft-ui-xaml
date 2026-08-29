// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Storyboard.h"
#include "TimelineCollection.h"
#include "DCompAnimationConversionContext.h"
#include "XStringBuilder.h"
#include "UIElement.h"
#include "VisualState.h"
#include "TypeTableStructs.h"
#include "DOPointerCast.h"

void CStoryboard::SynchronizeDCompAnimationAfterResume(double timeManagerTime)
{
    if (IsInActiveState())
    {
        SeekDCompAnimationInstances(timeManagerTime + m_rTimeDelta);
    }
}

void CStoryboard::FireDCompAnimationCompleted()
{
    OnDCompAnimationCompleted();

    for (auto item : *m_pChild)
    {
        static_cast<CTimeline*>(item)->OnDCompAnimationCompleted();
    }
}

bool CStoryboard::IsPaused() const
{
    return m_fIsPaused;
}

CompositionAnimationConversionResult CStoryboard::MakeCompositionAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    return __super::MakeCompositionAnimationVirtual(myContext);
}

