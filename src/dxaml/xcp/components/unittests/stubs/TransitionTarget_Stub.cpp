// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

// Windows headers define "GetClassName" to "GetClassNameW", so CDependencyObject::GetClassName in the stub
// becomes CDependencyObject::GetClassNameW, which then fails to link against the real CDO::GetClassName
#ifdef GetClassName
#undef GetClassName
#endif

#include "TransitionTarget.h"


CTransitionTarget::~CTransitionTarget()
{
}

_Check_return_ HRESULT CTransitionTarget::InitInstance()
{
    return E_NOTIMPL;
}

void CTransitionTarget::NWSetTransformDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
}

void CTransitionTarget::NWSetClipDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
}

void CTransitionTarget::NWSetOpacityDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
}

void CTransitionTarget::NWSetPropertyDirtyOnTarget(_In_ CUIElement* pTarget, DirtyFlags flags)
{
}

void CTransitionTarget::GetClipTransform(
    _In_ const XRECTF& bounds,
    _Out_ CMILMatrix *pMatrix
    )
{
    pMatrix->SetToIdentity();
}

void CTransitionTarget::ApplyClipTransformOrigin(
    _In_ const XRECTF& bounds,
    _Inout_ CMILMatrix *pMatrix
    )
{
}

void CTransitionTarget::TransformBounds(
    _Inout_ XRECTF& bounds
    )
{
}

void CTransitionTarget::ApplyClip(
    _In_ XRECTF bounds,
    _Inout_ XRECTF *pClipRect
    )
{
}

_Check_return_ HRESULT CTransitionTarget::ApplyClip(
    _In_ XRECTF bounds,
    _Inout_ TransformAndClipStack *pTransformsAndClips
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTransitionTarget::ClipToTransitionTarget(
    _Inout_ XRECTF& bounds,
    _Inout_ XPOINTF& target,
    _Out_ bool *pHit
    )
{
    return E_NOTIMPL;
}

_Check_return_ HRESULT CTransitionTarget::ClipToTransitionTarget(
    _Inout_ XRECTF& bounds,
    _Inout_ HitTestPolygon& target,
    _Out_ bool *pHit
    )
{
    return E_NOTIMPL;
}
