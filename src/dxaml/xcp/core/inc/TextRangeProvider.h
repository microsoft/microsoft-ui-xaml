// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextRangeProvider final : public CDependencyObject
{
protected:
    CTextRangeProvider(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CTextRangeProvider);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextRangeProvider>::Index;
    }

    bool ControlsManagedPeerLifetime() override
    {
        // Don't let GC happen with strengthen the reference on the managed peer while
        // a native object is holding this object.
        return true;
    }
};
