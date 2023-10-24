// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PointerKeyFrame.h"
#include "KeyTime.h"

// Scans the internal keyframe collection and makes it uniform
_Check_return_ HRESULT CPointerKeyFrameCollection::InitializeKeyFrames(_In_ XFLOAT rOneIterationDuration)
{
    XUINT32 nFrameCount = 0;
    m_isSorted = TRUE;
    XFLOAT min = 0.0f;

    nFrameCount = GetCount();

    // If our duration is zero or less, then coerce this value to avoid a division by zero
    //   or unexpected behavior with negative time.
    if (rOneIterationDuration <= 0.0f)
    {
        rOneIterationDuration = 1.0f;
    }

    // At some point we should use stl::min_element here

    if (nFrameCount > 0)
    {
        min = static_cast<CPointerKeyFrame*>((*this)[0])->m_pointerValue;
    }

    // Find the smallest value.
    for (XUINT32 i = 1; i < nFrameCount; i++)
    {
        XFLOAT current = static_cast<CPointerKeyFrame*>((*this)[i])->m_pointerValue;

        if (min > current)
        {
            m_isSorted = FALSE;
            min = current;
        }
    }

    for (auto item : (*this))
    {
        CPointerKeyFrame* currentFrame = static_cast<CPointerKeyFrame*>(item);
        XFLOAT current = currentFrame->m_pointerValue;

        currentFrame->m_keyTime = KeyTimeVOHelper::Create(
            GetContext(),
            KeyTimeVO::s_defaultTimeSpanInSec,
            (current - min) / rOneIterationDuration);
    }

    return S_OK;
}
