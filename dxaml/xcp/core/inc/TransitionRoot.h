// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CTransitionRoot final : public CCanvas
{
private:
    CTransitionRoot(_In_ CCoreServices *pCore): CCanvas(pCore) { };

public:
    DECLARE_CREATE(CTransitionRoot);

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CTransitionRoot>::Index;
    }

    void GetChildrenInRenderOrder(
        _Outptr_opt_result_buffer_(*puiChildCount) CUIElement ***pppUIElements,
        _Out_ XUINT32 *puiChildCount
        ) override;

    void ReEvaluateRequiresCompNode();

protected:
    _Check_return_ HRESULT GenerateChildOuterBounds(
        _In_opt_ HitTestParams *hitTestParams,
        _Out_ XRECTF_RB* pBounds
        ) override;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const XPOINTF& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

    _Check_return_ HRESULT BoundsTestChildren(
        _In_ const HitTestPolygon& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        ) final;

private:
    template <typename HitType>
    _Check_return_  HRESULT BoundsTestChildrenImpl(
        _In_ const HitType& target,
        _In_ CBoundedHitTestVisitor* pCallback,
        _In_opt_ const HitTestParams *hitTestParams,
        _In_ bool canHitDisabledElements,
        _In_ bool canHitInvisibleElements,
        _Out_opt_ BoundsWalkHitResult* pResult
        );
};
