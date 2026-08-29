// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//
#pragma once

class CWindow: public CDependencyObject
{
protected:
    CWindow(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {
        SetIsCustomType();
    }

    ~CWindow() override
    {
    }

    // CWindow handles the private Window_Content property set by the parser in SetValue
    // (rather than storing it in the DP) in order to support markup for the Window element.
    _Check_return_ HRESULT SetValue(_In_ const SetValueParams& args) override;

public:
    DECLARE_CREATE(CWindow);

    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Window;
    }

    XUINT32 ParticipatesInManagedTreeInternal() override
    {
        return PARTICIPATES_IN_MANAGED_TREE;
    }

};

