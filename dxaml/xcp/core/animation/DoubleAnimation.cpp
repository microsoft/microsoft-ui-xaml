// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "DoubleKeyFrame.h"
#include "Timemgr.h"
#include <CValueConvert.h>

KnownTypeIndex CDoubleAnimation::GetTypeIndex() const
{
    return DependencyObjectTraits<CDoubleAnimation>::Index;
}

_Check_return_ HRESULT CDoubleAnimation::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;

    CDoubleAnimation * pDoubleAnimation = new CDoubleAnimation(pCreate->m_pCore);

    IFC(ValidateAndInit(pDoubleAnimation, ppObject));

    pDoubleAnimation = NULL;

Cleanup:
    ReleaseInterface(pDoubleAnimation);
    RRETURN(hr);
}

// Generic interpolation method - this MUST be overridden by derived types
// to provide a way to blend to values of the given animation type
void CDoubleAnimation::InterpolateCurrentValue(XFLOAT rPercentEnd)
{
    // Now that we have EasingFunctions we can go above 1.0 and below 0.0.
    // Easing functions can give an unnormalized value outside of the range 0 to 1 so
    // don't validate that rPercentEnd is from 0 to 1 but handle the cases where is is.

    XFLOAT rPercentStart = 1.0f - rPercentEnd;
    XFLOAT rResult = m_vFrom * rPercentStart + rPercentEnd * m_vTo;

    m_vCurrentValue = rResult;
}

// Computes the m_pTo field from the m_pBy and m_pFrom fields using a type-specific addition;
void CDoubleAnimation::ComputeToUsingFromAndBy()
{
    // Compute type-specific To value
    m_vTo = m_vFrom + m_vBy;
}

// Initializes the base value of the animation from either the animation target or from a handoff.
_Check_return_ HRESULT CDoubleAnimation::GetAnimationBaseValue()
{
    // we should have a reference, but if we don't have an actual target, keep current value

    ASSERT(GetTargetObjectWeakRef());

    auto targetObject = GetTargetObjectWeakRef().lock();

    if (targetObject)
    {
        CValue value;
        value.SetFloat(0.0f);

        IFC_RETURN(targetObject->GetValueByIndex(m_pTargetDependencyProperty->GetIndex(), &value));

        CValue rawValue;
        IFC_RETURN(CValueConvert::EnsurePropertyValueUnboxed(value, rawValue));

        if (rawValue.GetType() == valueFloat)
        {
            m_vBaseValue = rawValue.AsFloat();
        }
        if (rawValue.GetType() == valueDouble)
        {
            m_vBaseValue = static_cast<XFLOAT>(rawValue.AsDouble());
        }
        else if (rawValue.GetType() == valueObject)
        {
            // We are getting one of the DO animation properties ( To/From/By )
            // which are not stored as value types.
            // GetValue may return null if we are trying to query an animation property
            // that has never been set - if that's the case then both it and the basevalue
            // have been initialized to the same default in the animation constructor.

            CDependencyObject* rawValueAsObject = rawValue.AsObject();

            if (rawValueAsObject)
            {
                m_vBaseValue = static_cast<CDouble*>(rawValueAsObject)->m_eValue;
            }
        }
    }

    return S_OK;
}

// Find the element(s) whose properties are targeted by this animation.
// Returns whether or not the animation is independent.
_Check_return_ HRESULT CDoubleAnimation::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    // Assume the animation is not independent until we find a target UIElement.
    bool isIndependentAnimation = false;

    // If we reached a target UIElement, notify it of the animation.
    const bool isTargetUIElement = pTargetObject->OfTypeByIndex<KnownTypeIndex::UIElement>();
    if (isTargetUIElement)
    {
        CUIElement *pElement = static_cast<CUIElement*>(pTargetObject);

        //
        // Approved list of property targets for independent animation
        //
        IndependentAnimationType animationType = IndependentAnimationType::None;

        switch (m_pTargetDependencyProperty->GetIndex())
        {
            // UIElement properties.
            case KnownPropertyIndex::Canvas_Left:
            case KnownPropertyIndex::Canvas_Top:
            {
                ASSERT(pSender == NULL);
                CDependencyObject *pParent = pElement->GetParentInternal(false);

                // Animate Canvas.Left/Top only if the parent is a canvas.
                if (pParent != NULL &&
                    pParent->OfTypeByIndex<KnownTypeIndex::Canvas>())
                {
                    animationType = IndependentAnimationType::Offset;
                    isIndependentAnimation = TRUE;
                }
                break;
            }

            case KnownPropertyIndex::UIElement_Opacity:
                ASSERT(pSender == NULL);
                animationType = IndependentAnimationType::ElementOpacity;
                isIndependentAnimation = TRUE;
                break;

            // Transform properties.
            case KnownPropertyIndex::RotateTransform_CenterX:
            case KnownPropertyIndex::RotateTransform_CenterY:
            case KnownPropertyIndex::RotateTransform_Angle:
            case KnownPropertyIndex::ScaleTransform_CenterY:
            case KnownPropertyIndex::ScaleTransform_ScaleX:
            case KnownPropertyIndex::ScaleTransform_ScaleY:
            case KnownPropertyIndex::ScaleTransform_CenterX:
            case KnownPropertyIndex::SkewTransform_CenterX:
            case KnownPropertyIndex::SkewTransform_CenterY:
            case KnownPropertyIndex::SkewTransform_AngleX:
            case KnownPropertyIndex::SkewTransform_AngleY:
            case KnownPropertyIndex::TranslateTransform_X:
            case KnownPropertyIndex::TranslateTransform_Y:
            case KnownPropertyIndex::CompositeTransform_CenterX:
            case KnownPropertyIndex::CompositeTransform_CenterY:
            case KnownPropertyIndex::CompositeTransform_ScaleX:
            case KnownPropertyIndex::CompositeTransform_ScaleY:
            case KnownPropertyIndex::CompositeTransform_SkewX:
            case KnownPropertyIndex::CompositeTransform_SkewY:
            case KnownPropertyIndex::CompositeTransform_Rotation:
            case KnownPropertyIndex::CompositeTransform_TranslateX:
            case KnownPropertyIndex::CompositeTransform_TranslateY:
                // Render transform animations are independent.
                if (pSender->OfTypeByIndex<KnownTypeIndex::Transform>())
                {
                    animationType = IndependentAnimationType::Transform;
                    isIndependentAnimation = TRUE;
                }
                // Clip transform animations are independent.
                else if (pSender->OfTypeByIndex<KnownTypeIndex::RectangleGeometry>())
                {
                    animationType = IndependentAnimationType::ElementClip;
                    isIndependentAnimation = TRUE;
                }
                else if (pSender->OfTypeByIndex<KnownTypeIndex::TransitionTarget>())
                {
                    // TransitionTarget exposes two transform properties, one that animates a transform
                    // and one that actually maps to a clip animation. Use the original animation target
                    // to determine which is actually being animated here.
                    auto transform = GetTargetObjectWeakRef().lock();
                    CTransitionTarget *pTransitionTargetNoRef = static_cast<CTransitionTarget*>(pSender);
                    if (pTransitionTargetNoRef->m_pxf == transform)
                    {
                        animationType = IndependentAnimationType::Transform;
                    }
                    else
                    {
                        ASSERT(pTransitionTargetNoRef->m_pClipTransform == transform);
                        animationType = IndependentAnimationType::TransitionClip;
                    }
                    isIndependentAnimation = TRUE;
                }
                break;

            // PlaneProjection properties
            case KnownPropertyIndex::PlaneProjection_RotationY:
            case KnownPropertyIndex::PlaneProjection_RotationZ:
            case KnownPropertyIndex::PlaneProjection_RotationX:
            case KnownPropertyIndex::PlaneProjection_CenterOfRotationX:
            case KnownPropertyIndex::PlaneProjection_CenterOfRotationY:
            case KnownPropertyIndex::PlaneProjection_CenterOfRotationZ:
            case KnownPropertyIndex::PlaneProjection_LocalOffsetX:
            case KnownPropertyIndex::PlaneProjection_LocalOffsetY:
            case KnownPropertyIndex::PlaneProjection_LocalOffsetZ:
            case KnownPropertyIndex::PlaneProjection_GlobalOffsetX:
            case KnownPropertyIndex::PlaneProjection_GlobalOffsetY:
            case KnownPropertyIndex::PlaneProjection_GlobalOffsetZ:
                ASSERT(pSender->OfTypeByIndex<KnownTypeIndex::PlaneProjection>());
                animationType = IndependentAnimationType::ElementProjection;
                isIndependentAnimation = TRUE;
                break;

            // CompositeTransform3D properties
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
                ASSERT(pSender->OfTypeByIndex<KnownTypeIndex::CompositeTransform3D>());
                animationType = IndependentAnimationType::ElementTransform3D;
                isIndependentAnimation = TRUE;
                break;

            // Transition target properties.
            case KnownPropertyIndex::TransitionTarget_Opacity:
                ASSERT(pSender->OfTypeByIndex<KnownTypeIndex::TransitionTarget>());
                animationType = IndependentAnimationType::TransitionOpacity;
                isIndependentAnimation = TRUE;
                break;

            default:
                // Not independent.
                ASSERT(!isIndependentAnimation);
                break;
        }

        // If this UIElement is a valid IA target, add it to the list for this animation.
        if (isIndependentAnimation && pIATargets)
        {
            IATarget target;

            ASSERT(animationType != IndependentAnimationType::None);
            target.animationType = animationType;
            target.targetWeakRef = xref::get_weakref(pElement);

            pIATargets->push_back(target);
        }
    }
    // If we haven't reached a UIElement, keep walking up.
    else if (pTargetObject->OfTypeByIndex<KnownTypeIndex::Transform>()
        || pTargetObject->OfTypeByIndex<KnownTypeIndex::TransformCollection>()
        || pTargetObject->OfTypeByIndex<KnownTypeIndex::Projection>()
        || pTargetObject->OfTypeByIndex<KnownTypeIndex::Transform3D>()
        || pTargetObject->OfTypeByIndex<KnownTypeIndex::TransitionTarget>()
        || pTargetObject->OfTypeByIndex<KnownTypeIndex::RectangleGeometry>())
    {
        IFC_RETURN(CheckElementUsingAnimatedTarget(
            pTargetObject,
            pIATargets,
            &isIndependentAnimation));
    }
    // Since a shareable resource being animated might be defined in a resource dictionary,
    // do not disable the animation because of it.  The animation will only be independent
    // if it's hooked up to a non-HW cached UIElement elsewhere, however.
    else if (pTargetObject->OfTypeByIndex<KnownTypeIndex::ResourceDictionary>())
    {
        isIndependentAnimation = TRUE;
    }

    *pIsIndependentAnimation = isIndependentAnimation;

    return S_OK;
}

_Check_return_ HRESULT CDoubleAnimationUsingKeyFrames::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    // Create type
    xref_ptr<CDoubleAnimationUsingKeyFrames> animation;
    animation.init(new CDoubleAnimationUsingKeyFrames(pCreate->m_pCore));

    // Create internal members
    xref_ptr<CDoubleKeyFrameCollection> keyFrames;
    IFC_RETURN(CDoubleKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(keyFrames.ReleaseAndGetAddressOf()), pCreate));
    IFC_RETURN(animation->SetValueByKnownIndex(KnownPropertyIndex::DoubleAnimationUsingKeyFrames_KeyFrames, keyFrames.get()));

    animation->m_pDPValue = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::DoubleKeyFrame_Value);
    ASSERT(animation->m_pDPValue != nullptr);

    //do base-class initialization and return new object
    IFC_RETURN(ValidateAndInit(animation.detach(), ppObject));

    return S_OK;
}
