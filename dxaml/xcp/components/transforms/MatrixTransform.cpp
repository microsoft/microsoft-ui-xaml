// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "MatrixTransform.h"
#include "WinRTExpressionConversionContext.h"
#include "ExpressionHelper.h"
#include "CMatrix.h"
#include <DependencyObjectDCompRegistry.h>

void CMatrixTransform::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    _Inout_opt_ WUComp::ICompositionPropertySet* pTransformGroupPS
    )
{
    // Even a transform that is clean might need to create an expression, if it's part of a transform group where some other
    // transform started animating, so check both the dirty flag and that we already have an expression. The corollary is that
    // calling this method on a clean transform will still create an expression for it.
    if (m_isWinRTExpressionDirty || m_spWinRTExpression == nullptr)
    {
        pWinRTContext->CreateMatrixTransform(
            false /* useMatrix4x4 */,
            m_pMatrix != nullptr ? &m_pMatrix->m_matrix : nullptr,
            m_spWinRTExpression.GetAddressOf()
            );

        if (GetDCompObjectRegistry() != nullptr)
        {
            GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
        }

        m_isWinRTExpressionDirty = false;
    }

    if (pTransformGroupPS)
    {
        pWinRTContext->AddExpressionToTransformGroupPropertySet(m_spWinRTExpression.Get(), pTransformGroupPS);
    }
}

void CMatrixTransform::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWinRTExpression.Reset();
}

void CMatrixTransform::SetMatrix(_In_ const CMILMatrix& matrix)
{
    if (m_pMatrix == nullptr)
    {
        CREATEPARAMETERS cp(GetContext());
        IFCFAILFAST(CMatrix::Create(reinterpret_cast<CDependencyObject**>(&m_pMatrix), &cp));
    }
    m_pMatrix->m_matrix = matrix;
}