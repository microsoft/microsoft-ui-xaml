// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "DynamicTimelineHelper.h"

#include <CDependencyObject.h>
#include <dopointercast.h>

#include <DynamicTimeline.h>
#include <storyboard.h>
#include <TimelineCollection.h>

void Jupiter::Animation::PropagateDynamicTimelineGenerationOptionToChildren(_In_ CStoryboard* target, DynamicTimelineGenerationMode mode)
{
    if (!target->m_pChild) return;
    for (auto& child : *target->m_pChild)
    {
        auto asStoryboard = do_pointer_cast<CStoryboard>(child);
        if (asStoryboard)
        {
            PropagateDynamicTimelineGenerationOptionToChildren(asStoryboard, mode);
            continue;
        }

        auto asDynamicTimeline = do_pointer_cast<CDynamicTimeline>(child);
        if (asDynamicTimeline)
        {
            asDynamicTimeline->SetGenerationMode(mode);
            continue;
        }
    }
}

_Check_return_ HRESULT Jupiter::Animation::TryGenerateDynamicChildrenStoryboardsForChildren(_In_ CStoryboard* target, _Out_opt_ bool* result)
{
    bool didAllGenerate = true;

    if (target->m_pChild)
    {
        for (auto& child : *target->m_pChild)
        {
            auto asStoryboard = do_pointer_cast<CStoryboard>(child);
            if (asStoryboard)
            {
                bool storyboardCanGenerate = false;
                IFC_RETURN(Jupiter::Animation::TryGenerateDynamicChildrenStoryboardsForChildren(asStoryboard, &storyboardCanGenerate));
                didAllGenerate = didAllGenerate && storyboardCanGenerate;
                continue;
            }

            auto asDynamicTimeline = do_pointer_cast<CDynamicTimeline>(child);
            if (asDynamicTimeline)
            {
                bool canGenerate = asDynamicTimeline->CanBeModified();
                if (canGenerate)
                {
                    IFC_RETURN(asDynamicTimeline->GenerateChildren());
                }

                didAllGenerate = didAllGenerate && canGenerate;
                continue;
            }
        }
    }

    if (result)
    {
        *result = didAllGenerate;
    }
    return S_OK;
}

