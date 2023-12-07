// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "KeyFrameCollection.h"
#include <PointerKeyFrame.h>

// Returns the flattened sorted collection to the caller.
const CDOCollection::storage_type& CPointerKeyFrameCollection::GetSortedCollection()
{
    // Note: This thing can be replaced with std::stable_sort.

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
                // Sort on the source values
                leftValue = static_cast<CPointerKeyFrame*>((*this)[min])->m_pointerValue;
                rightValue = static_cast<CPointerKeyFrame*>((*this)[j])->m_pointerValue;

                if (leftValue > rightValue)
                {
                    min = j;
                }
            }
            if ( min != i)
            {
                swap(min, i);
            }
        }

        m_isSorted = true;
    }

   return GetCollection();
}

