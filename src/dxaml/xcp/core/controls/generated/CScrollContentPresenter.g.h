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

#include "ContentPresenter.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CScrollContentPresenter : public CContentPresenter
{
protected:
    CScrollContentPresenter(_In_ CCoreServices *pCore)
        : CContentPresenter(pCore)
    {
        SetIsCustomType();
    }

    ~CScrollContentPresenter() override = default;

public:
    DECLARE_CREATE(CScrollContentPresenter);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::ScrollContentPresenter;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
