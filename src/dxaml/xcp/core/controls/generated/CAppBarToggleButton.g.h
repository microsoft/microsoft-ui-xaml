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

#include "CToggleButton.g.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CAppBarToggleButton : public CToggleButton
{
protected:
    CAppBarToggleButton(_In_ CCoreServices *pCore)
        : CToggleButton(pCore)
    {
        SetIsCustomType();
    }

    ~CAppBarToggleButton() override = default;

public:
    DECLARE_CREATE(CAppBarToggleButton);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::AppBarToggleButton;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
