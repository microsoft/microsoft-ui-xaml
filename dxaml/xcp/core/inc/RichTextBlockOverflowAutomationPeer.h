// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRichTextBlockOverflowAutomationPeer: public CFrameworkElementAutomationPeer
{
protected:
    CRichTextBlockOverflowAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CFrameworkElementAutomationPeer(pCore, value)
    {
        SetIsCustomType();
    }

    ~CRichTextBlockOverflowAutomationPeer() override
    {
    }

public:
    DECLARE_CREATE_AP(CRichTextBlockOverflowAutomationPeer);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::RichTextBlockOverflowAutomationPeer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal) final;

};