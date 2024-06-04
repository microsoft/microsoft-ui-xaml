// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

class CSize final : public CDependencyObject
{
private:
    CSize(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    _Check_return_ HRESULT static Create(_Outptr_ CDependencyObject **ppObject, _In_ CREATEPARAMETERS *pCreate);

    static _Check_return_ HRESULT Validate(
        _In_ XSIZEF * pSize
        );

    KnownTypeIndex GetTypeIndex() const override;

    XSIZEF m_size = {};
};
