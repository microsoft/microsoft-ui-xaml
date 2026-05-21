// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "MatrixTransform.h"


_Check_return_ HRESULT CMatrixTransform::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    return E_NOTIMPL;
}

CMatrixTransform::~CMatrixTransform()
{
}

CMatrixTransform::CMatrixTransform(_In_ const CMatrixTransform& original, _Out_ HRESULT& hr)
    : CTransform(original, hr)
{
}

void CMatrixTransform::GetTransform(_Out_ CMILMatrix *pMatrix)
{
    pMatrix->SetToIdentity();
};
