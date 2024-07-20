// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CDependencyObject.h"

class CMatrix final : public CDependencyObject
{
private:
    CMatrix(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    _Check_return_ HRESULT FromString(
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ HRESULT FromFloatArray(
        _In_ CREATEPARAMETERS *pCreate
        );

public:
#if defined(__XAML_UNITTESTS__)
    CMatrix()   // !!! FOR UNIT TESTING ONLY !!!
        : CMatrix(nullptr)
    {}
#endif

    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        );

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Matrix;
    }

    // CMatrix fields

    CMILMatrix m_matrix = CMILMatrix(TRUE);
};

class CMatrix4x4 final : public CDependencyObject
{
private:
    CMatrix4x4(_In_ CCoreServices *pCore)
        : CDependencyObject(pCore)
    {}

    _Check_return_ HRESULT FromString(
        _In_ CREATEPARAMETERS *pCreate
        );

    _Check_return_ HRESULT FromFloatArray(
        _In_ CREATEPARAMETERS *pCreate
        );

public:
    // Creation method
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate
        )
    {
        HRESULT hr = S_OK;

        CMatrix4x4* _this = new CMatrix4x4(pCreate->m_pCore);
        CDependencyObject *pTemp = NULL;
        IFC(ValidateAndInit(_this, &pTemp));

        if (pCreate->m_value.GetType() == valueString)
        {
            IFC(_this->FromString(pCreate));
        }
        else if (pCreate->m_value.GetType() == valueFloatArray)
        {
            IFC(_this->FromFloatArray(pCreate));
        }

        *ppObject = pTemp;
        _this = NULL;

    Cleanup:
        if (pTemp) delete _this;
        RRETURN(hr);
    }

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return KnownTypeIndex::Matrix3D;
    }

    // CMatrix4x4 fields
public:
    CMILMatrix4x4 m_matrix = CMILMatrix4x4(TRUE);
};
