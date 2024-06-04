// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Transform3D.h"
#include <DependencyObjectDCompRegistry.h>

CTransform3D::~CTransform3D()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CTransform3D::SetDCompResourceDirty()
{
    if (!m_isWinRTExpressionDirty)
    {
        m_isWinRTExpressionDirty = true;
    }
}

void CTransform3D::NWSetRenderDirty(
    _In_ CDependencyObject *pTarget,
    DirtyFlags flags
    )
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Transform3D>());

    CTransform3D *pTransform3D = static_cast<CTransform3D*>(pTarget);

    if (!pTransform3D->m_isWinRTExpressionDirty
        && !flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pTransform3D->SetDCompResourceDirty();
    }

    __super::NWSetRenderDirty(pTarget, flags);
}

void CTransform3D::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWinRTExpression.Reset();
    m_isWinRTExpressionDirty = true;
}

void CTransform3D::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    // We're assuming that an expression is needed. If nothing is animated then no expression is needed. Check the dirty flag
    // first - if no expression is needed, then the comp tree node walk should have come through, released the expression, and
    // cleared the dirty flag.
    if (m_isWinRTExpressionDirty)
    {
        // We're attaching the animation so we get the completed event. Use the cached element sizes from before, so that only
        // the dirty flag is checked. Passing in a different element size will result in us updating the expression again, which
        // we don't want - we only want to update it if it was missed by the render walk entirely.
        MakeWinRTExpression(context, m_elementWidth, m_elementHeight);
    }
}

const CMILMatrix4x4& CTransform3D::GetTransformMatrix() const
{
    // Note that here we simply return the matrix, and m_matTransform3D might not be up to date.
    // This will occur if we attempt to access m_matTransform3D after a local property has been modified,
    // but before it is rendered. This is in line with CPlaneProjection::GetProjectionMatrix().
    return m_matTransform3D;
}

void CTransform3D::ClearWUCExpression()
{
    m_spWinRTExpression.Reset();

    m_isWinRTExpressionDirty = false;
}
