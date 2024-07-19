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

#include "CControl.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CAppBarSeparator : public CControl
{
protected:
    CAppBarSeparator(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {
        SetIsCustomType();
    }

    ~CAppBarSeparator() override = default;

public:
    DECLARE_CREATE(CAppBarSeparator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::AppBarSeparator;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
