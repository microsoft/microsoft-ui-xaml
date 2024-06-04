// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <DependencyObjectTraits.h>
#include <DependencyObjectTraits.g.h>

class CPropertyPath : public CDependencyObject
{
public:
    CPropertyPath(_In_ CCoreServices *pCore) : CDependencyObject(pCore)
    {
    }

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CPropertyPath>::Index;
    }

    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate);

    xstring_ptr m_strPath;
};