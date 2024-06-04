// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "CoreWindowRootScale.h"

#include "CDependencyObject.h"
#include "TransformGroup.h"
#include "TransformCollection.h"
#include "MatrixTransform.h"
#include "Matrix.h"
#include "CMatrix.h"

_Check_return_ HRESULT
RootScaleCreateTransform(CCoreServices* pCoreServices, const float scale, _Out_ CValue* result)
{
    CREATEPARAMETERS cp(pCoreServices);
    CValue value;
    {
        xref_ptr<CDependencyObject> matrix;
        IFC_RETURN(CMatrix::Create(matrix.ReleaseAndGetAddressOf(), &cp));
        value.SetFloat(scale);
        IFC_RETURN(matrix.get()->SetValueByKnownIndex(KnownPropertyIndex::Matrix_M11, value));
        IFC_RETURN(matrix.get()->SetValueByKnownIndex(KnownPropertyIndex::Matrix_M22, value));
        {
            xref_ptr<CDependencyObject> matrixTransform;
            IFC_RETURN(CMatrixTransform::Create(matrixTransform.ReleaseAndGetAddressOf(), &cp));
            value.WrapObjectNoRef(matrix.get());
            IFC_RETURN(matrixTransform.get()->SetValueByKnownIndex(KnownPropertyIndex::MatrixTransform_Matrix, value));
            {
                xref_ptr<CDependencyObject> transformCollection;
                IFC_RETURN(CTransformCollection::Create(transformCollection.ReleaseAndGetAddressOf(), &cp));
                value.WrapObjectNoRef(matrixTransform.get());
                IFC_RETURN(transformCollection.get()->SetValue(transformCollection.get()->GetContentProperty(), value));
                {
                    xref_ptr<CDependencyObject> transformGroup;
                    IFC_RETURN(CTransformGroup::Create(transformGroup.ReleaseAndGetAddressOf(), &cp));
                    value.WrapObjectNoRef(transformCollection.get());
                    IFC_RETURN(transformGroup.get()->SetValue(transformGroup.get()->GetContentProperty(), value));
                    result->SetObjectAddRef(transformGroup.get());
                }
            }
        }
    }

    return S_OK;
}

_Check_return_ HRESULT
CoreWindowRootScale::CreateTransform(_Out_ CValue* result)
{
    const float scale = GetRootVisualScale();
    IFC_RETURN(RootScaleCreateTransform(m_pCoreServices, scale, result));
    return S_OK;
}

_Check_return_ HRESULT
CoreWindowRootScale::CreateReverseTransform(_Out_ CValue* result)
{
    const float scale = 1.0f / GetRootVisualScale();
    IFC_RETURN(RootScaleCreateTransform(m_pCoreServices, scale, result));
    return S_OK;
}
