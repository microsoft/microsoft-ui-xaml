// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "DynamicTimeline.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CPointerUpThemeAnimation : public CDynamicTimeline
{
protected:
    CPointerUpThemeAnimation(_In_ CCoreServices *pCore)
        : CDynamicTimeline(pCore)
    {
        SetIsCustomType();
    }

    ~CPointerUpThemeAnimation() override = default;

public:
    DECLARE_CREATE(CPointerUpThemeAnimation);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::PointerUpThemeAnimation;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
