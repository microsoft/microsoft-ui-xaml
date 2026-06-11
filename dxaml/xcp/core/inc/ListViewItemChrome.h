// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CListViewItemChrome : public CListViewBaseItemChrome
{
protected:
    CListViewItemChrome(_In_ CCoreServices *pCore):CListViewBaseItemChrome(pCore) { }
    ~CListViewItemChrome() override { }

public:
    DECLARE_CREATE(CListViewItemChrome);
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CListViewItemChrome>::Index;
    }
};
