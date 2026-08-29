// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyFrameCollection.h"
#include "DCompAnimationConversionContext.h"
#include <KeyFrame.h>
#include <KeyTime.h>

// Returns the flattened sorted collection to the caller.
const CDOCollection::storage_type& CKeyFrameCollection::GetSortedCollection()
{
    // Note: Aside from the key time checks, this thing can be replaced with std::stable_sort.

    unsigned int min = 0;
    float leftValue = 0.0f;
    float rightValue = 0.0f;

    if (!m_isSorted)
    {
        for (unsigned int i = 0; i < GetCount(); i++)
        {
            min = i;
            for (unsigned int j = i + 1; j < GetCount(); j++)
            {
                // if we have non-TimeSpan KeyTime, fail
                if (static_cast<CKeyFrame*>((*this)[min])->m_keyTime == nullptr
                    || static_cast<CKeyFrame*>((*this)[j])->m_keyTime == nullptr)
                {
                    OriginateInvalidKeyFrameError();
                }

                // Sort on the TimeSpan values
                leftValue = static_cast<float>(static_cast<CKeyFrame*>((*this)[min])->m_keyTime->Value().GetTimeSpanInSec());
                rightValue = static_cast<float>(static_cast<CKeyFrame*>((*this)[j])->m_keyTime->Value().GetTimeSpanInSec());

                if (leftValue > rightValue)
                {
                    min = j;
                }
            }

            if (min != i)
            {
                swap(min, i);
            }
        }

         m_isSorted = true;
    }

    return GetCollection();
}

CompositionAnimationConversionResult CKeyFrameCollection::AddCompositionKeyFrames(
    _In_ CompositionAnimationConversionContext* context,
    _Inout_ WUComp::IKeyFrameAnimation* animation)
{
    for (auto item : *this)
    {
        CKeyFrame* pKeyFrameNoRef = static_cast<CKeyFrame*>(item);
        IFC_ANIMATION(pKeyFrameNoRef->AddCompositionKeyFrame(context, animation));
    }

    return CompositionAnimationConversionResult::Success;
}

