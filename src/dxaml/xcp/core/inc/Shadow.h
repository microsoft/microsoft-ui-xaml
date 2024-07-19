// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "MultiParentShareableDependencyObject.h"

#pragma once

// Base Shadow class.  This class can not be directly created.
class CShadow : public CMultiParentShareableDependencyObject
{
protected:
    CShadow(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    CShadow(_In_ const CShadow& original, _Out_ HRESULT& hr)
        : CMultiParentShareableDependencyObject(original, hr)
    {}

public:
// Creation method

    DECLARE_CREATE(CShadow);

// CDependencyObject overrides

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CShadow>::Index;
    }
};
