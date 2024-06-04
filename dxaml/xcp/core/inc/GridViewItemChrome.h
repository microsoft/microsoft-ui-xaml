// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CGridViewItemChrome : public CListViewBaseItemChrome
{
protected:
    CGridViewItemChrome(_In_ CCoreServices *pCore):CListViewBaseItemChrome(pCore) { }
    ~CGridViewItemChrome() override { }

public:
    DECLARE_CREATE(CGridViewItemChrome);
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CGridViewItemChrome>::Index;
    }
};
