// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CMatrixTransform::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    return CMatrix::Create((CDependencyObject **)&m_pMatrix, pCreate);
}

CMatrixTransform::~CMatrixTransform()
{
    ReleaseInterface(m_pMatrix);
}

CMatrixTransform::CMatrixTransform(_In_ const CMatrixTransform& original, _Out_ HRESULT& hr)
    : CTransform(original,hr)
{
    CREATEPARAMETERS cp(GetContext());
    CMatrix *pLocalMatrix = NULL;

    m_pMatrix = NULL;

    IFC(hr);

    if(original.m_pMatrix)
    {
        IFC(CreateDO(&pLocalMatrix, &cp));

        memcpy(&(pLocalMatrix->m_matrix), &(original.m_pMatrix->m_matrix), sizeof(CMILMatrix));

        IFC(SetValueByKnownIndex(KnownPropertyIndex::MatrixTransform_Matrix, pLocalMatrix)); // Adds ref to matrix
    }

Cleanup:
    ReleaseInterface(pLocalMatrix);
}

void CMatrixTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    ASSERT(pMatrix != nullptr);

    if (m_pMatrix)
    {
        *pMatrix = m_pMatrix->m_matrix;
    }
    else
    {
        pMatrix->SetToIdentity();
    }
};
