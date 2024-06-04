// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CAppBar.g.h"

class CCommandBar: public CAppBar
{
protected:
    CCommandBar(_In_ CCoreServices *pCore)
        : CAppBar(pCore)
    {
        SetIsCustomType();
    }

    _Check_return_ HRESULT EnterImpl(_In_ CDependencyObject *pNamescopeOwner, EnterParams params) final;
    _Check_return_ HRESULT LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params) final;

    ~CCommandBar() override
    {
    }

public:
    DECLARE_CREATE(CCommandBar);

    KnownTypeIndex GetTypeIndex() const final
    {
        return KnownTypeIndex::CommandBar;
    }

    XUINT32 ParticipatesInManagedTreeInternal() final
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

};
