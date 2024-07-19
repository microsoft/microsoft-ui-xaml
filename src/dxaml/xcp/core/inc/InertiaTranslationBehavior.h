// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CInertiaTranslationBehavior final : public CDependencyObject
{
protected:
    CInertiaTranslationBehavior(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CInertiaTranslationBehavior);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInertiaTranslationBehavior>::Index;
    }

public:
// CInertiaTranslationBehavior fields
    XFLOAT          m_fDeceleration = 0.0f;
    XFLOAT          m_fDisplacement = 0.0f;
};
