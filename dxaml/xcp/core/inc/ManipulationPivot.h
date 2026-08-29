// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CManipulationPivot final : public CDependencyObject
{
protected:
    CManipulationPivot(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CManipulationPivot);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CManipulationPivot>::Index;
    }

public:
// CPointerTouchPoint fields
    XPOINTF         m_ptCenter  = {};
    XFLOAT          m_fRadius   = 0.0f;
};