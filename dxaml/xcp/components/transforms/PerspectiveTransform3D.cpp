// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PerspectiveTransform3D.h"
#include "WinRTExpressionConversionContext.h"
#include <DependencyObjectDCompRegistry.h>
#include <stack_vector.h>
#include "UIElement.h"

void CPerspectiveTransform3D::UpdateTransformMatrix(float elementWidth, float elementHeight)
{
    float centerX = (elementWidth / 2) + m_rOffsetX;
    float centerY = (elementHeight / 2) + m_rOffsetY;

    m_matTransform3D.SetToPerspectiveTransform3D(centerX, centerY, m_rDepth);
}

bool CPerspectiveTransform3D::IsRenderedTransform2D() const
{
    return false;
}

bool CPerspectiveTransform3D::HasDepth()
{
    return true;
}

void CPerspectiveTransform3D::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWinRTExpression.Reset();
}

void CPerspectiveTransform3D::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    float elementWidth,
    float elementHeight
    )
{
    if (m_isWinRTExpressionDirty
        || elementWidth != m_elementWidth
        || elementHeight != m_elementHeight)
    {
        UpdateTransformMatrix(elementWidth, elementHeight);

        pWinRTContext->CreateMatrixTransform3D(
            &m_matTransform3D,
            m_spWinRTExpression.GetAddressOf()
            );

        if (GetDCompObjectRegistry() != nullptr)
        {
            GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
        }

        m_isWinRTExpressionDirty = false;
    }

    m_elementWidth = elementWidth;
    m_elementHeight = elementHeight;
}
