// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTextProvider final : public CDependencyObject
{
protected:
    CTextProvider(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CTextProvider);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTextProvider>::Index;
    }
};
