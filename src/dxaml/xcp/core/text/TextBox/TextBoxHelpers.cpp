// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <math.h>
#include "TextSelectionSettings.h"

#include "Storyboard.h"
#include "TimelineCollection.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "ObjectAnimationUsingKeyFrames.h"
#include "KeyTime.h"
#include "Timespan.h"
#include "DoubleKeyFrame.h"
#include "ObjectKeyFrame.h"
#include "DurationVO.h"
#include "KeyTimeVO.h"

using namespace RichTextServices;

#define DEFAULT_TEXTBOX_BLINK_TIME 500 // In milliseconds

//------------------------------------------------------------------------
//  Summary:
//      Appends a double discrete key frame into the collection. Equivalent of:
//      <DiscreteDoubleKeyFrame Value="rValue" KeyTime="{rEndTime}"/>
//
//      Note that the rValue gets set at rEndTime.
//------------------------------------------------------------------------
/* static */
_Check_return_ HRESULT CTextBoxHelpers::AppendDoubleDiscreteKeyFrame(
    _In_ CCoreServices                 *pCore,
    _In_ XFLOAT                         rValue,
    _In_ XFLOAT                         rEndTime,
    _Inout_ CDoubleKeyFrameCollection  *pKeyFramesCollection
    )
{
    xref_ptr<CDiscreteDoubleKeyFrame> doubleKeyFrame;
    CValue                          valueTemp;
    CREATEPARAMETERS                cp(pCore);

    IFC_RETURN(CreateDO(doubleKeyFrame.ReleaseAndGetAddressOf(), &cp));

    IFC_RETURN(doubleKeyFrame->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_Value, rValue));

    valueTemp.Set<valueVO>(KeyTimeVOHelper::CreateTimeSpan(pCore, rEndTime).detach());
    IFC_RETURN(doubleKeyFrame->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_KeyTime, valueTemp));

    IFC_RETURN(pKeyFramesCollection->Append(doubleKeyFrame));

    return S_OK;
}

float CTextBoxHelpers::GetCaretBlinkingPeriod()
{
    // Duration of a state, on or off, in milliseconds. Full blink cycle is 2X this time.
    XUINT32 iCaretBlinkTimeInMs = INFINITE;
    const bool bCaretBlinkingEnabled = !!::GetSystemMetrics(SM_CARETBLINKINGENABLED);

    // If we failed to get the SM_CARETBLINKINGENABLED system metric (i.e., GetLastError() returned an error)
    // then that's because it's not defined on the current platform. If it's not defined, then the default is for the caret to blink,
    // so we'll have it blink in that case.
    if (FAILED(HRESULT_FROM_WIN32(GetLastError())) || bCaretBlinkingEnabled)
    {
        iCaretBlinkTimeInMs = ::GetCaretBlinkTime();
    }

    // Return a default value of 500ms if the OS default is zero.
    if (iCaretBlinkTimeInMs == 0)
    {
        iCaretBlinkTimeInMs = DEFAULT_TEXTBOX_BLINK_TIME;
    }

    // Zero is indication of an error.
    FAIL_FAST_ASSERT(iCaretBlinkTimeInMs > 0);

    const XUINT32 blinkTimeUpperBound = 10000;
    if (iCaretBlinkTimeInMs == INFINITE)
    {
        iCaretBlinkTimeInMs = 0;
    }
    else if (iCaretBlinkTimeInMs > blinkTimeUpperBound)
    {
        iCaretBlinkTimeInMs  = blinkTimeUpperBound;
    }
    const float blinkInSeconds = static_cast<float>(iCaretBlinkTimeInMs) / 1000.0f;

    return blinkInSeconds * 2.0f;
}

//------------------------------------------------------------------------
//  Summary:
//      Creates a storyboard that can be used for 'blinking' (alternating
//      between 2 values with linear ramp up and down) double property on a
//      given object. This is used for the caret blinking animation.
//
//      If noSmoothTransitions is set to true, the ramp times are set to 0
//      to set the simple on/off blinking behavior.
//
//      The linear ramps are simulated with a sequence discrete keyframes
//      in order to lower the CPU usage by lowering the frame refresh rate.
//      The CARET_ANIMATION_RAMP_FRAME_RATE value is used to calculate
//      the number of discrete keyframes per linear approximation.
//
//      The storyboard created is roughly equivalent to:
//
//         <Storyboard>
//          <DoubleAnimationUsingKeyFrames RepeatBehavior="Forever" BeginTime="0:0:0" Duration="{blinkPeriodInSeconds}">
//           <DiscreteDoubleKeyFrame Value="maxOpacity" KeyTime="{UpPlateauTime}"/>
//              Sequence of <DiscreteDoubleKeyFrame Value="x" KeyTime="{t}"/> that simulate the following linear keyframe:
//                  <LinearDoubleKeyFrame   Value="0"           KeyTime="{UpPlateauTime+RampDownTime}"/>
//           <DiscreteDoubleKeyFrame Value="0"           KeyTime="{UpPlateauTime+RampDownTime+DownPlateauTime}"/>
//              Sequence of <DiscreteDoubleKeyFrame Value="x" KeyTime="{t}"/> that simulate the following linear keyframe:
//                  <LinearDoubleKeyFrame   Value="maxOpacity" KeyTime="{UpPlateauTime+RampDownTime+DownPlateauTime+DownPlateauTime}"/>
//          </DoubleAnimationUsingKeyFrames>
//         </Storyboard>
//------------------------------------------------------------------------
/*static*/
_Check_return_ HRESULT CTextBoxHelpers::CreateCaretAnimationStoryboard(
        _Outptr_ CStoryboard                        **resultStoryboard,
        _In_ CCoreServices                           *core,
        _In_ CDependencyObject                       *targetObject,
        _In_ const CDependencyProperty               *targetProperty,
        _In_ float                                    maxOpacity,
        _In_ float                                    blinkPeriodInSeconds
        )
{
    xref_ptr<CDoubleAnimationUsingKeyFrames> doubleAnimation;
    xref_ptr<CTimeSpan>                      beginTime;
    xref_ptr<CStoryboard>                    storyboard = NULL;
    xref_ptr<CDoubleKeyFrameCollection>      keyFramesCollection = NULL;

    CValue  valueTemp;
    CValue  valueZero;
    valueZero.SetFloat(0.0f);

    CREATEPARAMETERS cpZeroTime(core, valueZero);
    CREATEPARAMETERS cp(core);

    ASSERT(resultStoryboard);
    ASSERT(targetObject);
    ASSERT(targetProperty);

    // Programmatically build the animation storyboard.
    //

    //
    // Create the objects involved.
    //
    IFC_RETURN(CreateDO(beginTime.ReleaseAndGetAddressOf(), &cpZeroTime));
    IFC_RETURN(CreateDO(storyboard.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(CreateDO(doubleAnimation.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(CDoubleKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(keyFramesCollection.ReleaseAndGetAddressOf()), &cp));

    // The timing curve settings have the timing in fractions of 1.0f,
    // where 1.0f represents a full OS blinking interval set by the OS
    const TextSelectionSettings *tss = TextSelectionSettings::Get();
    // Sanity check
    ASSERT(tss->m_rCaretAnimationUpPlateauFraction <= 1.0f);
    ASSERT(tss->m_rCaretAnimationUpPlateauFraction >= 0.0f);
    ASSERT(tss->m_rCaretAnimationRampDownFraction <= 1.0f);
    ASSERT(tss->m_rCaretAnimationRampDownFraction >= 0.0f);
    ASSERT(tss->m_rCaretAnimationDownPlateauFraction <= 1.0f);
    ASSERT(tss->m_rCaretAnimationDownPlateauFraction >= 0.0f);
    ASSERT(tss->m_rCaretAnimationRampUpFraction <= 1.0f);
    ASSERT(tss->m_rCaretAnimationRampUpFraction >= 0.0f);
    ASSERT(tss->m_rCaretAnimationUpPlateauFraction +
           tss->m_rCaretAnimationRampDownFraction +
           tss->m_rCaretAnimationDownPlateauFraction +
           tss->m_rCaretAnimationRampUpFraction <= 1.0f);

    //
    // Calculate the timings and append keyframes to a collection
    //
    beginTime->m_rTimeSpan = 0.0f;

    // Make the ramps durations 0, but add their original durations to the plateaus.

    // Switch to lower plateau value when the time for upper plateau and its ramp down ends.
    IFC_RETURN(AppendDoubleDiscreteKeyFrame(
        core,
        0.0f,                   // rValue
        (tss->m_rCaretAnimationUpPlateauFraction + tss->m_rCaretAnimationRampDownFraction) * blinkPeriodInSeconds, // rEndTime
        keyFramesCollection.get()));

    // Switch to upper plateau value once the lower plateau ends, which is the end of blinking interval.
    IFC_RETURN(AppendDoubleDiscreteKeyFrame(
        core,
        maxOpacity,             // rValue
        blinkPeriodInSeconds,   // rEndTime
        keyFramesCollection.get()));

    //
    // Hook the rest of the objects together
    //
    valueTemp.WrapObjectNoRef(keyFramesCollection.get());
    IFC_RETURN(doubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::DoubleAnimationUsingKeyFrames_KeyFrames, valueTemp));

    valueTemp.Set<valueVO>(RepeatBehaviorVOHelper::CreateForever(core).detach());
    IFC_RETURN(doubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_RepeatBehavior, valueTemp));

    valueTemp.WrapObjectNoRef(beginTime.get());
    IFC_RETURN(doubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_BeginTime, valueTemp));

    valueTemp.Set<valueVO>(DurationVOHelper::CreateTimeSpan(core, blinkPeriodInSeconds).detach());
    IFC_RETURN(doubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_Duration, valueTemp));

    // The Storyboard.TargetObject and TargetName properties are strings, which is resolved
    //  by the XAML parser to actual object and property.  We don't have the parser
    //  luxury here and have to set them directly.
    IFC_RETURN(doubleAnimation->SetTargetObject(targetObject));
    doubleAnimation->SetTargetProperty(targetProperty);
    doubleAnimation->m_enableDependentAnimation = true; // Enable dependent animation so that it works under CacheMode=BitmapCache

    IFC_RETURN(storyboard->AddChild(doubleAnimation));

    *resultStoryboard = storyboard.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Given a rectangle in local coordinates, and a RenderTransform
//      that changes it into screen coordinates, get a rectangle
//      with pixel-snapped coordinates.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetPixelSnappedRectangle(
    _In_ const CMILMatrix    *pRenderTransform,
    _In_ EditRectangleKind    rectKind,
    _In_ XFLOAT               strokeWidth,
    _Inout_ XRECTF           *pRect
    )
{
    // Check the applicable RenderTransform to see if it's practical to try to snap to pixels.
    if (TransformAllowsSnapToPixels(pRenderTransform))
    {
        IFC_RETURN(SnapRectToPixel(pRenderTransform, strokeWidth, rectKind, pRect));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Check to see if trying to snap to pixels is meaningful in
//      the presence of the given transform.
//------------------------------------------------------------------------
_Check_return_ bool CTextBoxHelpers::TransformAllowsSnapToPixels(
    _In_ const CMILMatrix *pTransform
    )
{
    // When M21 and M12 are both zero, it means zero or more of the following:
    //  * ScaleTransform
    //  * TranslateTransform
    //  * RotateTransform with 0 or 180 degrees rotation
    //  * SkewTransform with 0 or 180 degrees X/Y angles

    // When M11 and M22 are both zero, it means zero or more of the following:
    //  * ScaleTransform
    //  * TranslateTransform
    //  * RotateTransform with 90 or 270 degrees rotation
    //  * SkewTransform with 0 or 180 degrees X/Y angles

    // If it fails both checks, we have at least one of the following:
    //  * RotateTransform whose angle is not an exact multiple of 90 degrees
    //  * SkewTransform with at least one angle that's neither 0 nor 180 degrees
    //
    // And we will not try to snap to pixel when both checks fail.

    return ( (pTransform->GetM21() == 0 && pTransform->GetM12() == 0) ||
             (pTransform->GetM11() == 0 && pTransform->GetM22() == 0) );
}


//------------------------------------------------------------------------
//
//  Method:  CTextBoxHelpers::SnapRectToPixel
//
//  Summary:
//      Snaps a rectangle to screen coordinates according to its type,
//      selection rectangle, caret rectangle, or focus rectangle.
//
//        o  Snaps a selection background rectangle to whole screen pixel
//           edges by expanding the rectangle.
//
//        o  Snaps a selection background rectangle that is part of a multiple
//           rectangle selection by rounding each corner to the nearest pixel.
//
//        o  Snaps a caret rectangle to whole screen pixels edges by rounding
//           the origin and the extent.
//
//        o  Snaps a focus rectangle to whole screen pixels edges by rounding
//           the outer bounds of the rectangles stroke, with halves rounding
//           towards the interior of the rectangle.
//
//      Takes account of negative scale factors for x and/or y.
//
//      If the transform is not invertible, or the resulting rect covers less
//      than a whole screen pixel, the rect remains unchanged.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::SnapRectToPixel(
    _In_    const CMILMatrix  *pRenderTransform,
    _In_    XFLOAT             strokeWidth,
    _In_    EditRectangleKind  rectKind,
    _Inout_ XRECTF            *pRect
)
{
    CMILMatrix inverseTransform = *pRenderTransform;
    XFLOAT     halfStrokeWidth  = strokeWidth / 2.0f;
    XPOINTF    screenOrigin;      // May not be top left if transform inverts either axis
    XPOINTF    screenFarCorner;   // May not be bottom right if transform inverts either axis

    // Calculate the inverse transform.
    if (!inverseTransform.Invert())
    {
        // If the transform is not invertible then we won't be able to convert
        // the screen coordinates back to local coordinates, so we leave the
        // rectangle unchanged.
        return S_OK;
    }

    // Determine where outer bounds of the rectangle (including any stroke) end
    // up on the screen.
    screenOrigin.x = pRect->X - halfStrokeWidth;
    screenOrigin.y = pRect->Y - halfStrokeWidth;
    pRenderTransform->Transform(&screenOrigin, &screenOrigin, 1);
    screenFarCorner.x = pRect->X + pRect->Width + halfStrokeWidth;
    screenFarCorner.y = pRect->Y + pRect->Height + halfStrokeWidth;
    pRenderTransform->Transform(&screenFarCorner, &screenFarCorner, 1);

    switch (rectKind)
    {
    case SelectionRectangle:
        // Snap outwards, allowing for axis inversion.

        if (screenOrigin.x < screenFarCorner.x)
        {
            screenOrigin.x    = floor(screenOrigin.x);
            screenFarCorner.x = ceil(screenFarCorner.x);
        }
        else
        {
            screenOrigin.x    = ceil(screenOrigin.x);
            screenFarCorner.x = floor(screenFarCorner.x);
        }

        if (screenOrigin.y < screenFarCorner.y)
        {
            screenOrigin.y    = floor(screenOrigin.y);
            screenFarCorner.y = ceil(screenFarCorner.y);
        }
        else
        {
            screenOrigin.y    = ceil(screenOrigin.y);
            screenFarCorner.y = floor(screenFarCorner.y);
        }
        break;

    case MultiSelectionRectangle:
        screenOrigin.x    = floor(screenOrigin.x + 0.5f);
        screenOrigin.y    = floor(screenOrigin.y + 0.5f);
        screenFarCorner.x = floor(screenFarCorner.x + 0.5f);
        screenFarCorner.y = floor(screenFarCorner.y + 0.5f);
        break;

    case CaretRectangle:
        // Round the caret origin.
        screenOrigin.x = floor(screenOrigin.x + 0.5f);
        screenOrigin.y = floor(screenOrigin.y);

        // Round the caret width
        if (screenFarCorner.x >= screenOrigin.x)
        {
            screenFarCorner.x = screenOrigin.x + floor(screenFarCorner.x - screenOrigin.x + 0.5f);
        }
        else
        {
            screenFarCorner.x = screenOrigin.x - floor(screenOrigin.x - screenFarCorner.x + 0.5f);
        }
        if (screenFarCorner.y >= screenOrigin.y)
        {
            screenFarCorner.y = screenOrigin.y + floor(screenFarCorner.y - screenOrigin.y + 0.5f);
        }
        else
        {
            screenFarCorner.y = screenOrigin.y - floor(screenOrigin.y - screenFarCorner.y + 0.5f);
        }
        break;

    case FocusRectangle:
        // Round both the origin and the far corner of the stroke boundary.
        // Note that we need halves to round inwards.
        // We round haves up with the converse: floor(x+0.5f).
        // We round haves down with the converse: ceil(x-0.5f).

        if (screenOrigin.x < screenFarCorner.x)
        {
            // Origin is to left of far side
            screenOrigin.x    = floor(screenOrigin.x + 0.5f);
            screenFarCorner.x = ceil(screenFarCorner.x - 0.5f);
        }
        else
        {
            // Origin is to right of far side
            screenOrigin.x    = ceil(screenOrigin.x - 0.5f);
            screenFarCorner.x = floor(screenFarCorner.x + 0.5f);
        }

        if (screenOrigin.y < screenFarCorner.y)
        {
            // Origin is above far side
            screenOrigin.y    = floor(screenOrigin.y + 0.5f);
            screenFarCorner.y = ceil(screenFarCorner.y - 0.5f);
        }
        else
        {
            // Origin is below far side
            screenOrigin.y    = ceil(screenOrigin.y - 0.5f);
            screenFarCorner.y = floor(screenFarCorner.y + 0.5f);
        }
        break;
    default:
        ASSERT(FALSE); // assert to catch new enums being introduced without updating this switch
        break;
    }

    // So long as the result covers at least one screen pixel, convert back to
    // local coordinates in the output rectangle.
    if (    (    (screenFarCorner.x - screenOrigin.x >=  1.0f)
             ||  (screenFarCorner.x - screenOrigin.x <= -1.0f))
        &&  (    (screenFarCorner.y - screenOrigin.y >=  1.0f)
             ||  (screenFarCorner.y - screenOrigin.y <= -1.0f)))
    {
        inverseTransform.Transform(&screenOrigin, &screenOrigin, 1);
        pRect->X = screenOrigin.x  + halfStrokeWidth;
        pRect->Y = screenOrigin.y  + halfStrokeWidth;
        inverseTransform.Transform(&screenFarCorner, &screenFarCorner, 1);
        pRect->Width  = screenFarCorner.x - screenOrigin.x - strokeWidth;
        pRect->Height = screenFarCorner.y - screenOrigin.y - strokeWidth;
    }

    return S_OK; //RRETURN_REMOVAL
}

//------------------------------------------------------------------------
//  Summary:
//      Gets the inherited flow direction given the dependency object.
//
//      TODO: Support returning paragraph level flow direction in future.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetFlowDirection(
    _In_  CFrameworkElement       *pElement,
    _Out_ DirectUI::FlowDirection *pFlowDirection
    )
{
    IFCPTR_RETURN(pElement);
    IFCPTR_RETURN(pFlowDirection);

    // [TODO]
    // support retrieving flow direction from TextElement DO. [Bug 75411]
    *pFlowDirection = (pElement->IsRightToLeft() ? DirectUI::FlowDirection::RightToLeft : DirectUI::FlowDirection::LeftToRight);

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//      Creates a zero-duration, zero-time key frame Storyboard.  This is
//  used to temporarily "set" a value.  The original value is restored when
//  this Storyboard is stopped.
//
//      Caller is responsible for calling ReleaseInterface() on *ppStoryboard.
//------------------------------------------------------------------------
//
//  Conceptually similar to the following XAML:
//
//<Storyboard>
//    <ObjectAnimationUsingKeyFrames Storyboard.Target=[pTargetObject] Storyboard.TargetProperty=[pTargetProperty] Duration="0">
//        <DiscreteObjectKeyFrame KeyTime="0" Value=[pTargetValue] />
//    </ObjectAnimationUsingKeyFrames>
//</Storyboard>
_Check_return_ HRESULT CTextBoxHelpers::CreateZeroDurationStoryboard(
    _In_ CCoreServices *pCore,
    _In_ CDependencyObject *pTargetObject,
    _In_ const CDependencyProperty *pTargetProperty,
    _In_ CValue *pTargetValue,
    _Outptr_ CStoryboard **ppStoryboard)
{
    HRESULT                         hr = S_OK;
    CStoryboard                    *pStoryboard = nullptr;
    CObjectAnimationUsingKeyFrames *pKeyFrameAnimation = nullptr;
    CDiscreteObjectKeyFrame        *pKeyFrame = nullptr;

    CValue           valueTemp;
    CValue           valueZero;
                     valueZero.SetFloat(0.0f);
    CREATEPARAMETERS cpZero(pCore, valueZero);
    CREATEPARAMETERS cp(pCore);

    // pStoryboard = new Storyboard
    IFC(CreateDO(&pStoryboard, &cp));

    // pKeyFrame = new DiscreteObjectKeyFrame
    IFC(CreateDO(&pKeyFrame, &cp));

    // pKeyFrame.KeyTime = 0
    valueTemp.Set<valueVO>(KeyTimeVOHelper::CreateTimeSpan(pCore, 0.0).detach());
    IFC(pKeyFrame->SetValue(pKeyFrame->GetPropertyByIndexInline(KnownPropertyIndex::ObjectKeyFrame_KeyTime), valueTemp));

    IFC(pKeyFrame->SetValue(
        pKeyFrame->GetPropertyByIndexInline(KnownPropertyIndex::ObjectKeyFrame_Value), *pTargetValue));

    // pKeyFrameAnimation = new ObjectAnimationUsingKeyFrames
    IFC(CreateDO(&pKeyFrameAnimation, &cp));

    // pKeyFrameAnimation.Children.Add(pKeyFrame)  (Uses parser code path to automatically add children to content properties.)
    valueTemp.WrapObjectNoRef(pKeyFrame);
    IFC(pKeyFrameAnimation->SetValueByKnownIndex(KnownPropertyIndex::ObjectAnimationUsingKeyFrames_KeyFrames, valueTemp));
    ReleaseInterface(pKeyFrame);

    // pKeyFrameAnimation.Duration = 0;
    valueTemp.Set<valueVO>(DurationVOHelper::CreateTimeSpan(pCore, 0.0).detach());
    IFC(pKeyFrameAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_Duration, valueTemp));

    // pKeyFrameAnimation.TargetProperty = pTargetProperty
    IFC(pKeyFrameAnimation->SetTargetObject(pTargetObject));
    pKeyFrameAnimation->SetTargetProperty(pTargetProperty);

    // pStoryboard.Children.Add(pKeyFrameAnimation)
    valueTemp.WrapObjectNoRef(pKeyFrameAnimation);
    IFC(pStoryboard->SetValueByKnownIndex(KnownPropertyIndex::Storyboard_Children, valueTemp));
    ReleaseInterface(pKeyFrameAnimation);

    *ppStoryboard = pStoryboard;
    pStoryboard = nullptr;

Cleanup:
    ReleaseInterface(pKeyFrame);
    ReleaseInterface(pKeyFrameAnimation);
    ReleaseInterface(pStoryboard);
    return hr;
}

// Given a Storyboard reference created by CreateZeroDurationStoryboard()
// (Or at least one with identical structure) update the parameters that were
// passed in.
_Check_return_ HRESULT CTextBoxHelpers::UpdateZeroDurationStoryboard(
    _In_ CStoryboard *pStoryboard,
    _In_opt_ CDependencyObject *pTargetObject,
    _In_opt_ const CDependencyProperty *pTargetProperty,
    _In_opt_ CValue *pTargetValue)
{
    HRESULT hr = S_OK;
    CDependencyObject *pTempObject = nullptr;
    CObjectAnimationUsingKeyFrames *pKeyFrameAnimation = nullptr;
    CDiscreteObjectKeyFrame        *pKeyFrame = nullptr;

    IFC(pStoryboard->StopPrivate());

    pTempObject = static_cast<CDependencyObject*>(pStoryboard->m_pChild->GetItemWithAddRef(0));
    IFC(DoPointerCast(pKeyFrameAnimation, pTempObject));
    pTempObject = static_cast<CDependencyObject*>(pKeyFrameAnimation->m_pKeyFrames->GetItemWithAddRef(0));
    IFC(DoPointerCast(pKeyFrame, pTempObject));
    pTempObject = nullptr;

    if(pTargetObject)
    {
        IFC(pKeyFrameAnimation->SetTargetObject(pTargetObject));
    }
    if(pTargetProperty)
    {
        pKeyFrameAnimation->SetTargetProperty(pTargetProperty);
    }
    if(pTargetValue)
    {
        IFC(pKeyFrame->SetValue(
            pKeyFrame->GetPropertyByIndexInline(KnownPropertyIndex::ObjectKeyFrame_Value), *pTargetValue));
    }

    IFC(pStoryboard->BeginPrivate(TRUE /* = Is a top level Storyboard */));
Cleanup:
    ReleaseInterface(pTempObject);
    ReleaseInterface(pKeyFrame);
    ReleaseInterface(pKeyFrameAnimation);
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Get TextFormatter instance from the TextFormatter Cache pool.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextFormatterFactory::GetTextFormatter(
    _In_ CCoreServices *pCore,
    _Outptr_ TextFormatter **ppTextFormatter
    )
{
    HRESULT hr = S_OK;
    TextFormatterCache *pTextFormatterCache = nullptr;

    IFC(GetTextFormatterCache(pCore, &pTextFormatterCache));

    IFC(RichTextServicesHelper::MapTxErr(pTextFormatterCache->AcquireTextFormatter(ppTextFormatter)));

Cleanup:
    return hr;
}

//------------------------------------------------------------------------
//  Summary:
//      Release TextFormatter instance back into the
//      TextFormatter Cache pool.
//------------------------------------------------------------------------
void CTextFormatterFactory::ReleaseTextFormatter(
    _In_ CCoreServices *pCore,
    _In_opt_ TextFormatter *pTextFormatter
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    TextFormatterCache *pTextFormatterCache = nullptr;

    IFC(GetTextFormatterCache(pCore, &pTextFormatterCache));

    pTextFormatterCache->ReleaseTextFormatter(pTextFormatter);

Cleanup:
    ;
}

//------------------------------------------------------------------------
//  Summary:
//      Retreives the TextFormatterCache from CCoreServices.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextFormatterFactory::GetTextFormatterCache(
    _In_ CCoreServices *pCore,
    _Outptr_ TextFormatterCache **ppTextFormatterCache
    )
{
    CTextCore *pTextCore = nullptr;

    IFC_RETURN(pCore->GetTextCore(&pTextCore));
    IFCPTR_RETURN(pTextCore);

    IFC_RETURN(pTextCore->GetTextFormatterCache(ppTextFormatterCache));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Translates from DirectUI::TextAlignment to RichTextServices::TextAlignment.
//
//---------------------------------------------------------------------------
RichTextServices::TextAlignment::Enum CTextBoxHelpers::GetRichTextServicesTextAlignment(
    _In_ DirectUI::TextAlignment alignment
    )
{
    RichTextServices::TextAlignment::Enum rtsAlignment = RichTextServices::TextAlignment::Left;

    switch (alignment)
    {
        case DirectUI::TextAlignment::Left:
            rtsAlignment = RichTextServices::TextAlignment::Left;
            break;

        case DirectUI::TextAlignment::Right:
            rtsAlignment = RichTextServices::TextAlignment::Right;
            break;

        case DirectUI::TextAlignment::Center:
            rtsAlignment = RichTextServices::TextAlignment::Center;
            break;

        case DirectUI::TextAlignment::Justify:
            rtsAlignment = RichTextServices::TextAlignment::Justify;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    return rtsAlignment;
}

//------------------------------------------------------------------------
//  Summary:
//      Checks whether the CPlainTextPosition is in the middle of a surrogate
//  pair or in the middle of a Carriage Return + Line Feed combination.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::IsNotInSurrogateCRLF(
    _In_  ITextContainer *pContainer,
    _In_  XUINT32         offset,
    _Out_ bool          *pIsNotInSurrogateCRLF)
{
    XUINT32     numCharacters         = 0;
    const WCHAR *pCharacters          = nullptr;
    bool isNotInSurrogateCRLF = true; // Assume it's TRUE initially
    XUINT32 numPositions = 0;

    IFCEXPECT_RETURN(pContainer != nullptr);
    pContainer->GetPositionCount(&numPositions);
    IFCEXPECT_RETURN(offset <= numPositions);

    IFC_RETURN(pContainer->GetRun(offset, nullptr, nullptr, nullptr, nullptr, &pCharacters, &numCharacters));
    if (numCharacters && pCharacters != nullptr)
    {
        if (IS_TRAILING_SURROGATE(pCharacters[0]))
        {
            isNotInSurrogateCRLF = FALSE;
        }
        // See if we are in the middle of a CR+LF sequence.
        else if (pCharacters[0] == UNICODE_LINE_FEED && offset > 0)
        {
            // There's a LF here.  Go back one character and look for CR.
            const WCHAR *pLookForCR = nullptr;
            XUINT32 cLookForCR = 0;

            IFC_RETURN(pContainer->GetRun(offset - 1, nullptr, nullptr, nullptr, nullptr, &pLookForCR, &cLookForCR));
            if(cLookForCR && pLookForCR != nullptr && pLookForCR[0] == UNICODE_CARRIAGE_RETURN)
            {
                // We are in the middle of a CR+LF sequence.
                isNotInSurrogateCRLF = FALSE;
            }
        }
    }

    *pIsNotInSurrogateCRLF = isNotInSurrogateCRLF;

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//  Verifies that 2 text positions are proper. Position1 should
//  not be logical greater than Position2 and both positions should not
//  exceed the length of size container.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::VerifyPositionPair(
        _In_ ITextContainer *pTextContainer,
        _In_ XUINT32 iTextPosition1,
        _In_ XUINT32 iTextPosition2
    )
{
    const XUINT32 cPositions = GetMaxTextPosition(pTextContainer);

    // Verify that their offset is within the range of the container.
    IFCEXPECT_RETURN(iTextPosition1 <= cPositions);
    IFCEXPECT_RETURN(iTextPosition2 <= cPositions);

    // Verify that the positions are ordered.
    IFCEXPECT_RETURN(iTextPosition1 <= iTextPosition2);

    return S_OK;
}

//------------------------------------------------------------------------
//  Summary:
//  Returns the maximum valid text position taking into account the 
//  special empty RichTextBlock case.
//------------------------------------------------------------------------
XUINT32 CTextBoxHelpers::GetMaxTextPosition(
    _In_ ITextContainer* pTextContainer)
{
    XUINT32 cPositions = 0;

    pTextContainer->GetPositionCount(&cPositions);

    // Empty RichTextBlocks actually have one position, even when empty, as the ContentEnd
    // position is still there. It is always the position just after the end of content.
    if (cPositions == 0 &&
        pTextContainer->GetOwnerUIElement() != nullptr &&
        pTextContainer->GetOwnerUIElement()->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        cPositions = 1;
    }

    return cPositions;
}

//------------------------------------------------------------------------
//  Summary:
//  Retrieves the text between 2 given positions from a text container.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetText(
        _In_ ITextContainer *pTextContainer,
        _In_ XUINT32         charPosition1,
        _In_ XUINT32         charPosition2,
        _In_ bool           insertNewlines,
        _Out_ CString      **ppString)
{
    xref_ptr<CString> text;
    XUINT32 characterPosition  = charPosition1;
    XUINT32 length             = 0;
    XUINT32 textLength         = 0;
    XUINT32 runTextLength      = 0;
    const WCHAR *pRunCharacters = nullptr;
    XUINT32 newLineLength      = 2;
    const WCHAR *pNewline      = L"\r\n";
    bool replaceWithNewLine = false;
    XStringBuilder textBuilder;

    // Allocate a string of 0 length.
    CREATEPARAMETERS createParameters(pTextContainer->GetCore());
    IFC_RETURN(CreateDO(text.ReleaseAndGetAddressOf(), &createParameters));

    IFC_RETURN(VerifyPositionPair(pTextContainer, charPosition1, charPosition2));

    // Walk the text content once to determine how many characters there are.
    pTextContainer->GetPositionCount(&length);

    // The first iteration counts the text length to allocate the string builder.
    // The second iteration then fills the string builder.
    for (int i = 0; i < 2; i++)
    {
        while (characterPosition < length &&
               characterPosition < charPosition2)
        {
            runTextLength = 0;
            pRunCharacters = nullptr;
            replaceWithNewLine = FALSE;

            if (insertNewlines)
            {
                TextNestingType nestingType;
                CTextElement    *pNestedElement;
                IFC_RETURN(pTextContainer->GetRun(characterPosition, nullptr, nullptr, &nestingType, &pNestedElement, &pRunCharacters, &runTextLength));

                if (pNestedElement != nullptr
                 && ((nestingType == CloseNesting  && pNestedElement->GetTypeIndex() == KnownTypeIndex::Paragraph)
                 || (nestingType == NestedContent && pNestedElement->GetTypeIndex() == KnownTypeIndex::LineBreak)))
                {
                    replaceWithNewLine = TRUE;
                }
            }
            else
            {
                IFC_RETURN(pTextContainer->GetRun(characterPosition, nullptr, nullptr, nullptr, nullptr, &pRunCharacters, &runTextLength));
            }

            // Count up real text characters, exclude reserved positions
            if (pRunCharacters != nullptr || replaceWithNewLine)
            {
                // Append these characters to the CString
                if(characterPosition + runTextLength > charPosition2)
                {
                    runTextLength = charPosition2 - characterPosition;
                }

                XUINT32 charsCount = (replaceWithNewLine) ? newLineLength : runTextLength;
                const WCHAR *pChars = (replaceWithNewLine) ? pNewline      : pRunCharacters;
                if (i == 0)
                {
                    textLength += charsCount;
                }
                else
                {
                    IFC_RETURN(textBuilder.Append(pChars, charsCount));
                }
            }

            ASSERT(runTextLength > 0);
            characterPosition += runTextLength;
        }

        if (i == 0)
        {
            // Walk the text again and concatenate the text into a string.
            // Allocate a string of exactly the right size.
            IFC_RETURN(textBuilder.Initialize(textLength));
        }

        characterPosition = charPosition1;
    }

    // No need to delete m_strString because we just created it empty.
    IFC_RETURN(textBuilder.DetachString(&(text->m_strString)));
    *ppString = text.detach();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  SelectWord
//
//  Synopsis:
//
//      Given a coordinate position, select the word at that position.  Called
//  in response to double-clicking in text editor area.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::SelectWordFromTextPosition(
    _In_  ITextContainer *pTextContainer,
    _In_  CTextPosition   hitPosition,
    _In_  IJupiterTextSelection *pTextSelection,
    _In_  FindBoundaryType forwardFindBoundaryType,
    _In_  TagConversion tagConversion
    )
{
    TextGravity         eHitGravity            = LineForwardCharacterForward;
    CTextPosition       wordStartTextPosition;
    CTextPosition       wordEndTextPosition;
    bool isValidPosition = true;

    ASSERT(forwardFindBoundaryType != FindBoundaryType::Backward);

    IFC_RETURN(GetAdjacentWordSelectionBoundaryPosition(
        pTextContainer,
        hitPosition,
        forwardFindBoundaryType,
        tagConversion,
       &wordEndTextPosition,
       &eHitGravity
    ));

    // We will start the search for the start word boundary one position before the word end
    // (which is the last character of the word we're going to select),
    // as GetAdjacentWordSelectionBoundaryPosition will not move backwards if on a break
    // Since we start at the last character of the to-be-selected word, it will find the appropriate break backwards

    // The reason why we do this and not use hitPosition directly is that, in BLUE:89619, when double-clicking in the latter half of the
    // last character, the hitPosition will be in the not visible </run> tag, and it will be the word break backwards
    // By instead moving to the previous insertion position of wordEndTextPosition, we land in the character (because it skips the invisible tag)
    // and we find the right break backwards

    // wordEndTextPosition should be greater than hitPosition unless hitPosition is really the last position
    // in the block. So its previous insertion position will surely be in a position before, unless the block is totally empty
    // (in which case we don't want to select anything)
    IFC_RETURN(wordEndTextPosition.GetPreviousInsertionPosition(&isValidPosition, &hitPosition));
    if (isValidPosition)
    {
        IFC_RETURN(GetAdjacentWordSelectionBoundaryPosition(
            pTextContainer,
            hitPosition,
            FindBoundaryType::Backward,
            tagConversion,
           &wordStartTextPosition,
            nullptr
        ));

        IFC_RETURN(pTextSelection->Select(
            wordStartTextPosition, // Anchor position
            wordEndTextPosition,   // Moving position
            eHitGravity
        ));
    }

    return S_OK;
}

//------------------------------------------------------------------------

//  Summary:
//      Simple wrapper class to wrap an ITextContainer according to the ISimpleTextBackend interface
//------------------------------------------------------------------------
class CTextContainerWrapper : public ISimpleTextBackend
{
public:
    CTextContainerWrapper(_In_ ITextContainer* pTextContainer, _In_ TagConversion tagConversion)
        : m_pTextContainerNoRef(pTextContainer)
        , m_tagConversion(tagConversion)
    {}

//------------------------------------------------------------------------
//  Summary:
//      Helper to retrieve the character pointed to by the
//      offset from the backing store.
//
//      NOTE: This always returns a single unicode character.
//            Bug 73241: This does not handle Unicode characters larger
//            than 16 bits correctly.
//            See CTextLineRider::GetCharacter for a reference implementation
//            which returns 32-bit characters.
//------------------------------------------------------------------------
    WCHAR GetCharacter(_In_ XUINT32 offset) override
    {
        ASSERT(m_pTextContainerNoRef != nullptr);

        HRESULT  hr = S_OK; // WARNING_IGNORES_FAILURES
        const WCHAR *pChar = nullptr;
        XUINT32  cChar = 0;
        bool takeOwnership = false;
        WCHAR    character = UNICODE_NEXT_LINE;

        IFC(m_pTextContainerNoRef->GetText(offset, offset+1, FALSE /*insertNewlines*/, &cChar, &pChar, &takeOwnership));
        if ((cChar == 1) && (pChar != nullptr))
        {
            character = *pChar;
        }
        else if (m_tagConversion != TagConversion::None)// This means we're in the border (open/close) of a tag
        {
            if (takeOwnership)
            {
                delete[] pChar;
                pChar = nullptr;
            }
            takeOwnership = TRUE;
            CTextElement* borderElement;
            TextNestingType nestingType;
            IFC(m_pTextContainerNoRef->GetRun(offset, nullptr, nullptr, &nestingType, &borderElement, &pChar, &cChar));
            if (borderElement == nullptr)
            {
                // On success, GetRun shouldn't return a null borderElement, but sometimes it does
                // The only elements that may do that are CBlockCollection and CInlineCollection
                // Considering it as a paragraph break seems the best fallback here
                character = UNICODE_PARAGRAPH_SEPARATOR;
            }
            else
            {
                switch (borderElement->GetTypeIndex())
                {
                case KnownTypeIndex::Paragraph:
                    character = UNICODE_PARAGRAPH_SEPARATOR;
                    break;
                case KnownTypeIndex::LineBreak:
                    character = UNICODE_LINE_SEPARATOR;
                    break;
                case KnownTypeIndex::Run:
                    character = UNICODE_SPACE;
                    break;
                case KnownTypeIndex::InlineUIContainer:
                    character = (nestingType == OpenNesting) ? UNICODE_NEXT_LINE : UNICODE_SPACE;
                    break;
                default:
                    character = UNICODE_SPACE;
                    break;
                }
            }
        }

    Cleanup:
        if (takeOwnership)
        {
            delete[] pChar;
        }

        return character;
    }


private:
    ITextContainer* m_pTextContainerNoRef;
    const TagConversion m_tagConversion;
};

//------------------------------------------------------------------------
//
//  Method:   GetClosestNonWhitespaceWordBoundary
//
//  Synopsis:
//      Given a text position, finds the closest non-whitespace word boundary
//  (ties favor the left boundary)
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetClosestNonWhitespaceWordBoundary(
        _In_  ITextContainer *pTextContainer,
        _In_  CTextPosition   hitPosition,
        _In_  TagConversion   tagConversion,
        _Out_ CTextPosition  *pClosestPosition)
{
    CTextPosition leftNonWhitespace;
    CTextPosition rightNonWhitespace;
    uint32_t hitOffset;
    uint32_t leftOffset;
    uint32_t rightOffset;
    CTextContainerWrapper backend(pTextContainer, tagConversion);

    IFC_RETURN(CSelectionWordBreaker::GetAdjacentNonWhitespaceCharacter(hitPosition, &backend, FindBoundaryType::Backward, &leftNonWhitespace));
    IFC_RETURN(CSelectionWordBreaker::GetAdjacentNonWhitespaceCharacter(hitPosition, &backend, FindBoundaryType::ForwardExact, &rightNonWhitespace));

    IFC_RETURN(hitPosition.GetOffset(&hitOffset));
    IFC_RETURN(leftNonWhitespace.GetOffset(&leftOffset));
    IFC_RETURN(rightNonWhitespace.GetOffset(&rightOffset));

    ASSERT(leftOffset <= hitOffset);
    ASSERT(hitOffset <= rightOffset);

    *pClosestPosition = ((hitOffset - leftOffset) <= (rightOffset - hitOffset)) ? leftNonWhitespace : rightNonWhitespace;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAdjacentWordBoundaryPosition
//
//  Synopsis:
//      Given a text position and desired direction, find the next word boundary
//  position using the given function, in the specified direction and determine text gravity.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBoxHelpers::GetAdjacentWordBoundaryPosition(
        _In_opt_  ITextContainer   *pTextContainer,
        _In_      CTextPosition     textPosition,
        _In_      FindBoundaryType  findType,
        _In_      TagConversion     tagConversion,
        _In_      HRESULT (*GetAdjacentBoundaryPosition)(CTextPosition,
                                                         ISimpleTextBackend*,
                                                         FindBoundaryType,
                                                         CTextPosition*),
        _Out_     CTextPosition    *pAdjacentPosition,
        _Out_opt_ TextGravity      *peAdjacentGravity)
{
    CTextContainerWrapper backend(pTextContainer, tagConversion);

    IFC_RETURN(GetAdjacentBoundaryPosition(textPosition, &backend, findType, pAdjacentPosition));

    if (peAdjacentGravity)
    {
        // The gravity is normally the opposite of the direction
        *peAdjacentGravity = IsForwardDirection(findType) ? LineForwardCharacterBackward : LineBackwardCharacterForward;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetAdjacentWordNavigationBoundaryPosition
//
//  Synopsis:
//      Given a text position and desired direction, find the next word boundary
//  position for navigation in the specified direction and determine text gravity.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetAdjacentWordNavigationBoundaryPosition(
    _In_opt_  ITextContainer      *pTextContainer,
    _In_      CTextPosition        textPosition,
    _In_      FindBoundaryType     findType,
    _In_      TagConversion        tagConversion,
    _Out_     CTextPosition       *pAdjacentPosition,
    _Out_opt_ TextGravity         *peAdjacentGravity
)
{
    return GetAdjacentWordBoundaryPosition(pTextContainer,
        textPosition,
        findType,
        tagConversion,
        &CSelectionWordBreaker::GetAdjacentWordNavigationBoundary,
        pAdjacentPosition,
        peAdjacentGravity);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Given a text position and desired direction, find the next word boundary
//  position in the specified direction and determine text gravity.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxHelpers::GetAdjacentWordSelectionBoundaryPosition(
    _In_opt_  ITextContainer      *pTextContainer,
    _In_      CTextPosition        textPosition,
    _In_      FindBoundaryType     findType,
    _In_      TagConversion        tagConversion,
    _Out_     CTextPosition       *pAdjacentPosition,
    _Out_opt_ TextGravity         *peAdjacentGravity
)
{
    return GetAdjacentWordBoundaryPosition(pTextContainer,
        textPosition,
        findType,
        tagConversion,
        &CSelectionWordBreaker::GetAdjacentWordSelectionBoundary,
        pAdjacentPosition,
        peAdjacentGravity);
}

