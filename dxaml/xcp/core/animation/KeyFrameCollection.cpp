// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyFrameCollection.h"
#include "KeyFrame.h"
#include "KeyTime.h"

// Scans the internal keyframe collection and makes it uniform
_Check_return_ HRESULT CKeyFrameCollection::InitializeKeyFrames(_In_ XFLOAT rOneIterationDuration)
{
    m_isSorted = true;
    XFLOAT min = 0.0f;

    // If our duration is zero or less, then coerce this value to avoid a division by zero
    //   or unexpected behavior with negative time.
    if (rOneIterationDuration <= 0.0f)
    {
        rOneIterationDuration = 1.0f;
    }

    for (auto item : (*this))
    {
        CKeyFrame* currentFrame = static_cast<CKeyFrame*>(item);
        const KeyTimeVO::Wrapper* currentKeyTime = currentFrame->m_keyTime;

        if (!currentKeyTime)
        {
            IFC_RETURN(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_KEYTIME));
        }

        {
            const auto& value = currentKeyTime->Value();
            float newPercent = static_cast<float>(value.GetTimeSpanInSec()) / rOneIterationDuration;

            if (newPercent != value.GetPercent())
            {
                currentFrame->m_keyTime = KeyTimeVOHelper::Create(
                    GetContext(),
                    value.GetTimeSpanInSec(),
                    newPercent);

                currentKeyTime = currentFrame->m_keyTime;
            }
        }

        if (min > currentKeyTime->Value().GetPercent())
        {
            m_isSorted = false;
        }

        min = currentKeyTime->Value().GetPercent();
    }

    return S_OK;
}

void CKeyFrameCollection::OriginateInvalidKeyFrameError()
{
    IFCFAILFAST(SetAndOriginateError(E_NER_INVALID_OPERATION, RuntimeError, AG_E_RUNTIME_SB_BEGIN_INVALID_KEYTIME));
}
