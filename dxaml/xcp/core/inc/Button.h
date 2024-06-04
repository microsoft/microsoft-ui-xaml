// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ContentControl.h"
#include "CButtonBase.g.h"

class CButton : public CButtonBase
{
protected:
    CButton(_In_ CCoreServices *pCore)
        : CButtonBase(pCore)
    {
        SetIsCustomType();
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

    ~CButton() override
    {
    }

public:
    DECLARE_CREATE(CButton);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Button;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

};
