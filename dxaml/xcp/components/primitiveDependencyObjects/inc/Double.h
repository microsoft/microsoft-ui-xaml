// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>

class CDouble 
    : public CDependencyObject
{
private:
    CDouble(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
    static _Check_return_ HRESULT CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value);

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);

    KnownTypeIndex GetTypeIndex() const override;

    bool DoesAllowMultipleAssociation() const override { return true; }

    XFLOAT m_eValue = 0.0f;
};

// Converter to convert strings to doubles when they are "auto"
class CLengthConverter : public CDependencyObject
{
private:
    CLengthConverter(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

public:
// Creation method

    static _Check_return_ HRESULT Create(_Outptr_ CDependencyObject **ppObject,
                          _In_ CREATEPARAMETERS *pCreate);
    static _Check_return_ HRESULT CreateCValue(_In_ CREATEPARAMETERS* pCreate, _Inout_ CValue& value);
};
