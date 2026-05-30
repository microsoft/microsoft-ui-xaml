// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <core.h>
#include <corep.h>
#include <UIElement.h>
#include <TransitionTarget.h>
#include <LayoutTransitionElement.h>
#include <Panel.h>
#include <Canvas.h>
#include <TransitionRoot.h>
#include <CompositeTransform.h>


_Check_return_ XFLOAT CLayoutTransitionElement::GetActualOffsetX()
{
    return 0;
}

_Check_return_ XFLOAT CLayoutTransitionElement::GetActualOffsetY()
{
    return 0;
}
void CLayoutTransitionElement::GetShouldFlipRTL(
    _Out_ bool *pShouldFlipRTL,
    _Out_ bool *pShouldFlipRTLInPlace
    )
{
}

void CLayoutTransitionElement::ClearPCRenderData()
{
}

void CLayoutTransitionElement::ClearSecondaryRenderData()
{
}

void CLayoutTransitionElement::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
}

_Check_return_ HRESULT CLayoutTransitionElement::GenerateChildOuterBounds(
    _In_opt_ HitTestParams *hitTestParams,
    _Out_ XRECTF_RB* pBounds
    )
{
    *pBounds = {0, 0, 0, 0};
    return S_OK;
}

_Check_return_ HRESULT CLayoutTransitionElement::BoundsTestChildren(
    _In_ const XPOINTF& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    if(pResult != nullptr)
    {
        *pResult = BoundsWalkHitResult::Continue;
    }
    return S_OK;
}

_Check_return_ HRESULT CLayoutTransitionElement::BoundsTestChildren(
    _In_ const HitTestPolygon& target,
    _In_ CBoundedHitTestVisitor* pCallback,
    _In_opt_ const HitTestParams *hitTestParams,
    _In_ bool canHitDisabledElements,
    _In_ bool canHitInvisibleElements,
    _Out_opt_ BoundsWalkHitResult* pResult
    )
{
    if(pResult != nullptr)
    {
        *pResult = BoundsWalkHitResult::Continue;
    }
    return S_OK;
}

void CLayoutTransitionElement::LeavePCSceneSubgraph()
{
}
