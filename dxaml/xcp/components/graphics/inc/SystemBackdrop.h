// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MultiParentShareableDependencyObject.h"
#include "DependencyObjectTraits.h"
#include "DependencyObjectTraits.g.h"

class CSystemBackdrop : public CMultiParentShareableDependencyObject
{
protected:
    CSystemBackdrop(_In_ CCoreServices *pCore);
    ~CSystemBackdrop() override = default;

public:
// Creation method

    DECLARE_CREATE(CSystemBackdrop);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CSystemBackdrop>::Index;
    }
};
