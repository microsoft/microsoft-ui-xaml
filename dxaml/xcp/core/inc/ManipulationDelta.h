// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CManipulationDelta final : public CDependencyObject
{
protected:
    CManipulationDelta(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CManipulationDelta);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CManipulationDelta>::Index;
    }

public:
// CPointerTouchPoint fields
    XPOINTF         m_ptTranslation = {};
    XFLOAT          m_fScale        = 0.0f;
    XFLOAT          m_fRotation     = 0.0f;
    XFLOAT          m_fExpansion    = 0.0f;
};
