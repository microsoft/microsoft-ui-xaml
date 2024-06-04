// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once


class CFlyout: public CFlyoutBase
{
protected:
    CFlyout(_In_ CCoreServices *pCore)
        : CFlyoutBase(pCore)
    {
        SetIsCustomType();
    }

    ~CFlyout() override
    {
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) override;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) override;
  
public:
    DECLARE_CREATE(CFlyout);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Flyout;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

};
