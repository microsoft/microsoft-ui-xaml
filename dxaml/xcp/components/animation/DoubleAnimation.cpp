// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleAnimation.h"
#include "DCompAnimationConversionContext.h"
#include "Duration.h"
#include "CValue.h"
#include "TypeTableStructs.h"
#include "TranslateTransform.h"
#include "ScaleTransform.h"
#include "RotateTransform.h"
#include "SkewTransform.h"
#include "CompositeTransform.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include "EasingFunctions.h"
#include <TimeMgr.h>
#include "UIElement.h"
#include "TransitionTarget.h"
#include "PlaneProjection.h"
#include "CompositeTransform3D.h"
#include <DependencyObjectDCompRegistry.h>
#include <DCompTreeHost.h>
#include <FloatUtil.h>
#include <DoubleUtil.h>

CDoubleAnimation::CDoubleAnimation()
    : CAnimation(nullptr)
{
}

CDoubleAnimation::~CDoubleAnimation()
{
    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->UnregisterObject(this);
    }
}

void CDoubleAnimation::AttachDCompAnimations()
{
    // m_hasIndependentAnimation implies m_hasControlOfTarget, due to how it's calculated in CAnimation::UpdateAnimation. We
    // check both as a formality. m_hasIndependentAnimation needs to be checked to ensure that the animation is actually
    // active. Otherwise, an animation can reattach itself immediately after detaching itself earlier in the tick.
    if (m_hasControlOfTarget && HasIndependentAnimation() && m_shouldAttachDCompAnimationInstance)
    {
        AttachDCompAnimationInstancesToTarget();
        m_shouldAttachDCompAnimationInstance = false;
    }
}

void static ForceUnsetDCompAnimation(KnownPropertyIndex propertyIndex, CDependencyObject *pDO) noexcept
{
    switch (propertyIndex)
    {
        case KnownPropertyIndex::Canvas_Left:
        {
            // TODO_WinRT: Handle this case for WUC.
            // Note: this mechanism detaches animations too early and breaks handoff scenarios. Instead, we should be using a
            // mechanism like CTimeManager::m_translatesWithEndingAnimations to handle objects that have stopped animating but
            // are not part of the live tree.
            break;
        }

        case KnownPropertyIndex::Canvas_Top:
        {
            break;
        }

        case KnownPropertyIndex::TranslateTransform_X:
        case KnownPropertyIndex::TranslateTransform_Y:
        case KnownPropertyIndex::ScaleTransform_CenterX:
        case KnownPropertyIndex::ScaleTransform_CenterY:
        case KnownPropertyIndex::ScaleTransform_ScaleX:
        case KnownPropertyIndex::ScaleTransform_ScaleY:
        case KnownPropertyIndex::RotateTransform_Angle:
        case KnownPropertyIndex::RotateTransform_CenterX:
        case KnownPropertyIndex::RotateTransform_CenterY:
        case KnownPropertyIndex::SkewTransform_CenterX:
        case KnownPropertyIndex::SkewTransform_CenterY:
        case KnownPropertyIndex::SkewTransform_AngleX:
        case KnownPropertyIndex::SkewTransform_AngleY:
        case KnownPropertyIndex::CompositeTransform_CenterX:
        case KnownPropertyIndex::CompositeTransform_CenterY:
        case KnownPropertyIndex::CompositeTransform_ScaleX:
        case KnownPropertyIndex::CompositeTransform_ScaleY:
        case KnownPropertyIndex::CompositeTransform_SkewX:
        case KnownPropertyIndex::CompositeTransform_SkewY:
        case KnownPropertyIndex::CompositeTransform_Rotation:
        case KnownPropertyIndex::CompositeTransform_TranslateX:
        case KnownPropertyIndex::CompositeTransform_TranslateY:
            // These transforms no longer use legacy DComp transforms.
            // There's nothing to unset and no static values to push to legacy DComp.
            break;

        case KnownPropertyIndex::UIElement_Opacity:
        case KnownPropertyIndex::TransitionTarget_Opacity:
            // Opacity animation is set on a DComp Visual. There is no leak here
            // as the visual is destroyed together with the comp tree node holding it.
            break;

        case KnownPropertyIndex::PlaneProjection_CenterOfRotationX:
        case KnownPropertyIndex::PlaneProjection_CenterOfRotationY:
        case KnownPropertyIndex::PlaneProjection_CenterOfRotationZ:
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetX:
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetY:
        case KnownPropertyIndex::PlaneProjection_GlobalOffsetZ:
        case KnownPropertyIndex::PlaneProjection_LocalOffsetX:
        case KnownPropertyIndex::PlaneProjection_LocalOffsetY:
        case KnownPropertyIndex::PlaneProjection_LocalOffsetZ:
        case KnownPropertyIndex::PlaneProjection_RotationX:
        case KnownPropertyIndex::PlaneProjection_RotationY:
        case KnownPropertyIndex::PlaneProjection_RotationZ:
        case KnownPropertyIndex::CompositeTransform3D_CenterX:
        case KnownPropertyIndex::CompositeTransform3D_CenterY:
        case KnownPropertyIndex::CompositeTransform3D_CenterZ:
        case KnownPropertyIndex::CompositeTransform3D_RotationX:
        case KnownPropertyIndex::CompositeTransform3D_RotationY:
        case KnownPropertyIndex::CompositeTransform3D_RotationZ:
        case KnownPropertyIndex::CompositeTransform3D_ScaleX:
        case KnownPropertyIndex::CompositeTransform3D_ScaleY:
        case KnownPropertyIndex::CompositeTransform3D_ScaleZ:
        case KnownPropertyIndex::CompositeTransform3D_TranslateX:
        case KnownPropertyIndex::CompositeTransform3D_TranslateY:
        case KnownPropertyIndex::CompositeTransform3D_TranslateZ:
            // These transforms no longer use legacy DComp transforms.
            // There's nothing to unset and no static values to push to legacy DComp.
            break;
    }
}

void CDoubleAnimation::ApplyDCompAnimationInstanceToTarget(_In_opt_ IUnknown* pDCompAnimation)
{
    if (GetTargetObjectWeakRef() && m_pTargetDependencyProperty != nullptr)
    {
        CDependencyObject* pDO = GetTargetObjectWeakRef().lock();

        if (pDO != nullptr)
        {
            bool isTranslateXY = false;

            KnownPropertyIndex dcompAnimationPropertyIndex = CAnimation::MapToDCompAnimationProperty(m_pTargetDependencyProperty->GetIndex());
            if (pDCompAnimation == nullptr)
            {
                m_fAnimationApplied = false;
            }
            else
            {
                ASSERT(dcompAnimationPropertyIndex != KnownPropertyIndex::UnknownType_UnknownProperty);

                // A DComp animation instance that has handoff already evaluated is not allowed to be set on anything, even the same
                // property on the same object. Therefore, check if the animation instance is already set on the target object/property
                // and no-op.
                if (m_fAnimationApplied)
                {
                    IUnknown* pCurrentDCompAnimationUnknown = pDO->GetDCompAnimation(dcompAnimationPropertyIndex);

                    // DComp objects have tear-off interfaces, so merely casting a pointer isn't enough. Actually QI to IUnknown.
                    xref_ptr<IUnknown> pDCompAnimationUnknown;
                    VERIFYHR(pDCompAnimation->QueryInterface(IID_PPV_ARGS(pDCompAnimationUnknown.ReleaseAndGetAddressOf())));
                    if (pCurrentDCompAnimationUnknown == pDCompAnimationUnknown.get())
                    {
                        return;
                    }
                }
                m_fAnimationApplied = true;
            }

            // Record the target dependency object. Normally the render walk will walk to it and update its WUC expression, which
            // will connect and start the WUC animation that we set here. However, if the DO is in a collapsed part of the tree, it
            // won't get walked. We still want to start the animation, so we'll need to walk to it explicitly via the list of
            // registered targets.
            //
            // In the case where pDCompAnimation is null, we also want to record the target dependency object. We want to visit it
            // later to explicitly disconnect the old WUC animation from the WUC expression, otherwise infinite WUC animations can
            // keep ticking. Note that the WUC expression itself will also be disconnected from the WUC visual if the Xaml element
            // is no longer animating.
            CTimeManager* timeManagerNoRef = GetTimeManager();
            if (timeManagerNoRef != nullptr)    // Test stubs return null
            {
                timeManagerNoRef->AddTargetDO(pDO);
            }

            switch (m_pTargetDependencyProperty->GetIndex())
            {
                case KnownPropertyIndex::Canvas_Left:
                {
                    CUIElement* pElement = static_cast<CUIElement*>(pDO);
                    pElement->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pElement->SetIsCanvasLeftAnimationDirty();
                    CUIElement::NWSetTransformDirty(pElement, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::Canvas_Top:
                {
                    CUIElement* pElement = static_cast<CUIElement*>(pDO);
                    pElement->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pElement->SetIsCanvasTopAnimationDirty();
                    CUIElement::NWSetTransformDirty(pElement, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::TranslateTransform_X:
                {
                    CTranslateTransform* pTransform = static_cast<CTranslateTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    isTranslateXY = true;
                    break;
                }

                case KnownPropertyIndex::TranslateTransform_Y:
                {
                    CTranslateTransform* pTransform = static_cast<CTranslateTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    isTranslateXY = true;
                    break;
                }

                case KnownPropertyIndex::ScaleTransform_CenterX:
                {
                    CScaleTransform* pTransform = static_cast<CScaleTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::ScaleTransform_CenterY:
                {
                    CScaleTransform* pTransform = static_cast<CScaleTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::ScaleTransform_ScaleX:
                {
                    CScaleTransform* pTransform = static_cast<CScaleTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::ScaleTransform_ScaleY:
                {
                    CScaleTransform* pTransform = static_cast<CScaleTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::RotateTransform_Angle:
                {
                    CRotateTransform* pTransform = static_cast<CRotateTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isAngleAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::RotateTransform_CenterX:
                {
                    CRotateTransform* pTransform = static_cast<CRotateTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::RotateTransform_CenterY:
                {
                    CRotateTransform* pTransform = static_cast<CRotateTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::SkewTransform_CenterX:
                {
                    CSkewTransform* pTransform = static_cast<CSkewTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::SkewTransform_CenterY:
                {
                    CSkewTransform* pTransform = static_cast<CSkewTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::SkewTransform_AngleX:
                {
                    CSkewTransform* pTransform = static_cast<CSkewTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isAngleXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::SkewTransform_AngleY:
                {
                    CSkewTransform* pTransform = static_cast<CSkewTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isAngleYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_CenterX:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_CenterY:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_ScaleX:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_ScaleY:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_SkewX:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isSkewXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_SkewY:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isSkewYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_Rotation:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isRotateAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_TranslateX:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isTranslateXAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform_TranslateY:
                {
                    CCompositeTransform* pTransform = static_cast<CCompositeTransform*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isTranslateYAnimationDirty = true;
                    CTransform::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::UIElement_Opacity:
                {
                    CUIElement* pElement = static_cast<CUIElement*>(pDO);
                    pElement->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pElement->SetIsOpacityAnimationDirty(true);
                    CUIElement::NWSetOpacityDirty(pElement, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::TransitionTarget_Opacity:
                {
                    CTransitionTarget* pTransitionTarget = static_cast<CTransitionTarget*>(pDO);
                    pTransitionTarget->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransitionTarget->m_isOpacityAnimationDirty = true;
                    CTransitionTarget::NWSetOpacityDirty(pTransitionTarget, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_CenterOfRotationX:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isCenterOfRotationXAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_CenterOfRotationY:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isCenterOfRotationYAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_CenterOfRotationZ:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isCenterOfRotationZAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_GlobalOffsetX:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isGlobalOffsetXAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_GlobalOffsetY:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isGlobalOffsetYAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_GlobalOffsetZ:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isGlobalOffsetZAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_LocalOffsetX:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isLocalOffsetXAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_LocalOffsetY:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isLocalOffsetYAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_LocalOffsetZ:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isLocalOffsetZAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_RotationX:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isRotationXAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_RotationY:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isRotationYAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::PlaneProjection_RotationZ:
                {
                    CPlaneProjection* pProjection = static_cast<CPlaneProjection*>(pDO);
                    pProjection->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pProjection->m_isRotationZAnimationDirty = true;
                    CProjection::NWSetRenderDirty(pProjection, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_CenterX:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterXAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_CenterY:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterYAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_CenterZ:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isCenterZAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_RotationX:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isRotateXAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_RotationY:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isRotateYAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_RotationZ:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isRotateZAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_ScaleX:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleXAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_ScaleY:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleYAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_ScaleZ:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isScaleZAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_TranslateX:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isTranslateXAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_TranslateY:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isTranslateYAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }

                case KnownPropertyIndex::CompositeTransform3D_TranslateZ:
                {
                    CCompositeTransform3D* pTransform = static_cast<CCompositeTransform3D*>(pDO);
                    pTransform->SetDCompAnimation(pDCompAnimation, dcompAnimationPropertyIndex);
                    pTransform->m_isTranslateZAnimationDirty = true;
                    CTransform3D::NWSetRenderDirty(pTransform, DirtyFlags::Render);
                    break;
                }
            }

            // If the target object is not participating in the visual tree it will never be walked.
            // Force unset the animation here to prevent it from leaking.
            if (!pDCompAnimation)
            {
                // This removes the DComp animation from the DComp transform too early, and can break animation handoff
                // scenarios. DComp animation handoff requires that the new animation overwrite the old animation directly,
                // and this will overwrite the old animation with null before the new animation can be set. This has the
                // very visible impact of breaking the office cursor animation.
                // In the future we should defer this clear to the end of the frame, after the new animation has had a
                // chance to overwrite the old animation. For now, the workaround is to only force a clear here for
                // animations that aren't TranslateTransform.X/Y (which the office cursor uses), or for animations on
                // objects that aren't in the live tree (the office cursor is in the live tree).
                if (!isTranslateXY || !pDO->IsActive())
                {
                    ForceUnsetDCompAnimation(m_pTargetDependencyProperty->GetIndex(), pDO);
                }
            }
        }
    }
}

void CDoubleAnimation::AttachDCompAnimationInstancesToTarget()
{
    if (m_spWUCAnimation != nullptr)
    {
        ApplyDCompAnimationInstanceToTarget(m_spWUCAnimation.Get());
    }
}

void CDoubleAnimation::DetachDCompAnimationInstancesFromTarget()
{
    if (m_hasControlOfTarget)
    {
        // Called from ResolveLocalTarget before the target property or object changes. Unset the animation on the current target.
        // Called from FinalizeIteration when the animation stops. Unset the animation on the current target.
        // Note that if this animation no longer controls its target, we don't want to detach the target's animations. Otherwise
        // we could unintentionally remove the animation that has taken over control of the target.
        ApplyDCompAnimationInstanceToTarget(nullptr);
    }

    if (ShouldDiscardDCompAnimationOnDetach())
    {
        m_spWUCAnimation.Reset();
        m_wucAnimator.Reset();
        DetachDCompCompletedHandlerOnStop();
        CTimeline::SetDCompAnimationDirty(this, DirtyFlags::None /* flags - ignored */);
    }
}

void CDoubleAnimation::ResolvePendingDCompAnimationOperations()
{
    if (m_wucAnimator)
    {
        if (m_hasPendingSeekForDComp)
        {
            // Xaml seeks to positive infinity in order to complete storyboards (see CStoryboard::CompleteInternal).
            // This isn't allowed by DComp, which will get us an E_INVALIDARG. No-op this case. The DComp animation
            // will be detached from the tree anyway, since this timeline is at the end and is either Filling or Stopped.
            if (m_pendingDCompSeekTime != XFLOAT_INF)
            {
                // Xaml allows an animation to be seeked back arbitrarily far. DComp does not allow an animation to be
                // seeked back beyond the initial segment. We'll add a segment at time 0, but the app might still request
                // to seek to a negative time. In those cases we'll clamp the seek to 0 to avoid the E_INVALIDARG. The
                // other option is to no-op the seek, which seems worse than clamping.
                SeekDCompAnimationInstances(m_pendingDCompSeekTime < 0 ? 0 : m_pendingDCompSeekTime);
            }
            m_hasPendingSeekForDComp = false;
        }

        if (m_hasPendingPauseForDComp)
        {
            FAIL_FAST_ASSERT(!m_hasPendingResumeForDComp);
            IFCFAILFAST(m_wucAnimator->Pause());
            m_hasPendingPauseForDComp = false;
        }

        if (m_hasPendingResumeForDComp)
        {
            FAIL_FAST_ASSERT(!m_hasPendingPauseForDComp);
            IFCFAILFAST(m_wucAnimator->Start());    // Start is the resume API for WUC.
            m_hasPendingResumeForDComp = false;
        }
    }
}

void CDoubleAnimation::SeekDCompAnimationInstances(double globalSeekTime)
{
    if (m_wucAnimator)
    {
        // If there is a custom timeline driving the WUC animation, WUC will internally forward this seek to the legacy
        // DComp animation. Legacy DComp animations use signed ints for seeking, so a large UINT will become negative.
        // This then causes DComp to think that the legacy DComp animation is invalid, and to not evaluate it further,
        // which then means the WUC animation stops progressing and ticks forever.
        //
        // WUC limits its animations to a maximum length of 24 days, so we'll clamp the seek offset to 24 days.
        if (DirectUI::DoubleUtil::IsInfinity(globalSeekTime)
            || globalSeekTime > CAnimation::s_maxWUCKeyFrameAnimationDurationInSeconds)
        {
            m_wucAnimator->Seek(CAnimation::s_maxWUCKeyFrameAnimationDurationInTicks);
        }
        else
        {
            UINT64 offset100ns = static_cast<UINT64>(globalSeekTime * 10000000);
            m_wucAnimator->Seek(offset100ns);
        }
    }

    m_shouldSynchronizeDCompAnimationAfterResume = false;
}

void CDoubleAnimation::PauseDCompAnimationsOnSuspend()
{
    // This timeline may not be active. If it's a child of a storyboard with a longer duration, this timeline could be
    // Filling or Stopped while the parent is Active.

    if (m_wucAnimator)
    {
        IFCFAILFAST(m_wucAnimator->Pause());
    }
}

void CDoubleAnimation::ResumeDCompAnimationsOnResume()
{
    // This timeline may not be active. If it's a child of a storyboard with a longer duration, this timeline could be
    // Filling or Stopped while the parent is Active.

    if (m_wucAnimator)
    {
        IFCFAILFAST(m_wucAnimator->Start());    // Start is the resume API for WUC.
        m_shouldSynchronizeDCompAnimationAfterResume = true;
    }
}

void CDoubleAnimation::ReleaseDCompResources()
{
    __super::ReleaseDCompResources();
}

CompositionAnimationConversionResult CDoubleAnimation::MakeCompositionKeyFrameAnimationVirtual(_Inout_ CompositionAnimationConversionContext* myContext)
{
    const auto& scalarAnimation = myContext->CreateScalarLinearAnimation(
        m_vFrom,
        m_vTo,
        !m_isToAnimation,
        m_vBy,
        m_hasHandoff,
        static_cast<CEasingFunctionBase*>(m_pEasingFunction));

    scalarAnimation.As(&m_spWUCAnimation);
    IFC_ANIMATION(myContext->ApplyProperties(m_spWUCAnimation.Get()));
    UpdateWUCAnimationTelemetryString(myContext);

    if (GetDCompObjectRegistry() != nullptr)
    {
        GetDCompObjectRegistry()->EnsureObjectWithDCompResourceRegistered(this);
    }

    return CompositionAnimationConversionResult::Success;
}

_Check_return_ HRESULT CDoubleAnimation::ValueAssign(
    AssignmentOperand destOperand,
    AssignmentOperand sourceOperand)
{
    ASSERT(sourceOperand != destOperand);

    float* sourceValue = GetInternalOperand(sourceOperand);
    float* destValue = GetInternalOperand(destOperand);
    *destValue = *sourceValue;

    return S_OK;
}

_Check_return_ HRESULT CDoubleAnimation::ValueAssign(
    _Out_ CValue& destOperand,
    AssignmentOperand sourceOperand)
{
    float* sourceValue = GetInternalOperand(sourceOperand);
    destOperand.SetFloat(*sourceValue);

    return S_OK;
}

_Check_return_ HRESULT CDoubleAnimation::ValueAssign(
    AssignmentOperand destOperand,
    _In_ const CValue& sourceOperand)
{
    ASSERT(sourceOperand.GetType() == valueFloat || sourceOperand.GetType() == valueDouble);

    float* destValue = GetInternalOperand(destOperand);

    if (sourceOperand.GetType() == valueFloat)
    {
        *destValue = sourceOperand.AsFloat();
    }
    else
    {
        ASSERT(sourceOperand.GetType() == valueDouble);
        *destValue = static_cast<float>(sourceOperand.AsDouble());
    }

    return S_OK;
}

float* CDoubleAnimation::GetInternalOperand(AssignmentOperand operand)
{
    switch (operand)
    {
        case AssignmentOperand::From:
            return &m_vFrom;

        case AssignmentOperand::To:
            return &m_vTo;

        case AssignmentOperand::By:
            return &m_vBy;

        case AssignmentOperand::BaseValue:
            return &m_vBaseValue;

        case AssignmentOperand::NonAnimatedBaseValue:
            return &m_vNonAnimatedBaseValue;

        case AssignmentOperand::CurrentValue:
            return &m_vCurrentValue;

        default:
            XCP_FAULT_ON_FAILURE(false);
            return nullptr;
    }
}
