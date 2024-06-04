// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT CCompositeTransform::SetValue(_In_ const SetValueParams& args)
{
    bool fIsOurProperty;

    // Need to catch this case because the change to Auto for Width/Height
    // on FrameworkElements could potentially trickle down to these transforms
    // and making them invalid.
    fIsOurProperty = args.m_pDP && (
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_CenterX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_CenterY ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_ScaleX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_ScaleY ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_SkewX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_SkewY ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_Rotation ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_TranslateX ||
        args.m_pDP->GetIndex() == KnownPropertyIndex::CompositeTransform_TranslateY);

    if (fIsOurProperty &&
        args.m_value.IsFloatingPoint() &&
        _isnan(args.m_value.AsDouble()))
    {
        CValue zeroValue;
        zeroValue.SetFloat(0.0f);
        IFC_RETURN(CTransform::SetValue(SetValueParams(args.m_pDP, zeroValue)));
    }
    else
    {
        IFC_RETURN(CTransform::SetValue(args));
    }

    //
    // If this transform is the clip transform of a transition target, mark the target as animated. Animated targets
    // will apply a clip, and unanimated targets will not. The animation will come from either a theme animation or
    // a theme transition.
    //
    // One place to mark animated transition targets is from CUIElement::SetRequiresComposition, but that method only
    // gets called for independent ticks. The app is free to pause and seek a theme animation, which should still apply
    // a clip, but SetRequiresComposition will not be called.
    //
    // Another place to mark animated transition targets is from the theme animations and theme transitions themselves.
    // Theme animations are resolved in CDynamicTimeline::OnBegin, where they call out to GetDynamicTimelines in the
    // DXAML layer and the various DirectUI::Timeline::CreateTimelines overrides. This is close to where animation targets
    // are resolved and transition targets are created. Theme transitions are resolved in CTransition::SetupTransition,
    // where they call out to DirectUI::Transition::CreateStoryboardsForTransition in the DXAML layer and the various
    // DirectUI::Transition::CreateStoryboardImpl overrides. This happens during layout, and it's harder to mark the
    // animation targets here.
    //
    // m_hasMarkedParentAsAnimated is used as an optimization to not repeatedly look up the parent of this transform.
    // Transition clip transforms are not shared between transition targets and are never reassigned.
    //
    if (m_isTransitionClipTransform && !m_hasMarkedParentAsAnimated)
    {
        CDependencyObject *pParentNoRef = GetParentItem(0); // Doesn't take a ref count

        ASSERT(GetParentCount() == 1 && pParentNoRef != nullptr && pParentNoRef->OfTypeByIndex<KnownTypeIndex::TransitionTarget>());

        CTransitionTarget *pTransitionTargetNoRef = reinterpret_cast<CTransitionTarget *>(pParentNoRef);
        pTransitionTargetNoRef->SetHasClipAnimation();

        m_hasMarkedParentAsAnimated = TRUE;
    }

    return S_OK;
}

void CCompositeTransform::NWSetDirty(_In_ CDependencyObject *pTarget, DirtyFlags flags)
{
    ASSERT(pTarget->OfTypeByIndex<KnownTypeIndex::CompositeTransform>());

    CCompositeTransform *pTransform = static_cast<CCompositeTransform*>(pTarget);

    // It's important to set the dirty bit first, in case anything queries the transform during dirty flag propagation.
    // CUIElement::NWSetTransformDirty will do so, for example, to determine if the transform is
    // axis-alignment-preserving or not.
    pTransform->m_fDirty = TRUE;
    pTransform->SetDCompResourceDirty();

    // Always propagate flags and let parent decide if to propagate further.
    //
    // Transforms may change bounds but the transform group is not bounds
    // aware. Terminating the dirty path here based on the render flag
    // may prevent the parent from updating bounds when required to due
    // to a transform change.
    pTransform->NWPropagateDirtyFlag(flags);
}
