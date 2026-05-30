// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

class CRect final : public CDependencyObject
{
private:
    CRect(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    KnownTypeIndex GetTypeIndex() const override;

    XRECTF m_rc = {};
};
