// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CManipulationVelocities final : public CDependencyObject
{
protected:
    CManipulationVelocities(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method
    DECLARE_CREATE(CManipulationVelocities);

// CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CManipulationVelocities>::Index;
    }

public:
// CPointerTouchPoint fields
    XPOINTF         m_ptLinear      = {};
    XFLOAT          m_fAngular      = 0.0f;
    XFLOAT          m_fExpansion    = 0.0f;
};
