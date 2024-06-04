// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"

namespace Diagnostics
{
    struct SetterUnsealer;
}

// Remark: This needs to be a MultiParentShareableDO as these Setters
// can become part of a based on collection in a different style.
//
// When added to secondary collections, we remember all parents
// to allow the reference tracking to work correctly. 
class CSetterBase : public CMultiParentShareableDependencyObject
{
    friend struct Diagnostics::SetterUnsealer;
protected:
    CSetterBase(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

public:
// Creation method

    DECLARE_CREATE(CSetterBase);
    void SetIsSealed() { m_bIsSealed = true; }

// CDependencyObject overrides
public:
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSetterBase>::Index;
    }

    bool m_bIsSealed = false;
};
