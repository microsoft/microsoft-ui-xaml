// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

CTransitionTarget::~CTransitionTarget()
{
    ReleaseInterface(m_pxf);
    ReleaseInterface(m_pClipTransform);

    auto dcompRegistry = GetDCompObjectRegistry();
    if (dcompRegistry)
    {
        dcompRegistry->UnregisterObject(this);
    }
}

//------------------------------------------------------------------------
//  Synopsis:
//      Initializes the instance with instances of the leaves we might animate.
//      The value of this class is in these being non-null and not settable,
//      this enables consumers to rely on them being animateable.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTransitionTarget::InitInstance()
{
    HRESULT hr = S_OK;

    CREATEPARAMETERS cp(GetContext());
    CCompositeTransform* pTransform = NULL;
    CCompositeTransform* pClipGeometryTransform = NULL;

    // transform
    IFC(CCompositeTransform::Create((CDependencyObject**) &pTransform, &cp));
    IFC(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_CompositeTransform, pTransform));

    // clip transform
    IFC(CCompositeTransform::Create((CDependencyObject**) &pClipGeometryTransform, &cp));
    pClipGeometryTransform->SetIsTransitionClipTransform();
    IFC(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_ClipTransform, pClipGeometryTransform));

Cleanup:
    ReleaseInterface(pTransform);
    ReleaseInterface(pClipGeometryTransform);
    RRETURN(hr);
}

bool CTransitionTarget::IsDirty() const
{
    return m_currentDirtyMode != TransitionTargetDirtyMode::Dirty_None;
}

void CTransitionTarget::Clean()
{
    m_currentDirtyMode = TransitionTargetDirtyMode::Dirty_None;
}

//------------------------------------------------------------------------
//  Synopsis: This RENDERCHANGEDPFN marks this TransitionTargets transform as
//            dirty for rendering.
//------------------------------------------------------------------------
void CTransitionTarget::NWSetTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::TransitionTarget>());

    CTransitionTarget *pThis = static_cast<CTransitionTarget*>(pTarget);
    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pThis->m_currentDirtyMode |= TransitionTargetDirtyMode::Dirty_Transform;
    }
    pThis->NWPropagateDirtyFlag(flags | DirtyFlags::Render | DirtyFlags::Bounds);
}

//------------------------------------------------------------------------
//  Synopsis: This RENDERCHANGEDPFN marks this TransitionTargets clip as
//            dirty for rendering.
//------------------------------------------------------------------------
void CTransitionTarget::NWSetClipDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::TransitionTarget>());

    CTransitionTarget *pThis = static_cast<CTransitionTarget*>(pTarget);
    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pThis->m_currentDirtyMode |= TransitionTargetDirtyMode::Dirty_Clip;
    }
    pThis->NWPropagateDirtyFlag(flags | DirtyFlags::Render | DirtyFlags::Bounds);
}

//------------------------------------------------------------------------
//  Synopsis: This RENDERCHANGEDPFN marks this TransitionTargets opacity as
//            dirty for rendering.
//------------------------------------------------------------------------
void CTransitionTarget::NWSetOpacityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::TransitionTarget>());

    CTransitionTarget *pThis = static_cast<CTransitionTarget*>(pTarget);
    if (!flags_enum::is_set(flags, DirtyFlags::Independent))
    {
        pThis->m_currentDirtyMode |= TransitionTargetDirtyMode::Dirty_Opacity;
    }
    pThis->NWPropagateDirtyFlag(flags | DirtyFlags::Render);
}

void CTransitionTarget::NWSetPropertyDirtyOnTarget(_In_ CUIElement* pTarget, DirtyFlags flags)
{
    if ((m_currentDirtyMode & TransitionTargetDirtyMode::Dirty_Transform) == TransitionTargetDirtyMode::Dirty_Transform)
    {
        CUIElement::NWSetTransformDirty(pTarget, flags);
    }

    if ((m_currentDirtyMode & TransitionTargetDirtyMode::Dirty_Clip) == TransitionTargetDirtyMode::Dirty_Clip)
    {
        CUIElement::NWSetClipDirty(pTarget, flags);
    }

    if ((m_currentDirtyMode & TransitionTargetDirtyMode::Dirty_Opacity) == TransitionTargetDirtyMode::Dirty_Opacity)
    {
        CUIElement::NWSetOpacityDirty(pTarget, flags);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the transform determined by m_pClipTransform, m_ptClipTransformOrigin,
//      and the bounding box representing a clip.
//
//------------------------------------------------------------------------
void
CTransitionTarget::GetClipTransform(
    _In_ const XRECTF& bounds,
    _Out_ CMILMatrix *pMatrix
    )
{
    m_pClipTransform->GetTransform(pMatrix);
    if (!pMatrix->IsIdentity())
    {
        ApplyClipTransformOrigin(bounds, pMatrix);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies m_ptClipTransformOrigin to the bounds param.
//
//------------------------------------------------------------------------
void
CTransitionTarget::ApplyClipTransformOrigin(
    _In_ const XRECTF& bounds,
    _Inout_ CMILMatrix *pMatrix
    )
{
    // TODO: JCOMP: This math is duplicated in CUIElement::ApplyTransform.
    // TODO: JCOMP: The origin adjustment can be skipped if pMatrix is purely a translation.
    // Adjust for origin
    if (m_ptClipTransformOrigin.x != 0.0f || m_ptClipTransformOrigin.y != 0.0f)
    {
        CMILMatrix matAdjust(TRUE);
        XFLOAT rScaledX = bounds.Width * m_ptClipTransformOrigin.x;
        XFLOAT rScaledY = bounds.Height * m_ptClipTransformOrigin.y;

        // Move to the origin before our transform
        matAdjust.SetDx(-1.0f * rScaledX);
        matAdjust.SetDy(-1.0f * rScaledY);

        pMatrix->Prepend(matAdjust);

        // Move back after the transform has been applied
        matAdjust.SetDx(rScaledX);
        matAdjust.SetDy(rScaledY);

        pMatrix->Append(matAdjust);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies m_pClipTransform and m_ptClipTransformOrigin to the bounds param.
//
//------------------------------------------------------------------------
void
CTransitionTarget::TransformBounds(
    _Inout_ XRECTF& bounds
    )
{
    CMILMatrix clipMatrix;
    m_pClipTransform->GetTransform(&clipMatrix);

    if (!clipMatrix.IsIdentity())
    {
        ApplyClipTransformOrigin(bounds, &clipMatrix);

        clipMatrix.TransformBounds(&bounds, &bounds);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies some specified bounds as a clip rect, if this transition
//      target has a clip animation.
//
//------------------------------------------------------------------------
void
CTransitionTarget::ApplyClip(
    _In_ XRECTF bounds,
    _Inout_ XRECTF *pClipRect
    )
{
    if (m_hasClipAnimation)
    {
        TransformBounds(bounds);
        IntersectRect(pClipRect, &bounds);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Applies some specified bounds as a clip rect, if this transition
//      target has a clip animation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionTarget::ApplyClip(
    _In_ XRECTF bounds,
    _Inout_ TransformAndClipStack *pTransformsAndClips
    )
{
    if (m_hasClipAnimation)
    {
        TransformBounds(bounds);

        HWClip transitionClipHW;
        transitionClipHW.Set(&bounds);
        IFC_RETURN(pTransformsAndClips->IntersectLocalSpaceClip(&transitionClipHW));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether the point is inside the implicit TransitionTarget clip.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionTarget::ClipToTransitionTarget(
    _Inout_ XRECTF& bounds,
    _Inout_ XPOINTF& target,
    _Out_ bool *pHit
    )
{
    if (m_hasClipAnimation)
    {
        TransformBounds(bounds);
        *pHit = DoesRectContainPoint(bounds, target);
    }
    else
    {
        *pHit = TRUE;
    }

    RRETURN(S_OK);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Determines whether the polygon intersects the implicit TransitionTarget clip.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTransitionTarget::ClipToTransitionTarget(
    _Inout_ XRECTF& bounds,
    _Inout_ HitTestPolygon& target,
    _Out_ bool *pHit
    )
{
    if (m_hasClipAnimation)
    {
        TransformBounds(bounds);

        if (target.IntersectsRect(bounds))
        {
            IFC_RETURN(target.ClipToRect(bounds));
            *pHit = !target.IsEmpty();
        }
        else
        {
            *pHit = FALSE;
        }
    }
    else
    {
        *pHit = TRUE;
    }

    return S_OK;
}

void CTransitionTarget::ReplaceTransform(_In_ CCompositeTransform* newTransform)
{
    if (m_pxf != newTransform)
    {
        IFCFAILFAST(ClearValueByIndex(KnownPropertyIndex::TransitionTarget_CompositeTransform));
        ASSERT(!m_pxf);
        IFCFAILFAST(SetValueByKnownIndex(KnownPropertyIndex::TransitionTarget_CompositeTransform, newTransform));
        ASSERT(m_pxf == newTransform);  // Field was set by property system
    }
}