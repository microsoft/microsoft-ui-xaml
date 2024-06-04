// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Matrix3DProjection.h"
#include <DependencyObjectDCompRegistry.h>
#include "WinRTExpressionConversionContext.h"
#include "UIElement.h"

void CMatrix3DProjection::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    float elementWidth,
    float elementHeight
    )
{
    UNREFERENCED_PARAMETER(elementWidth);
    UNREFERENCED_PARAMETER(elementHeight);

    if (m_isWinRTProjectionDirty)
    {
        // Since Matrix3DProjection can't be animated, there's no need to update the WinRT Expression that we already have,
        // so we can just make a new one every time.

        // Elements are supposed to be flattened at each element with a projection. Apply the flatten operation directly
        // into the 3D matrix supplied by the app.
        CMILMatrix4x4 projectionMatrix(m_pMatrix->m_matrix);
        pWinRTContext->CreateMatrixTransform3D(&projectionMatrix, &m_spWinRTProjection);

        if (GetDCompObjectRegistry() != nullptr)
        {
            GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
        }

        m_isWinRTProjectionDirty = false;
    }
}
