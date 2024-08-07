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

#include "FrameworkElementAutomationPeer.h"
#include <DeclareMacros.h>
#include <Indexes.g.h>
#include <minxcptypes.h>

class CSplitViewPaneAutomationPeer : public CFrameworkElementAutomationPeer
{
protected:
    CSplitViewPaneAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CFrameworkElementAutomationPeer(pCore, value)
    {
        SetIsCustomType();
    }

    ~CSplitViewPaneAutomationPeer() override = default;

public:
    DECLARE_CREATE_AP(CSplitViewPaneAutomationPeer);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::SplitViewPaneAutomationPeer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }
};
