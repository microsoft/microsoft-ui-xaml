// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CRichTextBlockAutomationPeer: public CFrameworkElementAutomationPeer
{
protected:
    CRichTextBlockAutomationPeer(_In_ CCoreServices *pCore, _In_ CValue &value)
        : CFrameworkElementAutomationPeer(pCore, value)
    {
        SetIsCustomType();
    }

    ~CRichTextBlockAutomationPeer() override
    {
    }

public:
    DECLARE_CREATE_AP(CRichTextBlockAutomationPeer);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::RichTextBlockAutomationPeer;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

    _Check_return_ HRESULT HasKeyboardFocusHelper(_Out_ BOOLEAN* pRetVal) final;
    _Check_return_ HRESULT IsKeyboardFocusableHelper(_Out_ BOOLEAN* pRetVal) final;
};