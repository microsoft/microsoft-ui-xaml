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

#include "CMenuFlyoutItemBase.g.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CMenuFlyoutSeparator : public CMenuFlyoutItemBase
{
protected:
    CMenuFlyoutSeparator(_In_ CCoreServices *pCore)
        : CMenuFlyoutItemBase(pCore)
    {
        SetIsCustomType();
    }

    ~CMenuFlyoutSeparator() override = default;

public:
    DECLARE_CREATE(CMenuFlyoutSeparator);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::MenuFlyoutSeparator;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
