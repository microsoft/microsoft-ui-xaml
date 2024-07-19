// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ColorAnimation.h"
#include "ColorAnimationUsingKeyFrames.h"
#include "ColorKeyFrame.h"
#include "Timemgr.h"
#include <CColor.h>
#include <CValueConvert.h>

using namespace DirectUI;

template< XUINT32 ChannelOffset >
inline XUINT32 ExtractChannel(const XUINT32 Color)
{
    return (Color >> ChannelOffset) & 0xFF;
}

inline XUINT8 ComputeColorMultiply(const XINT32 FromColor, const XINT32 ToColor, const XINT32 MultiplicationFactor)
{
    return ByteSaturate(FromColor + (((ToColor - FromColor) * MultiplicationFactor) / 256));
}

template< XUINT32 ChannelOffset >
inline XUINT32 ComputeChannelColor(const XINT32 FromColor, const XINT32 ToColor, const XINT32 MultiplicationFactor)
{
    return ((XUINT32)ComputeColorMultiply(FromColor, ToColor, MultiplicationFactor)) << ChannelOffset;
}

_Check_return_ HRESULT CColorAnimation::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;

    CColorAnimation * pAnimation = new CColorAnimation(pCreate->m_pCore);

    IFC(ValidateAndInit(pAnimation, ppObject));

    pAnimation = NULL;

Cleanup:
    ReleaseInterface(pAnimation);
    RRETURN(hr);
}

// Generic interpolation method - this MUST be overridden by derived types
// to provide a way to blend to values of the given animation type
void CColorAnimation::InterpolateCurrentValue(XFLOAT rPercentEnd)
{
// Easing functions can give an unnormalized value outside of the range 0 to 1 so
// don't validate that rPercentEnd is from 0 to 1 but handle the cases where is is.

    if (rPercentEnd < 0.0f || rPercentEnd > 1.0f)
    {
        // Get the to and from values, if the percentage is negative swap the to and from values.
        const XUINT32 FromColor = (rPercentEnd >= 0.0f) ? m_vFrom : m_vTo;
        const XUINT32 ToColor = (rPercentEnd >= 0.0f) ? m_vTo : m_vFrom;

        // Handle if this is a negative percentage.
        if (rPercentEnd < 0.0f)
        {
            // The to and from have already been flipped so change the percentage to match.
            rPercentEnd = 1.0f - rPercentEnd;
        }

        // Compute interpolation
        const XUINT32 uBlend = XcpRound(rPercentEnd * 256.0f);

        // 32bit - ARGB
        const XUINT32 BlueShift     = 0x00;
        const XUINT32 GreenShift    = 0x08;
        const XUINT32 RedShift      = 0x10;
        const XUINT32 AlphaShift    = 0x18;

        // Extra color channels from source
        const XINT32 AlphaFrom = ExtractChannel< AlphaShift >(FromColor);
        const XINT32 RedFrom = ExtractChannel< RedShift >(FromColor);
        const XINT32 GreenFrom = ExtractChannel< GreenShift >(FromColor);
        const XINT32 BlueFrom = ExtractChannel< BlueShift >(FromColor);

        const XINT32 AlphaTo = ExtractChannel< AlphaShift >(ToColor);
        const XINT32 RedTo = ExtractChannel< RedShift >(ToColor);
        const XINT32 GreenTo = ExtractChannel< GreenShift >(ToColor);
        const XINT32 BlueTo = ExtractChannel< BlueShift >(ToColor);

        const XUINT32 FinalAlpha = ComputeChannelColor< AlphaShift >(AlphaFrom, AlphaTo, uBlend);
        const XUINT32 FinalRed = ComputeChannelColor< RedShift >(RedFrom, RedTo, uBlend);
        const XUINT32 FinalGreen = ComputeChannelColor< GreenShift >(GreenFrom, GreenTo, uBlend);
        const XUINT32 FinalBlue = ComputeChannelColor< BlueShift >(BlueFrom, BlueTo, uBlend);

        // Compute color
        const XUINT32 rgbResult = (FinalAlpha | FinalRed | FinalGreen | FinalBlue);

        // Set value into our Dependency object
        m_vCurrentValue = rgbResult;
    }
    else
    {
        const XUINT32 FromColor = m_vFrom;
        const XUINT32 ToColor = m_vTo;

        // Compute interpolation
        const XUINT32 uBlend = XcpRound(rPercentEnd * 256.0f);
        const XUINT32 uInvBlend = 256 - uBlend;

        // Convert to our fast blending format
        const XUINT32 uColorFrom00aa00gg = ((FromColor) >> 8) & 0x00ff00ff;
        const XUINT32 uColorFrom00rr00bb = (FromColor) & 0x00ff00ff;

        const XUINT32 uColorTo00aa00gg = ((ToColor) >> 8) & 0x00ff00ff;
        const XUINT32 uColorTo00rr00bb = (ToColor) & 0x00ff00ff;

        // Blend
        const XUINT32 uBlendedToColoraa00gg00 = ((uColorTo00aa00gg * uBlend) & 0xff00ff00);
        const XUINT32 uBlendedToColor00rr00bb = (((uColorTo00rr00bb * uBlend ) >> 8) & 0x00ff00ff);

        const XUINT32 uBlendedFromColoraa00gg00 = ((uColorFrom00aa00gg * uInvBlend) & 0xff00ff00);
        const XUINT32 uBlendedFromColor00rr00bb = (((uColorFrom00rr00bb * uInvBlend ) >> 8) & 0x00ff00ff);

        // Compute color
        const XUINT32 rgbResult = (uBlendedFromColoraa00gg00 + uBlendedToColoraa00gg00)
            | (uBlendedFromColor00rr00bb + uBlendedToColor00rr00bb);

        // Set value into our Dependency object
        m_vCurrentValue = rgbResult;
    }
}

// Computes the m_pTo field from the m_pBy and m_pFrom fields using a type-specific addition
void CColorAnimation::ComputeToUsingFromAndBy()
{
    // Compute type-specific add
    // For Color do a clamped component-wise add

    // Split "From" value into channels
    XUINT32 uAFrom = ( m_vFrom >> 24 ) & 0x000000ff ;
    XUINT32 uRFrom = ( m_vFrom >> 16 ) & 0x000000ff ;
    XUINT32 uGFrom = ( m_vFrom >> 8 ) & 0x000000ff ;
    XUINT32 uBFrom = ( m_vFrom >> 0 ) & 0x000000ff ;

    // Add "By" channel values
    uAFrom += ( m_vBy >> 24 ) & 0x000000ff ;
    uRFrom += ( m_vBy >> 16 ) & 0x000000ff ;
    uGFrom += ( m_vBy >> 8 ) & 0x000000ff ;
    uBFrom += ( m_vBy >> 0 ) & 0x000000ff ;

    // Clamp channel values at 0xff
    uAFrom = MIN( uAFrom, 0x000000ff );
    uRFrom = MIN( uRFrom, 0x000000ff );
    uGFrom = MIN( uGFrom, 0x000000ff );
    uBFrom = MIN( uBFrom, 0x000000ff );

    // Reconstruct Color
    m_vTo = (uAFrom << 24) + (uRFrom << 16) + (uGFrom << 8) + uBFrom;
}

// Initializes the base value of the animation from either the animation target or from a handoff.
_Check_return_ HRESULT CColorAnimation::GetAnimationBaseValue()
{
    // we should have a reference, but if we don't have an actual target, keep current value
    ASSERT(GetTargetObjectWeakRef());

    auto targetObject = GetTargetObjectWeakRef().lock();

    if (targetObject)
    {
        CValue value;
        value.SetColor(0);

        IFC_RETURN(targetObject->GetValueByIndex(m_pTargetDependencyProperty->GetIndex(), &value));

        CValue rawValue;
        IFC_RETURN(CValueConvert::EnsurePropertyValueUnboxed(value, rawValue));

        if (rawValue.GetType() == valueColor)
        {
            m_vBaseValue = rawValue.AsColor();
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
                m_vBaseValue = static_cast<CColor*>(rawValueAsObject)->m_rgb;
            }
        }
    }

    return S_OK;
}

// Find the element(s) whose properties are targeted by this animation.
// Returns whether or not the animation is independent.
_Check_return_ HRESULT CColorAnimation::FindIndependentAnimationTargetsRecursive(
    _In_ CDependencyObject *pTargetObject,
    _In_opt_ CDependencyObject *pSender,
    _Inout_opt_ IATargetVector *pIATargets,
    _Out_ bool *pIsIndependentAnimation
    )
{
    // Assume the animation is not independent until we find a target UIElement.
    bool isIndependentAnimation = false;

    // If we reached a target UIElement, notify it of the animation.
    // NOTE: Independent color animations are not supported for TextBox/PasswordBox/RichEditBox.
    //           They are supported for TextBlock, RichTextBlock, Glyphs, and other UIElements (Shape, Border, Panel, etc).
    if (pTargetObject->OfTypeByIndex<KnownTypeIndex::UIElement>()
        && !pTargetObject->OfTypeByIndex<KnownTypeIndex::TextBoxView>())
    {
        CUIElement *pElement = static_cast<CUIElement*>(pTargetObject);

        //
        // Approved list of property targets for independent animation
        //
        // Note: This doesn't distinguish between stroke & fill or between border & background.
        // If one of them is animated, then the other will be cloned as well. The clone just won't
        // be animated on the render thread. If this is a problem, we can split them up.
        //
        IndependentAnimationType animationType = IndependentAnimationType::None;

        ASSERT(!m_pTargetDependencyProperty->IsSparse());
        switch (m_pTargetDependencyProperty->GetIndex())
        {
        // Brush properties.
        case KnownPropertyIndex::SolidColorBrush_Color:
            // Brush color animations are independent.
            ASSERT(pSender != NULL && pSender->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>());

            animationType = IndependentAnimationType::BrushColor;
            isIndependentAnimation = TRUE;
            break;

        default:
            // Not independent.
            ASSERT(!isIndependentAnimation);
            break;
        }

        if (isIndependentAnimation)
        {
            bool hasForegroundProperty = false;
            xref_ptr<CBrush> pForegroundBrush;
            IFC_RETURN(pElement->GetForegroundBrush(&hasForegroundProperty, pForegroundBrush.ReleaseAndGetAddressOf()));

            // The Foreground property is inherited. If the animating brush is set as the Foreground, the animation
            // needs to be inherited as well. It also might end up being dependent if a descendent is cached.
            if (hasForegroundProperty && pForegroundBrush == pSender)
            {
                // FindInheritedIATargets will add pElement as a target as well.
                IFC_RETURN(FindInheritedIndependentAnimationTargets(
                    pElement,
                    pForegroundBrush,
                    pIATargets));
            }
            // If this ColorAnimation was on a property other than the Foreground, just add pElement to the targets.
            else if (pIATargets)
            {
                IATarget target;

                ASSERT(animationType != IndependentAnimationType::None);
                target.animationType = animationType;
                target.targetWeakRef = xref::get_weakref(pElement);
                pIATargets->push_back(target);
            }
        }
    }
    // If we haven't reached a UIElement, keep walking up.
    else if (pTargetObject->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
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

// Assumes the animated brush is originally set as an inherited property (Foreground).
// Walks through the subgraph to verify no element inheriting this property is cached,
// and that all all elements inherited the property are marked as animation targets as
// well.
_Check_return_ HRESULT CColorAnimation::FindInheritedIndependentAnimationTargets(
    _In_ CUIElement *pElement,
    _In_ const CBrush *pAnimatedBrush,
    _Inout_opt_ IATargetVector *pIATargets
    )
{
    CUIElementCollection *pChildCollectionNoRef = static_cast<CUIElementCollection*>(pElement->GetChildren());

    bool hasForegroundProperty = false;
    xref_ptr<CBrush> pForegroundBrush;

    // Check if this element supports the Foreground property.
    IFC_RETURN(pElement->GetForegroundBrush(&hasForegroundProperty, pForegroundBrush.ReleaseAndGetAddressOf()));

    if (hasForegroundProperty)
    {
        // If the Foreground is the animated brush, then the animation is inherited here.
        if (pForegroundBrush == pAnimatedBrush)
        {
            // Mark the element as animated.
            if (pIATargets)
            {
                IATarget target;
                target.animationType = IndependentAnimationType::BrushColor;
                target.targetWeakRef = xref::get_weakref(pElement);

                pIATargets->push_back(target);
            }
        }
        // Otherwise, the element is not inheriting the Foreground animation since it has a different Foreground
        // brush set. The breadth walk should stop, since the animation will not inherit any further in this branch.
        else
        {
            pChildCollectionNoRef = NULL;
        }
    }

    // Walk through the children looking for more elements that might inherit this animation.
    if (pChildCollectionNoRef)
    {
        for (XUINT32 i = 0; i < pChildCollectionNoRef->GetCount(); i++)
        {
            xref_ptr<CUIElement> pChild;
            pChild.attach(static_cast<CUIElement*>(pChildCollectionNoRef->GetItemWithAddRef(i)));

            if (pChild)
            {
                IFC_RETURN(FindInheritedIndependentAnimationTargets(pChild, pAnimatedBrush, pIATargets));
            }

        }
    }

    return S_OK;
}

_Check_return_ HRESULT CColorAnimationUsingKeyFrames::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate)
{
    // Create type
    xref_ptr<CColorAnimationUsingKeyFrames> animation;
    animation.init(new CColorAnimationUsingKeyFrames(pCreate->m_pCore));

    // Create internal members
    xref_ptr<CColorKeyFrameCollection> keyFrames;
    IFC_RETURN(CColorKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(keyFrames.ReleaseAndGetAddressOf()), pCreate));
    IFC_RETURN(animation->SetValueByKnownIndex(KnownPropertyIndex::ColorAnimationUsingKeyFrames_KeyFrames, keyFrames.get()));

    animation->m_pDPValue = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::ColorKeyFrame_Value);
    ASSERT(animation->m_pDPValue != nullptr);

    //do base-class initialization and return new object
    IFC_RETURN(ValidateAndInit(animation.detach(), ppObject));

    return S_OK;
}
