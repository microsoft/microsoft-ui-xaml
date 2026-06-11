// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "RichEditGripperChild.h"
#include "TextBoxBase.h"
#include "TextBoxView.h"
#include "HoldingEventArgs.h"
#include <XamlOneCoreTransforms.h>

#include <textserv.h>
#include "RootScale.h"

const XFLOAT CRichEditGripperChild::c_rPoleHeight = 20.0; // should be updated by RichEdit
const XFLOAT CRichEditGripperChild::c_rHitTargetWidthHeight = 72.0;

// From greenlines: Hit target rect extends 38 pixels from the bottom of the selection (which is the
// top of the gripper canvas). Since the hit target rect is 72 pixels tall, it needs to extend 34 pixels
// above the top of the gripper canvas.
const XFLOAT CRichEditGripperChild::c_rHitTargetVerticalOffset = -34.0f;

static const XFLOAT c_rPcpPoleWidth = 4.0f;
static const XFLOAT c_rPcpPoleBorder = 1.0f;
static const XFLOAT c_rPcpPoleScale = 1.3f;
static const XFLOAT c_rPoleDescent = 0.5f;

const int CRichEditGripperChild::c_nGripperWidth = 20;
const int CRichEditGripperChild::c_nGripperDescent = 21;
const XFLOAT CRichEditGripperChild::c_rStrokeThickness = 2.0;
const XFLOAT CRichEditGripperChild::c_rPoleThickness = 2.0;
const XFLOAT CRichEditGripperChild::c_rInternalDiameter = 14.0;
const XFLOAT CRichEditGripperChild::c_rPaddingDiameter = 17.5;

// Add some padding for measuing if the gripper is out of textbox bound vertically.
// This is to deal with the rounding issue: from floating point to ULONG in TxGetViewportRect call by RichEdit.
static const XFLOAT c_rPoleOutOfBoundsPadding = 2.0f;

/* static */ const XFLOAT CRichEditGripperChild::POLE_HEIGHT_UNCHANGED = -1.0f;

CRichEditGripperChild::CRichEditGripperChild(_In_ CCoreServices *pCore)
    : CCanvas(pCore)
{
}

//------------------------------------------------------------------------
//
//  Method:   CRichEditGripperChild::Create
//
//  Synopsis: Bare DO initialization. Most of the initialization is done in
//  InitializeGripper method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditGripperChild::Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_ CREATEPARAMETERS *pCreate)
{


    if (pCreate->m_value.GetType() == valueString)
    {
        IFC_RETURN(E_NOTIMPL);
    }
    else
    {
        xref_ptr<CRichEditGripperChild> pObj;
        pObj.attach(new CRichEditGripperChild(pCreate->m_pCore));
        IFC_RETURN(ValidateAndInit(pObj, ppObject));
        *ppObject = pObj.detach();
    }

    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::InitializeGripper(
    _In_ ITextServices *pTextServices,
    _In_ RichEditGripperCommon::GripperType gripperType,
    _In_ ITextHost2 *pTextHost,
    _In_ CTextBoxBase *pTextBoxBase)
{
    xref_ptr<CBrush> pGripperFillBrush;
    xref_ptr<CBrush> pTransparentBrush;
    xref_ptr<CSolidColorBrush> pGripperDispBrush;
    xref_ptr<CEllipse> pEllipseInternal;
    xref_ptr<CEllipse> pEllipsePadding;
    xref_ptr<CTouchableRectangle> pRectHitTarget;
    xref_ptr<CRectangle> pRectangle;
    xref_ptr<CRectangle> pRectPcpPole;
    xref_ptr<CRectangle> pRectPcpPoleBg;
    CDependencyObject* pdo = nullptr;
    auto                   core = GetContext();
    CREATEPARAMETERS       cp(core);
    CValue                 v;
    v.SetColor(0x0);       // Transparent color
    CREATEPARAMETERS       cpTransparentColor(core, v);

    XFLOAT fDescent;
    XFLOAT fExternalOffset;

    m_rPoleHeight = c_rPoleHeight;

    fDescent = static_cast<XFLOAT>(c_nGripperDescent - c_nGripperWidth);
    fExternalOffset = (c_rPaddingDiameter - c_rInternalDiameter) / 2.0f;

    // Make sure our peer is pegged until it's in the tree, so that it can protect the ellipses that we're about
    // to create.  Those ellipses might not need a peer, but they will if one of the brushes we're about to get has one.
    IFC_RETURN(EnsurePeerDuringCreate());

    IFC_RETURN(core->GetTextSelectionGripperBrush(pGripperFillBrush.ReleaseAndGetAddressOf()));
    IFC_RETURN(core->GetSystemTextSelectionBackgroundBrush(pGripperDispBrush.ReleaseAndGetAddressOf()));

    m_pTextServices = pTextServices;
    m_gripperType = gripperType;
    m_pTextBoxBase = pTextBoxBase;

    //
    // Transparent brush
    //
    IFC_RETURN(CSolidColorBrush::Create(&pdo, &cpTransparentColor));
    pTransparentBrush.attach(static_cast<CBrush*>(pdo));
    pdo = nullptr;

    //
    // Hit target rectangle
    //

    IFC_RETURN(CTouchableRectangle::Create(&pdo, &cp));
    pRectHitTarget.attach(static_cast<CTouchableRectangle*>(pdo));
    pdo = nullptr;

    v.SetFloat(c_rHitTargetWidthHeight);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat(c_rHitTargetWidthHeight);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pTransparentBrush.get()));

    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pTransparentBrush.get()));

    v.SetFloat(c_rStrokeThickness);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, v));

    //
    // Internal ellipse
    //
    IFC_RETURN(CEllipse::Create(&pdo, &cp));
    pEllipseInternal.attach(static_cast<CEllipse*>(pdo));
    pdo = nullptr;
    IFC_RETURN(pEllipseInternal->EnsurePeerDuringCreate()); // Peg until it's in the tree, the way the parser would

    v.SetBool(TRUE);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::UIElement_UseLayoutRounding, v));

    v.SetFloat(c_rInternalDiameter);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat(c_rInternalDiameter);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pTransparentBrush.get()));

    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pGripperDispBrush.get()));

    v.SetFloat(c_rStrokeThickness);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, v));


    //
    // Padding ellipse
    //
    IFC_RETURN(CEllipse::Create(&pdo, &cp));
    pEllipsePadding.attach(static_cast<CEllipse*>(pdo));
    pdo = nullptr;
    IFC_RETURN(pEllipsePadding->EnsurePeerDuringCreate()); // Peg until it's in the tree, the way the parser would

    v.SetFloat(c_rPaddingDiameter);
    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperFillBrush.get()));

    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, pGripperFillBrush.get()));

    v.SetFloat(c_rStrokeThickness);
    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::Shape_StrokeThickness, v));

    //
    // Rectangle (pole)
    //
    IFC_RETURN(CRectangle::Create(&pdo, &cp));
    pRectangle.attach(static_cast<CRectangle*>(pdo));
    pdo = nullptr;
    IFC_RETURN(pRectangle->EnsurePeerDuringCreate()); // Peg until it's in the tree, the way the parser would

    v.SetFloat(0.0f);
    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat(m_rPoleHeight + fExternalOffset + fDescent);
    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperDispBrush.get()));

    //
    // PCP pole
    //
    IFC_RETURN(CRectangle::Create(&pdo, &cp));
    pRectPcpPole.attach(static_cast<CRectangle*>(pdo));
    pdo = nullptr;
    IFC_RETURN(pRectPcpPole->EnsurePeerDuringCreate()); // Peg until it's in the tree, the way the parser would

    v.SetFloat(0.0f);
    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat((m_rPoleHeight * c_rPcpPoleScale) + fExternalOffset + fDescent);
    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperDispBrush.get()));

    //
    // PCP pole background
    //
    IFC_RETURN(CRectangle::Create(&pdo, &cp));
    pRectPcpPoleBg.attach(static_cast<CRectangle*>(pdo));
    pdo = nullptr;
    IFC_RETURN(pRectPcpPoleBg->EnsurePeerDuringCreate()); // Peg until it's in the tree, the way the parser would

    v.SetFloat(0.0f);
    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));

    v.SetFloat((m_rPoleHeight * c_rPcpPoleScale) + fExternalOffset + fDescent);
    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, pGripperFillBrush.get()));

    //
    // Canvas
    //
    v.SetFloat(c_rPaddingDiameter);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, v));
    v.SetFloat(c_rPaddingDiameter + m_rPoleHeight + fDescent);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

    //
    // Put them together into the canvas
    //
    IFC_RETURN(AddChild(pEllipseInternal));
    IFC_RETURN(AddChild(pEllipsePadding));
    IFC_RETURN(AddChild(pRectHitTarget));
    IFC_RETURN(AddChild(pRectangle));
    IFC_RETURN(AddChild(pRectPcpPole));
    IFC_RETURN(AddChild(pRectPcpPoleBg));

    //
    // Position for padding circle inside the canvas
    //
    v.SetSigned(3);
    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    v.SetFloat(fDescent);
    IFC_RETURN(pEllipsePadding->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    //
    // Position for internal circle inside the canvas
    //
    v.SetSigned(5);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    v.SetFloat(fDescent + (c_rPaddingDiameter - c_rInternalDiameter) / 2);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    v.SetFloat((c_rPaddingDiameter - c_rInternalDiameter) / 2);
    IFC_RETURN(pEllipseInternal->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    //
    // Position for hit target rectangle inside the canvas
    //
    v.SetFloat((c_rPaddingDiameter - c_rHitTargetWidthHeight)/2.0f);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetFloat(c_rHitTargetVerticalOffset);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    v.SetSigned(6);
    IFC_RETURN(pRectHitTarget->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Position for pole inside the canvas
    //
    v.SetFloat((c_rPaddingDiameter - c_rPoleThickness) / 2.0f);
    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetFloat(-(m_rPoleHeight - c_rPoleDescent));
    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    v.SetSigned(4);
    IFC_RETURN(pRectangle->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Position for PCP pole inside the canvas
    //
    v.SetFloat(((c_rPaddingDiameter - c_rPcpPoleWidth) / 2.0f) + c_rPcpPoleBorder);
    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetFloat(-((m_rPoleHeight * c_rPcpPoleScale) - c_rPcpPoleBorder));
    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

    v.SetSigned(2);
    IFC_RETURN(pRectPcpPole->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Position for PCP pole background inside the canvas
    //
    v.SetFloat((c_rPaddingDiameter - c_rPcpPoleWidth) / 2.0f);
    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Left, v));

    v.SetFloat(-(m_rPoleHeight * c_rPcpPoleScale));
    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));
    v.SetSigned(1);
    IFC_RETURN(pRectPcpPoleBg->SetValueByKnownIndex(KnownPropertyIndex::Canvas_ZIndex, v));

    //
    // Save all shapes for further adjustments. We do this now, because
    // CheckAndRepositionShapesForPoleHeightChange() and UpdateThemeColor()
    // will use them.
    //
    m_pEllipseInternalNoAddRef = pEllipseInternal.get();
    m_pEllipsePaddingNoAddRef = pEllipsePadding.get();
    m_pRectHitTargetNoAddRef = pRectHitTarget.get();
    m_pRectanglePoleNoAddRef   = pRectangle.get();
    m_pRectPcpPoleNoAddRef = pRectPcpPole.get();
    m_pRectPcpPoleBgNoAddRef = pRectPcpPoleBg.get();

    //
    // Do not create the Safety Zone's input processor here.  RichEdit will take
    // care of safety zones for us.
    //

    return S_OK;
}


_Check_return_ HRESULT CRichEditGripperChild::CheckAndRepositionShapesForPoleHeightChange(XFLOAT poleHeight)
{
    CValue v;

    if (poleHeight != m_rPoleHeight)
    {
        XFLOAT fDescent = static_cast<XFLOAT>(c_nGripperDescent - c_nGripperWidth);
        XFLOAT fExternalOffset = (c_rPaddingDiameter - c_rInternalDiameter) / 2.0f;

        m_rPoleHeight = poleHeight;

        v.SetFloat(m_rPoleHeight + fExternalOffset + fDescent);
        IFC_RETURN(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

        v.SetFloat(-(m_rPoleHeight - c_rPoleDescent));
        IFC_RETURN(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

        v.SetFloat((m_rPoleHeight * c_rPcpPoleScale) + fExternalOffset + fDescent);
        IFC_RETURN(m_pRectPcpPoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

        v.SetFloat(-((m_rPoleHeight * c_rPcpPoleScale) - c_rPcpPoleBorder));
        IFC_RETURN(m_pRectPcpPoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

        v.SetFloat((m_rPoleHeight * c_rPcpPoleScale) + fExternalOffset + fDescent);
        IFC_RETURN(m_pRectPcpPoleBgNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));

        v.SetFloat(-(m_rPoleHeight * c_rPcpPoleScale));
        IFC_RETURN(m_pRectPcpPoleBgNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Canvas_Top, v));

        v.SetFloat(c_rPaddingDiameter + m_rPoleHeight + fDescent);
        IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, v));
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the position of the gripper. Coordinate is relative to
//      world. Also changes the gripper pole height, unless the lineHeight
//      is POLE_HEIGHT_UNCHANGED
//
//      Note: this method does not force the coordinate property changes if
//      the new coordinates matches the previous ones.
//
//      Note: it is assumed that the Show method will be called immediately
//      following any call to this method.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CRichEditGripperChild::UpdateCenterWorldCoordinate(
    _In_ const XPOINTF& centerWorldCoordinate,
    _In_ XFLOAT lineHeight)
{
    XPOINTF centerScaledCoordinate;
    XRECTF_RB controlRect;
    XFLOAT scaledLineHeight;

    ASSERT((lineHeight >= 0.0f) || (lineHeight == POLE_HEIGHT_UNCHANGED));
    ASSERT(POLE_HEIGHT_UNCHANGED < 0.0f);
    // TODO (TFS:64369): pass line height=0.0f if the context is rotated or skewed
    if (lineHeight >= 0.0f)
    {
        IFC_RETURN(CheckAndRepositionShapesForPoleHeightChange(lineHeight));
    }

    if (XamlOneCoreTransforms::IsEnabled())
    {
        scaledLineHeight = (lineHeight - c_rPoleDescent);
        centerScaledCoordinate.x = centerWorldCoordinate.x;
        centerScaledCoordinate.y = centerWorldCoordinate.y;
    }
    else
    {
        const float scale = RootScale::GetRasterizationScaleForElement(this);
        scaledLineHeight = (lineHeight - c_rPoleDescent) * scale;

        // Actually inverse scale: undoing the scaling provided by RichEdit because Jupiter
        // will scale the position later.
        centerScaledCoordinate.x = centerWorldCoordinate.x / scale;
        centerScaledCoordinate.y = centerWorldCoordinate.y / scale;
    }

    m_centerWorldCoordinate = centerWorldCoordinate; // Cached for tracing

    IFC_RETURN(m_pTextBoxBase->GetView()->GetGlobalBounds(&controlRect));
    if ((centerWorldCoordinate.y - scaledLineHeight + c_rPoleOutOfBoundsPadding) < controlRect.top)
    {
        m_bPoleOutOfBounds = TRUE;
    }
    else
    {
        m_bPoleOutOfBounds = FALSE;
    }

    // Note that we're not converting to local coordinate.  The popup is not
    // currently parented to the TextBox due to bug...  If we do change the
    // parenting once that bug is fixed, we will probably need to convert to
    // a local coordinate here.
    UpdateCenterLocalCoordinate(centerScaledCoordinate);

    return S_OK;
}

XRECT_WH CRichEditGripperChild::GetGripperScreenRect()
{
    float fDescent = static_cast<float>(c_nGripperDescent - c_nGripperWidth);
    XRECT_WH prect;
    prect.X = (XINT32)(m_centerWorldCoordinate.x - (c_rPaddingDiameter / 2.0f));
    prect.Width = (XINT32)c_rPaddingDiameter;
    prect.Y = (XINT32)(m_centerWorldCoordinate.y - (c_rPaddingDiameter / 2.0f) - m_rPoleHeight - fDescent);
    prect.Height = (XINT32)(c_rPaddingDiameter + m_rPoleHeight + fDescent);
    return prect;

}

_Check_return_ HRESULT CRichEditGripperChild::UpdateThemeColor()
{
    HRESULT hr = S_OK;
    CSolidColorBrush *pGripperDispBrush = NULL;
    CValue v;

    IFC(GetContext()->GetSystemTextSelectionBackgroundBrush(&pGripperDispBrush));
    v.SetObjectNoRef(pGripperDispBrush);

    if (m_pEllipseInternalNoAddRef != NULL)
    {
        IFC(m_pEllipseInternalNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, v));
    }

    if (m_pRectanglePoleNoAddRef != NULL)
    {
        IFC(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, v));

        IFC(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Stroke, v));
    }
    if (m_pRectPcpPoleNoAddRef != NULL)
    {
        IFC(m_pRectPcpPoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, v));
    }

Cleanup:
    SetContentDirty(DirtyFlags::Render); //Set content dirty here to avoid 'double' rendering checks on theme change.
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the position of the gripper. Coordinate is relative to
//      parent UIElement.
//
//------------------------------------------------------------------------
void CRichEditGripperChild::UpdateCenterLocalCoordinate(
    _In_ const XPOINTF& centerLocalCoordinate
    )
{
    if (m_centerLocalCoordinate.x != centerLocalCoordinate.x || m_centerLocalCoordinate.y != centerLocalCoordinate.y)
    {
        m_centerLocalCoordinate.x = centerLocalCoordinate.x;
        m_centerLocalCoordinate.y = centerLocalCoordinate.y;

        SetContentDirty(DirtyFlags::Bounds);
        CUIElement::NWSetContentDirty(this, DirtyFlags::Bounds);
    }
    SetContentDirty(DirtyFlags::Render);
}

void CRichEditGripperChild::SetContentDirty(_In_ DirtyFlags flag)
{
    CUIElement::NWSetContentDirty(this, flag);
    CUIElement::NWSetContentDirty(m_pEllipsePaddingNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pEllipseInternalNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pRectanglePoleNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pRectHitTargetNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pRectPcpPoleNoAddRef, flag);
    CUIElement::NWSetContentDirty(m_pRectPcpPoleBgNoAddRef, flag);
}

_Check_return_ XFLOAT CRichEditGripperChild::GetActualOffsetX()
{
    return m_centerLocalCoordinate.x - (c_rPaddingDiameter / 2.0f);
}

_Check_return_ XFLOAT CRichEditGripperChild::GetActualOffsetY()
{
    return m_centerLocalCoordinate.y;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CRichEditGripperChild::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    pBounds->left   = 0.0f;
    pBounds->top    = 0.0f;
    pBounds->right  = 0.0f;
    pBounds->bottom = 0.0f;

    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::ShowPole(RichEditGripperCommon::PoleMode ePoleMode)
{
    CValue val;

    if (ePoleMode != m_ePoleMode)
    {
        // First, hide any pole that's currently visible (know we need to hide because new mode is different)
        switch (m_ePoleMode)
        {
        case RichEditGripperCommon::PoleMode::Selection:
            // hide selection pole
            val.SetFloat(0.0f);
            IFC_RETURN(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            break;

        case RichEditGripperCommon::PoleMode::Pcp:
            // hide super-sized PCP pole
            val.SetFloat(0.0f);
            IFC_RETURN(m_pRectPcpPoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            IFC_RETURN(m_pRectPcpPoleBgNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            break;
        }

        // Second, show the correct pole for the new mode
        switch (ePoleMode)
        {
        case RichEditGripperCommon::PoleMode::Selection:
            // show selection-mode pole
            val.SetFloat(c_rPoleThickness);
            IFC_RETURN(m_pRectanglePoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            break;

        case RichEditGripperCommon::PoleMode::Pcp:
            // show super-sized PCP-mode pole
            val.SetFloat(c_rPcpPoleWidth - (c_rPcpPoleBorder * 2.0f));
            IFC_RETURN(m_pRectPcpPoleNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            val.SetFloat(c_rPcpPoleWidth);
            IFC_RETURN(m_pRectPcpPoleBgNoAddRef->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, val));
            break;
        }

        // Update our state to the new mode
        m_ePoleMode = ePoleMode;
    }

    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::Show(bool bForSelection)
{
    CValue val;
    DirectUI::Visibility visibility = DirectUI::Visibility::Visible; // visibility for the entire gripper
    RichEditGripperCommon::PoleMode ePoleMode = RichEditGripperCommon::PoleMode::Hidden;

    // Logic to tell whether and how the gripper pole should be shown
    if (bForSelection)
    {
        if (!m_bPoleOutOfBounds)
        {
            ePoleMode = RichEditGripperCommon::PoleMode::Selection;
        }
    }
    else
    {
        if (m_bActiveDrag)
        {
            ePoleMode = RichEditGripperCommon::PoleMode::Pcp;
        }
    }

    IFC_RETURN(ShowPole(ePoleMode));

    val.Set(visibility);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, val));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::Hide()
{
    DirectUI::Visibility visibility = DirectUI::Visibility::Collapsed;
    CValue val;
    bool bCaptureReleased = false;

    INT32 pointerId;
    IFC_RETURN(m_pRectHitTargetNoAddRef->ReleaseTouchRectPointerCapture(&bCaptureReleased, &pointerId));

    if (bCaptureReleased)
    {
        // If the gripper being hidden had the pointer capture, that means it
        // might not release the capture in the normal way (through
        // CTouchableRectangle::OnPointerReleased).  That can leave us
        // in a bad state, so simulate the end of the drag manipulation here.
        IFC_RETURN(OnManipulation(
            false,
            true, // bEnd
            static_cast<XINT32>(m_centerWorldCoordinate.x),
            static_cast<XINT32>(m_centerWorldCoordinate.y),
            pointerId));
    }

    val.Set(visibility);
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, val));

    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::OnHitTargetTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pTextBoxBase->OnGripperTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::OnHitTargetRightTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pTextBoxBase->OnGripperRightTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::OnHitTargetDoubleTapped(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pTextBoxBase->OnGripperDoubleTapped(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::OnPointerPressedWithBarrelButtonDown(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(m_pTextBoxBase->OnGripperPressedWithBarrelButtonDown(pEventArgs));
    return S_OK;
}

_Check_return_ HRESULT CRichEditGripperChild::OnManipulation(
    bool begin,
    bool end,
    XINT32 x0,
    XINT32 y0,
    INT32 pointerId)
{
    SELHANDLETOUCHOPPARAMS params;
    LRESULT lres = 0;

    XPOINTF pointF = { static_cast<float>(x0), static_cast<float>(y0) };
    IFC_RETURN(m_pTextBoxBase->ClientToTextBox(&pointF));

    params.pt.x = static_cast<LONG>(pointF.x);
    params.pt.y = static_cast<LONG>(pointF.y);

    if (begin)
    {
        m_ptLastManipClient.x = params.pt.x;
        m_ptLastManipClient.y = params.pt.y;

        if (!m_bActiveDrag)
        {
            if (m_pPeerGripperNoAddRef->m_pPopupChild->InActiveDrag()) // this can happen when PointerPressed on peer gripper. In that case, clear active flag and pointer capture
            {
                CTouchableRectangle *pPeerHitTarget = m_pPeerGripperNoAddRef->GetHitTarget();
                IFC_RETURN(pPeerHitTarget->ReleaseTouchRectPointerCapture(nullptr,nullptr));
                m_pPeerGripperNoAddRef->m_pPopupChild->SetInActiveDrag(FALSE);
            }
            else
            {
                IFC_RETURN(m_pTextBoxBase->SetGripperBeingManipulated(true));
            }

            SetInActiveDrag(TRUE);
        }
    }

    if (params.pt.x - m_ptLastManipClient.x != 0 ||
        params.pt.y - m_ptLastManipClient.y != 0 ||
        begin || end)
    {
        params.pointerId = pointerId;

        // Hook up XAML's gripper with RichEdit's touch.cpp by sending EM_SELHANDLETOUCHOP
        params.id = m_gripperType == RichEditGripperCommon::GripperType::End ? ITouchSelectionHandlesHost::thEnd : ITouchSelectionHandlesHost::thStart;
        params.op = ITouchSelectionHandlesHost::toDrag;
        if (end)
        {
            // End of active drag will cause RichEdit to hide gripper inside TxSendMessage call below, need to set m_bActiveDrag to FALSE here otherwise hiding will be ignored.
            SetInActiveDrag(FALSE);
            params.op |= ITouchSelectionHandlesHost::TouchInputType::toEnd;
        }

        if (begin)
        {
            params.op |= ITouchSelectionHandlesHost::TouchInputType::toStart;
        }

        IFC_RETURN(m_pTextServices->TxSendMessage(EM_SELHANDLETOUCHOP, (WPARAM)&params, NULL, &lres));
        m_ptLastManipClient.x = params.pt.x;
        m_ptLastManipClient.y = params.pt.y;
    }

    if (end)
    {
        IFC_RETURN(m_pTextBoxBase->SetGripperBeingManipulated(false));
    }

    return S_OK;
}

void
CRichEditGripperChild::SetPeer(
    _In_ CRichEditGripper *pPeerGripper)
{
    m_pPeerGripperNoAddRef = pPeerGripper;
}

CRichEditGripper* CRichEditGripperChild::GetPeerNoRef()
{
    return m_pPeerGripperNoAddRef;
}

XPOINTF CRichEditGripperChild::GetPosition()
{
    return m_centerWorldCoordinate;
}

CTouchableRectangle* CRichEditGripperChild::GetHitTarget()
{
    return m_pRectHitTargetNoAddRef;
}

_Check_return_ bool CRichEditGripperChild::HasTextSelection()
{
    if (m_pTextBoxBase)
    {
        return m_pTextBoxBase->HasSelection();
    }

    return false;
}

_Check_return_ HRESULT  CRichEditGripperChild::OnGripperHeld(_In_ CEventArgs* pEventArgs)
{
    if (m_pTextBoxBase)
    {
        IFC_RETURN(m_pTextBoxBase->OnGripperHeld(pEventArgs));
    }
    return S_OK;
}


