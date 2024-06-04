// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CInertiaExpansionBehavior final : public CDependencyObject
{
protected:
    CInertiaExpansionBehavior(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CInertiaExpansionBehavior);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInertiaExpansionBehavior>::Index;
    }

public:
// CInertiaExpansionBehavior  fields
    XFLOAT          m_fDeceleration = 0.0f;
    XFLOAT          m_fExpansion    = 0.0f;
};
