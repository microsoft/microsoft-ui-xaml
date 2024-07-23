// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "Gripper.h"
#include "TextSelectionManager.h"
#include "TextSelectionSettings.h"

#include "Storyboard.h"
#include "DoubleAnimation.h"
#include "DoubleAnimationUsingKeyFrames.h"
#include "DoubleKeyFrame.h"
#include "KeyTime.h"
#include "KeySpline.h"
#include "TimeSpan.h"
#include "Duration.h"
#include <FrameworkTheming.h>
#include "RichEditGripperChild.h"

// Sentinel values to trigger the create initialization of the pole parameters
static const XFLOAT POLE_HEIGHT_INITIALIZATION_NEEDED = -1.0f;
static const XINT32 OUTLINE_COLOR_INITIALIZATION_NEEDED = 0x0; // Transparent
/* static */ const XFLOAT CTextSelectionGripper::POLE_HEIGHT_UNCHANGED = -1.0f;

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::Create
//
//  Synopsis: Bare DO initialization. Most of the initialization is done in
//  InitializeGripper method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate)
{
    HRESULT hr = S_OK;
    CTextSelectionGripper *pObj = NULL;
    if (pCreate->m_value.GetType() == valueString)
    {
        IFC(E_NOTIMPL);
    }
    else
    {
        pObj = new CTextSelectionGripper(pCreate->m_pCore);
        IFC(ValidateAndInit(pObj, ppObject));
        *ppObject = pObj;
        pObj = NULL;
    }

Cleanup:
    ReleaseInterface(pObj); // Follow the standard pattern
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::InitializeGripper
//
//  Synopsis: Initializes a new instance of CTextSelectionGripper
//
//  The gripper is a Canvas object containing three shapes: two Ellipses and
//  one Rectangle. The objects are owned and ref-counted by the Canvas only,
//  but their weak references are saved in the member variables for the
//  further runtime adjustments:
//
//   m_pEllipseExternalNoAddRef -- external white ellipse that also provides
//                                 the background color.
//                                 This is marked as "A" on the ASCII art below.
//
//   m_pEllipseInternalNoAddRef -- smaller internal ellipse that has dark outline.
//                                 it is centered together with the external
//                                 ellipse.
//                                 This is marked as "B" on the ASCII art below.
//
//   m_pRectanglePoleNoAddRef   -- pole that sticks out of the internal ellipse
//                                 upwards, forming an inverted lollipop shape.
//                                 This is marked as "C" on the ASCII art below.
//             CC
//             CC                  Note that the area inside the A contour and
//             CC                  inside the B contour have the color of A.
//             CC
//             CC
//        AAAAACCAAAAA
//       A     CC     A
//      A   BBBCCBBB   A
//      A  B        B  A
//      A  B        B  A
//      A   BBBBBBBB   A
//       A            A
//        AAAAAAAAAAAA
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::InitializeGripper(
        _In_ TextSelectionManager *pTextSelectionManager
        )
{
    HRESULT                hr   = S_OK;
    CBrush                *pGripperFillBrush = NULL;
    CBrush                *pTransparentBrush = NULL;
    CEllipse              *pEllipseInternal = NULL;
    CEllipse              *pEllipseExternal = NULL;
    CRectangle            *pRectangle = NULL;
    CDependencyObject     *pdo = NULL;
    CStoryboard           *pShowAnimation = NULL;
    CStoryboard           *pHideAnimation = NULL;
    auto                   core = GetContext();
    CREATEPARAMETERS       cp(core);
    CValue                 v;
    v.SetColor(0x0);       // Transparent color
    CREATEPARAMETERS       cpTransparentColor(core, v);

    m_pTextSelectionManager = pTextSelectionManager;
    IFC(core->GetTextSelectionGripperBrush(&pGripperFillBrush));

    //
    // Transparent brush
    //
    IFC(CSolidColorBrush::Create(&pdo, &cpTransparentColor));
    IFC(DoPointerCast(pTransparentBrush, pdo));
    pdo = NULL;

    //
    // Internal ellipse
    //
    IFC(CEllipse::Create(&pdo, &cp));
    IFC(DoPointerCast(pEllipseInternal, pdo));
    pdo = NULL;

    v.SetFloat(m_rInternalDiameter);
    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat(m_rInternalDiameter);
    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperFillBrush));

    v.SetFloat(m_rStrokeThickness);
    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, v));

    //
    // External ellipse
    //
    IFC(CEllipse::Create(&pdo, &cp));
    IFC(DoPointerCast(pEllipseExternal, pdo));
    pdo = NULL;

    v.SetFloat(m_rPaddingDiameter);
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat(m_rPaddingDiameter);
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperFillBrush));
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pGripperFillBrush));

    v.SetFloat(0.0f);
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, v));

    //
    // Rectangle (pole)
    //
    IFC(CRectangle::Create(&pdo, &cp));
    IFC(DoPointerCast(pRectangle, pdo));
    pdo = NULL;

    v.SetFloat(m_rPoleThickness);
    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperFillBrush));
    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pTransparentBrush));

    //
    // Canvas
    //
    v.SetFloat(m_rPaddingDiameter);
    IFC(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    //
    // Put them together into the canvas
    //
    IFC(AddChild(pEllipseInternal));
    IFC(AddChild(pEllipseExternal));
    IFC(AddChild(pRectangle));

    //
    // Position for external circle inside the canvas
    //
    v.SetFloat(0.0f);
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetSigned(1);
    IFC(pEllipseExternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Position for internal circle inside the canvas
    //
    v.SetFloat((m_rPaddingDiameter - m_rInternalDiameter) / 2.0f);
    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetSigned(2);
    IFC(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Position for pole inside the canvas
    //
    v.SetFloat(m_rPaddingDiameter / 2.0f - m_rPoleThickness / 2.0f);
    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetFloat(0.0f);
    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    v.SetSigned(3);
    IFC(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Save all shapes for further adjustments. We do this now, because
    // CheckAndRepositionShapesForPoleHeightChange() and CheckAndAdjustStrokeColor()
    // will use them.
    //
    m_pEllipseInternalNoAddRef = pEllipseInternal;
    m_pEllipseExternalNoAddRef = pEllipseExternal;
    m_pRectanglePoleNoAddRef   = pRectangle;

    //
    // Set all other initial coordinates, sizes and colors that depend on context
    //
    IFC(CheckAndRepositionShapesForPoleHeightChange(m_rInternalDiameter));
    IFC(CheckAndAdjustStrokeColor());

    //
    // Create fade animations
    //
    IFC(CreateFadeStoryboard(&pShowAnimation, core, this, GetPropertyByIndexInline(KnownPropertyIndex::UIElement_Opacity), 1.0f, TRUE /* fFadeIn */));
    IFC(CreateFadeStoryboard(&pHideAnimation, core, this, GetPropertyByIndexInline(KnownPropertyIndex::UIElement_Opacity), 1.0f, FALSE /* fFadeIn */));

    //
    // Create the Safety Zone's input processor.
    //
    m_pInputProcessor = IXcpHandleInputProcessor::s_CreateInstance();
    m_pInputProcessor->SetHiMetricsPerPixel(HiMetricsPerPixel());
    //
    // Wrap handler into a CValue and assign it to the animation complete event
    //
    v.SetInternalHandler(OnHideAnimationComplete);
    IFC(pHideAnimation->AddEventListener(EventHandle(KnownEventIndex::Timeline_Completed), &v, EVENTLISTENER_INTERNAL, NULL, FALSE));

    v.SetInternalHandler(OnShowAnimationComplete);
    IFC(pShowAnimation->AddEventListener(EventHandle(KnownEventIndex::Timeline_Completed), &v, EVENTLISTENER_INTERNAL, NULL, FALSE));

    SetVisibility(DirectUI::Visibility::Collapsed);

    m_pShowAnimation = pShowAnimation;
    pShowAnimation = NULL;
    m_pHideAnimation = pHideAnimation;
    pHideAnimation = NULL;

Cleanup:
    ReleaseInterface(pGripperFillBrush);
    ReleaseInterface(pTransparentBrush);
    ReleaseInterface(pEllipseInternal);
    ReleaseInterface(pEllipseExternal);
    ReleaseInterface(pRectangle);
    ReleaseInterface(pShowAnimation);
    ReleaseInterface(pHideAnimation);
    ReleaseInterface(pdo);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::SetContentDirtyFlag
//
//  Synopsis: sets the NW content dirty flag on this element and all of its
//  children
//
//------------------------------------------------------------------------
void CTextSelectionGripper::SetContentDirty(_In_ DirtyFlags flag)
{
    CUIElement::NWSetContentDirty(this, flag);
    CUIElement::NWSetContentDirty(m_pEllipseExternalNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pEllipseInternalNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pRectanglePoleNoAddRef, flag);
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::CheckAndRepositionShapesForPoleHeightChange
//
//  Synopsis: update all sizes and positions that depend on the m_rPoleHeight.
//  Also does the initial positioning for Create
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::CheckAndRepositionShapesForPoleHeightChange(
    _In_ XFLOAT rPoleHeight
    )
{
    HRESULT hr = S_OK;
    CBrush                 *pGripperStrokeBrush = NULL;
    CDependencyObject      *pdo                 = NULL;
    CValue v;

    ASSERT(rPoleHeight >= 0.0f);
    if (m_rPoleHeight == POLE_HEIGHT_INITIALIZATION_NEEDED)
    {
        // Make sure we do the first initialization if it was not done yet
        m_rPoleHeight = 0.0f;
        rPoleHeight = m_rInternalDiameter / 2.0f;
    }

    if (m_rPoleHeight != rPoleHeight)
    {
        //
        // Store the height for further use in other and as a cached value to avoid excessive resets in this method
        //
        m_rPoleHeight = rPoleHeight;

        //
        // Pole size
        //
        if (m_rPoleHeight != 0.0f)
        {
            // We allow one border thickness to flow into the central circle's border
            // to create a solid joint.
            v.SetFloat(m_rPoleHeight + (m_rPaddingDiameter - m_rInternalDiameter) / 4.0f + m_rPoleThickness);
        }
        else
        {
            // Disable drawing of pole across the outer padding and the offset
            // when the requested height is zero. This fully hides the pole.
            v.SetFloat(0.0f);
        }

        IFC(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

        //
        // Canvas size
        //
        v.SetFloat(m_rPaddingDiameter + m_rPoleHeight);
        IFC(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

        //
        // Offset of the external ellipse
        //
        v.SetFloat(m_rPoleHeight);
        IFC(m_pEllipseExternalNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

        //
        // Offset of the internal ellipse
        //
        v.SetFloat(m_rPoleHeight + (m_rPaddingDiameter - m_rInternalDiameter) / 2.0f);
        IFC(m_pEllipseInternalNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));
    }

Cleanup:
    ReleaseInterface(pGripperStrokeBrush);
    ReleaseInterface(pdo);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::CheckAndAdjustStrokeColor
//
//  Synopsis: update all the color of the gripper based on the selection
//  color and accessibility mode.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::CheckAndAdjustStrokeColor()
{
    HRESULT hr = S_OK;
    CBrush                 *pGripperStrokeBrush = NULL;
    CDependencyObject      *pdo                 = NULL;
    CValue                  v;
    XINT32                  strokeColor;
    auto                    core = GetContext();

    //
    // Determine the stroke color
    //
    if (!core->GetFrameworkTheming()->HasHighContrastTheme())
    {
        // Pull the color from selection manager, stripping the opacity value
        //
        // TODO (https://task.ms/win81/88933): consider overriding/altering the color to make it more
        // contrast on a white background in case if it is too close to white.

        // TODO: this conditional is a temporary workaround - soon we'll be able to avoid using this
        // TODO: code on phone since we'll just render a visual asset (PS#82656).
        if (nullptr == m_pTextSelectionManager)
        {
            strokeColor = 0xFF0000FF; // Blue
        }
        else
        {
            strokeColor = m_pTextSelectionManager->GetSelectionBackgroundBrush()->m_rgb | 0xFF000000;
        }
    }
    else
    {
        // In high contrast mode, TextSelectionManager does not use the value
        // returned by GetSelectionBackgroundBrush(). Instead it queries the
        // context. Do the same.
        CSolidColorBrush *pBrush = NULL;
        IFC(core->GetSystemTextSelectionBackgroundBrush(&pBrush));
        strokeColor = pBrush->m_rgb | 0xFF000000;
        ReleaseInterface(pBrush);
    }

    //
    // Re-set the color of the stroke if it does not match the cached value or
    // if we are setting the color for the first time (no selection manager yet).
    //
    // If the m_StrokeColor is OUTLINE_COLOR_INITIALIZATION_NEEDED, it can never
    // match the strokeColor value because OUTLINE_COLOR_INITIALIZATION_NEEDED is
    // chosen with full transparency, so this will always trigger a color update
    // below.
    //
    ASSERT(strokeColor != OUTLINE_COLOR_INITIALIZATION_NEEDED);
    if (m_StrokeColor != strokeColor)
    {
        v.SetColor(strokeColor);
        CREATEPARAMETERS cpGripperStrokeColor(core, v);
        IFC(CSolidColorBrush::Create(&pdo, &cpGripperStrokeColor));
        IFC(DoPointerCast(pGripperStrokeBrush, pdo));
        pdo = NULL;

        //
        // Color for internal ellipsis
        //
        IFC(m_pEllipseInternalNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pGripperStrokeBrush));

        //
        // Color for pole
        //
        IFC(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperStrokeBrush));

        //
        // Cache the color value to avoid excessive resets
        //
        m_StrokeColor = strokeColor;
    }

Cleanup:
    ReleaseInterface(pGripperStrokeBrush);
    ReleaseInterface(pdo);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:     CTextSelectionGripper::OnShowAnimationComplete
//
//  Synopsis:   Show animation complete event handler
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT CTextSelectionGripper::OnShowAnimationComplete(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        )
{
    xref_ptr<CStoryboard> storyboard(static_cast<CStoryboard*>(pSender));
    xref_ptr<CTextSelectionGripper> thisTextSelectionGripper(static_sp_cast<CTextSelectionGripper>(storyboard->GetTargetObject()));
    IFCPTR_RETURN(thisTextSelectionGripper);

    TraceTouchSelectionGripperShowEndInfo(
        thisTextSelectionGripper->m_isStartGripper,
        static_cast<XINT32>(thisTextSelectionGripper->m_centerWorldCoordinate.x),
        static_cast<XINT32>(thisTextSelectionGripper->m_centerWorldCoordinate.y));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:     CTextSelectionGripper::OnHideAnimationComplete
//
//  Synopsis:   Hide animation complete event handler
//
//------------------------------------------------------------------------
/* static */ _Check_return_ HRESULT CTextSelectionGripper::OnHideAnimationComplete(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        )
{
    xref_ptr<CStoryboard> storyboard(static_cast<CStoryboard*>(pSender));
    xref_ptr<CTextSelectionGripper> thisTextSelectionGripper(static_sp_cast<CTextSelectionGripper>(storyboard->GetTargetObject()));
    IFCPTR_RETURN(thisTextSelectionGripper);
    IGNOREHR(thisTextSelectionGripper->HideImmediately());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::CTextSelectionGripper
//
//  Synopsis: constructor
//
//------------------------------------------------------------------------
CTextSelectionGripper::CTextSelectionGripper(_In_ CCoreServices *pCore)
    : CCanvas(pCore)
{
    m_centerLocalCoordinate.x = 0;
    m_centerLocalCoordinate.y = 0;
    m_centerWorldCoordinate.x = 0;
    m_centerWorldCoordinate.y = 0;
    m_pOwner    = NULL;
    m_pTextView = NULL;
    m_pPopupNoAddRef = NULL;
    m_pShowAnimation = NULL;
    m_pHideAnimation = NULL;
    m_pTextSelectionManager = NULL;
    m_pInputProcessor = NULL;
    m_uiCapturePointerId = -1;

    // The internals of the Canvas are owned and released by the Canvas which is the base
    // class for this object. We only keep the weak references for the adjustments of
    // their properties.
    m_pEllipseInternalNoAddRef  = NULL;
    m_pEllipseExternalNoAddRef  = NULL;
    m_pRectanglePoleNoAddRef    = NULL;

    m_hideGripper               = TRUE;
    m_hasPointerCapture         = FALSE;
    m_isAnimating               = FALSE;
    m_isHideAnimation           = FALSE;
    m_isStartGripper            = FALSE;

    // Convert and save the configuration data
    m_rInternalDiameter = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleInternalOutlineDiameter);
    m_rStrokeThickness  = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleOutlineThickness);
    m_rPoleThickness    = ConvertToPixels(TextSelectionSettings::Get()->m_rHandlePostThickness);
    m_rPaddingDiameter  = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleExternalOutlineDiameter);
    m_rGripperTopToCenterHeight = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleTopToCenterHeight);

    m_rHitTestBoxWidth  = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleTouchTargetWidth);
    m_rHitTestBoxHeight = ConvertToPixels(TextSelectionSettings::Get()->m_rHandleTouchTargetHeight);

    m_rPoleHeight = POLE_HEIGHT_INITIALIZATION_NEEDED;
    m_StrokeColor = OUTLINE_COLOR_INITIALIZATION_NEEDED;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::~CTextSelectionGripper
//
//  Synopsis: destructor
//
//------------------------------------------------------------------------
CTextSelectionGripper::~CTextSelectionGripper()
{
    IGNOREHR(GetContext()->UnregisterGripper(this));
    delete m_pInputProcessor;
    m_pInputProcessor = nullptr;
    ReleaseInterface(m_pShowAnimation);
    ReleaseInterface(m_pHideAnimation);
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::OnPointerMoved
//
//  Synopsis: Event handler for PointerMoved event on CTextSelectionGripper.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::OnPointerMoved(
    _In_ CEventArgs* pEventArgs
)
{
    auto const pPointerEventArgs = static_cast<CPointerEventArgs* const>(pEventArgs);

    if (pPointerEventArgs->m_bHandled || !m_hasPointerCapture)
    {
        return S_OK;
    }

    XPOINTF lineTopLocal;
    lineTopLocal = m_centerLocalCoordinate;
    lineTopLocal.y -= static_cast<XUINT32>(m_rPaddingDiameter/2.0f);

    XPOINTF lineTopWorld;
    IFC_RETURN(ConvertToWorldCoordinate(lineTopLocal, &lineTopWorld));

    XPOINTF centerWorld;
    IFC_RETURN(ConvertToWorldCoordinate(m_centerLocalCoordinate, &centerWorld));
    XUINT32 lineHeightWorld = XcpAbs(static_cast<XUINT32>(centerWorld.y - lineTopWorld.y));

    //BLUE: 139223 - Safety Zone Scaling is slightly wrong at 4x scaling.
    XPOINT systemPerceivedHitPointPixels;
    IFC_RETURN(m_pInputProcessor->OnGripperDrag(ConvertToIntPoint(pPointerEventArgs->GetGlobalPoint()), lineHeightWorld, &systemPerceivedHitPointPixels));

    XPOINTF systemPerceivedHitPointWorld;
    systemPerceivedHitPointWorld.x = static_cast<XFLOAT>(systemPerceivedHitPointPixels.x);
    systemPerceivedHitPointWorld.y = static_cast<XFLOAT>(systemPerceivedHitPointPixels.y);

    ASSERT(m_pTextSelectionManager);

    IFC_RETURN(m_pTextSelectionManager->NotifyGripperPositionChanged(this, systemPerceivedHitPointWorld, pPointerEventArgs->m_pPointer->m_uiPointerId));

    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::OnPointerPressed
//
//  Synopsis: Event handler for PointerPressed event on CTextSelectionGripper.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::OnPointerPressed(
    _In_ CEventArgs* pEventArgs
)
{
    auto const pPointerEventArgs = static_cast<CPointerEventArgs* const>(pEventArgs);

    if (pPointerEventArgs->m_bHandled ||
        m_hasPointerCapture) // Ignore the tap with the other pointer
                             // while the gripper is being manipulated.
    {
        return S_OK;
    }

    const bool inputIsPen = pPointerEventArgs->m_pointerDeviceType == DirectUI::PointerDeviceType::Pen;
    if (inputIsPen && pPointerEventArgs->m_pPointer->m_bBarrelButtonPressed)
    {
        IFC_RETURN(m_pTextSelectionManager->NotifyPenPressedWithBarrelButton(this, pEventArgs));
        return S_OK;
    }

    bool hasPointerCapture = false;
    IFC_RETURN(CapturePointer(pPointerEventArgs->m_pPointer, &hasPointerCapture));
    m_hasPointerCapture = hasPointerCapture;
    if (m_hasPointerCapture)
    {
        m_uiCapturePointerId = pPointerEventArgs->m_pPointer->m_uiPointerId;

        XPOINTF centerWorld;
        IFC_RETURN(ConvertToWorldCoordinate(m_centerLocalCoordinate, &centerWorld));

        XPOINTF gripperSelectionPointLocal = m_centerLocalCoordinate;
        gripperSelectionPointLocal.y = m_centerLocalCoordinate.y - m_rPaddingDiameter/2.0f - m_rGripperTopToCenterHeight;

        XPOINTF gripperSelectionPointWorld;
        IFC_RETURN(ConvertToWorldCoordinate(gripperSelectionPointLocal, &gripperSelectionPointWorld));

        XUINT32 diameterPixels = XcpAbs(static_cast<XUINT32>(centerWorld.y - gripperSelectionPointWorld.y));
        XPOINTF centerOffsetFromAnchor;  // The selection point is just above the top of the handle.
        centerOffsetFromAnchor.x = 0;
        centerOffsetFromAnchor.y = 0;

        //  An example gripper with Hi drawn being displayed.
        //
        //             CC H H I
        //             CC HHH I
        //             CC H H I
        //             CC
        //        AAAAACCAAAAA
        //       A     CC     A
        //      A   BBBCCBBB   A
        //      A  B        B  A
        //      A  B        B  A
        //      A   BBBBBBBB   A
        //       A            A
        //        AAAAAAAAAAAA
        //
        //  The first parameter to SetGripperDisplayLocation should be the bottom of the line of text that is displaying Hi.
        //  If this wasn't ASCI art, the last row of Cs wouldn't be drawn.
        m_pInputProcessor->SetGripperDisplayLocation(ConvertToIntPoint(gripperSelectionPointWorld), ConvertToIntPoint(centerOffsetFromAnchor), diameterPixels);

        XPOINTF lineTopLocal = m_centerLocalCoordinate;
        lineTopLocal.y -= m_rPoleHeight;
        XPOINTF lineTopWorld;
        IFC_RETURN(ConvertToWorldCoordinate(lineTopLocal, &lineTopWorld));

        XUINT32 lineHeightWorld = XcpAbs(static_cast<XUINT32>(centerWorld.y - lineTopWorld.y));
        IFC_RETURN(m_pInputProcessor->OnStartGripperDrag(ConvertToIntPoint(pPointerEventArgs->GetGlobalPoint()), lineHeightWorld));

        // On rare occasions, the gripper may lose its pointer release event, ending up stuck
        // as as if it is still captured. Force the other gripper out of captured state if it
        // is in such state.
        m_pTextSelectionManager->GetOtherGripper(this)->ReleaseGripperPointerCapture();
    }

    IFC_RETURN(m_pTextSelectionManager->NotifyGripperPressed(this));

    pPointerEventArgs->m_bHandled = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::OnPointerReleased
//
//  Synopsis: Event handler for PointerReleased event on CTextSelectionGripper.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::OnPointerReleased(
    _In_ CEventArgs* pEventArgs
)
{
    CPointerEventArgs* pPointerEventArgs = static_cast<CPointerEventArgs*>(pEventArgs);
    if (pPointerEventArgs->m_bHandled)
    {
        return S_OK;
    }

    IFC_RETURN(PointerReleasedWorker(pPointerEventArgs->m_pPointer->m_uiPointerId, &pPointerEventArgs->m_bHandled));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::PointerReleasedWorker
//
//  Synopsis: Event handler worker routine for PointerReleased event
//            on CTextSelectionGripper which could be called without EventArgs
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::PointerReleasedWorker(
    _In_ XUINT32 uiPointerId,
    _Inout_opt_ bool *pfHandled
)
{
    auto scopeGuard = wil::scope_exit([&]
    {
        // reset active gripper so subsequent tap to select uses appropriate default
        m_pTextSelectionManager->SetActiveGripper(nullptr);
    });

    if (uiPointerId == m_uiCapturePointerId)
    {
        // Checks if the pointer is captured, then releases it if so
        IFC_RETURN(ReleaseGripperPointerCapture());
    }

    ASSERT(m_pTextSelectionManager);
    // Update gripper position only if the gripper did not get firmly hidden during the
    // manipulation. Prime example of a firm hide event is a focus loss.
    if (!m_hideGripper)
    {
        ASSERT(m_pPopupNoAddRef != nullptr);
        if (m_pOwner != m_pPopupNoAddRef->GetUIElementParentInternal())
        {
            // Update the parent as the gripper has moved into a different linked text container.
            IFC_RETURN(UpdateParent());

            // Re-parenting also removes the visibility of the popup. Reopen it to make it visible.
            IFC_RETURN(m_pPopupNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, true));
        }

        // set active gripper so copy icon can be displayed at correct end of selection.
        IFC_RETURN(m_pTextSelectionManager->SetActiveGripper(this));

        IFC_RETURN(m_pTextSelectionManager->SnapGrippersToSelection());

        if (pfHandled != nullptr)
        {
            *pfHandled = true;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::ReleaseGripperPointerCapture
//
//  Synopsis: Checks if the pointer is captured, then releases the capture
//            and performs the show animation if necessary.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::ReleaseGripperPointerCapture()
{
    if (m_hasPointerCapture)
    {
        m_hasPointerCapture = FALSE;
        m_pInputProcessor->OnEndGripperDrag();
        IFC_RETURN(ReleasePointerCaptures());
        m_uiCapturePointerId = -1;

        // There are two ways the gripper could have been hidden due to clipping: by making
        // it fully transparent if it has a capture, or by collapsing it if it does not.
        //
        // Make sure correct visibility state is restored by UpdateVisibility()
        IFC_RETURN(UpdateVisibility());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::OnRightTapped
//
//  Synopsis: Event handler for OnRightTapped event on CTextSelectionGripper.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::OnRightTapped(
    _In_ CEventArgs* pEventArgs
)
{
    CRightTappedEventArgs* pRightTappedEventArgs = static_cast<CRightTappedEventArgs*>(pEventArgs);
    if (pRightTappedEventArgs->m_bHandled || pRightTappedEventArgs->m_pointerDeviceType != DirectUI::PointerDeviceType::Touch)
    {
        return S_OK;
    }

    ASSERT(m_pTextSelectionManager);
    IFC_RETURN(m_pTextSelectionManager->ShowContextMenu(
        pRightTappedEventArgs->GetGlobalPoint(),
        FALSE /*isSelectionEmpty*/,
        TRUE /*isTouchInput*/));

    pRightTappedEventArgs->m_bHandled = TRUE;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::MeasureOverride(
    _In_ XSIZEF availableSize,
    _Out_ XSIZEF& desiredSize
    )
{
    // The Gripper does not affect layout.
    desiredSize.width  = 0;
    desiredSize.height = 0;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the position of the gripper. Coordinate is relative to
//      parent UIElement.
//
//------------------------------------------------------------------------
void CTextSelectionGripper::UpdateCenterLocalCoordinate(
    _In_ const XPOINTF& centerLocalCoordinate
    )
{
    if (m_centerLocalCoordinate.x != centerLocalCoordinate.x || m_centerLocalCoordinate.y != centerLocalCoordinate.y)
    {
        m_centerLocalCoordinate.x = centerLocalCoordinate.x;
        m_centerLocalCoordinate.y = centerLocalCoordinate.y;
        SetContentDirty(DirtyFlags::Bounds);
        CUIElement::NWSetContentDirty(this, DirtyFlags::Bounds);

        TraceTouchSelectionGripperRepositionInfo(
            m_isStartGripper,
            static_cast<XINT32>(m_centerWorldCoordinate.x),
            static_cast<XINT32>(m_centerWorldCoordinate.y));
    }
    SetContentDirty(DirtyFlags::Render);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts XPOINTF an XPOINT for interaction with the safety zone
//      processing code.
//
//------------------------------------------------------------------------
XPOINT CTextSelectionGripper::ConvertToIntPoint(_In_ const XPOINTF& coordinate)
{
    XPOINT pixels;
    pixels.x = static_cast<XINT32>(coordinate.x);
    pixels.y = static_cast<XINT32>(coordinate.y);
    return pixels;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a world coordinate to a local one relative to
//      the gripper's parent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::ConvertToLocalCoordinate(
    _In_ const XPOINTF& worldCoordinate,
    _Out_ XPOINTF *pLocalCoordinate)
{
    HRESULT hr = S_OK;
    ITransformer *pTransformer = NULL;
    CUIElement *pParent = m_pPopupNoAddRef->GetUIElementParentInternal();

    // At this point the popup must have a parent UIElement.
    IFCEXPECT_ASSERT(pParent != NULL);
    IFC(pParent->TransformToRoot(&pTransformer));
    IFC(pTransformer->ReverseTransform(&worldCoordinate, pLocalCoordinate, 1));

Cleanup:
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a local coordinate relative to the gripper's parent
//      to a world coordinate.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::ConvertToWorldCoordinate(
    _In_ const XPOINTF& localCoordinate,
    _Out_ XPOINTF *pWorldCoordinate)
{
    HRESULT hr = S_OK;
    ITransformer *pTransformer = NULL;
    CUIElement *pParent = m_pPopupNoAddRef->GetUIElementParentInternal();

    // At this point the popup must have a parent UIElement.
    IFCEXPECT_ASSERT(pParent != NULL);
    IFC(pParent->TransformToRoot(&pTransformer));
    IFC(pTransformer->Transform(&localCoordinate, pWorldCoordinate, 1));

Cleanup:
    ReleaseInterface(pTransformer);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the position of the gripper. Coordinate is relative to
//      world. Also changes the gripper pole height, unless the lineHeight
//      is POLE_HEIGHT_UNCHANGED
//
//      Note: this method does not force the coordinate property changes if
//      the new coordinates matches matches the previous ones.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::UpdateCenterWorldCoordinate(
    _In_ const XPOINTF& centerWorldCoordinate,
    _In_ XFLOAT lineHeight
    )
{
    XPOINTF centerLocalCoordinate;

    ASSERT((lineHeight >= 0.0f) || (lineHeight == POLE_HEIGHT_UNCHANGED));
    ASSERT(POLE_HEIGHT_UNCHANGED < 0.0f);
    // TODO (TFS:64369): pass line height=0.0f if the context is rotated or skewed
    if (lineHeight >= 0.0f)
    {
        IFC_RETURN(CheckAndRepositionShapesForPoleHeightChange(lineHeight));
    }

    if ((m_centerWorldCoordinate.x != centerWorldCoordinate.x) ||
        (m_centerWorldCoordinate.y != centerWorldCoordinate.y))
    {
        m_centerWorldCoordinate = centerWorldCoordinate; // Cached for tracing
        IFC_RETURN(ConvertToLocalCoordinate(centerWorldCoordinate, &centerLocalCoordinate));
        UpdateCenterLocalCoordinate(centerLocalCoordinate);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::Show
//
//  Synopsis: Makes the CTextSelectionGripper visible.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::Show(bool fAnimate)
{
    // "Show" only if hidden or in process of hiding
    if (m_hideGripper || m_isHideAnimation)
    {
        TraceTouchSelectionGripperShowBeginInfo(
            m_isStartGripper,
            static_cast<XINT32>(m_centerWorldCoordinate.x),
            static_cast<XINT32>(m_centerWorldCoordinate.y));

        if (m_isHideAnimation)
        {
            IFC_RETURN(m_pHideAnimation->StopPrivate());
            m_isHideAnimation = FALSE;
        }

        IFC_RETURN(UpdateParent());

        // Open the Popup.
        ASSERT(m_pPopupNoAddRef != NULL);
        IFC_RETURN(m_pPopupNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, TRUE));

        // Register for visibility updates.
        IFC_RETURN(GetContext()->RegisterGripper(this));

        m_hideGripper = FALSE;

        if (fAnimate)
        {
            IFC_RETURN(m_pShowAnimation->BeginPrivate(TRUE));
        }
        else
        {
             TraceTouchSelectionGripperShowEndInfo(
                 m_isStartGripper,
                 static_cast<XINT32>(m_centerWorldCoordinate.x),
                 static_cast<XINT32>(m_centerWorldCoordinate.y));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::Hide
//
//  Synopsis: Makes the CTextSelectionGripper invisible.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::Hide(bool fAnimate)
{
    HRESULT hr = S_OK;
    if (!m_hideGripper) // Not fully hidden yet
    {
        TraceTouchSelectionGripperHideBeginInfo(
                m_isStartGripper,
                static_cast<XINT32>(m_centerWorldCoordinate.x),
                static_cast<XINT32>(m_centerWorldCoordinate.y));

        if (fAnimate)
        {
            if (!m_isHideAnimation) // Ignore if already animating
            {
                IGNOREHR(m_pShowAnimation->StopPrivate());
                IFC(m_pHideAnimation->BeginPrivate(TRUE));
                m_isHideAnimation = TRUE;
            }
        }
        else
        {
            hr = HideImmediately();
        }
    }
Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT CTextSelectionGripper::HideImmediately()
{
    CValue val;

    // Stop any potential asynchronous activity first
    IGNOREHR(m_pShowAnimation->StopPrivate());
    IGNOREHR(m_pHideAnimation->StopPrivate());
    m_isHideAnimation = FALSE;

    // Close the Popup.
    ASSERT(m_pPopupNoAddRef != NULL);
    IFC_RETURN(m_pPopupNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Popup_IsOpen, FALSE));

    // Unregister for visibility updates and hide the gripper.
    IFC_RETURN(GetContext()->UnregisterGripper(this));

    val.Set(DirectUI::Visibility::Collapsed);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, val));

    m_hideGripper = TRUE;

    // Make sure the pointer capture is released
    ReleaseGripperPointerCapture();

    TraceTouchSelectionGripperHideEndInfo(
        m_isStartGripper,
        static_cast<XINT32>(m_centerWorldCoordinate.x),
        static_cast<XINT32>(m_centerWorldCoordinate.y));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::ConvertToPixels
//
//  Synopsis: Converts from millimeters to device pixels.
//
//------------------------------------------------------------------------
XFLOAT CTextSelectionGripper::ConvertToPixels(_In_ XFLOAT distanceInMillimeters)
{
    // TODO: hard code for now until the API is available.
    XFLOAT dpi = 96.0f;
    return dpi * distanceInMillimeters / 25.4f;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::HiMetricsPerPixel
//
//  Synopsis: The number of touch pixels per screen pixel.
//
//------------------------------------------------------------------------
XFLOAT CTextSelectionGripper::HiMetricsPerPixel()
{
    XFLOAT dpi = 96.0f;
    const XFLOAT HiMetricsPerInch = 2540.0f;  // The number of .01mm units in 1 inch.
    return HiMetricsPerInch / dpi;
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::SetOwner
//
//  Synopsis: Sets the UIElement that has the text selected by
//            the CTextSelectionGripper.
//            Note: we do not update the visual parent here because this
//            this will cause the Input manager state to be lost
//            (in CDependencyObject::LeaveImpl) which we cannot allow in
//            scenarios where the gripper is moving across
//            Linked Text Containers and continuously changing its owner.
//            We update the visual parent to be m_pOwner when we show
//            the gripper back again.
//
//------------------------------------------------------------------------
void CTextSelectionGripper::SetOwner(
    _In_ CUIElement *pOwner,
    _In_ bool isGripperMoving)
{
    if (m_pOwner != pOwner)
    {
        m_pOwner = pOwner;
    }
}

//------------------------------------------------------------------------
//
//  Method:   CTextSelectionGripper::UpdateParent
//
//  Synopsis: Sets the UIElement that has the CTextSelectionGripper in
//            its visual tree.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextSelectionGripper::UpdateParent()
{
    HRESULT hr = S_OK;
    CUIElement *pCurrentParent = m_pPopupNoAddRef->GetUIElementParentInternal();
    ITransformer *pTransformerCurrentParent = NULL;
    ITransformer *pTransformerNewParent = NULL;
    XPOINTF centerWorldCoordinate;

    // At this point the popup must have a parent UIElement.
    IFCEXPECT_ASSERT(m_pOwner != NULL);
    IFCEXPECT_ASSERT(pCurrentParent != NULL);

    if (pCurrentParent != m_pOwner)
    {
        IFC(m_pPopupNoAddRef->PegManagedPeer());
        IFC(pCurrentParent->RemoveChild(m_pPopupNoAddRef));
        IFC(m_pOwner->AddChild(m_pPopupNoAddRef));
        m_pPopupNoAddRef->UnpegManagedPeer();

        IFC(m_pOwner->TransformToRoot(&pTransformerNewParent));
        IFC(pCurrentParent->TransformToRoot(&pTransformerCurrentParent));
        IFC(pTransformerCurrentParent->Transform(&m_centerLocalCoordinate, &centerWorldCoordinate, 1));
        IFC(pTransformerNewParent->ReverseTransform(&centerWorldCoordinate, &m_centerLocalCoordinate, 1));

    }

Cleanup:
    ReleaseInterface(pTransformerCurrentParent);
    ReleaseInterface(pTransformerNewParent);
    RRETURN(hr);
}

_Check_return_ XFLOAT CTextSelectionGripper::GetActualOffsetX()
{
    return m_centerLocalCoordinate.x - m_rPaddingDiameter / 2.0f;
}

_Check_return_ XFLOAT CTextSelectionGripper::GetActualOffsetY()
{
    return m_centerLocalCoordinate.y - (m_rPaddingDiameter / 2.0f + m_rPoleHeight);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns offset from the gripper center to the bottom of the line it attaches to.
//      Used by the center coordinate calculation and to find out if the gripper is clipped.
//
//      NB: this should be updated if the gripper is given any additional offset.
//
//------------------------------------------------------------------------
XFLOAT CTextSelectionGripper::GetGripperCenterToLineEdgeOffset()
{
    return m_rPaddingDiameter / 2.0f;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextSelectionGripper::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    //
    // The bounds are used for hit testing, and the gripper is hit testable in an area that's
    // larger than the shape it draws. So falsely report that the bounds are bigger than the
    // bounds of the drawn shape.
    //
    // TODO (TFS:61368): intersection control
    //
    pBounds->left   = m_rPaddingDiameter / 2.0f - m_rHitTestBoxWidth / 2.0f;
    pBounds->top    = m_rPoleHeight;
    pBounds->right  = pBounds->left + m_rHitTestBoxWidth;
    pBounds->bottom = pBounds->top + m_rHitTestBoxHeight;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextSelectionGripper::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    //
    // All hits within the hit test bounds count as a hit. Note that right now we report the
    // element bounds to be larger than it should be in order to match the hit test bounds.
    //

    *pHit = !m_isAnimating && !IsGripperClipped();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a polygon intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextSelectionGripper::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    HRESULT hr = S_OK;

    //
    // All hits within the hit test bounds count as a hit. Note that right now we report the
    // element bounds to be larger than it should be in order to match the hit test bounds.
    //
    *pHit = !m_isAnimating && !IsGripperClipped();

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Walks up the visual tree and checks whether the edge of the
//      selection rect that this gripper controls is clipped or not.
//
//------------------------------------------------------------------------
bool CTextSelectionGripper::IsGripperClipped()
{
    //
    // TODO (TFS:61368): intersection control
    //

    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    bool isClipped = false;
    XRECTF_RB innerRect;
    XRECTF_RB outerRect;
    ITransformer *pTransformerParent = NULL;
    ITransformer *pTransformerOwner = NULL;
    XPOINTF centerWorldCoordinate;
    XPOINTF centerOwnerLocalCoordinate;
    CUIElement *pParent = m_pPopupNoAddRef->GetUIElementParentInternal();

    // At this point the popup must have a parent UIElement.
    IFCEXPECT_ASSERT(pParent != NULL);
    IFCEXPECT_ASSERT(m_pOwner != NULL);

    if (pParent != m_pOwner)
    {
        IFC(pParent->TransformToRoot(&pTransformerParent));
        IFC(m_pOwner->TransformToRoot(&pTransformerOwner));
        IFC(pTransformerParent->Transform(&m_centerLocalCoordinate, &centerWorldCoordinate, 1));
        IFC(pTransformerOwner->ReverseTransform(&centerWorldCoordinate, &centerOwnerLocalCoordinate, 1));
    }
    else
    {
        centerOwnerLocalCoordinate = m_centerLocalCoordinate;
    }

    // We are creating a rect as small as possible around the corresponding edge
    // of the selection rect.
    innerRect.left = centerOwnerLocalCoordinate.x - 1;
    innerRect.right = innerRect.left + 2;
    // The gripper center is offset from the bottom edge contains at least a gripper radius.
    innerRect.bottom = centerOwnerLocalCoordinate.y - GetGripperCenterToLineEdgeOffset();
    innerRect.top = innerRect.bottom - 1;

    IFC(m_pOwner->TransformToWorldSpace(&innerRect, &outerRect, false /* ignoreClipping */, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));

    isClipped = (outerRect.top == outerRect.bottom);
Cleanup:
    ReleaseInterface(pTransformerParent);
    ReleaseInterface(pTransformerOwner);
    return isClipped;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Used by the hwwalk to determine whether to draw the gripper or not
//
//------------------------------------------------------------------------
bool CTextSelectionGripper::IsGripperVisible()
{
    return !m_isAnimating && !m_hideGripper && !IsGripperClipped();
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Coerces the visibility property on the gripper based on its ancestor's state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextSelectionGripper::UpdateVisibility()
{
    bool isAncestorTransformAnimating = false;

    // Walk up the tree just like in CUIElement::GetRedirectionTransformsAndParentCompNode, where the Popup would
    // collect the transform animation flag from its ancestors in the render walk.
    CUIElement *pCurrentNoRef = m_pPopupNoAddRef;
    while (pCurrentNoRef != NULL)
    {
        // If the current element is hidden for a LayoutTransition, use the LayoutTransitionElement's properties instead.
        CUIElement *pTransformElementNoRef;
        if (pCurrentNoRef->IsHiddenForLayoutTransition())
        {
            // TODO: HWPC: During a portaling transition, this only checks the first LTE, but the grippers will be hidden anyway.
            CLayoutTransitionElement *pLTENoRef;
            IFC_RETURN(pCurrentNoRef->GetLayoutTransitionElements()->get_item(0, pLTENoRef));
            pTransformElementNoRef = pLTENoRef;
        }
        else
        {
            pTransformElementNoRef = pCurrentNoRef;
        }

        if (pTransformElementNoRef->IsTransformOrOffsetAffectingPropertyIndependentlyAnimating())
        {
            isAncestorTransformAnimating = TRUE;
            break;
        }

        pCurrentNoRef = pCurrentNoRef->GetUIElementAdjustedParentInternal(TRUE /*public parents only*/);
    }

    m_isAnimating = isAncestorTransformAnimating;

    // When the text control is located inside the touch scroll view and the touch occurs
    // inside that control, it is considered a start of a scroll animation. This causes an
    // immediate hide of grippers upon touching down on the text. This occurs even if there
    // was no actual scrolling involved. As a result, when the user taps on the text to dismiss
    // the grippers, the grippers get hidden, then show upon releasing the touch, then the hide
    // animation is started to dismiss the grippers. This sequence looks like a flickering
    // to the user.
    //
    // To avoid this flicker, do not make the gripper visible if the gripper's hide animation
    // is already started. Instead, it will stay collapsed till the end of gripper hide animation.
    //
    // Note: this solution depends on the ordering of method calls upon the release of touch pointer.
    // CTextSelectionGripper::Hide goes first, then CTextSelectionGripper::UpdateVisibility.
    // If the order changes to opposite, a more complicated solution may be required, such as
    // measuring the timing between UpdateVisibility and Hide.

    if (!(m_isHideAnimation && !m_isAnimating))
    {
        CValue val;
        bool fHide = (m_isAnimating || IsGripperClipped());
        if (fHide && m_hasPointerCapture)
        {
            // Special case of hiding the gripper while it has a pointer capture:
            // make it fully transparent instead of Collapsed to prevent the loss of capture.
            val.SetFloat(0.0f);
            IFC_RETURN(m_pPopupNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, val));
        }
        else
        {
            val.Set(fHide ? DirectUI::Visibility::Collapsed : DirectUI::Visibility::Visible);
            IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, val));
            if (!fHide)
            {
                // In case if the gripper was previously hidden by setting it fully transparent,
                // restore the full opacity.
                val.SetFloat(1.0f);
                IFC_RETURN(m_pPopupNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, val));
            }
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Generates a gripper show/hide animation storyboard
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextSelectionGripper::CreateFadeStoryboard(
    _Outptr_ CStoryboard   **ppStoryboard,
    _In_ CCoreServices         *pCore,
    _In_ CDependencyObject     *pTargetObject,
    _In_ const CDependencyProperty   *pTargetProperty,
    _In_ XFLOAT                 rFullTimeInSeconds,
    _In_ bool                  fFadeIn             // TRUE: fade-in animation, FALSE: fade-out
    )
{
    HRESULT                         hr               = S_OK;
    ThemingData::OpacitySplineTransform *pTransforms = NULL;
    XINT32                          iTransformCount  = 0;
    CDoubleAnimationUsingKeyFrames *pDoubleAnimation = NULL;
    CLinearDoubleKeyFrame          *pDoubleKeyFrame0 = NULL; // Jump to initial opacity
    CTimeSpan                      *pBeginTime       = NULL;
    CStoryboard                    *pStoryboard      = NULL;
    CDoubleKeyFrameCollection      *pKeyFramesCollection = NULL;
    CKeySpline                     *pKeySpline       = NULL;
    CDoubleKeyFrame                *pDoubleKeyFrame  = NULL;
    CValue                          valueTemp;
    CValue                          valueZero;

    valueZero.SetFloat(0.0f);
    CREATEPARAMETERS cpZeroTime(pCore, valueZero);
    CREATEPARAMETERS cp(pCore);

    //
    // Retrieve the animation theme data
    //
    if (fFadeIn)
    {
        IFC(gps->GetFadeInThemeAnimationData(&iTransformCount, &pTransforms));
    }
    else
    {
        IFC(gps->GetFadeOutThemeAnimationData(&iTransformCount, &pTransforms));
    }

    IFCEXPECT(iTransformCount > 0);

    //
    // Programmatically build the animation storyboard.
    //

    // Pre-create the common objects
    IFC(CreateDO(&pBeginTime, &cpZeroTime));
    IFC(CreateDO(&pDoubleKeyFrame0, &cp));
    IFC(CreateDO(&pDoubleAnimation, &cp));
    IFC(CreateDO(&pStoryboard, &cp));
    IFC(CDoubleKeyFrameCollection::Create(reinterpret_cast<CDependencyObject **>(&pKeyFramesCollection), &cp));

    pBeginTime->m_rTimeSpan = 0.0f;

    //
    // Set up and add the initial value into the frames collection
    //
    valueTemp.SetFloat(fFadeIn ? 0.0f : 1.0f);
    IFC(pDoubleKeyFrame0->SetValue(
        pDoubleKeyFrame0->GetPropertyByIndexInline(KnownPropertyIndex::DoubleKeyFrame_Value), valueTemp));

    valueTemp.Set<valueVO>(KeyTimeVOHelper::CreateTimeSpan(pCore, 0.0).detach());
    IFC(pDoubleKeyFrame0->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_KeyTime, valueTemp));

    IFC(pKeyFramesCollection->Append(pDoubleKeyFrame0));

    //
    // Generate key frames and add them into the frames collection
    //
    {
        XFLOAT rLastTransitionEndTime = 0.0f;

        for (XINT32 i = 0; i < iTransformCount; i++)
        {
            if ((pTransforms[i].p1 == 0.0f) && (pTransforms[i].p2 == 1.0f))
            {
                // The PVL spline control values of 0 and 1 respectively are universally
                // treated as magic values to indicate a linear segment instead of a spline.
                IFC(CreateDO2<CLinearDoubleKeyFrame>(&pDoubleKeyFrame, &cp));
            }
            else
            {
                IFC(CreateDO2<CSplineDoubleKeyFrame>(&pDoubleKeyFrame, &cp));
                IFC(CreateDO(&pKeySpline, &cp));

                pKeySpline->m_ControlPoint1.x = pTransforms[i].p1;
                pKeySpline->m_ControlPoint1.y = 0.0f;
                pKeySpline->m_ControlPoint2.x = pTransforms[i].p2;
                pKeySpline->m_ControlPoint2.y = 0.0f;

                IFC(pDoubleKeyFrame->SetValueByKnownIndex(KnownPropertyIndex::SplineDoubleKeyFrame_KeySpline, pKeySpline));

                ReleaseInterface(pKeySpline);
            }

            valueTemp.Set<valueVO>(KeyTimeVOHelper::CreateTimeSpan(pCore, rLastTransitionEndTime + pTransforms[i].durationTime).detach());
            IFC(pDoubleKeyFrame->SetValueByKnownIndex(KnownPropertyIndex::DoubleKeyFrame_KeyTime, valueTemp));

            valueTemp.SetFloat(pTransforms[i].endValue);
            IFC(pDoubleKeyFrame->SetValue(
                pDoubleKeyFrame->GetPropertyByIndexInline(KnownPropertyIndex::DoubleKeyFrame_Value), valueTemp));

            IFC(pKeyFramesCollection->Append(pDoubleKeyFrame));

            // Prepare for the next iteration
            rLastTransitionEndTime += pTransforms[i].durationTime;
            ReleaseInterface(pDoubleKeyFrame);
        }
    }

    //
    // Connect rest of the objects together
    //
    IFC(pDoubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::DoubleAnimationUsingKeyFrames_KeyFrames, pKeyFramesCollection));

    IFC(pDoubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_BeginTime, pBeginTime));

    valueTemp.Set<valueVO>(
        DurationVOHelper::CreateTimeSpan(
            pCore,
            pTransforms[iTransformCount - 1].startTime + pTransforms[iTransformCount - 1].durationTime).detach());
    IFC(pDoubleAnimation->SetValueByKnownIndex(KnownPropertyIndex::Timeline_Duration, valueTemp));

    // The Storyboard.TargetObject and TargetName properties are strings, which is resolved
    //  by the XAML parser to actual object and property.  We don't have the parser
    //  luxury here and have to set them directly.
    IFC(pDoubleAnimation->SetTargetObject(pTargetObject));
    pDoubleAnimation->SetTargetProperty(pTargetProperty);
    pDoubleAnimation->m_enableDependentAnimation = true; // Enable dependent animation so that it works under CacheMode=BitmapCache

    IFC(pStoryboard->AddChild(pDoubleAnimation));
    IFC(pStoryboard->SetTargetObject(pTargetObject)); // Save the target object to be used by completion callback

    *ppStoryboard = pStoryboard;
    pStoryboard   = NULL;

Cleanup:
    delete[] pTransforms;
    ReleaseInterface(pDoubleAnimation);
    ReleaseInterface(pDoubleKeyFrame0);
    ReleaseInterface(pDoubleKeyFrame);
    ReleaseInterface(pBeginTime);
    ReleaseInterface(pKeySpline);
    ReleaseInterface(pStoryboard);
    ReleaseInterface(pKeyFramesCollection);
    RRETURN(hr);
}
