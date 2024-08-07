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

class CDatePicker : public CControl
{
protected:
    CDatePicker(_In_ CCoreServices *pCore)
        : CControl(pCore)
    {
        SetIsCustomType();
    }

    ~CDatePicker() override = default;

public:
    DECLARE_CREATE(CDatePicker);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::DatePicker;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
