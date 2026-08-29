// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Projection.h"
#include <DependencyObjectDCompRegistry.h>
#include "UIElement.h"

CProjection::~CProjection()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CProjection::SetDCompResourceDirty()
{
    if (!m_isWinRTProjectionDirty)
    {
        m_isWinRTProjectionDirty = true;
    }
}

void CProjection::NWSetRenderDirty(
    _In_ CDependencyObject *pTarget,
    DirtyFlags flags
    )
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::Projection>());

    CProjection *pProjection = static_cast<CProjection*>(pTarget);

    // When an independent animation goes into the Filling state, its last tick is still considered "independent"
    // (see CAnimation::UpdateAnimation). It still needs to invalidate the cached 4x4 matrix so we can update it
    // with the value from final frame of the animation.
    pProjection->SetCached3DTransformDirty();

    if (!pProjection->m_isWinRTProjectionDirty
        && !flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pProjection->SetDCompResourceDirty();
    }

    __super::NWSetRenderDirty(pTarget, flags);
}

void CProjection::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();

    m_spWinRTProjection.Reset();
    m_isWinRTProjectionDirty = true;
}

void CProjection::MakeWinRTExpression(
    _Inout_ WinRTExpressionConversionContext* pWinRTContext,
    float elementWidth,
    float elementHeight
    )
{
    ASSERT(FALSE);  // Should be implemented by derived projection types
}

void CProjection::ClearWUCExpression()
{
    m_spWinRTProjection.Reset();

    m_isWinRTProjectionDirty = false;
}

void CProjection::EnsureWUCAnimationStarted(_Inout_ WinRTExpressionConversionContext* context)
{
    // We're assuming that an expression is needed. If nothing is animated then no expression is needed. Check the dirty flag
    // first - if no expression is needed, then the comp tree node walk should have come through, released the expression, and
    // cleared the dirty flag.
    if (m_isWinRTProjectionDirty)
    {
        // We're attaching the animation so we get the completed event. The element size doesn't matter to generic projections, so
        // use 1x1. Plane projections will override this and provide their cached size.
        MakeWinRTExpression(context, 1, 1);
    }
}

CMILMatrix4x4 CProjection::GetProjectionMatrix() const
{
    return CMILMatrix4x4(true);
}

// Creates the 3D transform that represents the model transform and perspective
CMILMatrix4x4 CProjection::GetOverallProjectionMatrix(_In_ const XSIZEF &elementSize)
{
    if (m_is3DTransformDirty
        || elementSize.width != m_previousElementSize.width
        || elementSize.height != m_previousElementSize.height)
    {
        // Build 3D Pipeline: [Centering] x [Model Transform] x [Projection] x [Undo centering] x [Flatten]

        // The centering matrix translates the element from having its top left corner at (0,0)
        // to having its center at (0,0).
        m_overallMatrix.SetToTranslation(-elementSize.width / 2.0f, -elementSize.height / 2.0f, 0.0f);

        // The model transform comes from the various properties set on the <PlaneProjection>.
        // It applies transformations to the surface to be projected.
        Update3DTransform(elementSize.width, elementSize.height);
        m_overallMatrix.Append(m_mat3DTransform);

        // The projection transform comes from preset values. It does the perspective projection.
        // See CPlaneProjection::Update3DProjection and CPlaneProjection::m_crNearPlane/
        // m_crFarPlane/m_crFieldOfView/m_crZOffset.
        const CMILMatrix4x4 projection = GetProjectionMatrix();
        if (!projection.IsIdentity())
        {
            m_overallMatrix.Append(projection);
        }

        // The element is then moved back from from being centered at (0,0) to having its top left
        // corner at (0,0).
        CMILMatrix4x4 matScreenMapping;
        matScreenMapping.SetToTranslation(elementSize.width / 2.0f, elementSize.height / 2.0f, 0.0f);
        m_overallMatrix.Append(matScreenMapping);

        m_previousElementSize = elementSize;
    }

    return m_overallMatrix;
}
