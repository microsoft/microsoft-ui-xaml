// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TimelineGroup.h"
#include <TimelineCollection.h>

void CTimelineGroup::InitializeIteration()
{
    if (m_pChild != nullptr)
    {
        for (auto item : *m_pChild)
        {
            static_cast<CTimeline*>(item)->InitializeIteration();
        }
    }

    __super::InitializeIteration();
}

