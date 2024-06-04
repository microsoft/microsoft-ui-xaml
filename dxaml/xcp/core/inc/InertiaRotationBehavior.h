// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CInertiaRotationBehavior final : public CDependencyObject
{
protected:
    CInertiaRotationBehavior(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CInertiaRotationBehavior);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CInertiaRotationBehavior>::Index;
    }

public:
// CInertiaRotationBehavior fields
    XFLOAT          m_fDeceleration = 0.0f;
    XFLOAT          m_fRotation     = 0.0f;
};
