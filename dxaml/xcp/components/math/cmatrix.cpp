// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "CMatrix.h"
#include <StringConversions.h>

_Check_return_ HRESULT CMatrix::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    ASSERT(pCreate->m_value.GetType() == valueString);
    XUINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
    return MatrixFromString(cString, pString, &cString, &pString, &m_matrix);
}

_Check_return_ HRESULT CMatrix::FromFloatArray(_In_ CREATEPARAMETERS *pCreate)
{
    //we expect exactly 6 floats in the array
    ASSERT((pCreate->m_value.GetType() == valueFloatArray) && (pCreate->m_value.GetArrayElementCount() == 6));

    XFLOAT* createValueArray = pCreate->m_value.AsFloatArray();

    //set values from float array
    m_matrix.SetM11(createValueArray[0]);
    m_matrix.SetM12(createValueArray[1]);
    m_matrix.SetM21(createValueArray[2]);
    m_matrix.SetM22(createValueArray[3]);
    m_matrix.SetDx(createValueArray[4]);
    m_matrix.SetDy(createValueArray[5]);
    return S_OK;
}

_Check_return_ HRESULT CMatrix::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;

    CMatrix* _this = new CMatrix(pCreate->m_pCore);
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

_Check_return_ HRESULT CMatrix4x4::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    ASSERT(pCreate->m_value.GetType() == valueString);
    XUINT32 cString = 0;
    const WCHAR* pString = pCreate->m_value.AsEncodedString().GetBufferAndCount(&cString);
    return Matrix4x4FromString(cString, pString, &cString, &pString, &m_matrix);
}

_Check_return_ HRESULT CMatrix4x4::FromFloatArray(_In_ CREATEPARAMETERS *pCreate)
{
    // We expect exactly 16 floats in the array
    if ((pCreate->m_value.GetType() != valueFloatArray) ||
        (pCreate->m_value.GetArrayElementCount() != 16))
    {
        return E_FAIL;
    }

    m_matrix = CMILMatrix4x4::FromFloatArray(pCreate->m_value);

    return S_OK;
}
