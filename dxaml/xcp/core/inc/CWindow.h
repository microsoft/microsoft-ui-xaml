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

    // CWindow needs to respond to property change notifications for Window_Content
    // in order to support nested markup for the Window element. 
    _Check_return_ HRESULT OnPropertyChanged(_In_ const PropertyChangedParams& args) override;

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

