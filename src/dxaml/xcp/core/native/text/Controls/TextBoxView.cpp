// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextBoxView.h"
#include "TextBoxBase.h"
#include "HWRenderTarget.h"
#include "TextBoxHelpers.h"
#include "WinTextCore.h"
#include "Real.h"
#include "TextSelectionSettings.h"
#include "UcdProperties.h"
#include "isapipresent.h"
#include "Storyboard.h"
#include "Timer.h"
#include "TimeSpan.h"
#include <RuntimeEnabledFeatures.h>
#include <DependencyLocator.h>
#include <ContentRenderer.h>
#include "application.h"
#include <XamlOneCoreTransforms.h>
#include <DoubleUtil.h>

using namespace RuntimeFeatureBehavior;

namespace TextBoxView_Internal{

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets RGB color from CBrush.
//
//---------------------------------------------------------------------------
inline XUINT32 BrushToRGB(
    _In_ CBrush *pBrush
    )
{
    CSolidColorBrush *pSCB = static_cast<CSolidColorBrush *>(pBrush);
    return (RGB((pSCB->m_rgb >> 16) & 0xff, (pSCB->m_rgb >> 8) & 0xff, pSCB->m_rgb & 0xff));
}

inline BYTE BrushToAlpha(
    _In_ CBrush *pBrush
)
{
    CSolidColorBrush *pSCB = static_cast<CSolidColorBrush *>(pBrush);
    return (pSCB->m_rgb >> 24); // m_rgb is actually argb
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a Size in pixels to himetirc (100th of a millimeter).
//
//  Notes:
//      Although this function is logically part of CTextBoxView, it's
//      defined here to keep SIZE windows type dependency out of CTextBoxView.h.
//
//---------------------------------------------------------------------------
void DPtoHIMETRIC(
    _In_ HDC hdc,
    _Inout_ SIZE *pSize
    )
{
    const XINT32 HIMETRIC_PER_INCH = 2540;
    XINT32 nMapMode = IsGetMapModePresent() ? ::GetMapMode(hdc) : MM_TEXT;
    if (nMapMode < MM_ISOTROPIC && nMapMode != MM_TEXT)
    {
        XamlOneCoreTransforms::FailFastIfEnabled();

        // When using a constrained map mode, map against physical inch
        SetMapMode(hdc, MM_HIMETRIC);
        DPtoLP(hdc, reinterpret_cast<POINT*>(pSize), 2);
        SetMapMode(hdc, nMapMode);
    }
    else
    {
        // Map against logical inch for non-constrained mapping modes
        // We should not pass the plateau scale to RichEdit because
        // XAML handles scale by setting transform on the root element in the VisualTree.
        pSize->cx = MulDiv(pSize->cx, HIMETRIC_PER_INCH, 96);
        pSize->cy = MulDiv(pSize->cy, HIMETRIC_PER_INCH, 96);
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Converts an XFLOAT constraint to a LONG value.
//
//  Notes:
//      This method clamps the original constraint to avoid overflow errors
//      when converting to himetric values.  In practice this means converting
//      XFLOAT_INF to some large constant.
//
//---------------------------------------------------------------------------
LONG ConstraintToLong(_In_ XFLOAT floatValue)
{
    LONG value;
    const XFLOAT maxValue = 100000; // Close to infinity but without overflowing when converting to
                                    // himetric.  At 96 dpi, this allows more than 86 feet of display
                                    // before we wrap.

    if (floatValue > maxValue)
    {
        value = static_cast<LONG>(maxValue);
    }
    else if (floatValue < -maxValue)
    {
        value = static_cast<LONG>(-maxValue);
    }
    else
    {
        value = static_cast<LONG>(floatValue);
    }

    return value;
}
} // Anonymous namespace

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Initializes a new instance.
//
//---------------------------------------------------------------------------
CTextBoxView::CTextBoxView(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
    , m_pLastTransformToRoot(NULL)
    , m_foregroundAlpha(0xFF)
    , m_fShowGrippersOnGotFocus(false)
    , m_showGrippersOnCMDismiss(false)
{
    m_pTextControl = NULL;
    m_pRenderTarget = NULL;
    m_eMouseCursor = MouseCursorIBeam;

    m_caretRect.X = 0;
    m_caretRect.Y = 0;
    m_caretRect.Width = 0;
    m_caretRect.Height = 0;
    m_requestedSize.width = 0;
    m_requestedSize.height = 0;

    EmptyRect(&m_invalidRect);
    m_forceRedraw = FALSE;

    memset(&m_scrollData, 0, sizeof(m_scrollData));

    m_pixelSnapped = false;

    m_directManipulationOffset.x = 0;
    m_directManipulationOffset.y = 0;

    m_rCaretBlinkingPeriod = 1.0f; // Init with a safe value
    m_fCaretTimeoutEnabled = FALSE;
    m_fInheritedPropertiesDirty = FALSE;
    m_isDMActive = FALSE;
    m_canShowCaret = FALSE;
    m_isMeasuring = FALSE;
    m_isRendering = FALSE;
    m_fShowGrippersOnAnimationComplete = FALSE;
    m_fShowGrippersOnSelectionChanged = FALSE;
    m_lastViewportOffset.x = m_lastViewportOffset.y = 0.0f;
    EmptyRect(&m_lastSelectionRect);
    m_resetCharFormatingMask = m_resetParaFormatingMask = FALSE;
    m_issueDispatcherViewInvalidation = FALSE;
    m_fBlinkPausedForGripper = FALSE;
    m_fLockSetScrollOffsets = FALSE;
    m_shouldShowRTLIndicator = FALSE;
    m_fScrollOffsetsChangedByDraw = FALSE;
    m_highContrastAdjustment = Convert(DirectUI::ElementHighContrastAdjustment::None);
    m_applicationHighContrastAdjustment = Convert(DirectUI::ApplicationHighContrastAdjustment::Auto);

    InitializeDefaultCharFormat();
    InitializeDefaultParagraphFormat();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Releases resources held by this class.
//
//---------------------------------------------------------------------------
CTextBoxView::~CTextBoxView()
{
    ReleaseInterface(m_pRenderTarget);
    SAFE_DELETE(m_pLastTransformToRoot);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Allocates and initializes a new instance.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_ CREATEPARAMETERS *pCreate
    )
{
    HRESULT hr = S_OK;
    CTextBoxView *pTextBoxView = new CTextBoxView(pCreate->m_pCore);


    *ppObject = pTextBoxView;
    RRETURN(hr);//RRETURN_REMOVAL
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the index of CTextBoxView class.
//
//---------------------------------------------------------------------------
KnownTypeIndex CTextBoxView::GetTypeIndex() const
{
    return DependencyObjectTraits<CTextBoxView>::Index;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Instructs the system that this element is always LTR.
//
//---------------------------------------------------------------------------
bool CTextBoxView::IsRightToLeft()
{
    // Hard code FlowDirection to LTR.
    // We do this because RichEdit doesn't expect mirroring and has its own logic to handle
    // RTL content.
    return false;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pulls all non-locally set inherited properties from the parent.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::PullInheritedTextFormatting()
{
    XUINT32 charFormatMask = 0;
    XUINT32 paraFormatMask = 0;

    if (m_pTextFormatting && m_pTextFormatting->IsOld())
    {
        // Get the parent properties that we will be inheriting from.
        xref_ptr<TextFormatting> pParentTextFormatting;
        IFC_RETURN(GetParentTextFormatting(pParentTextFormatting.ReleaseAndGetAddressOf()));

        if (m_pTextFormatting->m_pFontFamily != pParentTextFormatting->m_pFontFamily)
        {
            IFC_RETURN(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
            charFormatMask |= CFM_FACE;
        }

        // If BackPlate is active override text foreground to ensure high contrast with BackPlate.
        if (IsHighContrastAdjustmentActive() && IsEnabled())
        {
            CSolidColorBrush* pAlternativeForegroundBrush = m_pTextControl->GetContext()->GetSystemColorWindowTextBrushNoRef();

            if (TextBoxView_Internal::BrushToRGB(pAlternativeForegroundBrush) != m_charFormat.TextColor)
            {
                if (!m_pTextFormatting->m_freezeForeground)
                {
                    IFC_RETURN(m_pTextFormatting->SetForeground(this, pAlternativeForegroundBrush));
                }
                charFormatMask |= CFM_COLOR;
            }
        }
        else
        {
            if (pParentTextFormatting->m_pForeground != nullptr &&
                pParentTextFormatting->m_pForeground->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
            {
                // Update RichEdit only if the actual color changed
                if (   TextBoxView_Internal::BrushToRGB(pParentTextFormatting->m_pForeground) != m_charFormat.TextColor
                    || TextBoxView_Internal::BrushToAlpha(pParentTextFormatting->m_pForeground) != m_foregroundAlpha)
                {
                    if (!m_pTextFormatting->m_freezeForeground)
                    {
                        IFC_RETURN(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
                    }
                    charFormatMask |= CFM_COLOR;
                }
            }
        }

        if (!m_pTextFormatting->m_strLanguageString.Equals(pParentTextFormatting->m_strLanguageString))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            charFormatMask |= CFM_LCID;
        }

        if (!m_pTextFormatting->GetResolvedLanguageListStringNoRef().Equals(pParentTextFormatting->GetResolvedLanguageListStringNoRef()))
        {
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
            charFormatMask |= CFM_LCID;
        }

        if (m_pTextFormatting->m_eFontSize != pParentTextFormatting->m_eFontSize)
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
            charFormatMask |= CFM_SIZE;
        }

        if (m_pTextFormatting->m_nFontWeight != pParentTextFormatting->m_nFontWeight)
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
            charFormatMask |= CFM_WEIGHT;
        }

        if (m_pTextFormatting->m_nFontStyle != pParentTextFormatting->m_nFontStyle)
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
            charFormatMask |= CFM_ITALIC;
        }

        m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;

        if (m_pTextFormatting->m_nCharacterSpacing != pParentTextFormatting->m_nCharacterSpacing)
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
            charFormatMask |= CFM_SPACING;
        }

        if (m_pTextFormatting->m_nFlowDirection != pParentTextFormatting->m_nFlowDirection)
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;

            // RichEdit's model leaves alignment independent of FlowDirection, so we need to
            // additionally flip the alignment manually when FlowDirection changes.
            paraFormatMask |= (PFM_RTLPARA | PFM_ALIGNMENT);

            // Invalidate ScrollInfo to keep the scrollbars in sync with text content.
            IFC_RETURN(FxCallbacks::TextBox_InvalidateScrollInfo(this));
        }

        if (m_pTextFormatting->m_isTextScaleFactorEnabled != pParentTextFormatting->m_isTextScaleFactorEnabled)
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = pParentTextFormatting->m_isTextScaleFactorEnabled;
            charFormatMask |= (CFM_SIZE | CFM_SPACING);
        }

        m_pTextFormatting->SetIsUpToDate();

        if (charFormatMask)
        {
            IFC_RETURN(UpdateDefaultCharFormat(charFormatMask));
        }

        if (paraFormatMask)
        {
            IFC_RETURN(UpdateDefaultParagraphFormat(paraFormatMask));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      We need to inform RichEdit of a property change. We don't want to do that
//      synchronously here since pulling down inherited properties every time
//      one property changes is expensive. Schedule an internal event to do that work.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::MarkInheritedPropertyDirty(
    _In_ const CDependencyProperty* pdp,
    _In_ const CValue* pValue
    )
{

    IFC_RETURN(CFrameworkElement::MarkInheritedPropertyDirty(pdp, pValue));

    // Schedule a callback to invalidate RichEdit default formats.
    // It is inefficient to do it for each property, so this callback (OnInheritedPropertyChanged)
    // enables batching of all properties into a single RichEdit invalidation.
    if (!m_fInheritedPropertiesDirty)
    {
        if (m_pTextControl != NULL)
        {
            CEventManager *pEventManager = GetContext()->GetEventManager();
            IFCPTR_RETURN(pEventManager);
            xref_ptr<CEventArgs> pArgs;
            pArgs.attach(new CEventArgs());
            pEventManager->Raise(EventHandle(KnownEventIndex::Control_InheritedPropertyChanged), TRUE, m_pTextControl, pArgs);
        }
        m_fInheritedPropertiesDirty = TRUE;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Invalidates the FontSize property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::InvalidateFontSize()
{
    const CDependencyProperty *pFontSizeProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Control_FontSize);
    CValue value;

    // Font size affects both size and spacing, so we'll want to
    // update both of those.
    IFC_RETURN(UpdateDefaultCharFormat(CFM_SIZE | CFM_SPACING));

    m_pTextControl->InvalidateViewAndForceRedraw();

    IFC_RETURN(GetValueInherited(pFontSizeProperty, &value));
    IFC_RETURN(MarkInheritedPropertyDirty(pFontSizeProperty, &value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Async event callback indicating we have pending inherited property changes
//      to propagate to RichEdit.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::OnInheritedPropertyChanged()
{
    IFC_RETURN(EnsureTextFormattingForRead());
    m_fInheritedPropertiesDirty = FALSE;

    // This dispatcher callback might be called after Measure/Render caused by DependencyProperty change
    // has been already processed. In this situation we are going to miss the chance to update layout/rendering data.
    // Invalidate Measure/Render to force the proper update.
    InvalidateMeasure();
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepares default character formatting for content.
//
//  Notes:
//      1. Sets flags on the default character formatting RichEdit reads to determine which
//         fields contain set values. These flags are set here and never change.
//
//      2. Sets values on fields that are immutable (as far as this control is concerned),
//         such as CFM_CHARSET.  Mutable values are set later via calls to
//         UpdateDefaultCharFormat.  We can't do that now because this method is called from the
//         ctor and default values in the property system have not yet been set.
//
//------------------------------------------------------------------------
void CTextBoxView::InitializeDefaultCharFormat()
{
    memset(&m_charFormat, 0, sizeof(m_charFormat));
    m_charFormat.Size = sizeof(m_charFormat);
    m_charFormat.Mask |= CFM_CHARSET;
    m_charFormat.CharSet = DEFAULT_CHARSET;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prepares default block formatting for content.
//
//
//  Notes:
//      1. Sets flags on the default character formatting RichEdit reads to determine which
//         fields contain set values. These flags are set here and never change.
//
//      2. Sets values on fields that are immutable (as far as this control is concerned),
//         such as TabCount.  Mutable values are set later via calls to
//         UpdateDefaultParagraphFormat.  We can't do that now because this method is called from the
//         ctor and default values in the property system have not yet been set.
//
//------------------------------------------------------------------------
void CTextBoxView::InitializeDefaultParagraphFormat()
{
    memset(&m_paragraphFormat, 0, sizeof(m_paragraphFormat));

    m_paragraphFormat.Size = sizeof(m_paragraphFormat);
    m_paragraphFormat.Mask = PFM_TABSTOPS;
    m_paragraphFormat.TabCount = 1;
    m_paragraphFormat.Tabs[0] = lDefaultTab; // A constant in RichEdit.h, despite appearances.
}

_Check_return_ HRESULT CTextBoxView::OnBidiOptionsChanged()
{
    PARAFORMAT2 pParaFormat;
    ZeroMemory(&pParaFormat, sizeof(pParaFormat));
    pParaFormat.cbSize = sizeof(pParaFormat);

    BIDIOPTIONS bidiOptions;
    ZeroMemory(&bidiOptions, sizeof(bidiOptions));
    bidiOptions.cbSize = sizeof(BIDIOPTIONS);

    bidiOptions.wMask = BOM_CONTEXTREADING | BOM_CONTEXTALIGNMENT | BOE_FORCERECALC;
    bidiOptions.wEffects = BOE_FORCERECALC;

    const TextFormatting* pTextFormatting = NULL;
    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    DirectUI::TextAlignment alignment;
    IFC_RETURN(m_pTextControl->GetAlignment(&alignment));
    if (alignment == DirectUI::TextAlignment::DetectFromContent)
    {
        bidiOptions.wEffects |= BOE_CONTEXTALIGNMENT;
    }
    else
    {
        pParaFormat.dwMask = PFM_ALIGNMENT;
        pParaFormat.wAlignment  = TextAlignmentToParagraphAlignment(alignment, pTextFormatting->m_nFlowDirection);
    }

    if (m_pTextControl->GetTextReadingOrder() == DirectUI::TextReadingOrder::DetectFromContent)
    {
        bidiOptions.wEffects |= BOE_CONTEXTREADING;
    }
    else
    {
        pParaFormat.dwMask |= PFM_RTLPARA;
        if (pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
        {
            pParaFormat.wEffects |= PFE_RTLPARA;
        }
        else
        {
            pParaFormat.wEffects &= ~PFE_RTLPARA;
        }
    }

    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_SETBIDIOPTIONS, 0, (LPARAM)&bidiOptions, nullptr));

    if (pParaFormat.dwMask)
    {
       IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_SETPARAFORMAT, SPF_SETDEFAULT, (LPARAM)&pParaFormat, nullptr));
       // we have to send BIDI options again due to RichEdit limitation: context reading/alignment can be overwritten after EM_SETPARAFORMAT
       IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_SETBIDIOPTIONS, 0, (LPARAM)&bidiOptions, nullptr));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Reevaluates character formatting properties and notifies RichEdit
//      about any changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::UpdateDefaultCharFormat(
    _In_ XUINT32 mask
        // Mask of property values to evaluate, RichEdit CFM_* values.
    )
{
    const TextFormatting* pTextFormatting = NULL;

    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    if (m_resetCharFormatingMask == TRUE)
    {
        m_charFormat.Mask = 0;
        m_resetCharFormatingMask = FALSE;
    }
    //
    // FontFamily
    //

    // We intentionally ignore CFM_FACE here because RichEdit will callback into IProvideFontInfo
    // methods to retrieve the font face name.  It is important though that we call OnTxPropertyBitsChange
    // below with CFM_FACE.
    if (mask & CFM_FACE)
    {
        m_charFormat.Mask |= CFM_FACE;
        m_charFormat.Mask |= CFM_CHARSET;
        m_charFormat.CharSet = DEFAULT_CHARSET;
    }

    //
    // FontSize
    //

    if (mask & CFM_SIZE)
    {
        m_charFormat.Mask |= CFM_SIZE;

        // Value is in DIPs (1/96 inch), RichEdit wants twips (1/20 point).
        const INT32 TwipsPerPoint = 20;
        m_charFormat.Height = static_cast<XINT32>(pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale()) * 72 / 96 * TwipsPerPoint);

        // RichEdit only supports subpixel rendering (ideal mode) up to 130 pts, we need to turn on/off subpixel rendering by fliping TXTBIT_D2DPIXELSNAPPED bit on/off
        m_pixelSnapped = (m_charFormat.Height > 130 * TwipsPerPoint);
        if (m_pTextControl->GetTextServices() != nullptr)
        {
            IFC_RETURN(m_pTextControl->GetTextServices()->OnTxPropertyBitsChange(TXTBIT_D2DPIXELSNAPPED, m_pixelSnapped ? TXTBIT_D2DPIXELSNAPPED : 0));
        }
    }

    //
    // Foreground
    //

    if (mask & CFM_COLOR)
    {
        m_charFormat.Mask |= CFM_COLOR;

        // RichEdit supports only solid color brushes
        XUINT32 color = 0xFF000000;
        if (pTextFormatting->m_pForeground != NULL &&
            pTextFormatting->m_pForeground->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
        {
            color = TextBoxView_Internal::BrushToRGB(pTextFormatting->m_pForeground);
            m_foregroundAlpha = TextBoxView_Internal::BrushToAlpha(pTextFormatting->m_pForeground);
            if (m_pRenderTarget != nullptr)
            {
                m_pRenderTarget->SetForegroundAlpha(m_foregroundAlpha);
            }
        }
        m_charFormat.TextColor = color;
    }

    //
    // FontWeight
    //

    if (mask & CFM_WEIGHT)
    {
        m_charFormat.Mask |= CFM_WEIGHT;
        m_charFormat.Weight = static_cast<XINT16>(pTextFormatting->m_nFontWeight);
    }

    //
    // FontStyle
    //

    if (mask & CFM_ITALIC)
    {
        m_charFormat.Mask |= CFM_ITALIC;
        // RichEdit does not support StyleOblique, so treat it as StyleItalic
        m_charFormat.Effects = (m_charFormat.Effects & ~CFE_ITALIC) | ((pTextFormatting->m_nFontStyle != DirectUI::FontStyle::Normal) ? CFE_ITALIC : 0);
    }

    //
    // Spacing
    //
    if (mask & CFM_SPACING)
    {
        // Character spacing is measured in 1000ths of the font size.
        XFLOAT characterSpacingInPx = (pTextFormatting->m_nCharacterSpacing / 1000.0f) * pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale());
        m_charFormat.Spacing = (XINT16)(characterSpacingInPx * 72 / 96 * 20);
        m_charFormat.Mask |= CFM_SPACING;
    }

    //
    // Language
    //
    if (mask & CFM_LCID)
    {
        m_charFormat.Mask |= CFM_LCID;
        m_charFormat.Lcid = XStringPtrToLCID(pTextFormatting->GetResolvedLanguageStringNoRef());
    }

    //
    // Notify RichEdit.
    //
    if (m_pTextControl->GetTextServices() != NULL)
    {
        IFC_RETURN(m_pTextControl->GetTextServices()->OnTxPropertyBitsChange(TXTBIT_CHARFORMATCHANGE, TXTBIT_CHARFORMATCHANGE));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Reevaluates block formatting properties and notifies RichEdit
//      about any changes.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::UpdateDefaultParagraphFormat(
    _In_ XUINT32 mask
        // Mask of property values to evaluate, RichEdit PFM_* values.
    )
{
    const TextFormatting* pTextFormatting = NULL;
    BOOL fUpdateBidiSettingsNeeded = FALSE;

    IFC_RETURN(GetTextFormatting(&pTextFormatting));
    if (m_resetParaFormatingMask == TRUE)
    {
        m_paragraphFormat.Mask = 0;
        m_resetParaFormatingMask = FALSE;
    }

    //
    // TextAlignment
    //

    if (mask & PFM_ALIGNMENT)
    {
        DirectUI::TextAlignment alignment;
        IFC_RETURN(m_pTextControl->GetAlignment(&alignment));
        if (alignment != DirectUI::TextAlignment::DetectFromContent)
        {
            m_paragraphFormat.Alignment = TextAlignmentToParagraphAlignment(alignment, pTextFormatting->m_nFlowDirection);
            m_paragraphFormat.Mask |= PFM_ALIGNMENT;
        }
        else
        {
            m_paragraphFormat.Mask &=  ~PFM_ALIGNMENT;
            fUpdateBidiSettingsNeeded = TRUE;
        }
    }

    //
    // FlowDirection
    //

    if (mask & PFM_RTLPARA)
    {
        if (m_pTextControl->GetTextReadingOrder() != DirectUI::TextReadingOrder::DetectFromContent)
        {
            if (pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
            {
                m_paragraphFormat.Effects |= PFE_RTLPARA;
            }
            else
            {
                m_paragraphFormat.Effects &= ~PFE_RTLPARA;
            }
            m_paragraphFormat.Mask |= PFM_RTLPARA;
        }
        else
        {
            m_paragraphFormat.Mask &= ~PFM_RTLPARA;
            fUpdateBidiSettingsNeeded = TRUE;
        }
    }

    //
    // Notify RichEdit.
    //
    if (m_paragraphFormat.Mask && (m_pTextControl->GetTextServices() != NULL)) // only notify RE if Mask is set
    {
        IFC_RETURN(m_pTextControl->GetTextServices()->OnTxPropertyBitsChange(TXTBIT_PARAFORMATCHANGE, TXTBIT_PARAFORMATCHANGE));
        if (fUpdateBidiSettingsNeeded) // need to update BidiSettings with RE after paragragh settings updated for context alignment and reading
        {
            IFC_RETURN(OnBidiOptionsChanged());
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Converts a DirectUI TextAlignment into a RichEdit paragraph alignment.
//
//  Notes:
//      Because RichEdit models alignment independently of FlowDirection,
//      we must consider FlowDirection and reverse alignment appropriately.
//
//------------------------------------------------------------------------
XINT16 CTextBoxView::TextAlignmentToParagraphAlignment(
    _In_ DirectUI::TextAlignment alignment,
    _In_ DirectUI::FlowDirection flowDirection
    )
{
    XINT16 paragraphAlignment;

    switch (alignment)
    {
        case DirectUI::TextAlignment::Center:
            paragraphAlignment = PFA_CENTER;
            break;

        case DirectUI::TextAlignment::Left:
            paragraphAlignment = (DirectUI::FlowDirection::LeftToRight == flowDirection) ? PFA_LEFT : PFA_RIGHT;
            break;

        case DirectUI::TextAlignment::Right:
            paragraphAlignment = (DirectUI::FlowDirection::LeftToRight == flowDirection) ? PFA_RIGHT : PFA_LEFT;
            break;

        case DirectUI::TextAlignment::Justify:
            paragraphAlignment = PFA_JUSTIFY;
            break;

        default:
            ASSERT(FALSE);
            paragraphAlignment = PFA_LEFT;
            break;
    }

    return paragraphAlignment;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets the default character formatting.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::TxGetCharFormat(
    _Outptr_ const WCHARFORMAT **ppCharFormat
    )
{
    *ppCharFormat = &m_charFormat;
    m_resetCharFormatingMask = TRUE;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      TextServices callback, gets the default block formatting.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::TxGetParaFormat(
    _Outptr_ const XPARAFORMAT **ppParaFormat
    )
{
    *ppParaFormat = &m_paragraphFormat;
    m_resetParaFormatingMask = TRUE;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback, called when a caret is required.
//
//---------------------------------------------------------------------------
bool CTextBoxView::TxCreateCaret(
    _In_ XHANDLE bitmap,
    _In_ XINT32 width,
    _In_ XINT32 height
    )
{
    if (m_isRendering)
    {
        // If in the middle of rendering, post a dispatcher callback to change caret visibility.
        // We should not modify the tree in middle of render.
        FxCallbacks::TextBoxView_CaretVisibilityChanged(this);
        return true;
    }

    const bool oldShouldShowRTLIndicator = m_shouldShowRTLIndicator;
    m_shouldShowRTLIndicator = ((reinterpret_cast<uintptr_t>(bitmap) & CARET_RTL) == CARET_RTL);

    // Only regenerate the caret shape when a caret has already been created.
    if (m_spCaretElement != nullptr && oldShouldShowRTLIndicator != m_shouldShowRTLIndicator)
    {
        ResetCaretElement();

        EnsureCaretElement();
    }

    m_caretRect.Width = static_cast<XFLOAT>(width);
    m_caretRect.Height = static_cast<XFLOAT>(height);

    return UpdateCaretElement() == S_OK;
}

void CTextBoxView::ResetCaretElement()
{
    // Reset all caret related things, because we need a new type of caret now.
    RemoveChild(m_spCaretElement);
    m_spCaretElement.reset();
    m_spCaretTimer.reset();
    m_spCaretBlink.reset();
    m_spCaretStartTimer.reset();
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback, called when caret position changes.
//
//---------------------------------------------------------------------------
bool CTextBoxView::TxSetCaretPos(
    _In_ XINT32 x,
    _In_ XINT32 y
    )
{
    m_caretRect.X = static_cast<XFLOAT>(x);
    m_caretRect.Y = static_cast<XFLOAT>(y);

    return UpdateCaretElement() == S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback, called when caret visibility changes.
//
//---------------------------------------------------------------------------
_Check_return_ bool CTextBoxView::TxShowCaret(_In_ BOOL isCaretVisible)
{
    HRESULT hr = S_OK;

    m_canShowCaret = !!isCaretVisible;

    IFC(ShowOrHideCaret());

Cleanup:
    return (S_OK == hr);
}


//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback, queries the viewport of this control.
//
//  Notes:
//      We always tell RichEdit that the client rect is exactly as large as
//      content, ignoring the  true viewport. This lets us handle scrolling
//      and render bounds ourselves.
//
//---------------------------------------------------------------------------
HRESULT CTextBoxView::TxGetClientRect(_Out_ XRECT_RB *pClientRect)
{
    pClientRect->left = 0;
    pClientRect->top = 0;
    pClientRect->right = XcpCeiling(m_scrollData.ExtentWidth);
    pClientRect->bottom = XcpCeiling(m_scrollData.ExtentHeight);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IXamlTextHost callback, queries the viewport of this control.
//
//  Notes:
//      This is the call to report our true viewport rect. In ITextHost::TxGetClientRect,
//      we tell RichEdit that the client rect is exactly as large as
//      content, ignoring the  true viewport.
//
//---------------------------------------------------------------------------
HRESULT CTextBoxView::TxGetViewportRect(_Out_ XRECT_RB *pViewportRect)
{
    XRECTF_RB innerRect;
    XRECTF_RB outerRect;
    XPOINTF topLeft;
    XPOINTF bottomRight;

    innerRect.left = 0.0f;
    innerRect.top = 0.0f;
    innerRect.right = m_scrollData.ViewportWidth;
    innerRect.bottom = m_scrollData.ViewportHeight;

    innerRect.left += static_cast<float>(m_scrollData.HorizontalOffset);
    innerRect.top += static_cast<float>(m_scrollData.VerticalOffset);
    innerRect.right += static_cast<float>(m_scrollData.HorizontalOffset);
    innerRect.bottom += static_cast<float>(m_scrollData.VerticalOffset);

    // Applies all the parent clips and returns the clipped rect in world coordinates.
    IFC_RETURN(TransformToWorldSpace(&innerRect, &outerRect, false /* ignoreClipping */, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));

    if (outerRect.left == outerRect.right && outerRect.top == outerRect.bottom)
    {
        innerRect.left   = 0;
        innerRect.top    = 0;
        innerRect.right  = 0;
        innerRect.bottom = 0;
    }
    else
    {
        topLeft.x = outerRect.left;
        topLeft.y = outerRect.top;
        bottomRight.x = outerRect.right;
        bottomRight.y = outerRect.bottom;

        if (!XamlOneCoreTransforms::IsEnabled())
        {
            const float scale = RootScale::GetRasterizationScaleForElement(this);
            topLeft = topLeft / scale;
            bottomRight = bottomRight / scale;
        }
        IFC_RETURN(m_pTextControl->ClientToTextBox(&topLeft));
        IFC_RETURN(m_pTextControl->ClientToTextBox(&bottomRight));

        innerRect.left = topLeft.x;
        innerRect.top = topLeft.y;
        innerRect.right = bottomRight.x;
        innerRect.bottom = bottomRight.y;
    }

    pViewportRect->left = XcpFloor(innerRect.left);
    pViewportRect->top = XcpFloor(innerRect.top);
    pViewportRect->right = XcpCeiling(innerRect.right);
    pViewportRect->bottom = XcpCeiling(innerRect.bottom);

    return S_OK;
}

HRESULT CTextBoxView::TxGetContentPadding(_Out_ XRECT_RB *pContentPadding)
{
    CUIElement *pBaseElement;
    XPOINTF basePoints[2];
    xref_ptr<CDependencyObject> borderDO = m_pTextControl->GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"BorderElement"));
    if (auto border = do_pointer_cast<CBorder>(borderDO))
    {
        auto thickness = border->GetBorderThickness();
        basePoints[0].x = thickness.left;
        basePoints[0].y = thickness.top;
        basePoints[1].x = border->GetActualWidth() - thickness.right;
        basePoints[1].y = border->GetActualHeight() - thickness.bottom;
        pBaseElement = border;
    }
    else
    {
        basePoints[0].x = 0;
        basePoints[0].y = 0;
        basePoints[1].x = m_pTextControl->GetActualWidth();
        basePoints[1].y = m_pTextControl->GetActualHeight();
        pBaseElement = m_pTextControl;
    }

    XRECT_RB viewport;
    XPOINTF viewPoints[2] = { 0 };
    xref_ptr<CGeneralTransform> transform;
    IFC_RETURN(pBaseElement->TransformToVisual(this, &transform));
    IFC_RETURN(transform->TransformPoints(basePoints, viewPoints, 2));
    IFC_RETURN(TxGetViewportRect(&viewport));

    if (m_pTextFormatting && m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
    {
        pContentPadding->left = viewport.left - XcpCeiling(viewPoints[1].x);
        pContentPadding->right = XcpCeiling(viewPoints[0].x) - viewport.right;
    }
    else
    {
        pContentPadding->left = viewport.left - XcpCeiling(viewPoints[0].x);
        pContentPadding->right = XcpCeiling(viewPoints[1].x) - viewport.right;
    }
    pContentPadding->top = viewport.top - XcpCeiling(viewPoints[0].y);
    pContentPadding->bottom = XcpCeiling(viewPoints[1].y) - viewport.bottom;
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      ITextServices callback, called when a region is invalidated.
//
//---------------------------------------------------------------------------
void CTextBoxView::TxInvalidateRect(
    _In_ const XRECT_RB *pRect,
    _In_ BOOL eraseBackground
    )
{
    // Accumulate dirty regions for partial viewport update.
    if (pRect != NULL)
    {
        UnionRect(&m_invalidRect, pRect);
    }
    else
    {
        m_forceRedraw = TRUE;
    }

    if (!m_isRendering)
    {
        CUIElement::NWSetContentAndBoundsDirty(this, DirtyFlags::None);
    }
    else // don't set dirty flag inside rendering, instead call InvalidateView which will post async call
    {
        InvalidateView();
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Fx pinvoke handler, returns extent, viewport, and scrollbar offsets
//      required by the fx ScrollViewer.
//
//---------------------------------------------------------------------------
const CTextBoxView_ScrollData *CTextBoxView::GetScrollData() const
{
    return &m_scrollData;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Fx pinvoke handler, enables or disables scrolling.
//
//---------------------------------------------------------------------------
void CTextBoxView::SetScrollEnabled(
    _In_ bool canHorizontallyScroll,
    _In_ bool canVerticallyScroll
    )
{
    m_scrollData.CanHorizontallyScroll = canHorizontallyScroll;
    m_scrollData.CanVerticallyScroll = canVerticallyScroll;
}

// These macros compute how many integral pixels need to be scrolled based on the viewport size and mouse wheel delta.
// - First the maximum between 48 and 15% of the viewport size is picked.
// - Then that number is multiplied by (mouse wheel delta/120), 120 being the universal default value.
// - Finally if the resulting number is larger than the viewport size, then that viewport size is picked instead.
#define GetVerticalScrollWheelDelta(size, delta)   ((XFLOAT)(MIN(XcpFloor(size.height), XcpRound(delta * MAX(48.0, XcpRound(size.height * 0.15)) / 120.0))))
#define GetHorizontalScrollWheelDelta(size, delta) ((XFLOAT)(MIN(XcpFloor(size.width),  XcpRound(delta * MAX(48.0, XcpRound(size.width  * 0.15)) / 120.0))))

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Fx pinvoke handler, scrolls the viewport by a particular unit in a
//      particular direction.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::Scroll(
    _In_ CTextBoxView_ScrollCommand command,
    _In_ bool moveCaret,
    _In_ bool expandSelection,
    _In_ XUINT32 mouseWheelDelta,
    _Out_opt_ bool *pScrolled)
{
    const XFLOAT ScrollViewerLineDelta = 16;       // Equals default ScrollViewer delta.
    XFLOAT xDelta = 0;
    XFLOAT yDelta = 0;
    XDOUBLE prevHorizontalOffset = m_scrollData.HorizontalOffset;
    XDOUBLE prevVerticalOffset   = m_scrollData.VerticalOffset;

    switch (command)
    {
        case CTextBoxView_ScrollCommand::LineUp:
            yDelta = -ScrollViewerLineDelta;
            break;

        case CTextBoxView_ScrollCommand::LineDown:
            yDelta = ScrollViewerLineDelta;
            break;

        case CTextBoxView_ScrollCommand::LineLeft:
            xDelta = -ScrollViewerLineDelta;
            break;

        case CTextBoxView_ScrollCommand::LineRight:
            xDelta = ScrollViewerLineDelta;
            break;

        case CTextBoxView_ScrollCommand::PageUp:
            yDelta = -m_scrollData.ViewportHeight;
            break;

        case CTextBoxView_ScrollCommand::PageDown:
            yDelta = m_scrollData.ViewportHeight;
            break;

        case CTextBoxView_ScrollCommand::PageLeft:
            xDelta = -m_scrollData.ViewportWidth;
            break;

        case CTextBoxView_ScrollCommand::PageRight:
            xDelta = m_scrollData.ViewportWidth;
            break;

        case CTextBoxView_ScrollCommand::MouseWheelUp:
            yDelta = -GetVerticalScrollWheelDelta(DesiredSize, mouseWheelDelta);
            break;

        case CTextBoxView_ScrollCommand::MouseWheelDown:
            yDelta = GetVerticalScrollWheelDelta(DesiredSize, mouseWheelDelta);
            break;

        case CTextBoxView_ScrollCommand::MouseWheelLeft:
            xDelta = -GetHorizontalScrollWheelDelta(DesiredSize, mouseWheelDelta);
            break;

        case CTextBoxView_ScrollCommand::MouseWheelRight:
            xDelta = GetHorizontalScrollWheelDelta(DesiredSize, mouseWheelDelta);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    if (!moveCaret)
    {
        IFC_RETURN(SetScrollOffsets(static_cast<XFLOAT>(m_scrollData.HorizontalOffset) + xDelta, static_cast<XFLOAT>(m_scrollData.VerticalOffset) + yDelta));
    }
    else
    {
        XRECTF caretRect = GetCaretRect();
        XLONG newCaretCp = 0;
        XLONG selStart = 0;
        XLONG selEnd   = 0;
        XLONG flags    = 0;

        // We add half the caret height such that the hittesting point will be at the middle of the line.
        // Without this addition a sequence of PageUp,PageDown,PageUp or PageDown,PageUp,PageDown
        // may not result in the caret ending where it started.
        caretRect.Y += yDelta + caretRect.Height / 2;
        caretRect.X += xDelta;

        wrl::ComPtr<ITextDocument2> pDocument;
        IFC_RETURN(m_pTextControl->GetDocument(&pDocument));
        wrl::ComPtr<ITextRange2> pTextRange;
        IFC_RETURN(pDocument->RangeFromPoint2(static_cast<XLONG>(caretRect.X), static_cast<XLONG>(caretRect.Y), tomClientCoord, &pTextRange));
        IFC_RETURN(pTextRange->GetStart(&newCaretCp));

        if(!expandSelection)
        {
            IFC_RETURN(pTextRange->SetEnd(newCaretCp));
            IFC_RETURN(pTextRange->Select());
        }
        else
        {
            wrl::ComPtr<ITextSelection2> pSelection;
            IFC_RETURN(pDocument->GetSelection2(&pSelection));
            IFC_RETURN(pSelection->GetStart(&selStart));
            IFC_RETURN(pSelection->GetEnd(&selEnd));
            IFC_RETURN(pSelection->GetFlags(&flags));

            if (flags & tomSelStartActive)
            {
                if (newCaretCp <= selEnd)
                {
                    IFC_RETURN(pSelection->SetStart(newCaretCp));
                }
                else
                {
                    IFC_RETURN(pSelection->SetStart(selEnd));
                    IFC_RETURN(pSelection->SetEnd(newCaretCp));
                    IFC_RETURN(pSelection->SetFlags(flags & ~tomSelStartActive));
                }
            }
            else
            {
                if (newCaretCp >= selStart)
                {
                    IFC_RETURN(pSelection->SetEnd(newCaretCp));
                }
                else
                {
                    IFC_RETURN(pSelection->SetEnd(selStart));
                    IFC_RETURN(pSelection->SetStart(newCaretCp));
                    IFC_RETURN(pSelection->SetFlags(flags | tomSelStartActive));
                }
            }
        }
    }

    if (pScrolled)
    {
        *pScrolled = (prevHorizontalOffset != m_scrollData.HorizontalOffset || prevVerticalOffset != m_scrollData.VerticalOffset);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Scrolls the content by the specified x and y pixels.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ScrollView(
    _In_ XFLOAT xDelta,
    _In_ XFLOAT yDelta)
{
    RRETURN(SetScrollOffsets(static_cast<XFLOAT>(m_scrollData.HorizontalOffset) + xDelta,
        static_cast<XFLOAT>(m_scrollData.VerticalOffset) + yDelta));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Fx pinvoke handler, scrolls the viewport to a given offset.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::SetScrollOffsets(
    _In_ XFLOAT horizontalOffset,
    _In_ XFLOAT verticalOffset
    )
{
    HRESULT hr = S_OK;

    if(m_fLockSetScrollOffsets)
    {
        goto Cleanup;
    }
    m_fLockSetScrollOffsets = TRUE;

    horizontalOffset = NormalizeScrollOffset(horizontalOffset, m_scrollData.ViewportWidth, m_scrollData.ExtentWidth);
    verticalOffset = NormalizeScrollOffset(verticalOffset, m_scrollData.ViewportHeight, m_scrollData.ExtentHeight);

    if (horizontalOffset != m_scrollData.HorizontalOffset ||
        verticalOffset != m_scrollData.VerticalOffset)
    {
        IFC(SetScrollDataOffsets(horizontalOffset, verticalOffset));

        IFC(UpdateCaretElement());

        // Scrolling invalidates rendering data for entire viewport.
        NWSetContentDirty(this, DirtyFlags::Render);
        m_forceRedraw = TRUE;
        m_fScrollOffsetsChangedByDraw = TRUE;
    }

Cleanup:
    m_fLockSetScrollOffsets = FALSE;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Scrolls a target rect into view, accounting for optional alignment
//      ratios and optional additional offset.
//      Alignment ratios are either -1 (i.e. no alignment to apply) or between
//      0 and 1. For instance when the alignment ratio is 0, the near edge of
//      the targetRect needs to align with the near edge of the viewport.
//      'offset' is an additional amount of scrolling requested, beyond the
//      normal amount to bring the target into view and potentially align it.
//      That additional offset is only applied when the targetRect does not
//      step outside the extents.
//      The 'appliedOffset' returned specifies how much of 'offset' was applied
//      so that potential parent bring-into-view contributors can attempt to
//      apply the remainder offset.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::MakeVisible(
    const XRECTF& targetRect,
    double horizontalAlignmentRatio,
    double verticalAlignmentRatio,
    double offsetX,
    double offsetY,
    // Rect in local coordinates. Receives subset of targetRect actually visible, smaller than targetRect if viewport isn't large enough,
    // and does not include the potential scroll offsets.
    _Out_opt_ XRECTF* visibleBounds,
    _Out_opt_ double* appliedOffsetX,
    _Out_opt_ double* appliedOffsetY
    )
{
    if (visibleBounds)
    {
        visibleBounds->X = visibleBounds->Y = visibleBounds->Width = visibleBounds->Height = 0.0f;
    }

    if (appliedOffsetX)
    {
        *appliedOffsetX = 0.0;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = 0.0;
    }

    XRECTF adjustedTargetRect = targetRect;

    if (!DirectUI::DoubleUtil::IsNaN(horizontalAlignmentRatio))
    {
        // Account for the horizontal alignment ratio.
        ASSERT(horizontalAlignmentRatio >= 0.0 && horizontalAlignmentRatio <= 1.0);
        adjustedTargetRect.Y += static_cast<float>((adjustedTargetRect.Width - m_scrollData.ViewportWidth) * horizontalAlignmentRatio);
        adjustedTargetRect.Width = m_scrollData.ViewportWidth;
    }

    if (!DirectUI::DoubleUtil::IsNaN(verticalAlignmentRatio))
    {
        // Account for the vertical alignment ratio.
        ASSERT(verticalAlignmentRatio >= 0.0 && verticalAlignmentRatio <= 1.0);
        adjustedTargetRect.Y += static_cast<float>((adjustedTargetRect.Height - m_scrollData.ViewportHeight) * verticalAlignmentRatio);
        adjustedTargetRect.Height = m_scrollData.ViewportHeight;
    }

    double appliedOffsetXTmp = 0.0;
    double appliedOffsetYTmp = 0.0;
    float newHorizontalOffset = CalculateSegmentVisibleScrollOffset(adjustedTargetRect.X, adjustedTargetRect.Width, static_cast<float>(m_scrollData.HorizontalOffset), m_scrollData.ViewportWidth);
    float newVerticalOffset = CalculateSegmentVisibleScrollOffset(adjustedTargetRect.Y, adjustedTargetRect.Height, static_cast<float>(m_scrollData.VerticalOffset), m_scrollData.ViewportHeight);

    // If the target horizontal offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
    if (offsetX != 0.0 && newHorizontalOffset >= 0.0f)
    {
        float scrollableWidth = MAX(0.0f, static_cast<float>(m_scrollData.ExtentWidth - m_scrollData.ViewportWidth));

        if (newHorizontalOffset <= scrollableWidth)
        {
            if (offsetX > 0.0)
            {
                appliedOffsetXTmp = MIN(newHorizontalOffset, offsetX);
            }
            else
            {
                appliedOffsetXTmp = -MIN(scrollableWidth - newHorizontalOffset, -offsetX);
            }
            newHorizontalOffset -= static_cast<float>(offsetX);
        }
    }

    // If the target vertical offset is within bounds and an offset was provided, apply as much of it as possible while remaining within bounds.
    if (offsetY != 0.0 && newVerticalOffset >= 0.0f)
    {
        float scrollableHeight = MAX(0.0f, static_cast<float>(m_scrollData.ExtentHeight - m_scrollData.ViewportHeight));

        if (newVerticalOffset <= scrollableHeight)
        {
            if (offsetY > 0.0)
            {
                appliedOffsetYTmp = MIN(newVerticalOffset, offsetY);
            }
            else
            {
                appliedOffsetYTmp = -MIN(scrollableHeight - newVerticalOffset, -offsetY);
            }
            newVerticalOffset -= static_cast<float>(offsetY);
        }
    }

    IFC_RETURN(SetScrollOffsets(newHorizontalOffset, newVerticalOffset));

    // If adjustedTargetRect is within the extent, we expect at least the upper left corner of the target rect is visible.
    // Right/bottom edges are clipped if viewport is too small.
#ifdef DBG
    if (adjustedTargetRect.X + 0.0001 >= 0 && adjustedTargetRect.X < m_scrollData.ExtentWidth + 0.0001)
    {
        ASSERT(adjustedTargetRect.X + 0.0001 >= m_scrollData.HorizontalOffset + appliedOffsetXTmp && adjustedTargetRect.X < m_scrollData.HorizontalOffset + appliedOffsetXTmp + m_scrollData.ViewportWidth + 0.0001);
    }
    if (adjustedTargetRect.Y + 0.0001 >= 0 && adjustedTargetRect.Y < m_scrollData.ExtentHeight + 0.0001)
    {
        ASSERT(adjustedTargetRect.Y + 0.0001 >= m_scrollData.VerticalOffset + appliedOffsetYTmp && adjustedTargetRect.Y < m_scrollData.VerticalOffset + appliedOffsetYTmp + m_scrollData.ViewportHeight + 0.0001);
    }
#endif

    if (appliedOffsetX)
    {
        *appliedOffsetX = appliedOffsetXTmp;
    }

    if (appliedOffsetY)
    {
        *appliedOffsetY = appliedOffsetYTmp;
    }

    if (visibleBounds)
    {
        // Do not include the applied offset so that potential parent bring-into-view contributors ignore that shift.
        float adjustedHorizontalOffset = static_cast<float>(m_scrollData.HorizontalOffset + appliedOffsetXTmp);
        float adjustedVerticalOffset = static_cast<float>(m_scrollData.VerticalOffset + appliedOffsetYTmp);

        visibleBounds->X = MAX(0.0f, adjustedTargetRect.X - adjustedHorizontalOffset);
        visibleBounds->Y = MAX(0.0f, adjustedTargetRect.Y - adjustedVerticalOffset);
        visibleBounds->Width = MIN(adjustedTargetRect.Width, adjustedHorizontalOffset + m_scrollData.ViewportWidth - adjustedTargetRect.X);
        visibleBounds->Height = MIN(adjustedTargetRect.Height, adjustedVerticalOffset + m_scrollData.ViewportHeight - adjustedTargetRect.Y);
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Framework pinvoke, gets the content baseline.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetBaselineOffset(_Out_ XFLOAT *pBaselineOffset)
{
    ASSERT(FALSE);
    return E_NOTIMPL;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Sets the TextBoxBase that contains this view.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::SetOwnerTextControl(_In_ CTextBoxBase *pTextBox)
{
    m_pTextControl = pTextBox;

    // We should always use the mouse cursor of the TextBoxView
    pTextBox->SetCursor(m_eMouseCursor);

    IFC_RETURN(RefreshDefaultFormat());

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::RefreshDefaultFormat()
{
    IFC_RETURN(EnsureTextFormattingForRead());
    IFC_RETURN(UpdateDefaultCharFormat(CFM_ALL2));
    IFC_RETURN(UpdateDefaultParagraphFormat(PFM_ALL2));

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Synopsis:
//      Clears the TextBoxBase that contains this view.
//
//---------------------------------------------------------------------------
void CTextBoxView::ClearOwnerTextControl()
{
    m_pTextControl = NULL;
}

//------------------------------------------------------------------------
//
//  Method:  CTextBoxView::LostFocusEventListener
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::OnLostFocus()
{
    CFocusManager* focusManager = VisualTree::GetFocusManagerForElement(this);

    // BackPlate was disable to avoid obstructing the Caret when focused, enable BackPlate now.
    IFC_RETURN(ChangePlaceholderBackPlateVisibility(true));

    m_fShowGrippersOnGotFocus = false;

    IFC_RETURN(ShowOrHideCaret());
    if (AreGrippersVisible() && !focusManager->IsPluginFocused())
    {
        IFC_RETURN(HideSelectionGrippers());
        m_fShowGrippersOnGotFocus = true;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  CTextBoxView::GotFocusEventListener
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::OnGotFocus()
{
    // Placeholder Text BackPlate can obstruct the Caret when focused, disable BackPlate while focused.
    IFC_RETURN(ChangePlaceholderBackPlateVisibility(false));

    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(WM_SETTINGCHANGE, 0, 0, NULL));

    IFC_RETURN(ShowOrHideCaret());

    if (m_fShowGrippersOnGotFocus)
    {
        m_fShowGrippersOnGotFocus = false;
        IFC_RETURN(ShowSelectionGrippers());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Prints the textview using D2D walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams
    )
{

    // Jupiter rendering and printing use different D2D devices. When we print using our print D2D device
    // RichEdit tries to use the cached ID2D1Bitmap that was created on the render device.
    // Since resources are bound to the D2D device they are created on, this doesn't work.
    // Thus we need to invalidate RichEdit's cache before and after printing (after printing so
    // that rendering does not use the prining D2D device).
    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_CLEARIMAGECACHE, 0, 0, NULL));
    wrl::ComPtr<ID2D1RenderTarget> pRenderTarget;
    IFC_RETURN(static_cast<CD2DPrintTarget *>(printParams.GetRenderTarget())->GetRenderTarget(&pRenderTarget));
    IFC_RETURN(D2DRenderCommon(sharedPrintParams, printParams, pRenderTarget.Get()));
    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_CLEARIMAGECACHE, 0, 0, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method for printing the textview using D2D walk.
//      This is used by both D2D render walk and Print walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::D2DRenderCommon(
    _In_ const SharedRenderParams &sharedRP,
    _In_ const D2DRenderParams &d2dRP,
    _In_ ID2D1RenderTarget* pRenderTarget
    )
{
    IPALAcceleratedRenderTarget *pAcceleratedRender = d2dRP.GetD2DRenderTarget();
    RECTL contentBounds = { 0, 0, XcpCeiling(m_scrollData.ExtentWidth), XcpCeiling(m_scrollData.ExtentHeight) };
    RECT updateBounds = {
        XcpFloor(m_scrollData.HorizontalOffset),
        XcpFloor(m_scrollData.VerticalOffset),
        XcpCeiling(m_scrollData.HorizontalOffset + m_scrollData.ViewportWidth),
        XcpCeiling(m_scrollData.VerticalOffset + m_scrollData.ViewportHeight)
    };

    CMILMatrix localTransform(TRUE);
    bool frozen = false;

    // Translate the render target to match the current ScrollViewer offset.
    if (sharedRP.pCurrentTransform != NULL)
    {
        localTransform.Append(*sharedRP.pCurrentTransform);
    }

    IFC_RETURN(pAcceleratedRender->SetTransform(&localTransform));

    // Render the content.
    IFC_RETURN(m_pTextControl->IsFrozen(frozen));
    if (!frozen)
    {
        IFC_RETURN(m_pTextControl->GetTextServices()->OnTxPropertyBitsChange(TXTBIT_EXTENTCHANGE | TXTBIT_CLIENTRECTCHANGE, TXTBIT_EXTENTCHANGE | TXTBIT_CLIENTRECTCHANGE));
        IFC_RETURN(m_pTextControl->GetTextServices()->TxDrawD2D(pRenderTarget, &contentBounds, &updateBounds, TXTVIEW_ACTIVE));
    }

    // Restore the original render target transform.
    if (sharedRP.pCurrentTransform != nullptr)
    {
        IFC_RETURN(pAcceleratedRender->SetTransform(sharedRP.pCurrentTransform));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the desired size of this control for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF &desiredSize
    )
{
    auto scopeGuard = wil::scope_exit([&]
    {
        m_isMeasuring = FALSE;
    });

    bool isAutoGrowing = true;
    CValue highContrastAdjustment;
    bool isInitialLayout = (m_requestedSize.width == 0 && m_requestedSize.height == 0);

    m_isMeasuring = TRUE;

    IFC_RETURN(EnsureTextFormattingForRead());

    ASSERT(m_pTextControl);

    // Use the size passed to us by RichEdit via EN_REQUESTRESIZE message as
    // the desired size unless we are in an auto-growing state where we need to grow
    // the control extent upto the availableSize. In this case, we need to set the desired
    // size to the natural size of text otherwise we will start to wrap before we have
    // expanded the control to hit the availableSize.
    // The natural size is also used when the text control is empty or the text box only contains
    // non-visible characters (measured width = 0) so that its height takes
    // the font size into account and the control does not grow when the first character is entered.
    // For the initial measure, XAML needs to provide RichEdit the available size for layout,
    // Otherwise, RichEdit will use the emtpy Rectangle provided by TxGetClientRect to incorrectly format text into many lines.
    IFC_RETURN(m_pTextControl->IsAutoGrowing(isAutoGrowing));
    const bool requiresMeasureNaturalSize = isInitialLayout ||
                           (isAutoGrowing && (m_pTextControl->GetTextWrapping() == DirectUI::TextWrapping::Wrap)) ||
                            m_pTextControl->IsEmpty() ||
                            m_pTextControl->m_requestResize.right == m_pTextControl->m_requestResize.left;

    if (requiresMeasureNaturalSize)
    {
        IFC_RETURN(GetTextNaturalSize(availableSize, m_requestedSize));
    }
    else
    {
        m_requestedSize.width = static_cast<XFLOAT>(m_pTextControl->m_requestResize.right - m_pTextControl->m_requestResize.left);
        m_requestedSize.height = static_cast<XFLOAT>(m_pTextControl->m_requestResize.bottom - m_pTextControl->m_requestResize.top);
    }

    // ArrangeOverride is called with desired size, so we can never exceed availableSize if we
    // want a meaningful viewport.  (If we did return the naturalSize, we'd be arranged at that
    // size and then silently clipped to fit the original availableSize.)
    desiredSize.width = MIN(availableSize.width, m_requestedSize.width);

    desiredSize.height = MIN(availableSize.height, m_requestedSize.height);

    if (m_spCaretElement)
    {
        IFC_RETURN(m_spCaretElement->Measure(availableSize));
    }

    // ElementHighContrastAdjustment is an inherited property so we don't get a Property change when the element is added to the tree, we
    // check the property here to make sure it is current.
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, &highContrastAdjustment));
    IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(highContrastAdjustment.AsEnum())));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the natural size of text when laid out against the availableSize.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetTextNaturalSize(XSIZEF availableSize, XSIZEF& naturalSize)
{
    // NB: These are extra flags in textserv.h.
    const DWORD TXTNS_FITTOCONTENTWSP = 4;
    const DWORD TXTNS_INCLUDELASTLINE = 0x40000000;

    HRESULT hr = S_OK;
    HDC hdc = GetDC(NULL);
    XFLOAT viewportWidth = m_pTextControl->GetTextWrapping() == DirectUI::TextWrapping::Wrap ? availableSize.width : XFLOAT_INF;
    SIZE naturalSize2 = { TextBoxView_Internal::ConstraintToLong(viewportWidth), TextBoxView_Internal::ConstraintToLong(XFLOAT_INF) };
    SIZE zoomExtent = naturalSize2;

    TextBoxView_Internal::DPtoHIMETRIC(hdc, &zoomExtent);

    IFC(m_pTextControl->GetTextServices()->TxGetNaturalSize(
        DVASPECT_CONTENT,
        hdc,
        NULL /* hdcTargetDev */,
        NULL /* DVTARGETDEVICE */,
        TXTNS_FITTOCONTENTWSP | TXTNS_INCLUDELASTLINE,
        &zoomExtent,
        &naturalSize2.cx,
        &naturalSize2.cy));

    // Reserve a pixel for the caret.  See comments for TextServicesHost::TxGetViewInset.
    naturalSize2.cx += 1;
    naturalSize.width = static_cast<XFLOAT>(naturalSize2.cx);
    naturalSize.height = static_cast<XFLOAT>(naturalSize2.cy);

Cleanup:
    ReleaseDC(NULL, hdc);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the final size of this control for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF &newFinalSize
    )
{
    bool fInvalidateScrollInfo = false;
    XSIZEF extent;
    newFinalSize = finalSize;

    m_issueDispatcherViewInvalidation = TRUE;
    auto scopeGuard = wil::scope_exit([&]
    {
        m_issueDispatcherViewInvalidation = FALSE;
    });

    if (m_scrollData.ViewportWidth != finalSize.width ||
        m_scrollData.ViewportHeight != finalSize.height)
    {
        m_scrollData.ViewportWidth = finalSize.width;
        m_scrollData.ViewportHeight = finalSize.height;
        fInvalidateScrollInfo = true;
        m_forceRedraw = TRUE;
    }

    // If TextWrapping is enabled, cap the extent width to the finalSize.
    // Normally, RichEdit will automatically create a new line when wrapping is
    // enabled such that natural width does not exceed the finalSize.width.
    // However, this does not happen if you press space key at the end of a line.
    // Space does not result in line wrapping unless a non-space key is pressed (by design).
    extent.width = m_pTextControl->GetTextWrapping() == DirectUI::TextWrapping::Wrap ? finalSize.width : MAX(m_requestedSize.width, finalSize.width);
    extent.height = MAX(m_requestedSize.height, finalSize.height);

    //
    // Apply layout rounding to the extent. The viewport that's applied comes from the size passed into ArrangeOverride
    // and already has layout rounding applied. If the viewport is layout rounded and the extent is not, it can introduce
    // an endless cycle during layout, for example:
    //   1. The text in the text box view gives it a desired height of 23px.
    //   2. The text box runs layout on the text box view at 22.85px (23px, rounded for 1.4x plateau)
    //   3. The extent is unrounded at 23px, and the viewport is 22.85px. The viewport is too small for the extent, so
    //      the vertical scrollbar is made visible.
    //   4. The vertical scrollbar itself is at minimum 60px, so the text box now has a height of 60px. It runs layout
    //      on this text box view at 60px.
    //   5. Arrange now runs with a 60px extent and a 60px viewport. The viewport now fits the extent, so the vertical
    //      scrollbar is collapsed.
    //   6. The text box runs layout again, without the scrollbar, at 22.85px. Repeat at step 2.
    //
    if (GetUseLayoutRounding())
    {
        extent.width = LayoutRound(extent.width);
        extent.height = LayoutRound(extent.height);
    }

    const bool extentWidthChanged = m_scrollData.ExtentWidth != extent.width;
    const bool extentHeightChanged = m_scrollData.ExtentHeight != extent.height;
    if (extentWidthChanged || extentHeightChanged)
    {
        // RichEdit won't explicitly invalidate (via TxInvalidateRect callbacks) portions of the
        // viewport covered by extent changes when extent grows.  Do that here.
        InvalidateExtentAdditions(extent);

        // Update the extent and notify the ScrollViewer
        // Normalize also offsets, since updating extent might make those invalid.
        m_scrollData.ExtentWidth = extent.width;
        m_scrollData.ExtentHeight = extent.height;

        IFC_RETURN(SetScrollDataOffsets(
            NormalizeScrollOffset(static_cast<XFLOAT>(m_scrollData.HorizontalOffset), m_scrollData.ViewportWidth, m_scrollData.ExtentWidth),
            NormalizeScrollOffset(static_cast<XFLOAT>(m_scrollData.VerticalOffset), m_scrollData.ViewportHeight, m_scrollData.ExtentHeight)));

        // Tell RichEdit that extent has changed, avoid calling IinvalidateView again from EN_REQUESTRESIZE triggered by OnTxPropertyBitsChange
        {
            m_pTextControl->m_updatingExtent = true;
            auto scopeGuard2 = wil::scope_exit([this] {
                m_pTextControl->m_updatingExtent = false;
            });
            IFC_RETURN(m_pTextControl->GetTextServices()->OnTxPropertyBitsChange(TXTBIT_EXTENTCHANGE, TXTBIT_EXTENTCHANGE));
        }

    }
    else if (fInvalidateScrollInfo)
    {

        // Viewport width/height has been updated. Now normalize offsets, since updating viewport might make those invalid.
        IFC_RETURN(SetScrollDataOffsets(
            NormalizeScrollOffset(static_cast<XFLOAT>(m_scrollData.HorizontalOffset), m_scrollData.ViewportWidth, m_scrollData.ExtentWidth),
            NormalizeScrollOffset(static_cast<XFLOAT>(m_scrollData.VerticalOffset), m_scrollData.ViewportHeight, m_scrollData.ExtentHeight)));
    }

    if (m_spCaretElement != NULL)
    {
        XRECTF bounds = { 0, 0, finalSize.width, finalSize.height };
        IFC_RETURN(m_spCaretElement->Arrange(bounds));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Coerces a scrollbar offset to fit within the current viewport and
//      extent limits.
//
//------------------------------------------------------------------------
XFLOAT CTextBoxView::NormalizeScrollOffset(
    _In_ XFLOAT offset,
    _In_ XFLOAT viewportLength,
    _In_ XFLOAT extent
    )
{
    if (offset < 0)
    {
        offset = 0;
    }
    else if (viewportLength > extent)
    {
        offset = 0;
    }
    else if (offset + viewportLength > extent)
    {
        offset = extent - viewportLength;
    }

    return offset;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets a new scrollbar offset intended to move a segment into view.
//
//------------------------------------------------------------------------
XFLOAT CTextBoxView::CalculateSegmentVisibleScrollOffset(
    _In_ XFLOAT segmentOffset,
    _In_ XFLOAT segmentLength,
    _In_ XFLOAT currentScrollOffset,
    _In_ XFLOAT viewportLength
    )
{
    XFLOAT newScrollOffset = currentScrollOffset;
    const XFLOAT Epsilon = 0.0001f;

    if (segmentOffset <= currentScrollOffset + Epsilon)
    {
        // Target is clipped to the near edge of the viewport.
        // Align the near edge of the viewport with the near edge of the segment.
        newScrollOffset = segmentOffset;
    }
    else if (segmentOffset + segmentLength >= currentScrollOffset + viewportLength)
    {
        if (segmentLength < viewportLength)
        {
            // Viewport is big enough to show the segment. Align the segment along
            // the bottom/right of the viewport
            newScrollOffset = segmentOffset + segmentLength - viewportLength;
        }
        else
        {
            // Viewport is not big enough to show the entire segment. Clip the bottom/right.
            newScrollOffset = segmentOffset;
        }
    }

    return newScrollOffset;
}


XRECTF CTextBoxView::GetViewportContentRect() const
{
    XRECTF viewportRect;
    viewportRect.X = static_cast<XFLOAT>(m_scrollData.HorizontalOffset);
    viewportRect.Y = static_cast<XFLOAT>(m_scrollData.VerticalOffset);
    viewportRect.Width = static_cast<XFLOAT>(m_scrollData.ViewportWidth);
    viewportRect.Height = static_cast<XFLOAT>(m_scrollData.ViewportHeight);

    return viewportRect;
}

XRECTF_RB CTextBoxView::GetViewportGlobalRect()
{
    XRECTF_RB viewportRect;

    XRECTF_RB bounds;
    IFCFAILFAST(GetOuterBounds(nullptr /*hitTestParams*/, &bounds));
    bounds.left += static_cast<XFLOAT>(m_scrollData.HorizontalOffset);
    bounds.top += static_cast<XFLOAT>(m_scrollData.VerticalOffset);
    bounds.right += static_cast<XFLOAT>(m_scrollData.HorizontalOffset);
    bounds.bottom += static_cast<XFLOAT>(m_scrollData.VerticalOffset);

    CUIElement* parent = GetUIElementAdjustedParentInternal(FALSE);
    IFCFAILFAST(parent->TransformToWorldSpace(&bounds, &viewportRect, false /* ignoreClipping */, false /* ignoreClippingOnScrollContentPresenters */, false /* useTargetInformation */));

    return viewportRect;
}

bool CTextBoxView::IsCaretInViewport() const
{
    XRECTF caretRectInViewport = m_caretRect;
    caretRectInViewport.X -= static_cast<XFLOAT>(m_scrollData.HorizontalOffset);
    caretRectInViewport.Y -= static_cast<XFLOAT>(m_scrollData.VerticalOffset);

    return caretRectInViewport.X + caretRectInViewport.Width > 0 &&
           caretRectInViewport.Y + caretRectInViewport.Height > 0 &&
           caretRectInViewport.X < m_scrollData.ViewportWidth &&
           caretRectInViewport.Y < m_scrollData.ViewportHeight;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the caret is not currently displayed, parents the UIElement
//      used to render the caret to this control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ShowCaretElement()
{
    CValue valueVisible;
    valueVisible.Set(DirectUI::Visibility::Visible);
    EnsureCaretElement();

    ASSERT(GetFirstChildNoAddRef() == m_spCaretElement);
    IFC_RETURN(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, valueVisible));
    IFC_RETURN(UpdateCaretElement());
    IFC_RETURN(ResumeCaretBlink());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      If the caret is currently displayed, unparents the UIElement
//      used to render the caret from this control.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::HideCaretElement()
{
    if (m_spCaretBlink && m_spCaretElement && GetFirstChildNoAddRef())
    {
        CValue valueCollapsed;
        valueCollapsed.Set(DirectUI::Visibility::Collapsed);

        ASSERT(GetFirstChildNoAddRef() == m_spCaretElement);
        IFC_RETURN(PauseCaretBlink());
        IFC_RETURN(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Visibility, valueCollapsed));
    }

    return S_OK;
}

// If this TextBoxView has a FlowDirection of RightToLeft,
// this function returns the amount of shift required to transform a coordinate from an LTR
// coordinate space to an RTL coordinate space.
// If this TextBoxView has a FlowDirection of LeftToRight, this function returns 0.
float CTextBoxView::GetRightToLeftOffset()
{
    float offset = 0.0f;

    if (m_pTextFormatting != nullptr &&
        m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
    {
        offset = std::max<float>(0.0f, m_scrollData.ExtentWidth - m_scrollData.ViewportWidth);
    }

    return offset;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders content using primitive composition walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::HwRender(_In_ IContentRenderer* pContentRenderer)
{
    HRESULT hr = S_OK;

    const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
    HWRenderParams localRP = rp;
    HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);

    TransformAndClipStack transformsAndClips;
    CTransformToRoot localTransformToRoot = *(pContentRenderer->GetTransformToRoot());

    m_isRendering = TRUE;

    XRECTF_RB viewport;
    CMILMatrix viewportTransform;
    GetViewportAndViewportTransform(&viewport, &viewportTransform);

    XRECT_RB updateBounds = {
        XcpFloor(viewport.left),
        XcpFloor(viewport.top),
        XcpCeiling(viewport.right),
        XcpCeiling(viewport.bottom)
    };
    XRECT_RB contentBounds = { 0, 0, XcpCeiling(m_scrollData.ExtentWidth), XcpCeiling(m_scrollData.ExtentHeight) };

    IFC(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
    transformsAndClips.PrependTransform(viewportTransform);
    localTransformToRoot.Prepend(viewportTransform);
    localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

    {
        TransformToRoot2DOverride transformOverride(pContentRenderer, &localTransformToRoot);

        // Check the invalidate region and call RichEdit to draw if necessary
        IFC(HWNWRenderCommon(pContentRenderer, contentBounds, updateBounds));

        //
        // Convert stored drawing instructions to actual textures.
        //
        // m_pRenderTarget can be nullptr, we return immediately in HWNWRenderCommon() if HandwritingView is activated  //DEAD_CODE_REMOVAL
        if (m_pRenderTarget != nullptr)
        {
            IFC(m_pRenderTarget->HWRender(pContentRenderer));
        }
    }

Cleanup:
    m_isRendering = FALSE;

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the current viewport and transform.
//
//------------------------------------------------------------------------
void
CTextBoxView::GetViewportAndViewportTransform(
    _Out_ XRECTF_RB *pViewport,
    _Out_ CMILMatrix *pViewportTransform
    )
{
    XRECTF_RB viewport = {
        static_cast<XFLOAT>(m_scrollData.HorizontalOffset),
        static_cast<XFLOAT>(m_scrollData.VerticalOffset),
        static_cast<XFLOAT>(m_scrollData.HorizontalOffset) + m_scrollData.ViewportWidth,
        static_cast<XFLOAT>(m_scrollData.VerticalOffset) + m_scrollData.ViewportHeight
    };

    CMILMatrix viewportTransform(TRUE);

    InflateRectF(&viewport);

    *pViewport = viewport;
    *pViewportTransform = viewportTransform;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method to update the selection gripper visibility using the transform to root
//      and animation state on the ancestor chain.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::UpdateVisibility()
{
    HRESULT hr = S_OK;

    ASSERT(m_pTextControl->IsFocused());

    XRECTF_RB viewport;
    CMILMatrix viewportTransform;
    GetViewportAndViewportTransform(&viewport, &viewportTransform);

    CTransformToRoot transformToRoot;
    bool isAncestorTransformAnimating = false;
    CUIElement *pCurrentNoRef = this;

    XRECT selectionRect;
    IFC(GetSelectionRect(&selectionRect));

    // Walk up the tree just like in CUIElement::GetRedirectionTransformsAndParentCompNode, where the Popup would
    // collect the transform animation flag from its ancestors in the render walk.
    while (pCurrentNoRef != NULL)
    {
        // If the current element is hidden for a LayoutTransition, use the LayoutTransitionElement's properties instead.
        CUIElement *pTransformElementNoRef;
        if (pCurrentNoRef->IsHiddenForLayoutTransition())
        {
            // TODO: HWPC: During a portaling transition, this only checks the first LTE, but the grippers will be hidden anyway.
            CLayoutTransitionElement *pLTENoRef;
            IFC(pCurrentNoRef->GetLayoutTransitionElements()->get_item(0, pLTENoRef));
            pTransformElementNoRef = pLTENoRef;
        }
        else
        {
            pTransformElementNoRef = pCurrentNoRef;
        }

        if (pTransformElementNoRef->IsTransformOrOffsetAffectingPropertyIndependentlyAnimating())
        {
            isAncestorTransformAnimating = TRUE;
        }

        // Since we're walking up, apply projection first and then the 2D transform.
        if (pTransformElementNoRef->HasActiveProjection())
        {
            XSIZEF elementSize;
            IFC(pTransformElementNoRef->GetElementSizeForProjection(&elementSize));

            const CMILMatrix4x4 projectionTransform = pTransformElementNoRef->GetProjection()->GetOverallProjectionMatrix(elementSize);

            transformToRoot.Append(projectionTransform);
        }

        CMILMatrix localTransform;
        if (!pTransformElementNoRef->GetLocalTransform(TransformRetrievalOptions::IncludePropertiesSetInComposition, &localTransform))
        {
            transformToRoot.Append(localTransform);
        }

        pCurrentNoRef = pCurrentNoRef->GetUIElementAdjustedParentInternal(TRUE /*public parents only*/);
    }

    if (m_pLastTransformToRoot == NULL)
    {
        m_pLastTransformToRoot = new CTransformToRoot();
    }
    const bool hasTransformToRootChanged = !m_pLastTransformToRoot->IsSameAs(&transformToRoot);

    bool fMakeGrippersVisible = false;
    if (isAncestorTransformAnimating && AreGrippersVisible())
    {
        // Hide the grippers if they are visible and text is animating.
        IFC(HideSelectionGrippers());
        m_fShowGrippersOnAnimationComplete = TRUE;
    }
    else if (m_fShowGrippersOnAnimationComplete && !isAncestorTransformAnimating)
    {
        // Text has stopped animating and flag is set to indicate that
        // grippers should be made visible when the animation stops.
        // Check if the selection is visible and update the grippers accordingly.
        fMakeGrippersVisible = TRUE;
    }
    else if ( hasTransformToRootChanged
          || m_lastViewportOffset.x != viewport.left
          || m_lastViewportOffset.y != viewport.top
          || m_lastSelectionRect.X != selectionRect.X
          || m_lastSelectionRect.Y != selectionRect.Y
          || m_lastSelectionRect.Width != selectionRect.Width
          || m_lastSelectionRect.Height != selectionRect.Height)
    {
        // Text has shifted on the screen, but not via animation. Toggle the gripper
        // visibility to update their screen position.
        if (AreGrippersVisible())
        {
            IFC(HideSelectionGrippers());
            fMakeGrippersVisible = TRUE;
        }
        else if (m_fShowGrippersOnSelectionChanged)
        {
            // Grippers may be hidden because the selection was invisible, try to
            // show the grippers again when the position of selection changes
            fMakeGrippersVisible = TRUE;
        }
    }

    if (fMakeGrippersVisible)
    {
        bool fSelectionVisible = true;

        IFC(IsSelectionEdgeVisible(fSelectionVisible));
        if (fSelectionVisible)
        {
            // If selection is visible or the caret is visible, restore the state
            // of the grippers. If there is an active selection but it is hidden, then
            // wait till the selection is visible before restoring the grippers.
            IFC(ShowSelectionGrippers());
            m_fShowGrippersOnAnimationComplete = FALSE;
            m_fShowGrippersOnSelectionChanged = FALSE;
        }
        else
        {
            // If selection is currently invisible, try restore the state of grippers
            // after the position of selection moves.
            m_fShowGrippersOnSelectionChanged = TRUE;
        }
    }

    // Hide the grippers when there is no text in the control.
    // This could happen when user pressed the TextBox/RichEditBox,
    // RichEdit will always show the grippers when it receives PointerReleased event.
    if (m_pTextControl->IsEmpty() && AreGrippersVisible())
    {
        IFC(HideSelectionGrippers());
    }

    // If grippers are visible, we want to inform RichEdit the changes in text box view
    if (m_pTextControl->GetTextServices() != NULL && AreGrippersVisible())
    {
        IFC(m_pTextControl->GetTextServices()->TxSendMessage(EM_VISIBLERECTCHANGED, 0, 0, NULL));
    }

Cleanup:
    m_pLastTransformToRoot->Set(&transformToRoot);
    m_lastViewportOffset.x = viewport.left;
    m_lastViewportOffset.y = viewport.top;
    m_lastSelectionRect = selectionRect;

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Helper method that contains the common code between PC and
//      software rendering walks.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::HWNWRenderCommon(
    _In_ IContentRenderer* contentRenderer,
    XRECT_RB& contentBounds,
    XRECT_RB& updateBounds)
{
    HRESULT hr = S_OK;
    bool redrawContent = false;
    bool frozen = false;
    bool forceRedrawNextFrame = false;

    // Create render target, if necessary.
    if (m_pRenderTarget == nullptr)
    {
        CTextCore *pTextCore;
        IFC(m_pTextControl->GetContext()->GetTextCore(&pTextCore));
        IFC(HWRenderTarget::Create(pTextCore, &m_pRenderTarget));
        m_pRenderTarget->SetForegroundAlpha(m_foregroundAlpha);
        m_pRenderTarget->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), false /*useHyperlinkForeground*/);
    }

    m_pRenderTarget->SetIsColorFontEnabled(m_pTextControl->m_isColorFontEnabled);

    // DManip can update viewport and dirty DManip transform when m_forceRedraw is not set (i.e. paste very long text),
    // We should always force redraw if transform is dirty to keep glyph runs from potential clipping.
    // Scope redraw to only when clip happend in last DrawGlyphRun to limit performance impact
    if (m_fNWTransformDirty && m_pRenderTarget->LastDrawGlyphRunHasClipping())
    {
        m_forceRedraw = TRUE;
    }

    // Invalidate the render target and get new display content from RichEdit only if
    // the RichEdit control is not in a frozen state. Otherwise keep rendering whatever
    // content we have until RichEdit gets out of the frozen state.
    if (m_forceRedraw || !IsEmptyRect(m_invalidRect))
    {
        IFC(m_pTextControl->IsFrozen(frozen));
        if (!frozen)
        {
            // Invalidate affected area of the render target.
            if (m_forceRedraw)
            {
                m_pRenderTarget->InvalidateContent();
                redrawContent = true;
            }
            else
            {
                redrawContent = !!IntersectRect(&updateBounds, &m_invalidRect);
                if (redrawContent)
                {
                    m_pRenderTarget->InvalidateRegion(m_invalidRect);
                }
            }

            if (redrawContent)
            {
                m_fScrollOffsetsChangedByDraw = FALSE; // if this flag is set during the TxDrawD2D, we shall not clear the m_forceRedraw flag in Cleanup.

                //
                // We're seeing reentrancy problems here with the switch to lifted RichEdit (WinUIEdit) and lifted
                // CoreMessaging.
                //
                // (System) CoreMessaging is calling TextInputFramework, which then calls into WinUIEdit. WinUIEdit
                // itself uses lifted CoreMessaging, which is entirely separate from system CoreMessaging. This can
                // cause WinUIEdit to pump lifted CM messages when it calls things like spell checking. Xaml itself
                // runs off of lifted CM, so we can get called as a result of WinUIEdit pumping lifted CM messages.
                // This ends up with WinUIEdit calling us to render when we're in the middle of typing. This is not
                // captured by the IsFrozen check above. We then call back into WinUIEdit to render, and we hit a
                // failure because you're not allowed to render when you're still typing.
                //
                // This wouldn't be a problem if lifted CM was aware of system CM messages. If that were the case,
                // lifted CM wouldn't pump its messages because it sees that system CM is already pumping. Xaml would
                // then not get called and we won't have reentrancy problems. The long-term plan is to have lifted
                // CM hook into system CM so we get that ability.
                //
                // In the meantime, Xaml's workaround is to sniff for the known E_UNEXPECTED error returned by
                // WinUIEdit in the case of reentrancy, and try to render the TextBoxView again next frame. This
                // workaround can be removed once lifted CM and system CM are better integrated. See
                // Deliverable 24183892: [DCPP] Fairness algorithm in CoreMessaging
                //
                HRESULT drawHR = m_pTextControl->GetTextServices()->TxDrawD2D(m_pRenderTarget, reinterpret_cast<RECTL*>(&contentBounds), reinterpret_cast<RECT*>(&updateBounds), TXTVIEW_ACTIVE);
                if (drawHR == E_UNEXPECTED)
                {
                    contentRenderer->AddDirtyElementForNextFrame(this);
                    forceRedrawNextFrame = true;
                    hr = S_OK;
                }
                else
                {
                    IFC(drawHR);
                }
            }
        }
    }

    // If UIElement is marked dirty for rendering, invalidate render walk caches accumulated
    // in the drawing context.
    // Example of such situation is when Brush is changed inside, without replacing entire Brush object.
    // In such case all the render walk caches (textures/edge stores) need to be regenerated.
    if (NWIsContentDirty())
    {
        m_pRenderTarget->InvalidateRenderCache();
    }

Cleanup:
    if (forceRedrawNextFrame)
    {
        m_forceRedraw = true;
    }
    else if (!frozen && !m_fScrollOffsetsChangedByDraw)
    {
        m_forceRedraw = FALSE;
        EmptyRect(&m_invalidRect);
    }

    RRETURN(hr);
}

_Check_return_ HRESULT CTextBoxView::CalculateBaseLine(_In_ const wrl::ComPtr<ITextRange2>& spRange, _Out_ UINT32& lineHeight, _Out_ UINT32 &baselineY)
{
    long baselineX;
    long tempBaselineY;

    IFC_RETURN(spRange->GetPoint(tomClientCoord | tomAllowOffClient + TA_LEFT + TA_BASELINE, &baselineX, &tempBaselineY));
    baselineY = static_cast<UINT32>(tempBaselineY);

    long pointTopX, pointTopY;
    long pointBottomX, pointBottomY;

    IFC_RETURN(spRange->GetPoint(tomClientCoord | tomAllowOffClient + TA_TOP, &pointTopX, &pointTopY));
    IFC_RETURN(spRange->GetPoint(tomClientCoord | tomAllowOffClient + TA_BOTTOM, &pointBottomX, &pointBottomY));

    if (pointBottomY > pointTopY)
    {
        lineHeight = pointBottomY - pointTopY;
    }
    else
    {
        // This can happen during the resizing, text view goes out of sync between XAML and RE, we should just set it to zero and bail out.
        // View will get invalidated and another draw will happen soon
        lineHeight = 0;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::DrawBaseLine(_In_ const XRECT_RB& updateBounds, _In_ UINT32 crText)
{
    wrl::ComPtr<ITextDocument2> spDocument;
    wrl::ComPtr<ITextRange2> spRange;

    IFC_RETURN(m_pTextControl->GetDocument(&spDocument));
    IFC_RETURN(spDocument->RangeFromPoint2(updateBounds.left, updateBounds.top, tomClientCoord, &spRange));
    IFC_RETURN(spRange->Expand(tomLine, nullptr));

    UINT32 baseLineY = 0;
    UINT32 lineHeight = 0;
    IFC_RETURN(CalculateBaseLine(spRange, lineHeight, baseLineY));

    // Loop through viewport and draw all base lines.
    while (baseLineY < static_cast<UINT32> (updateBounds.bottom))
    {
        IFC_RETURN(m_pRenderTarget->DrawBaseLine(baseLineY, updateBounds.left, updateBounds.right, crText));
        long moved = 0;
        IGNOREHR(spRange->Move(tomLine, 1, &moved));
        if (moved > 0)
        {
            IFC_RETURN(CalculateBaseLine(spRange, lineHeight, baseLineY));
        }
        else
        {
            baseLineY += lineHeight;
        }

        if (lineHeight == 0)
        {
            // just bail out, view is out of sync
            break;
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Called whenever the extent grows, adds the new extent regions to
//      the invalid render rect.
//
//  Notes:
//      Because at the time when a backing store change happens we have not
//      yet re-measured the control, RichEdit won't explicitly invalidate
//      (via TxInvalidateRect callbacks) portions of the viewport covered
//      by extent changes when extent grows.  This method adds in those
//      new regions.
//
//------------------------------------------------------------------------
void CTextBoxView::InvalidateExtentAdditions(_In_ const XSIZEF &newExtent)
{
    XRECT_RB newExtentRect;

    if (m_scrollData.ExtentWidth < newExtent.width)
    {
        // Horizontal extent grew. Invalidate over the new content.
        newExtentRect.left = XcpFloor(m_scrollData.ExtentWidth);
        newExtentRect.top = 0;
        newExtentRect.right = XcpCeiling(newExtent.width);
        newExtentRect.bottom = XcpCeiling(newExtent.height);
        UnionRect(&m_invalidRect, &newExtentRect);
    }
    if (m_scrollData.ExtentHeight < newExtent.height)
    {
        // Vertical extent grew. Invalidate over the new content.
        newExtentRect.left = 0;
        newExtentRect.top = XcpFloor(m_scrollData.ExtentHeight);
        newExtentRect.right = XcpCeiling(newExtent.width);
        newExtentRect.bottom = XcpCeiling(newExtent.height);
        UnionRect(&m_invalidRect, &newExtentRect);
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      The caret opacity is full 100% if the caret width is 1 pixel,
//      otherwise it is a partial  value that comes from settings.
//
//------------------------------------------------------------------------
XFLOAT CTextBoxView::GetCaretOpacity() const
{
    return m_caretRect.Width > 1 ? TextSelectionSettings::Get()->m_rCaretAnimationMaxBlockOpacity : 1.0f;
}

// Returns true if the RTL indicator flag was set
bool CTextBoxView::ShouldShowRTLIndicator()
{
    return m_shouldShowRTLIndicator;
}

// Returns the caret indicator size (the size of the nubbin)
float CTextBoxView::GetRTLIndicatorSize()
{
    ASSERT(m_shouldShowRTLIndicator); // We should only end up in this method if we're in RTL.

    const double plateauScale = RootScale::GetRasterizationScaleForElement(this);
    double scaledSize = (m_shouldShowRTLIndicator) ? 2.0 : 0.0;

    // Do some basic calculation to help pixel-align the caret.
    if (plateauScale != 1.0 && m_shouldShowRTLIndicator) // Only scale if the RTL indicator is on.
    {
        scaledSize = XcpCeiling(scaledSize * plateauScale) / plateauScale;
    }

    return static_cast<float>(scaledSize);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Updates the size, position, and blink animation of the UIElement
//      used to render the caret to match current control state.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::UpdateCaretElement()
{
    HRESULT hr = S_OK;
    CTranslateTransform *pTranslation = NULL;
    xref_ptr<CTimeSpan> pTimeSpan;
    xref_ptr<CTimeSpan> pTimeSpanStart;
    CREATEPARAMETERS cp(GetContext());
    if (!m_pTextControl)
    {
        goto Cleanup;
    }

    if (m_isRendering)
    {
        // If in the middle of rendering, post a dispatcher callback to change caret visibility.
        // We should not modify the tree in middle of render.
        IFC(FxCallbacks::TextBoxView_CaretChanged(this));
        goto Cleanup;
    }

    if (m_spCaretElement != NULL)
    {
        XRECTF caretRect;
        CValue value;
        const CDependencyProperty *pRenderTransformProperty = m_spCaretElement->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_RenderTransform);
        bool isRenderTransformDefault = m_spCaretElement->IsPropertyDefault(pRenderTransformProperty);
        CValue oldWidth;

        caretRect = GetCaretRect();

        // Update caret size.
        const bool shouldShowRTLIndicator = ShouldShowRTLIndicator();
        const float caretIndicatorSize = (shouldShowRTLIndicator) ? GetRTLIndicatorSize() : 0.0f;
        value.SetFloat(caretRect.Width + caretIndicatorSize);
        IFC(m_spCaretElement->GetValue(m_spCaretElement->GetPropertyByIndexInline(KnownPropertyIndex::FrameworkElement_Width), &oldWidth));
        IFC(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Width, value));

        value.SetFloat(caretRect.Height);
        IFC(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_Height, value));


        // Update caret position.

        // Get existing or create new transform.
        if(isRenderTransformDefault)
        {
            CREATEPARAMETERS createParams(GetContext());
            IFC(CTranslateTransform::Create(reinterpret_cast<CDependencyObject**>(&pTranslation), &createParams));
        }
        else
        {
            IFC(m_spCaretElement->GetValue(pRenderTransformProperty, &value));
            IFC(DoPointerCast(pTranslation, value.DetachObject().detach()));
            ASSERT(pTranslation != NULL);
        }

        float offset = caretRect.X - caretIndicatorSize;
        pTranslation->m_eX = (offset >= 0.0f) ? offset : caretRect.X;
        pTranslation->m_eY = caretRect.Y;
        CTransform::NWSetRenderDirty(pTranslation, DirtyFlags::Render);
        value.WrapObjectNoRef(pTranslation);

        IFC(m_spCaretElement->SetValue(SetValueParams(pRenderTransformProperty, value)));

        float caretBlinkingPeriod = CTextBoxHelpers::GetCaretBlinkingPeriod();

        if (oldWidth.AsFloat() != caretRect.Width || caretBlinkingPeriod != m_rCaretBlinkingPeriod)
        {
            if (shouldShowRTLIndicator)
            {
                IFC(GenerateRTLCaret(caretRect.Width, caretRect.Height));
            }

            // Set the caret opacity based on whether it is block or I-beam.
            // Rebuild and restart the blink animation so that the base value change is picked up
            IFC(m_spCaretBlink->StopPrivate());
            IFC(m_spCaretTimer->Stop());
            XFLOAT rCaretMaxOpacity = GetCaretOpacity();
            value.SetFloat(rCaretMaxOpacity);
            IFC(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_Opacity, value));
            // Rebuild the storyboard with the new opacity value
            IFC(CTextBoxHelpers::CreateCaretAnimationStoryboard(
                m_spCaretBlink.ReleaseAndGetAddressOf(),
                GetContext(),
                m_spCaretElement,
                m_spCaretElement->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_Opacity),
                rCaretMaxOpacity,
                caretBlinkingPeriod));
            m_rCaretBlinkingPeriod = caretBlinkingPeriod;

            IFC(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));
            pTimeSpan->m_rTimeSpan = CheckCaretBlinkTimeout();
            IFC(m_spCaretTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval,
                pTimeSpan.get()));

            IFC(CreateDO(pTimeSpanStart.ReleaseAndGetAddressOf(), &cp));
            pTimeSpanStart->m_rTimeSpan = TextSelectionSettings::Get()->m_rCaretBlinkStart;
            IFC(m_spCaretStartTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval,
                pTimeSpanStart.get()));

            IFC(m_spCaretBlink->BeginPrivate(TRUE /* Top-level Storyboard */));
            IFC(m_spCaretStartTimer->Start());

            // If the caret doesn't show, pause the storyboard afterwards. We start and pause rather than leave the
            // storyboard stopped. When the caret should blink again, we'll resume the storyboard, and Resume can't be
            // called on a storyboard that was never started in the first place.
        }

        bool shouldRenderCaret;
        IFC(GetShouldRenderCaret(&shouldRenderCaret));

        if (shouldRenderCaret)
        {
            // Reset the blink animation so that the caret is visible, and
            // restart the timeout timer if configured so.
            IFC(ResumeCaretBlink());
        }
        else
        {
            IFC(PauseCaretBlink());
        }
    }

Cleanup:
    ReleaseInterface(pTranslation);
    return hr;
}

// Create the path geometry to represent the RTL caret.
_Check_return_ HRESULT
CTextBoxView::GenerateRTLCaret(XFLOAT width, XFLOAT height)
{
    CREATEPARAMETERS cp(GetContext());
    CValue cVal;

    xref_ptr<CPathGeometry> pathGeometry;
    xref_ptr<CPathFigure> pathFigure;
    CPathSegmentCollection *pathSegmentCollection = nullptr;
    CPathFigureCollection *pathFiguresNoRef = nullptr;

    const bool shouldShowRTLIndicator = ShouldShowRTLIndicator();
    ASSERT(shouldShowRTLIndicator); // We should only end up in this method if we're in RTL.

    const float caretIndicatorSize = GetRTLIndicatorSize();
    const float caretSizeAndWidth = caretIndicatorSize + width;
    const XPOINTF caretPointDefinitions[4] = { {caretSizeAndWidth, 0.0f}, {caretSizeAndWidth, height}, {caretIndicatorSize, height}, {caretIndicatorSize, caretIndicatorSize} };
    const XPOINTF startingPoint = {0.0f, 0.0f};

    IFC_RETURN(CPathGeometry::Create(reinterpret_cast<CDependencyObject **>(pathGeometry.ReleaseAndGetAddressOf()), &cp));
    IFC_RETURN(CPathFigure::Create(reinterpret_cast<CDependencyObject **>(pathFigure.ReleaseAndGetAddressOf()), &cp));

    cVal.SetBool(true);
    IFC_RETURN(pathFigure->SetValueByIndex(KnownPropertyIndex::PathFigure_IsFilled, cVal));

    // Set start point.
    cVal.WrapPoint(&startingPoint);
    IFC_RETURN(pathFigure->SetValueByIndex(KnownPropertyIndex::PathFigure_StartPoint, cVal));

    // Get and populate the line segments.
    IFC_RETURN(pathFigure->GetValueByIndex(KnownPropertyIndex::PathFigure_Segments, &cVal));
    pathSegmentCollection = do_pointer_cast<CPathSegmentCollection>(cVal.AsObject());
    ASSERT(pathSegmentCollection);

    for (int i = 0; i < 4; ++i)
    {
        IFC_RETURN(AddLineSegmentToSegmentCollection(GetContext(), pathSegmentCollection, caretPointDefinitions[i]));
    }

    // Populate the figure collection.
    IFC_RETURN(pathGeometry->GetValueByIndex(KnownPropertyIndex::PathGeometry_Figures, &cVal));
    pathFiguresNoRef = do_pointer_cast<CPathFigureCollection>(cVal.AsObject());

    cVal.WrapObjectNoRef(pathFigure.get());
    IFC_RETURN(CCollection::Add(pathFiguresNoRef, 1, &cVal, nullptr));

    cVal.WrapObjectNoRef(pathGeometry.get());
    IFC_RETURN(m_spCaretElement->SetValueByIndex(KnownPropertyIndex::Path_Data, cVal));

    return S_OK;
}

_Check_return_ HRESULT
CTextBoxView::AddLineSegmentToSegmentCollection(
    _In_ CCoreServices *pCore,
    _In_ CPathSegmentCollection *pSegments,
    _In_ XPOINTF point)
{
    CREATEPARAMETERS cp(pCore);
    CValue cVal;

    xref_ptr<CLineSegment> pSegment;

    IFC_RETURN(CLineSegment::Create(reinterpret_cast<CDependencyObject **>(&pSegment), &cp));

    cVal.WrapPoint(&point);
    IFC_RETURN(pSegment.get()->SetValueByIndex(KnownPropertyIndex::LineSegment_Point, cVal));

    cVal.WrapObjectNoRef(pSegment);
    IFC_RETURN(CCollection::Add(pSegments, 1, &cVal, NULL));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Calculates blink timeout value, caches the timeout enabled flag.
//      Returns timeout adjusted for the full number of blink periods.
//
//------------------------------------------------------------------------
XFLOAT CTextBoxView::CheckCaretBlinkTimeout()
{
    XFLOAT rTimeout = TextSelectionSettings::Get()->m_rCaretBlinkTimeout;
    XINT32 nPeriods = -1;
    if ((rTimeout > 0) && (m_rCaretBlinkingPeriod > 0))
    {
        nPeriods = static_cast<XINT32>(rTimeout / m_rCaretBlinkingPeriod);
        m_fCaretTimeoutEnabled = TRUE;
    }
    else
    {
        m_fCaretTimeoutEnabled = FALSE;
    }

    if (nPeriods <= 0)
    {
        // Fallback value is 3 periods
        nPeriods = 3;
    }

    return static_cast<XFLOAT>(nPeriods) * m_rCaretBlinkingPeriod;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Creates a UIElement used to render the caret if not already allocated.
//
//------------------------------------------------------------------------
void CTextBoxView::EnsureCaretElement()
{
    CValue valueCaretColor;
    valueCaretColor.SetColor(DefaultCaretColor); // white.

    CREATEPARAMETERS brushCreateParams(GetContext(), valueCaretColor);
    CValue value;
    xref_ptr<CSolidColorBrush> spBrush;

    if (nullptr == m_spCaretElement)
    {
        ASSERT(nullptr == m_spCaretBlink);

        CreateCaretShape();

        // Override default property values we care about.
        value.Set(DirectUI::HorizontalAlignment::Left);
        IFCFAILFAST(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_HorizontalAlignment, value));

        value.Set(DirectUI::VerticalAlignment::Top);
        IFCFAILFAST(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::FrameworkElement_VerticalAlignment, value));

        value.SetBool(FALSE);
        IFCFAILFAST(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_IsHitTestVisible, value));

        value.Set(DirectUI::ElementCompositeMode::DestInvert);
        IFCFAILFAST(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::UIElement_CompositeMode, value));

        IFCFAILFAST(CreateDO(spBrush.ReleaseAndGetAddressOf(), &brushCreateParams));
        IFCFAILFAST(m_spCaretElement->SetValueByKnownIndex(KnownPropertyIndex::Shape_Fill, spBrush.get()));
    }

    if (GetFirstChildNoAddRef() == nullptr)
    {
        IFCFAILFAST(AddChild(m_spCaretElement));

        IFCFAILFAST(SetupCaretBlinkAnimation());
    }
}

// Fill the m_spCaretElement field with either a Rect (LTR) or a Path (RTL)
void CTextBoxView::CreateCaretShape()
{
    CREATEPARAMETERS cp(GetContext());

    ASSERT(m_spCaretElement == nullptr);

    if (!ShouldShowRTLIndicator())
    {
        xref_ptr<CRectangle> spCaretElementRect;
        IFCFAILFAST(CreateDO(spCaretElementRect.ReleaseAndGetAddressOf(), &cp));
        m_spCaretElement = std::move(spCaretElementRect);
    }
    else
    {
        xref_ptr<CPath> spCaretElementPath;
        IFCFAILFAST(CreateDO(spCaretElementPath.ReleaseAndGetAddressOf(), &cp));
        m_spCaretElement = std::move(spCaretElementPath);
    }
}

_Check_return_ HRESULT CTextBoxView::SetupCaretBlinkAnimation()
{
    auto core = GetContext();
    CREATEPARAMETERS cp(core);

    CValue value;
    xref_ptr<CTimeSpan> pTimeSpan;
    xref_ptr<CTimeSpan> pTimeSpanStart;

    ASSERT(m_spCaretElement != nullptr);
    ASSERT(nullptr == m_spCaretBlink);

    m_rCaretBlinkingPeriod = CTextBoxHelpers::GetCaretBlinkingPeriod();

    // Add a storyboard to make the caret blink.  Leave it paused.
    IFC_RETURN(CTextBoxHelpers::CreateCaretAnimationStoryboard(
        m_spCaretBlink.ReleaseAndGetAddressOf(),
        core,
        m_spCaretElement,
        m_spCaretElement->GetPropertyByIndexInline(KnownPropertyIndex::UIElement_Opacity),
        GetCaretOpacity(),
        m_rCaretBlinkingPeriod));

    IFC_RETURN(m_spCaretBlink->BeginPrivate(TRUE /* Top-level Storyboard */));
    IFC_RETURN(m_spCaretBlink->PausePrivate());

    // Create and initialize the blink timeout timer
    IFC_RETURN(CreateDO(m_spCaretTimer.ReleaseAndGetAddressOf(), &cp));
    value.SetInternalHandler(OnCaretTimeout);
    IFC_RETURN(m_spCaretTimer->AddEventListener(
        EventHandle(KnownEventIndex::DispatcherTimer_Tick),
        &value,
        EVENTLISTENER_INTERNAL, nullptr, FALSE));

    IFC_RETURN(CreateDO(m_spCaretStartTimer.ReleaseAndGetAddressOf(), &cp));
    value.SetInternalHandler(OnCaretStart);
    IFC_RETURN(m_spCaretStartTimer->AddEventListener(
        EventHandle(KnownEventIndex::DispatcherTimer_Tick),
        &value,
        EVENTLISTENER_INTERNAL, nullptr, FALSE));

    IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));
    pTimeSpan->m_rTimeSpan = CheckCaretBlinkTimeout();
    IFC_RETURN(m_spCaretTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval,
        pTimeSpan.get()));

    IFC_RETURN(CreateDO(pTimeSpanStart.ReleaseAndGetAddressOf(), &cp));
    pTimeSpanStart->m_rTimeSpan = TextSelectionSettings::Get()->m_rCaretBlinkStart;
    IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &cp));
    IFC_RETURN(m_spCaretStartTimer->SetValueByKnownIndex(KnownPropertyIndex::DispatcherTimer_Interval,
        pTimeSpanStart.get()));

    IFC_RETURN(m_spCaretTimer->SetTargetObject(this));  // Make sure we can get "this" from the event handler
    IFC_RETURN(m_spCaretTimer->Stop());     // Make sure the timer is stopped

    IFC_RETURN(m_spCaretStartTimer->SetTargetObject(this));  // Make sure we can get "this" from the event handler
    IFC_RETURN(m_spCaretStartTimer->Stop());     // Make sure the timer is stopped

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:   The caret blink timeout timer fires this event
//
//------------------------------------------------------------------------
/* static */
_Check_return_ HRESULT CTextBoxView::OnCaretTimeout(
        _In_ CDependencyObject *pSender,
        _In_ CEventArgs* pEventArgs
        )
{
    xref_ptr<CDispatcherTimer> timer(static_cast<CDispatcherTimer*>(pSender));
    IFC_RETURN(timer->WorkComplete());
    IFC_RETURN(timer->Stop());

    xref_ptr<CTextBoxView> thisTextBoxView(static_sp_cast<CTextBoxView>(timer->GetTargetObject()));

    if (thisTextBoxView.get())
    {
        IFC_RETURN(thisTextBoxView->PauseCaretBlink());
    }
    return S_OK;
}

/* static */
_Check_return_ HRESULT CTextBoxView::OnCaretStart(
    _In_ CDependencyObject *pSender,
    _In_ CEventArgs* pEventArgs
)
{
    xref_ptr<CDispatcherTimer> timer(static_cast<CDispatcherTimer*>(pSender));
    IFC_RETURN(timer->WorkComplete());
    IFC_RETURN(timer->Stop());

    xref_ptr<CTextBoxView> thisTextBoxView(static_sp_cast<CTextBoxView>(timer->GetTargetObject()));
    IFCPTR_RETURN(thisTextBoxView.get());
    IFC_RETURN(thisTextBoxView->ResetCaretBlink());
    IFC_RETURN(thisTextBoxView->m_spCaretBlink->ResumePrivate());
    if (thisTextBoxView->m_fCaretTimeoutEnabled)
    {
        IFC_RETURN(thisTextBoxView->m_spCaretTimer->Start());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Repositions the caret blink animation to the opaque state (which
//      then alternates every few hundred ms with transparency).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ResetCaretBlink()
{
    CValue value;
    value.SetFloat(0.0f);
    CREATEPARAMETERS createParams(GetContext(), value);

    xref_ptr<CTimeSpan> pTimeSpan;
    IFC_RETURN(CreateDO(pTimeSpan.ReleaseAndGetAddressOf(), &createParams));

    IFC_RETURN(m_spCaretBlink->SeekPrivate(pTimeSpan));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Pauses the caret blinking because gripper has been grabbed - also disables
//      ResumeCaretBlink until gripper is released (and GripperResumeCaretBlink
//      is called).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GripperPauseCaretBlink()
{
    if (m_spCaretBlink)
    {
        IFC_RETURN(PauseCaretBlink());
        m_fBlinkPausedForGripper = TRUE;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resumes the caret blinking when a gripper is released also reenables
//      ResumeCaretBlink (which was disabled when gripper was grabbed).
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GripperResumeCaretBlink()
{
    if (m_spCaretBlink)
    {
        m_fBlinkPausedForGripper = FALSE;
        IFC_RETURN(ResumeCaretBlink());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Pauses the caret blinking, adjusting the animation position to
//      making sure it is frozen at the top opacity level. Also makes sure
//      the blink timeout timer is stopped.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::PauseCaretBlink()
{
    IFC_RETURN(ResetCaretBlink());
    IFC_RETURN(m_spCaretBlink->PausePrivate());
    IFC_RETURN(m_spCaretTimer->Stop());
    IFC_RETURN(m_spCaretStartTimer->Stop());
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Resumes caret blinking after it timed out or was paused for any
//      other reason. Restarts the blinking timeout if it is enabled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ResumeCaretBlink()
{
    if (!m_fBlinkPausedForGripper)
    {
        IFC_RETURN(m_spCaretStartTimer->Start());
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    pBounds->left = 0.0f;
    pBounds->top = 0.0f;

    pBounds->right = GetActualWidth();
    pBounds->bottom = GetActualHeight();

    RRETURN(hr);
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
_Check_return_ HRESULT CTextBoxView::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = DoesRectContainPoint(innerBounds, target);

    return S_OK;
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
_Check_return_ HRESULT CTextBoxView::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = target.IntersectsRect(innerBounds);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Used to ask the host for a default font.
//      The BSTR will be freed by the caller of this function.
//
//---------------------------------------------------------------------------
WCHAR* CTextBoxView::GetDefaultFont()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    xstring_ptr strFontFamilyName;
    BSTR fontName = NULL;
    const TextFormatting* pTextFormatting = NULL;

    IFC(GetTextFormatting(&pTextFormatting));
    IFC(pTextFormatting->m_pFontFamily->get_Source(&strFontFamilyName));
    fontName = SysAllocStringLen(strFontFamilyName.GetBuffer(), strFontFamilyName.GetCount());
    IFCOOMFAILFAST(fontName);

Cleanup:
    return fontName;
}

static const WCHAR DefaultLanguageString[] = L"en-US";
//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo override,  Used to ask the host for the name of
//      a given font face (while streaming out).
//
//---------------------------------------------------------------------------
WCHAR* CTextBoxView::GetSerializableFontName(
    _In_ XUINT32 fontFaceId
    )
{
    HRESULT hr = S_OK;
    BSTR fontName = NULL;
    const DWriteFontFace *pFontFace = NULL;
    IDWriteLocalizedStrings *pDWriteLocalizedStrings = NULL;
    XUINT32 localeIndex;
    BOOL localeExists;
    XUINT32 familyNameLength;
    WCHAR *pFamilyName = NULL;



    WinTextCore *pWinTextCore = NULL;
    IFC(GetWinTextCore(&pWinTextCore));

    // If this fails, RichEdit is likely giving us a bad fontFaceId.
    pFontFace = pWinTextCore->GetFontFaceNoAddRef(fontFaceId);
    IFCPTR(pFontFace);
    IFC(pFontFace->GetFontFamilyNames(&pDWriteLocalizedStrings));

    IFC(pDWriteLocalizedStrings->FindLocaleName(
        DefaultLanguageString,
        &localeIndex,
        &localeExists));

    if (!localeExists)
    {
        if(pDWriteLocalizedStrings->GetCount() > 0)
        {
            localeIndex = 0;
        }
        else
        {
            // We do not expect that the font will have no localized family names.
            IFC(E_UNEXPECTED);
        }
    }

    IFC(pDWriteLocalizedStrings->GetStringLength(
        localeIndex,
        &familyNameLength));

    pFamilyName = new WCHAR[familyNameLength + 1];
    IFC(pDWriteLocalizedStrings->GetString(
        localeIndex,
        pFamilyName,
        familyNameLength + 1));

    fontName = SysAllocStringLen(pFamilyName, familyNameLength);
    IFCOOMFAILFAST(fontName);

Cleanup:
    if (hr != S_OK)
    {
        SysFreeString(fontName);
        fontName = NULL;
    }

    ReleaseInterface(pDWriteLocalizedStrings);
    delete[] pFamilyName;

    return fontName;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo implementation, gets a handle to font family given
//      an initial language and text run.
//
//  Notes:
//      RichEdit uses the handle for future calls to GetFontFace.
//
//---------------------------------------------------------------------------
XUINT32 CTextBoxView::GetRunFontFaceId(
    _In_z_ const WCHAR *pCurrentFontName,
    _In_ XUINT32 weight,
    _In_ XUINT32 stretch,
    _In_ XUINT32 style,
    _In_ XUINT32 lcid,
    _In_reads_opt_(charCount) const WCHAR *pText,
    _In_ XUINT32 charCount,
    _In_ XUINT32 fontFaceIdCurrent,
    _Out_ XUINT32 *pRunCount
    )
{
    HRESULT hr = S_OK;
    XUINT32 fontId = 0;
    CSharedName *pSharedName = NULL;
    CTypefaceCollection *pFontCollection = NULL;
    CCompositeFontFamily *pCompositeFontFamily = NULL;
    IPALUri *pBaseUri = NULL;
    WCHAR character;

    if (0 == charCount)
    {
        // Empty document case. Use a zero length string instead.
        character = L'\0';
        pText = &character;
    }

    IFC(CSharedName::Create(xstrlen(pCurrentFontName), pCurrentFontName, &pSharedName));

    IFC(GetSystemFontCollection(&pFontCollection));

    pBaseUri = GetBaseUri();
    IFC(pFontCollection->LookupCompositeFontFamily(pSharedName, pBaseUri, &pCompositeFontFamily));

    IFC(GetFontFaceRun(
        pCompositeFontFamily,
        weight,
        stretch,
        style,
        pText,
        charCount,
        &fontId,
        pRunCount));

Cleanup:
    ReleaseInterface(pSharedName);
    ReleaseInterface(pFontCollection);
    ReleaseInterface(pBaseUri);

    if (SUCCEEDED(hr))
    {
        return fontId;
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      IProvideFontInfo implementation, converts a font id from
//      a previous GetRunFontFaceId call into an IDWriteFontFace.
//
//---------------------------------------------------------------------------
IDWriteFontFace* CTextBoxView::GetFontFace(
    _In_ XINT32 fontId
    )
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    const DWriteFontFace  *pFontFace = nullptr;
    IDWriteFontFace *pDWriteFontFace = nullptr;
    WinTextCore *pWinTextCore = nullptr;
    IFC(GetWinTextCore(&pWinTextCore));

    pFontFace = pWinTextCore->GetFontFaceNoAddRef(fontId);
    // If this fails, RichEdit is likely giving us a bad fontId.
    IFCPTR(pFontFace);

    pDWriteFontFace = pFontFace->GetFontFace();
    AddRefInterface(pDWriteFontFace);

Cleanup:
    return pDWriteFontFace;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Scans a range of text, looking for the first run that matches a single
//      font face.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetFontFaceRun(
    _In_ CCompositeFontFamily *pCompositeFontFamily,
    _In_ XUINT32 weight,
    _In_ XUINT32 stretch,
    _In_ XUINT32 style,
    _In_reads_(charCount) const WCHAR *pText,
    _In_ XUINT32 charCount,
    _Out_ XUINT32 *pFontId,
    _Out_ XUINT32 *pRunCount
    )
{
    // The use of zero for the optical size will cause the MapCharacters to fall
    // back to legacy versions (non optical).
    CFontFaceCriteria fontFaceCriteria(weight, style, stretch, 0.0f);
    DWriteFontFace *pDWriteFontFace;
    const TextFormatting* pTextFormatting = NULL;
    XFLOAT mappedScale;

    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    wrl::ComPtr<PALText::IFontFace> pFontFace;
    IFC_RETURN(pCompositeFontFamily->MapCharacters(
        pText,
        charCount,
        pTextFormatting->GetResolvedLanguageStringNoRef().GetBuffer(),
        pTextFormatting->GetResolvedLanguageListStringNoRef().GetBuffer(),
        NULL,
        fontFaceCriteria,
        &pFontFace,
        pRunCount,
        &mappedScale));

    ASSERT(pFontFace != NULL);

    pDWriteFontFace = reinterpret_cast<DWriteFontFace *>(pFontFace.Get());

    WinTextCore *pWinTextCore = NULL;
    IFC_RETURN(GetWinTextCore(&pWinTextCore));
    pWinTextCore->GetFontFaceId(pDWriteFontFace, pFontId);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the global font collection.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetSystemFontCollection(
    _Outptr_ CTypefaceCollection **ppFontCollection
    )
{
    CTextCore *pTextCore;
    IFontAndScriptServices *pFontAndScriptServices;
    IFontCollection *pFontCollection;

    IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
    IFC_RETURN(pFontAndScriptServices->GetSystemFontCollection(&pFontCollection));

    *ppFontCollection = reinterpret_cast<CTypefaceCollection *>(pFontCollection);

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the global Win Text Core
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetWinTextCore(
        _Outptr_ WinTextCore **ppWinTextCore
        )
{
    CTextCore *pTextCore;
    IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
    *ppWinTextCore = pTextCore->GetWinTextCore();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Direct manipulation started handler.
//
//------------------------------------------------------------------------
void CTextBoxView::OnDirectManipulationStarted()
{
    m_isDMActive = TRUE;
    IGNOREHR(ShowOrHideCaret());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Direct manipulation completed handler.
//
//------------------------------------------------------------------------
void CTextBoxView::OnDirectManipulationCompleted()
{
    m_isDMActive = FALSE;
    m_directManipulationOffset.x = 0.0f;
    m_directManipulationOffset.y = 0.0f;
    IGNOREHR(ShowOrHideCaret());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Direct manipulation delta handler.
//
//------------------------------------------------------------------------
void  CTextBoxView::OnDirectManipulationDelta(
    FLOAT xCumulativeTranslation,
    FLOAT yCumulativeTranslation,
    FLOAT zCumulativeFactor
    )
{
    ASSERT(m_isDMActive);
    m_directManipulationOffset.x = xCumulativeTranslation;
    m_directManipulationOffset.y = yCumulativeTranslation;
}

// Invokes the IScrollOwner::NotifyHorizontalOffsetChanging and IScrollOwner::NotifyVerticalOffsetChanging callbacks and updates the m_scrollData offset
// fields if the offsets change. Then invokes the IScrollOwner::InvalidateScrollInfo callback if the offsets change or forceInvalidScrollInfo is True.
_Check_return_ HRESULT
CTextBoxView::SetScrollDataOffsets(XDOUBLE horizontalOffset, XDOUBLE verticalOffset, bool forceInvalidScrollInfo)
{
    bool offsetChanging = horizontalOffset != m_scrollData.HorizontalOffset || verticalOffset != m_scrollData.VerticalOffset;

    if (offsetChanging)
    {
        IFC_RETURN(FxCallbacks::TextBox_NotifyOffsetsChanging(this, m_scrollData.HorizontalOffset, horizontalOffset, m_scrollData.VerticalOffset, verticalOffset));

        m_scrollData.HorizontalOffset = horizontalOffset;
        m_scrollData.VerticalOffset = verticalOffset;
    }

    if (offsetChanging || forceInvalidScrollInfo)
    {
        IFC_RETURN(FxCallbacks::TextBox_InvalidateScrollInfo(this));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Evaluates the expected visibility of the caret and then updates
//      state to match.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ShowOrHideCaret()
{
    if (!m_pTextControl)
    {
        return S_OK;
    }

    if (m_isRendering)
    {
        // If in the middle of rendering, post a dispatcher callback to change caret visibility.
        // We should not modify the tree in middle of render.
        IFC_RETURN(FxCallbacks::TextBoxView_CaretVisibilityChanged(this));
    }
    else
    {
        bool isCaretRendered;
        IFC_RETURN(GetShouldRenderCaret(&isCaretRendered));

        if (isCaretRendered)
        {
            IFC_RETURN(ShowCaretElement());
        }
        else
        {
            IFC_RETURN(HideCaretElement());
        }

        // Invalidate rendering to redraw the caret.
        NWSetContentDirty(this, DirtyFlags::Render);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns whether the caret should be rendered.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextBoxView::GetShouldRenderCaret(_Out_ bool *pShouldRenderCaret)
{
    auto contentRoot = VisualTree::GetContentRootForElement(this);
    static auto runtimeEnabledFeatureDetector = RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector();
    bool isCaretRendered = m_canShowCaret
        && !m_pTextControl->IsReadOnly()
        && m_pTextControl->IsFocused()
        && contentRoot->GetFocusManagerNoRef()->IsPluginFocused()
        && !IsDMActive()
        && !(contentRoot->GetAKExport().IsActive())
        && !runtimeEnabledFeatureDetector->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableTextBoxCaret);

    // Don't show the caret if we have a non-empty selection.
    if (isCaretRendered)
    {
        Microsoft::WRL::ComPtr<ITextSelection2> spSelection;
        long length;

        IFC_RETURN(m_pTextControl->GetSelection(&spSelection));
        IFC_RETURN(spSelection->GetCch(&length));

        isCaretRendered = (0 == length); // Length is negative when active end precedes anchor.
    }

    *pShouldRenderCaret = isCaretRendered;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to update the caret element
//
//------------------------------------------------------------------------
void CTextBoxView::CaretChanged()
{
    IGNOREHR(UpdateCaretElement());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to update the caret element visibility
//
//------------------------------------------------------------------------
void CTextBoxView::CaretVisibilityChanged()
{
    IGNOREHR(ShowOrHideCaret());
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Dispatcher callback to invalidate the view.
//
//------------------------------------------------------------------------
void CTextBoxView::InvalidateViewDispatcherCallback()
{
    HRESULT hr = S_OK; // WARNING_IGNORES_FAILURES
    CTextCore *pTextCore = NULL;
    IFC(GetContext()->GetTextCore(&pTextCore));
    pTextCore->GetWinTextCore()->ProcessInvalidation(this);

Cleanup:
    ;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Decides whether to invalidate the view now or issue a dispatcher
//      call for that.
//  Notes:
//      Sometimes we need to invalidate the view during arrange.
//      But this will cause the layout manager to go beyond the MaxLayoutIterations
//      when alot of textboxes are being layout.
//      Thus we defer the layout invalidation to be processed at a later time
//      when the layout manager has completed its layout pass.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::InvalidateView()
{
    // Skip this invalidation when CTextBoxView::MeasureOverride is
    // being processed. This condition occurs when CTextBoxView::MeasureOverride
    // sends the EM_REQUESTRESIZE message to RichEdit to request the current size.
    if (!m_isMeasuring)
    {
        if (m_issueDispatcherViewInvalidation || m_isRendering)
        {
            CTextCore *pTextCore = NULL;
            IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
            IFC_RETURN(pTextCore->GetWinTextCore()->DelayInvalidation(this));
        }
        else
        {
            m_pTextControl->InvalidateView();
            IFC_RETURN(m_pTextControl->UpdateLayout()); // scroll extent increased in CTextBoxView::MeasureOverride
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns the selection rect in local coordinates.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetSelectionRect(
    _Out_ XRECT *pSelectionRect
    )
{
    HRESULT hr = S_OK;

    ITextSelection2* pSelection = NULL;
    XRECT selectionRect;
    XLONG selStart, selEnd = 0;

    IFC(m_pTextControl->GetSelection(&pSelection));
    IFC(pSelection->GetStart(&selStart));
    IFC(pSelection->GetEnd(&selEnd));

    EmptyRect(pSelectionRect);

    if (selStart != selEnd)
    {
        XLONG x1, x2, y1, y2 = 0;
        XPOINT selectionTopLeft;
        XPOINT selectionBottomRight;

        IFC(pSelection->GetPoint(TA_LEFT | tomStart | tomAllowOffClient | TA_TOP, &x1, &y1));
        if (hr == S_FALSE)
        {
            goto Cleanup;
        }
        IFC(pSelection->GetPoint(TA_LEFT | tomEnd | tomAllowOffClient | TA_BOTTOM, &x2, &y2));
        if (hr == S_FALSE)
        {
            goto Cleanup;
        }

        selectionTopLeft.x = MIN(x1, x2);
        selectionTopLeft.y = MIN(y1, y2);
        selectionBottomRight.x = MAX(x1, x2);
        selectionBottomRight.y = MAX(y1, y2);

        if (m_pTextControl->ShouldUseVisualPixels())
        {
            XPOINTF selectionTopLeftF = { static_cast<float>(selectionTopLeft.x), static_cast<float>(selectionTopLeft.y) };
            XPOINTF selectionBottomRightF = { static_cast<float>(selectionBottomRight.x), static_cast<float>(selectionBottomRight.y) };
            // In this mode we use client space instead of screen space
            IFC(m_pTextControl->ClientToTextBox(&selectionTopLeftF)); // Convert from client coordinates to local space
            IFC(m_pTextControl->ClientToTextBox(&selectionBottomRightF)); // Convert from client coordinates to local space
            selectionTopLeft = { static_cast<LONG>(selectionTopLeftF.x), static_cast<LONG>(selectionTopLeftF.y) };
            selectionBottomRight = { static_cast<LONG>(selectionBottomRightF.x), static_cast<LONG>(selectionBottomRightF.y) };
        }
        else
        {
            IFC(m_pTextControl->ScreenToTextBox(&selectionTopLeft)); // Convert from screen coordinates to local space
            IFC(m_pTextControl->ScreenToTextBox(&selectionBottomRight)); // Convert from screen coordinates to local space
        }

        selectionRect.X = selectionTopLeft.x;
        selectionRect.Y = selectionTopLeft.y;

        // We use the difference of the coordinates in client space to get the width and height
        // locally . We can't use the screen coordinates because there maybe a transform applied
        // for example plateau scale, and so the width will be miscalculated to be larger/smaller
        // than it really is relative to the TextBoxView.
        selectionRect.Width  = selectionBottomRight.x - selectionTopLeft.x;
        selectionRect.Height = selectionBottomRight.y - selectionTopLeft.y;
    }
    else
    {
        selectionRect.X = static_cast<XINT32>(m_caretRect.X);
        selectionRect.Y = static_cast<XINT32>(m_caretRect.Y);
        selectionRect.Width = static_cast<XINT32>(m_caretRect.Width);
        selectionRect.Height = static_cast<XINT32>(m_caretRect.Height);
    }

    *pSelectionRect = selectionRect;

Cleanup:
    ReleaseInterface(pSelection);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Checks if the ends of the selection are within (or at least border
//      on) the viewport.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::IsSelectionEdgeVisible(
    _Out_ bool &fVisible
    )
{
    XRECT_RB viewportRect;
    XRECT_RB beginRect;
    XRECT_RB endRect;

    fVisible = FALSE;

    IFC_RETURN(TxGetViewportRect(&viewportRect));

    IFC_RETURN(GetSelectionEdgeRects(&beginRect, &endRect));

    fVisible = (DoRectsIntersectOrTouch(beginRect, viewportRect) || DoRectsIntersectOrTouch(endRect, viewportRect));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the rects for the start and end of the current selection range
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetSelectionEdgeRects(
    _Out_ XRECT_RB* pBeginRect,
    _Out_ XRECT_RB* pEndRect)
{
    ZeroMemory(pBeginRect, sizeof(XRECT_RB));
    ZeroMemory(pEndRect, sizeof(XRECT_RB));
    XRECT_RB* lpRects[2] = {pBeginRect, pEndRect};

    LRESULT unused;

    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_GETSELECTIONEDGES, (WPARAM)nullptr, (LPARAM)lpRects, &unused));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Gets the rects of the current selection's text lines.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::GetSelectionRects(_Inout_ std::vector<RECT>& rects)
{
    rects.clear();

    GETRECTSOFRANGE getRectsOfRange;

    xref_ptr<ITextSelection2> selection;
    IFC_RETURN(m_pTextControl->GetSelection(selection.ReleaseAndGetAddressOf()));

    IFC_RETURN(selection->GetStart(&getRectsOfRange.chrg.cpMin));
    IFC_RETURN(selection->GetEnd(&getRectsOfRange.chrg.cpMax));

    getRectsOfRange.pvArg = &rects;
    getRectsOfRange.pfnCallback = [](PVOID pvArg, LPCRECT pcrect) -> BOOL
    {
        std::vector<RECT> *rects = reinterpret_cast<std::vector<RECT>*>(pvArg);

        if (pcrect)
        {
            rects->push_back(*pcrect);
        }

        return TRUE;
    };

    LRESULT unused;
    IFC_RETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_GETRECTSOFRANGE, (WPARAM)&getRectsOfRange, (LPARAM)nullptr, &unused));

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Instructs RichEdit to hide the selection grippers.  Does nothing if
//      the grippers are not currently displayed.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::HideSelectionGrippers()
{
    RRETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_SETTOUCHOPTIONS, RTO_SHOWHANDLES, FALSE, NULL));
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Instructs RichEdit to show the selection grippers.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::ShowSelectionGrippers(bool ignoreDrag)
{
    // If there isn't any text in the control, we don't show the grippers.
    if (!m_pTextControl->IsEmpty())
    {
        RRETURN(m_pTextControl->GetTextServices()->TxSendMessage(EM_SETTOUCHOPTIONS, ignoreDrag?RTO_SHOWHANDLES_IGNOREDRAG:RTO_SHOWHANDLES, TRUE, NULL));
    }
    else
    {
        RRETURN(S_OK);
    }
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Queries RichEdit to determine the current visiblity state of the selection grippers.
//
//---------------------------------------------------------------------------
bool CTextBoxView::AreGrippersVisible()
{
    LRESULT result = 0;
    IGNOREHR(m_pTextControl->GetTextServices()->TxSendMessage(EM_GETTOUCHOPTIONS, RTO_SHOWHANDLES, NULL, &result));
    return !!result;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBoxView::OnContextMenuDismiss
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::OnContextMenuDismiss()
{
    if (m_showGrippersOnCMDismiss)
    {
        m_showGrippersOnCMDismiss = false;
        IFC_RETURN(ShowSelectionGrippers());
    }
    else
    {
        IFC_RETURN(ShowOrHideCaret());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBoxView::OnContextMenuOpen
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBoxView::OnContextMenuOpen(_In_ bool isTouchInvoked)
{
    m_showGrippersOnCMDismiss = isTouchInvoked;
    if (AreGrippersVisible())
    {
        IFC_RETURN(HideSelectionGrippers());
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify collection and its brush that the theme has changed.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBoxView::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    const TextFormatting* pTextFormatting = NULL;

    IFC_RETURN(CFrameworkElement::NotifyThemeChangedCore(theme, fForceRefresh));

    if (m_pTextControl == nullptr)
    {
        return S_OK;
    }

    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    // RichEdit supports only solid color brushes
    XUINT32 color = 0xFF000000;
    if (pTextFormatting->m_pForeground != NULL &&
        pTextFormatting->m_pForeground->GetTypeIndex() == KnownTypeIndex::SolidColorBrush)
    {
        color = TextBoxView_Internal::BrushToRGB(pTextFormatting->m_pForeground);
    }

    // Default charformat could be different between themes, such as foreground color, we need to notify RichEdit the new default charformat.
    if (m_charFormat.TextColor != color)
    {
        IFC_RETURN(UpdateDefaultCharFormat(CFM_COLOR));
    }


    // Theme change may have changed high contrast mode and we disable color fonts in high contrast,
    // so we need to have RichEdit re-push all of its rendering so we can reapply the color font
    // choice in DrawGlyphRun.
    if (m_pTextControl->m_isColorFontEnabled)
    {
        m_pTextControl->InvalidateViewAndForceRedraw();
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::NotifyApplicationHighContrastAdjustmentChanged()
{
    DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
    IFC_RETURN(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

    IFC_RETURN(OnApplicationHighContrastAdjustmentChanged(applicationHighContrastAdjustment));

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method to clean up all the device related resources
//      like brushes, textures etc. on this element.
//
//-----------------------------------------------------------------------------
void CTextBoxView::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);

    if (m_pRenderTarget != nullptr)
    {
        m_pRenderTarget->CleanupRealizations();
    }
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleanup all the device related resources on leave.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CTextBoxView::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));
    if (params.fIsLive)
    {
        if (m_pRenderTarget != nullptr)
        {
            m_pRenderTarget->CleanupRealizations();
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::UIElement_HighContrastAdjustment:
        {
            IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(args.m_pNewValue->AsEnum())));
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::OnHighContrastAdjustmentChanged(
    _In_ DirectUI::ElementHighContrastAdjustment newValue
    )
{
    if (Convert(newValue) != m_highContrastAdjustment)
    {
        m_highContrastAdjustment = Convert(newValue);

        // ApplicationHighContrastAdjustment is not propagated through the tree on app start up, we have to make this check to ensure our default value
        // matches the value set in the application XAML.
        if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Application))
        {
            DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
            IFC_RETURN(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

            m_applicationHighContrastAdjustment = Convert(applicationHighContrastAdjustment);
        }

        IFC_RETURN(UpdateHighContrastAdjustments());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::OnApplicationHighContrastAdjustmentChanged(
    _In_ DirectUI::ApplicationHighContrastAdjustment newValue
    )
{
    if (Convert(newValue) != m_applicationHighContrastAdjustment)
    {
        m_applicationHighContrastAdjustment = Convert(newValue);

        IFC_RETURN(UpdateHighContrastAdjustments());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBoxView::UpdateHighContrastAdjustments()
{
    if (m_pRenderTarget != nullptr)
    {
        m_pRenderTarget->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), false /*useHyperlinkForeground*/);
    }

    // If HighContrastAdjustment is enabled the BorderElement backgroud is overridden for the SystemColorWindowBrush, this ensures high contrast with the foreground color of the control.
    if (m_pTextControl)
    {
        xref_ptr<CDependencyObject> borderDO = m_pTextControl->GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"BorderElement"));

        if (auto border = do_pointer_cast<CBorder>(borderDO))
        {
            border->SetUseBackgroundOverride(IsHighContrastAdjustmentEnabled());
        }
    }

    InvalidateMeasure();
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);

    return S_OK;
}

bool CTextBoxView::IsHighContrastAdjustmentActive() const
{
    if (UseHighContrastSelection(GetContext()))
    {
        return IsHighContrastAdjustmentEnabled();
    }

    return false;
}

bool CTextBoxView::IsHighContrastAdjustmentEnabled() const
{
    if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Auto))
    {
        return true;
    }
    else if (m_highContrastAdjustment == Convert(DirectUI::ElementHighContrastAdjustment::Application) &&
        m_applicationHighContrastAdjustment == Convert(DirectUI::ApplicationHighContrastAdjustment::Auto))
    {
        return true;
    }

    return false;
}

_Check_return_ HRESULT CTextBoxView::ChangePlaceholderBackPlateVisibility(bool visible)
{
    xref_ptr<CDependencyObject> placeholderTextDO = m_pTextControl->GetTemplateChild(XSTRING_PTR_EPHEMERAL(L"PlaceholderTextContentPresenter"));

    if (placeholderTextDO)
    {
        CTextBlock* placeholderText = nullptr;

        if (auto textBlock = do_pointer_cast<CTextBlock>(placeholderTextDO))
        {
            placeholderText = textBlock;
        }
        else if (auto contentPresenter = do_pointer_cast<CContentPresenter>(placeholderTextDO))
        {
            placeholderText = contentPresenter->GetTextBlockNoRef();
        }
        else if (auto contentControl = do_pointer_cast<CContentControl>(placeholderTextDO))
        {
            placeholderText = contentControl->GetTextBlockNoRef();
        }

        if (placeholderText)
        {
            placeholderText->SetBackPlateVisibility(visible);
        }
    }

    return S_OK;
}


