// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CHyperlinkAutomationPeer: public CAutomationPeer
{
protected:
    CHyperlinkAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CAutomationPeer(pCore, value)
    {
        SetIsCustomType();
    }

    ~CHyperlinkAutomationPeer() override
    {
    }

public:
    DECLARE_CREATE_AP(CHyperlinkAutomationPeer);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::HyperlinkAutomationPeer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    CAutomationPeer* GetAPParent() override
    {
        // Hyperlink is focusable, but not derived from FrameworkElement,
        // hence this method might get called before our parent is actually
        // set and we will need to walk the tree.
        return __super::GetLogicalAPParent();
    }

};
