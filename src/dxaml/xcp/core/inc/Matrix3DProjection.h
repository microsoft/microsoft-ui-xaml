// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Projection.h"
#include "CMatrix.h"

class CMatrix3DProjection final : public CProjection
{
public:
    DECLARE_CREATE(CMatrix3DProjection);

    // CDependencyObject overrides
    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CMatrix3DProjection>::Index;
    }

    // CProjection overrides
    bool Is2DAligned() override
    {
        // m_pMatrix being NULL means either identity or exception, and exception is not in the specification, so
        // identity makes more senses.
        return (m_pMatrix ? m_pMatrix->m_matrix.IsIdentity() : TRUE);
    }

    void MakeWinRTExpression(
        _Inout_ WinRTExpressionConversionContext* pWinRTContext,
        float elementWidth,
        float elementHeight
        ) override;

    // No need to override GetProjectionMatrix.
    // Matrix3DProjection's projection matrix is always identity, as it does not declare m_crNearPlane, etc.

protected:
    ~CMatrix3DProjection() override;

    void Update3DTransform( _In_ XFLOAT rWidth, _In_ XFLOAT rHeight ) override;

private:
    CMatrix3DProjection(_In_ CCoreServices *pCore) : CProjection( pCore )
    {
        m_pMatrix = NULL;
    }

public:
    // Public API
    CMatrix4x4 *m_pMatrix;
};
