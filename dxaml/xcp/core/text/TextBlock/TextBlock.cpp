// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <functional>

#include "BlockLayoutEngine.h"
#include "DrawingContext.h"
#include "BlockNode.h"
#include "ParagraphNode.h"
#include "BlockNodeBreak.h"
#include "RectUtil.h"

#include "TextSelectionManager.h"
#include <math.h>
#include <ContentRenderer.h>
#include <FocusRectManager.h>
#include <TextHighlightRenderer.h>
#include <TextHighlighterCollection.h>
#include <TextHighlighter.h>
#include "HighlightRegion.h"
#include "application.h"

#include "TextBlockView.h"

#include <D2DTextDrawingContext.h>
#include <textsegment.h>
#include <compositortree.h>
#include <dwrite_2.h>
#include <dwrite_3.h>
#include <paltext.h>
#include <DWriteFontAndScriptServices.h>
#include <PALFontAndScriptServices.h>
#include <DWriteTextRenderer.h>
#include <DWriteFontCollection.h>
#include <fonts.h>
#include <DWriteTextAnalyzer.h>
#include "RootScale.h"

#include <TextAnalysis.h>

#undef max

using namespace Focus;
using namespace RichTextServices;

// TextBlock requires custom focus children collection since its focus children
// may be Hyperlinks, which are nested within its content. To correctly represent
// both and return them to FocusManager for tabbing, etc. a custom focus children
// collection is necessary.
class TextBlockFocusableChildrenCollection final : public CDOCollection
{
public:
    TextBlockFocusableChildrenCollection(_In_ CCoreServices *pCore)
        : CDOCollection(pCore, false)
    {}

    bool ShouldParentBePublic() const final { return false; }

    _Check_return_ HRESULT ValidateItem(_In_ CDependencyObject* pObject) final
    {
        if (pObject != nullptr && CFocusableHelper::IsFocusableDO(pObject))
        {
            return S_OK;
        }

        return E_INVALIDARG;
    }
};

CTextBlock::CTextBlock(_In_ CCoreServices *pCore)
    : CFrameworkElement(pCore)
    , m_strText()
{
    // Fields unique to TextBlock
    m_pInlines = nullptr;

    m_textWrapping = DirectUI::TextWrapping::NoWrap;
    m_isTextSelectionEnabled  = false;

    m_textMode = TextMode::DWriteLayout;
    m_shouldAddTextOnDeferredInlineCollectionCreation = FALSE;
    m_useHyperlinkForegroundOnBackPlate = FALSE;
    m_drawBackPlate = TRUE;
    m_fFastPathOptOutConditions = 0;
    m_hasBeenMeasured = FALSE;
    m_isDWriteTextLayoutDirty = FALSE;

    // Fields common to TextBlock and RichTextBlock
    m_eLineHeight = 0.0f; //implies no line height override
    m_textLineBounds = DirectUI::TextLineBounds::Full;
    m_textTrimming = DirectUI::TextTrimming::None;
    m_pFontContext = nullptr;
    m_pBlockLayout = nullptr;
    m_lineStackingStrategy = DirectUI::LineStackingStrategy::MaxHeight;
    m_pSelectionManager = nullptr;
    m_opticalMarginAlignment = DirectUI::OpticalMarginAlignment::None;
    m_textAlignment = DirectUI::TextAlignment::Left;
    m_highContrastAdjustment = Convert(DirectUI::ElementHighContrastAdjustment::None);
    m_applicationHighContrastAdjustment = Convert(DirectUI::ApplicationHighContrastAdjustment::Auto);

    // Fields common to TextBlock, RichTextBlock, and RichTextBlockOverflow
    m_redrawForArrange = false;
    m_padding.left = 0.0f;
    m_padding.top = 0.0f;
    m_padding.right = 0.0f;
    m_padding.bottom = 0.0f;
    m_maxLines = 0; // no max lines set
    m_layoutRoundingHeightAdjustment = 0.0f;
    m_selectionHighlight = nullptr;

    m_pTextView = nullptr;
    m_pPageNode = nullptr;

    m_textReadingOrder = DirectUI::TextReadingOrder::DetectFromContent;

    if (!pCore->IsInBackgroundTask())
    {
        IGNOREHR(SetCursor(MouseCursorDefault));
    }
    m_isColorFontEnabled = true;
}

CTextBlock::~CTextBlock()
{
    if (m_pInlines)
    {
        IGNOREHR(m_pInlines->Clear());
        VERIFYHR(m_pInlines->RemoveParent(this));
    }

    ReleaseInterface(m_pInlines);

    if (m_textHighlighters)
    {
        IGNOREHR(m_textHighlighters->Clear());
        VERIFYHR(m_textHighlighters->RemoveParent(this));
    }

    ReleaseInterface(m_textHighlighters);

    delete m_pFontContext;
    delete m_pBlockLayout;
    delete m_pPageNode;

    delete m_pTextView;
    VERIFYHR(TextSelectionManager::Destroy(&m_pSelectionManager));
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::EnterImpl
//
//  Synopsis:
//
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//      Following entering scope, applies xml/xaml whitespace rules to
//      content text in markup.
//
//      For xml:space="default", multiple whitespace/newlines are condensed
//      to a single whitespace.
//
//      Leading and trailing whitespace and newlines around text content (as
//      opposed to in Text=".." properties) is removed, except where it
//      immediately preceeds or follows more deeply nested elements.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params
    )
{
    // First bring this TextBlock into scope.
    IFC_RETURN(CFrameworkElement::EnterImpl(pNamescopeOwner, params));
    IFC_RETURN(CompressInlinesWhitespace(m_pInlines));

    if (params.fIsLive)
    {
        // Upon entering the live tree, a parent or ancestor may have changed and inherited properties
        // should be considered dirty.
        InvalidateContent();
    }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CTextBlock::LeaveImpl
//
//  Synopsis:   Called when the CTextBlock leaves the tree.
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::LeaveImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    LeaveParams params
    )
{
    if (IsFocused() && GetContext())
    {
        CFocusManager* pFocusManager = VisualTree::GetFocusManagerForElement(this);

        if(pFocusManager)
        {
            // Set the focus on the next focusable element.
            // If we remove the currently focused element from the live tree, inside a GettingFocus or LosingFocus handler,
            // we failfast. This is being tracked by Bug 9840123
            // Use InputActivationBehavior::NoActivate because removing this element from the tree shouldn't steal activation from another window/island.
            IFCFAILFAST(pFocusManager->SetFocusOnNextFocusableElement(static_cast<DirectUI::FocusState>(GetFocusState()), !params.fVisualTreeBeingReset, InputActivationBehavior::NoActivate));
        }

        // Ensure this control doesn't have a focus
        UpdateFocusState(DirectUI::FocusState::Unfocused);
    }

    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));

    if (params.fIsLive)
    {
        // Cleanup all the device related resources.
        if (m_pPageNode != nullptr)
        {
            m_pPageNode->CleanupRealizations();
        }

        if (m_pTextDrawingContext != nullptr)
        {
            m_pTextDrawingContext->ClearGlyphRunCaches();
        }

        // Deleting TextSelectionManager here so that Popup can be removed while tree is still alive,
        // If TextSelectionManager is created and not in a live tree, ~CTextBlock will do the deletion work.
        IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));

        // Upon leaving the live tree, a parent or ancestor may have changed and inherited properties
        // should be considered dirty.
        InvalidateContent();
     }

    return S_OK;
}

//-------------------------------------------------------------------------
//
//  Function:   CTextBlock::RecursiveInvalidateMeasure
//
//  Synopsis:   Recursively invalidate measure for the CTextBlock.
//              Also invalidate its content and content measure.
//
//-------------------------------------------------------------------------

void CTextBlock::RecursiveInvalidateMeasure()
{
    InvalidateContentMeasure();
    InvalidateContent();

    CUIElement::RecursiveInvalidateMeasure();
}

xref_ptr<CSolidColorBrush> CTextBlock::GetSelectionHighlightColor() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::TextBlock_SelectionHighlightColor, &result));
    return static_sp_cast<CSolidColorBrush>(result.DetachObject());
}

CHyperlink* CTextBlock::GetCurrentLinkNoRef() const
{
    return m_currentLink.get();
}

xref_ptr<CHyperlink> CTextBlock::GetPressedHyperlink() const
{
    return m_pressedHyperlink;
}

//------------------------------------------------------------------------
//
//  Method: CTextBlock::GetValue
//
//  Synopsis:
//      Performs any lazy computation required for properties such as
//      ActualWidth, ActualHeight and Text, before calling the standard
//      FrameworkElement GetValue implementation.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBlock::GetValue(
    _In_ const CDependencyProperty *pdp,
    _Out_ CValue *pValue
)
{
    switch (pdp->GetIndex())
    {
        case KnownPropertyIndex::TextBlock_Text:
        {
            // We lazily update TextBlock's text property on GetValue().
            // The drawback of this approach is - if there were edits to the inline collection
            // programmatically, and there is a binding to TextBlock.Text property
            // the binding target will not be updated. This limitation is being tracked
            // by Jolt bug #22750 for v.Next.

            if (m_pInlines != nullptr)
            {
                xref_ptr<CString> text;
                IFC_RETURN(m_pInlines->GetText(true /*insertNewlines*/, text.ReleaseAndGetAddressOf()));
                m_strText = text->m_strString;
            }
            // else if m_pInlines is null then m_strText should already be set to whatever is in the TextBlock.Text property.

            if (!m_strText.IsNullOrEmpty())
            {
                IFC_RETURN(SetPropertyIsLocal(pdp));
            }
            break;
        }

        case KnownPropertyIndex::TextBlock_Inlines:
        {
            IFC_RETURN(DeferredCreateInlineCollection());
            break;
        }

        case KnownPropertyIndex::TextBlock_SelectionHighlightColor:
        {
            if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_SelectionHighlightColor))
            {
                IFC_RETURN(CreateSelectionHighlightColor());
            }
            break;
        }

        case KnownPropertyIndex::TextBlock_TextHighlighters:
        {
            if (m_textHighlighters == nullptr)
            {
                IFC_RETURN(CreateTextHighlighters());

                // TextBlockView is required for the feature and pay for play so ensure it now.
                EnsureTextBlockView();
            }
            break;
        }
    }

    IFC_RETURN(CFrameworkElement::GetValue(pdp, pValue));
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::SetValue
//
//  Synopsis: Records a property value or a nested inline.
//
//  Special handling:
//      1) Inlines (run,span etc.) assigned to the TextBlock are appended
//         to the Inlines property.
//      2) An inline collection assigned directly to the Inlines property
//         is updated to point up (through m_pVisualParent) to this TextBlock.
//      3) For property settings that will require the text to be reformatted
//         call MarkTextDirty.
//      4) Direct assignment to the Text property causes construction of
//         an inline collection containing a run containing the string which
//         replaces any existing inline collection in the Inlines property.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::SetValue(_In_ const SetValueParams& args)
{
    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::TextBlock_FontFamily:
        {
            if (args.m_value.IsNull() &&
                (GetContext()->IsSettingValueFromManaged(this) || ParserOwnsParent()))
            {
                HRESULT hrToOriginate = E_NER_ARGUMENT_EXCEPTION;
                xephemeral_string_ptr parameters[2];

                ASSERT(parameters[0].IsNull());
                args.m_pDP->GetName().Demote(&parameters[1]);

                IGNOREHR(SetAndOriginateError(hrToOriginate, RuntimeError, AG_E_PROPERTY_INVALID, 2, parameters));
                IFC_RETURN(static_cast<HRESULT>(E_NER_ARGUMENT_EXCEPTION));
            }
            break;
        }

    case KnownPropertyIndex::TextBlock_LineHeight:
        // Validate LineHeight property setting is > 0.
        {
            if (args.m_value.GetType() == valueFloat || args.m_value.GetType() == valueDouble)
            {
                if (args.m_value.AsDouble() < 0.0f)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else
            {
                const CDouble* pDouble = nullptr;
                if (FAILED(DoPointerCast(pDouble, args.m_value)) ||
                    pDouble->m_eValue < 0.0f)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            break;
        }

    case KnownPropertyIndex::TextBlock_MaxLines:
        {
            if (args.m_value.GetType() == valueSigned)
            {
                if (args.m_value.AsSigned() < 0)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else
            {
                const CInt32* pInt32 = nullptr;
                if (FAILED(DoPointerCast(pInt32, args.m_value)) ||
                    pInt32->m_iValue < 0)
                {
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            break;
        }
    }

    IFC_RETURN(CFrameworkElement::SetValue(args));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::UpdateFastPathOptOutConditions
//
//  Synopsis: When some properties are changed, we need to update the internal TextBlock flags.
//  These flags are used to determine whether we can use the fast path(IDWriteTextLayout) or not.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::UpdateFastPathOptOutConditions(_In_ const CDependencyProperty* pDP)
{
    switch (pDP->GetIndex())
    {
        case KnownPropertyIndex::TextBlock_CharacterSpacing:
        {
            const TextFormatting* pTextFormatting = nullptr;
            IFC_RETURN(GetTextFormatting(&pTextFormatting));
            SetFastPathOptOutConditions(FastPathOptOutConditions::CharacterSpacing_Not_Default, pTextFormatting->m_nCharacterSpacing != 0);
            break;
        }

        case KnownPropertyIndex::TextBlock_TextTrimming:
        {
            SetFastPathOptOutConditions(FastPathOptOutConditions::TextTrimming_Is_Clip, m_textTrimming == DirectUI::TextTrimming::Clip);
            break;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::SetTextToInlineCollection()
{
    if (m_pInlines)
    {
        IFC_RETURN(m_pInlines->Clear());

        // Entire content is changing. Clear text selection.
        IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));

        m_currentLink.reset();
        m_pressedHyperlink.reset();
    }

    if (m_strText.IsNullOrEmpty())
    {
        IFC_RETURN(AddText(xstring_ptr::NullString(), RunFlagsSpaceDefault));
    }
    else
    {
        IFC_RETURN(AddText(m_strText, RunFlagsSpaceDefault))
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnPropertySetImpl(_In_ const CDependencyProperty* pDP, _In_ const CValue&, _In_ const CValue&, _In_ bool)
{
    IFC_RETURN(UpdateFastPathOptOutConditions(pDP));

    switch (pDP->GetIndex())
    {
    case KnownPropertyIndex::FrameworkElement_Language:
        {
            // Language list cannot be updated on the existing dwrite text layout, we have to create a new one, in the next Measure call.
            m_isDWriteTextLayoutDirty = TRUE;
            InvalidateContent();
            break;
        }

    case KnownPropertyIndex::FrameworkElement_FlowDirection:
    case KnownPropertyIndex::TextBlock_FontSize:
    case KnownPropertyIndex::TextBlock_FontFamily:
    case KnownPropertyIndex::TextBlock_FontWeight:
    case KnownPropertyIndex::TextBlock_FontStyle:
    case KnownPropertyIndex::TextBlock_FontStretch:
    case KnownPropertyIndex::TextBlock_CharacterSpacing:
    case KnownPropertyIndex::TextBlock_TextReadingOrder:
    case KnownPropertyIndex::TextBlock_Inlines:
        // It's OK to call InvalidateContent for Text property changing instead of OnContentChanged
        // since the selection manager is deleted below, so there is no need to clear selection.
        InvalidateContent();
        break;

        // TODO: InvalidateMeasure needs to be called on padding changes so that Measure is invalidate on the
        // PageNode, otherwise it may bypass measure by checking size constraint equality without accounting for padding.
        // PageNode/BlockNode's bypass logic should take padding into account.
    case KnownPropertyIndex::TextBlock_TextWrapping:
    case KnownPropertyIndex::TextBlock_Padding:
    case KnownPropertyIndex::TextBlock_LineHeight:
    case KnownPropertyIndex::TextBlock_MaxLines:
    case KnownPropertyIndex::TextBlock_TextLineBounds:
    case KnownPropertyIndex::TextBlock_LineStackingStrategy:
    case KnownPropertyIndex::TextBlock_TextAlignment:
    case KnownPropertyIndex::TextBlock_OpticalMarginAlignment:
    case KnownPropertyIndex::TextBlock_TextTrimming:
        InvalidateContentMeasure();
        break;

    case KnownPropertyIndex::TextBlock_IsColorFontEnabled:
        InvalidateContentArrange();
        break;

    case KnownPropertyIndex::TextBlock_Foreground:
        InvalidateRender();
        break;

    case KnownPropertyIndex::TextBlock_SelectionHighlightColor:
        UpdateSelectionHighlightColor();
        break;

        // If the Text property is set, it replaces the entire
        // content of the inlines.
    case KnownPropertyIndex::TextBlock_Text:
        {
            // Text property cannot be set on the existing dwrite text layout, we have to create a new one in the next Measure call.
            m_isDWriteTextLayoutDirty = TRUE;

            if (m_pInlines != nullptr)
            {
                SetTextToInlineCollection();
            }
            else
            {
                m_shouldAddTextOnDeferredInlineCollectionCreation = TRUE;
            }
            IFC_RETURN(OnContentChanged(pDP));
            // Since we just set the text, we just removed any complex content which may have previously existed
            ClearFastPathOptOutConditions(FastPathOptOutConditions::HasComplexContent);
        }
        break;
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnPropertyChanged(_In_ const PropertyChangedParams& args)
{
    IFC_RETURN(CFrameworkElement::OnPropertyChanged(args));

    switch (args.m_pDP->GetIndex())
    {
        case KnownPropertyIndex::TextBlock_IsTextSelectionEnabled:
        {
            if (m_textMode == TextMode::DWriteLayout)
            {
                if (m_isTextSelectionEnabled)
                {
                    // CInlineCollection(Implements ITextContainer) is required for hit-testing/selection CTextBlock.
                    IFC_RETURN(DeferredCreateInlineCollection());
                    EnsureTextBlockView();
                }
            }
            IFC_RETURN(OnSelectionEnabledChanged(args.m_pOldValue->As<valueBool>()));
            break;
        }
        case KnownPropertyIndex::Control_IsEnabled:
        {
            if (m_pTextDrawingContext)
            {
                m_pTextDrawingContext->SetControlEnabled(args.m_pNewValue->AsBool());
            }

            if (m_pPageNode)
            {
                if (m_pPageNode->GetDrawingContext() != nullptr)
                {
                    m_pPageNode->GetDrawingContext()->SetControlEnabled(args.m_pNewValue->AsBool());
                }
            }

            if (IsHighContrastAdjustmentActive())
            {
                InvalidateContentArrange();
            }
            break;
        }
        case KnownPropertyIndex::UIElement_HighContrastAdjustment:
        {
            IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(args.m_pNewValue->AsEnum())));
            break;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::PullInheritedTextFormatting()
{
    HRESULT         hr                    = S_OK;
    TextFormatting *pParentTextFormatting = nullptr;

    IFCEXPECT_ASSERT(m_pTextFormatting != nullptr);
    if (m_pTextFormatting->IsOld())
    {
        // Get the text core properties that we will be inheriting from.
        IFC(GetParentTextFormatting(&pParentTextFormatting));

        // Process each TextElement text core property one by one.

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_FontFamily))
        {
            IFC(m_pTextFormatting->SetFontFamily(this, pParentTextFormatting->m_pFontFamily));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_Foreground)
            && !m_pTextFormatting->m_freezeForeground)
        {
            IFC(m_pTextFormatting->SetForeground(this, pParentTextFormatting->m_pForeground));
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_Language))
        {
            m_pTextFormatting->SetLanguageString(pParentTextFormatting->m_strLanguageString);
            m_pTextFormatting->SetResolvedLanguageString(pParentTextFormatting->GetResolvedLanguageStringNoRef());
            m_pTextFormatting->SetResolvedLanguageListString(pParentTextFormatting->GetResolvedLanguageListStringNoRef());
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_FontSize))
        {
            m_pTextFormatting->m_eFontSize = pParentTextFormatting->m_eFontSize;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_FontWeight))
        {
            m_pTextFormatting->m_nFontWeight = pParentTextFormatting->m_nFontWeight;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_FontStyle))
        {
            m_pTextFormatting->m_nFontStyle = pParentTextFormatting->m_nFontStyle;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_FontStretch))
        {
            m_pTextFormatting->m_nFontStretch = pParentTextFormatting->m_nFontStretch;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_CharacterSpacing))
        {
            m_pTextFormatting->m_nCharacterSpacing = pParentTextFormatting->m_nCharacterSpacing;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_TextDecorations))
        {
            m_pTextFormatting->m_nTextDecorations = pParentTextFormatting->m_nTextDecorations;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_FlowDirection))
        {
            m_pTextFormatting->m_nFlowDirection = pParentTextFormatting->m_nFlowDirection;
        }

        if (IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_IsTextScaleFactorEnabled) &&
            IsPropertyDefaultByIndex(KnownPropertyIndex::FrameworkElement_IsTextScaleFactorEnabledInternal))
        {
            m_pTextFormatting->m_isTextScaleFactorEnabled = pParentTextFormatting->m_isTextScaleFactorEnabled;
        }

        m_pTextFormatting->SetIsUpToDate();
    }

Cleanup:
    ReleaseInterface(pParentTextFormatting);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::AddText
//
//  Synopsis: Adds a character string as a Run to the inlines.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::AddText(
    _In_ const xstring_ptr& strCharacters,
    _In_ RunFlags  flags
    )
{
    HRESULT  hr   = S_OK;
    CRun    *pRun = nullptr;

    CREATEPARAMETERS createParameters(GetContext());

    if (m_pInlines == nullptr)
    {
        IFC(CreateInlines());
    }

    IFC(CreateDO(&pRun, &createParameters));

    IFC(pRun->SetText(strCharacters, flags));

    IFC(m_pInlines->Append(pRun));

Cleanup:
    ReleaseInterface(pRun);
    return hr;
}

_Check_return_ HRESULT CTextBlock::GetActualWidth(_Out_ float* pWidth)
{
    *pWidth = 0;
    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_pTextLayout)
        {
            DWRITE_TEXT_METRICS m = {};
            if (SUCCEEDED(m_pTextLayout->GetMetrics(&m)))
            {
                *pWidth = m.width + CBorder::HelperCollapseThickness(m_padding).width;
            }
        }
        else
        {
            // For empty TextBlocks, we don't have a IDWTL object. If it is previous measured, just return the padding width.
            if (m_hasBeenMeasured)
            {
                *pWidth = CBorder::HelperCollapseThickness(m_padding).width;
            }
        }
    }
    else
    {
        *pWidth = m_pPageNode ? m_pPageNode->GetDesiredSize().width : 0.0f;
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetActualHeight(_Out_ float* pHeight)
{
    *pHeight = 0;
    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_pTextLayout)
        {
            DWRITE_TEXT_METRICS m = {};
            if (SUCCEEDED(m_pTextLayout->GetMetrics(&m)))
            {
                *pHeight = m.height + CBorder::HelperCollapseThickness(m_padding).height;
            }
        }
        else
        {
            // For empty TextBlocks, we don't have a IDWTL object. If it is previous measured, return line height + padding height.
            if (m_hasBeenMeasured)
            {
                float baseline;
                float lineAdvance;
                IFC_RETURN(GetLineHeight(&baseline, &lineAdvance));
                *pHeight = lineAdvance + CBorder::HelperCollapseThickness(m_padding).height;
            }
        }
    }
    else
    {
        *pHeight = m_pPageNode ? m_pPageNode->GetDesiredSize().height : 0.0f;
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetTextFormatter
//
//  Synopsis: Get TextFormatter instance.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GetTextFormatter(
    _Outptr_ TextFormatter **ppTextFormatter
    )
{
    IFC_RETURN(CTextFormatterFactory::GetTextFormatter(GetContext(), ppTextFormatter));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   ReleaseTextFormatter
//
//  Synopsis: Create a new TextFormatter instance.
//
//------------------------------------------------------------------------

void CTextBlock::ReleaseTextFormatter(
    _In_ TextFormatter *pTextFormatter
    )
{
    CTextFormatterFactory::ReleaseTextFormatter(GetContext(), pTextFormatter);
}

//------------------------------------------------------------------------
//  Summary:
//      Retrieve baseline offset from TextView.
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GetBaselineOffset(_Out_ float *pBaselineOffset)
{
    *pBaselineOffset = 0;

    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_pTextLayout)
        {
            DWRITE_TEXT_METRICS m = {};
            if (SUCCEEDED(m_pTextLayout->GetMetrics(&m)))
            {
                uint32_t actualLineCount = 0;
                if (m.lineCount > 0)
                {
                    std::vector<DWRITE_LINE_METRICS> lineInformation(m.lineCount);
                    IFC_RETURN(m_pTextLayout->GetLineMetrics(lineInformation.data(), m.lineCount, &actualLineCount));
                    *pBaselineOffset = lineInformation[0].baseline;
                }
            }
        }
        else
        {
            // For empty TextBlocks, we don't have a IDWTL object. If it is previous measured, return the baseline.
            if (m_hasBeenMeasured)
            {
                float baseline;
                float lineAdvance;
                IFC_RETURN(GetLineHeight(&baseline, &lineAdvance));
                *pBaselineOffset = baseline;
            }
        }
    }
    else
    {
        if (m_pPageNode != nullptr && !m_pPageNode->IsMeasureDirty())
        {
            *pBaselineOffset  = m_pPageNode->GetBaselineAlignmentOffset();
        }
    }
    return S_OK;
}

ITextView *CTextBlock::GetTextView() const
{
    return m_pTextView;
}

// Called when the BlockCollection is dirtied.
_Check_return_ HRESULT CTextBlock::OnContentChanged(
    _In_opt_ const CDependencyProperty* dp
    )
{
    bool shouldInvalidateContent = true;

    if (m_pInlines != nullptr && m_pInlines->GetCount() > 0)
    {
        SetFastPathOptOutConditions(FastPathOptOutConditions::HasComplexContent);
    }

    if (dp != nullptr &&
          (IsForegroundPropertyIndex(dp->GetIndex()) ||
            (dp->GetIndex() == KnownPropertyIndex::Hyperlink_FocusState)))
    {
        shouldInvalidateContent = false;
        InvalidateRender();
    }
    // TextBlock cannot host other blocks, so checks for following properties are not necessary:
    //      KnownPropertyIndex::Block_Margin
    //      KnownPropertyIndex::Paragraph_TextIndent

    if (shouldInvalidateContent)
    {
        InvalidateContent();

        // Clear selection, since content has changed selection start/end may no longer be valid.
        if (IsSelectionEnabled() &&
            m_pSelectionManager->GetTextSelection() != nullptr)
        {
            IFC_RETURN(m_pSelectionManager->GetTextSelection()->Select(0, 0, LineForwardCharacterBackward));
        }

        // Delete cached collection of focusable children on any content changes.
        // It will get repopulated on the next usage.
        if (m_focusableChildrenCollection != nullptr)
        {
            m_focusableChildrenCollection.reset();
        }
    }

    return S_OK;
}

// Ensure SelectionManager exist in case selection is enabled. We lazily create it only on first interaction
// with TextBlock but on Touch Devices where Narrator is running, Narrator eats input and creats corresponding
// UIA commands which leads to situation where Selection Manager has not been created yet even though Selection
// is enabled. Ensuring it's existence before returning is essential for that.
TextSelectionManager* CTextBlock::GetSelectionManager()
{
    IGNOREHR(EnsureTextSelectionManager());
    return m_pSelectionManager;
}

_Check_return_ HRESULT CTextBlock::SelectAll()
{
    IFC_RETURN(EnsureTextSelectionManager());

    if (IsSelectionEnabled())
    {
        IFC_RETURN(m_pSelectionManager->SelectAll());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetSelectedText(
    _In_ CDependencyObject *pObject,
    uint32_t cArgs,
    _Inout_updates_(cArgs) CValue *ppArgs,
    _In_opt_ IInspectable* pValueOuter,
    _Out_ CValue *pResult
    )
{
    xstring_ptr strString;
    CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(pObject);

    if (!pTextBlock || cArgs != 0)
    {
        IFC_RETURN(E_INVALIDARG);
    }

    pResult->Unset();

    IFC_RETURN(pTextBlock->EnsureTextSelectionManager());
    if (pTextBlock->m_pSelectionManager != nullptr)
    {
        IFC_RETURN(pTextBlock->m_pSelectionManager->GetSelectedText(&strString));
    }

    if (!strString.IsNull())
    {
        // Cast away const-ness since pResult does not take const string.
        pResult->SetString(strString);
    }
    else
    {
        // This property is not allowed to return nullptr value, return empty.
        pResult->SetString(xstring_ptr::EmptyString());
    }

    return S_OK;
}

//-----------------------------------------------------------------------------
//
// Summary: Called by JupiterWindowMobile when Phone copy button is pressed
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::CopySelectedText()
{
    if (IsSelectionEnabled())
    {
        IFC_RETURN(m_pSelectionManager->CopySelectionToClipboard());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetContentStart(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;

    // TextBlock always starts at 0.
    uint32_t contentStartPosition = 0;

    *ppTextPointerWrapper = nullptr;

    // For TextBlock on the fast path, inline collection is deferred created.
    // It's required to create the inline collection here if it hasn't yet been created,
    // and make sure the Text is added to the collection.
    IFC_RETURN(DeferredCreateInlineCollection());
    EnsureTextBlockView();

    IFCEXPECT_ASSERT_RETURN(GetTextContainer() != nullptr);
    textPosition = CPlainTextPosition(GetTextContainer(), contentStartPosition, LineForwardCharacterBackward);
    IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetContentEnd(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    uint32_t contentEndPosition = 0;

    *ppTextPointerWrapper = nullptr;

    // For TextBlock on the fast path, inline collection is deferred created.
    // It's required to create the inline collection here if it hasn't yet been created,
    // and make sure the Text is added to the collection.
    IFC_RETURN(DeferredCreateInlineCollection());
    EnsureTextBlockView();

    IFCEXPECT_ASSERT_RETURN(GetTextContainer() != nullptr);

    GetTextContainer()->GetPositionCount(&contentEndPosition);

    textPosition = CPlainTextPosition(GetTextContainer(), contentEndPosition, LineForwardCharacterForward);
    IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetSelectionStart(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    CTextPosition startTextPosition;

    *ppTextPointerWrapper = nullptr;

    if (IsSelectionEnabled() &&
        m_pSelectionManager->GetTextSelection() != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetTextSelection()->GetStartTextPosition(&startTextPosition));
        textPosition = CPlainTextPosition(startTextPosition.GetPlainPosition());
        IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetSelectionEnd(
    _Outptr_ CTextPointerWrapper **ppTextPointerWrapper
    )
{
    CPlainTextPosition textPosition;
    CTextPosition endTextPosition;

    *ppTextPointerWrapper = nullptr;

    if (IsSelectionEnabled() &&
        m_pSelectionManager->GetTextSelection() != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetTextSelection()->GetEndTextPosition(&endTextPosition));
        textPosition = CPlainTextPosition(endTextPosition.GetPlainPosition());
        IFC_RETURN(CTextPointerWrapper::Create(GetContext(), textPosition, ppTextPointerWrapper));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::Select(
    _In_ CTextPointerWrapper *pAnchorPosition,
    _In_ CTextPointerWrapper *pMovingPosition
    )
{
    if (pAnchorPosition->IsValid() &&
        pMovingPosition->IsValid())
    {
        IFC_RETURN(EnsureTextSelectionManager());
        if (IsSelectionEnabled())
        {
            IFC_RETURN(m_pSelectionManager->Select(
                pAnchorPosition->GetPlainTextPosition(),
                pMovingPosition->GetPlainTextPosition()));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::OnSelectionChanged
//
//  Synopsis: Selection changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render on this element
//            and all overflows, and raises SelectionChanged routed
//            event.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::OnSelectionChanged(
    uint32_t previousSelectionStartOffset,
    uint32_t previousSelectionEndOffset,
    uint32_t newSelectionStartOffset,
    uint32_t newSelectionEndOffset
    )
{
    // Since TextBlock is not a linked container, it doesn't need to check whether it contains selection offsets.
    // If selection changes, always invalidate render.
    InvalidateRender();
    IFC_RETURN(RaiseSelectionChangedEvent());

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::OnSelectionVisibilityChanged
//
//  Synopsis: Selection visibility changed notification called by TextSelectionManager
//            when selection has changed. Invalidates render on this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::OnSelectionVisibilityChanged(
    uint32_t selectionStartOffset,
    uint32_t selectionEndOffset
    )
{
    InvalidateRender();
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::EnsureTextDrawingContext()
{
    if (m_pTextDrawingContext == nullptr)
    {
        CTextCore *pTextCore;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        m_pTextDrawingContext = make_xref<D2DTextDrawingContext>(pTextCore);

        // Set DrawingContext properties to handle foreground color when BackPlate is enabled.
        m_pTextDrawingContext->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), m_useHyperlinkForegroundOnBackPlate);
        m_pTextDrawingContext->SetControlEnabled(IsEnabled());
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::EnsureBackPlateDependencies()
{
    // Create dependencies for the first time only when the BackPlate is active and HighCotrast is enabled.
    if (IsHighContrastAdjustmentActive())
    {
        if (m_textMode == TextMode::DWriteLayout)
        {
            IFC_RETURN(DeferredCreateInlineCollection());
            EnsureTextBlockView();
        }
    }

    return S_OK;
}

// Create the IDWriteTextLayout object based on the current properties.
_Check_return_ HRESULT CTextBlock::ConfigureDWriteTextLayout(
    const XSIZEF availableSize,
    const float dwriteBaseline,
    const float dwriteLineAdvance) noexcept
{
    ASSERT(m_textMode == TextMode::DWriteLayout);

    DWRITE_TEXT_RANGE textRange = {0, UINT_MAX};
    const TextFormatting* pTextFormatting = nullptr;
    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    // Scope to a valid WSS or DWrite will complain
    DWRITE_FONT_WEIGHT dwriteFontWeight = MIN(MAX(static_cast<DWRITE_FONT_WEIGHT>(1), static_cast<DWRITE_FONT_WEIGHT>(pTextFormatting->m_nFontWeight)), static_cast<DWRITE_FONT_WEIGHT>(999));

    DWRITE_FONT_STYLE dwriteFontStyle = MIN(MAX(DWRITE_FONT_STYLE_NORMAL, static_cast<DWRITE_FONT_STYLE>(pTextFormatting->m_nFontStyle)), DWRITE_FONT_STYLE_ITALIC);

    DWRITE_FONT_STRETCH dwriteFontStretch = MIN(MAX(DWRITE_FONT_STRETCH_ULTRA_CONDENSED, static_cast<DWRITE_FONT_STRETCH>(pTextFormatting->m_nFontStretch)), DWRITE_FONT_STRETCH_ULTRA_EXPANDED);

    xstring_ptr strFontFamilyName;
    Microsoft::WRL::ComPtr<IDWriteFactory6> dwriteFactory;
    DWriteFontAndScriptServices *pDWriteFontServices = nullptr;

    IFC_RETURN(pTextFormatting->m_pFontFamily->get_Source(&strFontFamilyName));
    if (wcscmp(strFontFamilyName.GetBuffer(), L"Global User Interface") == 0)
    {
        strFontFamilyName.Reset();
        CTextCore* pTextCore = nullptr;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        IFontAndScriptServices* pFontAndScriptServices = nullptr;
        IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
        IFC_RETURN(pFontAndScriptServices->GetDefaultFontNameString(&strFontFamilyName));
    }

    IFC_RETURN(GetDWriteFontAndScriptServices(&pDWriteFontServices));

    IFC_RETURN(pDWriteFontServices->GetDWriteFactory(&dwriteFactory));

    float scaledFontSize = pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale());

    xref_ptr<PALText::IFontCollection> systemFontCollection;
    IFC_RETURN(pDWriteFontServices->GetSystemFontCollection(systemFontCollection.ReleaseAndGetAddressOf()));

    DWRITE_READING_DIRECTION dwriteReadingDirection;
    DWRITE_TEXT_ALIGNMENT dwriteTextAlignment;

    IFC_RETURN(DetermineTextReadingOrderAndAlignment(dwriteFactory.Get(), pTextFormatting, &dwriteReadingDirection, &dwriteTextAlignment));

    DWRITE_WORD_WRAPPING dwriteWrapping = DWRITE_WORD_WRAPPING_WRAP;
    switch (m_textWrapping)
    {
        case DirectUI::TextWrapping::NoWrap:
        {
            dwriteWrapping = DWRITE_WORD_WRAPPING_NO_WRAP;
            break;
        }
        case DirectUI::TextWrapping::Wrap:
        {
            dwriteWrapping = DWRITE_WORD_WRAPPING_EMERGENCY_BREAK;
            break;
        }
        case DirectUI::TextWrapping::WrapWholeWords:
        {
            dwriteWrapping = DWRITE_WORD_WRAPPING_WHOLE_WORD;
            break;
        }
    }

    DWRITE_TRIMMING trimmingOptions = {
                (m_textTrimming == DirectUI::TextTrimming::CharacterEllipsis) ? DWRITE_TRIMMING_GRANULARITY_CHARACTER : DWRITE_TRIMMING_GRANULARITY_WORD,
                 0, // delimiter
                 0  // delimiter occurrence
         };
    DWRITE_OPTICAL_ALIGNMENT dwriteOpticalAlignment = m_opticalMarginAlignment == DirectUI::OpticalMarginAlignment::TrimSideBearings ? DWRITE_OPTICAL_ALIGNMENT_NO_SIDE_BEARINGS : DWRITE_OPTICAL_ALIGNMENT_NONE;

    if (m_pTextLayout == nullptr) // If there is no DWriteTextLayout object available, we need to create one.
    {
        Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormat;
        IFC_RETURN(pDWriteFontServices->GetDefaultDWriteTextFormat(&pTextFormat, DWriteFontCollection::IsTypographicCollection(systemFontCollection)));

        IGNOREHR(::TextAnalysis_SetLocaleNameList(pTextFormat.Get(), pTextFormatting->GetResolvedLanguageListStringNoRef().GetBuffer()));

        // When the TextBlock is empty, m_strText.GetBuffer() will be null,
        // we need to pass an explict empty string to DWTL in order to get correct layout.
        // Otherwise m_pTextLayout will be null.
        IFC_RETURN(dwriteFactory->CreateTextLayout(
            m_strText.GetCount() != 0 ? m_strText.GetBuffer() : L"",      // The string to be laid out and formatted.
            m_strText.GetCount(),  // The length of the string.
            pTextFormat.Get(),  // The text format to apply to the string (contains font information, etc).
            availableSize.width,         // The width of the layout box.
            availableSize.height,        // The height of the layout box.
            &m_pTextLayout // The IDWriteTextLayout interface pointer.
            ));
    }
    else
    {
        IFC_RETURN(m_pTextLayout->SetMaxWidth(availableSize.width));
        IFC_RETURN(m_pTextLayout->SetMaxHeight(availableSize.height));
    }

    IFC_RETURN(m_pTextLayout->SetFontFamilyName(strFontFamilyName.GetBuffer(), textRange));
    IFC_RETURN(m_pTextLayout->SetFontCollection(DWriteFontCollection::GetInternalCollection(systemFontCollection.get()), textRange));
    IFC_RETURN(m_pTextLayout->SetFontSize(scaledFontSize, textRange));
    IFC_RETURN(m_pTextLayout->SetLocaleName(pTextFormatting->m_strLanguageString.GetBuffer(), textRange));
    IFC_RETURN(m_pTextLayout->SetReadingDirection(dwriteReadingDirection));
    IFC_RETURN(m_pTextLayout->SetTextAlignment(dwriteTextAlignment));
    IFC_RETURN(m_pTextLayout->SetWordWrapping(dwriteWrapping));

    if (DWriteFontCollection::IsTypographicCollection(systemFontCollection.get()))
    {
        Microsoft::WRL::ComPtr<IDWriteTextLayout4> textLayout4;
        IFC_RETURN(m_pTextLayout.As(&textLayout4));
        auto axisValues = DWriteFontAndScriptServices::CreateFontAxisValueArray(pTextFormatting->m_eFontSize, dwriteFontWeight, dwriteFontStyle, dwriteFontStretch);
        IFC_RETURN(textLayout4->SetFontAxisValues(axisValues.data(), static_cast<UINT32>(axisValues.size()), textRange));
    }
    else
    {
        IFC_RETURN(m_pTextLayout->SetFontWeight(dwriteFontWeight, textRange));
        IFC_RETURN(m_pTextLayout->SetFontStyle(dwriteFontStyle, textRange));
        IFC_RETURN(m_pTextLayout->SetFontStretch(dwriteFontStretch, textRange));
    }

    ASSERT (m_textTrimming != DirectUI::TextTrimming::Clip);
    if (m_textTrimming != DirectUI::TextTrimming::None)
    {
        Microsoft::WRL::ComPtr<IDWriteInlineObject> pTrimmingSign;
        // We cannot create the trimming sign from the IDWriteTextLayout object,
        // because the previously set textRange doesn't include the trimming sign,
        // the trimming sign will end up with the default IDWriteTextFormat we cached.
        // For example, when font size = 30, even we call  m_pTextLayout->SetFontSize(30, {0, UINT_MAX}),
        // the trimming sign created will still have the default font size = 15.
        // In this case, we have to create a new IDWriteTextFormat object with all properties (font name, font size, weight, style, stretch) that could affect the trimming sign.
        // Note: Although the original reason for hardcoding en-us here is lost to history, it is believed by the dwrite team that the
        //       language of the trimming string doesn't matter and this provides consistent behavior.
        // TODO: consider caching the trimming sign.
        Microsoft::WRL::ComPtr<IDWriteTextFormat> trimmingFormat;
        if (DWriteFontCollection::IsTypographicCollection(systemFontCollection.get()))
        {
            Microsoft::WRL::ComPtr<IDWriteTextFormat3> trimmingFormat6;
            auto axisValues = DWriteFontAndScriptServices::CreateFontAxisValueArray(pTextFormatting->m_eFontSize, dwriteFontWeight, dwriteFontStyle, dwriteFontStretch);

        IFC_RETURN(dwriteFactory->CreateTextFormat(
                strFontFamilyName.GetBuffer(),
                DWriteFontCollection::GetInternalCollection(systemFontCollection.get()),
                axisValues.data(),
                static_cast<UINT32>(axisValues.size()),
                scaledFontSize,
                L"en-us",
                &trimmingFormat6
            ));
            IFC_RETURN(trimmingFormat6.As(&trimmingFormat));
        }
        else
        {
            IFC_RETURN(dwriteFactory->CreateTextFormat(
                strFontFamilyName.GetBuffer(),
                DWriteFontCollection::GetInternalCollection(systemFontCollection.get()),
                dwriteFontWeight,
                dwriteFontStyle,
                dwriteFontStretch,
                scaledFontSize,
                L"en-us",
                &trimmingFormat
            ));
        }
        IFC_RETURN(dwriteFactory->CreateEllipsisTrimmingSign(trimmingFormat.Get(), &pTrimmingSign));
        IFC_RETURN(m_pTextLayout->SetTrimming(&trimmingOptions, pTrimmingSign.Get()));
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat1> pTextFormat1;
    IFC_RETURN(m_pTextLayout.As(&pTextFormat1));

    IFC_RETURN(pTextFormat1->SetOpticalAlignment(dwriteOpticalAlignment));

    IFC_RETURN(m_pTextLayout->SetLineSpacing(DWRITE_LINE_SPACING_METHOD_UNIFORM, dwriteLineAdvance, dwriteBaseline));

    // Set or remove strikethrough
    bool hasStrikethrough = static_cast<uint32_t>(pTextFormatting->m_nTextDecorations) & static_cast<uint32_t>(DirectUI::TextDecorations::Strikethrough);
    IFC_RETURN(m_pTextLayout->SetStrikethrough(hasStrikethrough, textRange));

    // Set or remove underline
    bool hasUnderline = static_cast<uint32_t>(pTextFormatting->m_nTextDecorations) & static_cast<uint32_t>(DirectUI::TextDecorations::Underline);
    IFC_RETURN(m_pTextLayout->SetUnderline(hasUnderline, textRange));

    BOOL isFontNameValid;
    IFC_RETURN(pDWriteFontServices->IsFontNameValid(strFontFamilyName, &isFontNameValid));

    if (!isFontNameValid)
    {
        IFC_RETURN(pTextFormatting->m_pFontFamily->EnsureCompositeFontFamily(GetFontContext()));
        if (pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pFontFallback)
        {
            IDWriteFontFallback* fontFallback = static_cast<FontFallbackWrapper*>(pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pFontFallback)->m_fontFallback.Get();
            Microsoft::WRL::ComPtr<IDWriteTextLayout2> spTextLayout2;
            IFC_RETURN(m_pTextLayout.As(&spTextLayout2));
            IFC_RETURN(spTextLayout2->SetFontFallback(fontFallback));
        }
        if (pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pBaseFontCollection)
        {
            IDWriteFontCollection* dwriteFontCollection = DWriteFontCollection::GetInternalCollection(pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pBaseFontCollection);
            IFC_RETURN(m_pTextLayout->SetFontCollection(dwriteFontCollection, textRange));
        }
        if (pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pBaseFontName)
        {
            IFC_RETURN(m_pTextLayout->SetFontFamilyName(pTextFormatting->m_pFontFamily->m_pCompositeFontFamily->m_pBaseFontName->GetString(), textRange));
        }
    }

    // If Typography is not default, set it.
    if (!m_pInheritedProperties->m_typography.IsTypographyDefault())
    {
        Microsoft::WRL::ComPtr<IDWriteTypography> typography;
        IFC_RETURN(dwriteFactory->CreateTypography(typography.GetAddressOf()));

        // Add Typography features to spTypography.
        IFC_RETURN(m_pInheritedProperties->m_typography.UpdateDWriteTypographyFeatures(typography.Get()));

        // Apply to all the text.
        IFC_RETURN(m_pTextLayout->SetTypography(typography.Get(), textRange));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::DetermineTextReadingOrderAndAlignment
//
//  Synopsis: Calculate and set readingOrder and alignment based on Text BIDI
//  direction and property settings( FlowDirection, TextReadingOrder and TextAlignment)
//  Assuming BIDI RTL Text is 123Arabic!, LTR Text is English
//  FlowDirection         TextReadingOrder           TextAlignment               Intended Result
//  LTR                      DFC                             DFC                             XXXXXXX!123Arabic
//  LTR                      DFC                             LEFT                            !123ArabicXXXXXX
//  LTR                      DFC                             RIGHT                          XXXXXXX!123Arabic
//  LTR                      UseFlowDirection            DFC                             XXXXXXX123Arabic!
//  LTR                      UseFlowDirection            LEFT                            123Arabic!XXXXXX
//  LTR                      UseFlowDirection            RIGHT                          XXXXXXX123Arabic!
//  LTR                      DFC                             DFC                             EnglishXXXXXXXXX
//  LTR                      DFC                             LEFT                            EnglishXXXXXXXXX
//  LTR                      DFC                             RIGHT                          XXXXXXXXXEnglish
//  LTR                      UseFlowDirection            DFC                             EnglishXXXXXXXXX
//  LTR                      UseFlowDirection            LEFT                            EnglishXXXXXXXXX
//  LTR                      UseFlowDirection            RIGHT                          XXXXXXXXXEnglish
//  RTL                      DFC                             DFC                             XXXXXXX!123Arabic
//  RTL                      DFC                             LEFT                            XXXXXXX!123Arabic
//  RTL                      DFC                             RIGHT                          !123ArabicXXXXXX
//  RTL                      UseFlowDirection            DFC                             XXXXXXX!123Arabic
//  RTL                      UseFlowDirection            LEFT                            XXXXXXX!123Arabic
//  RTL                      UseFlowDirection            RIGHT                          !123ArabicXXXXXXX
//  RTL                      DFC                             DFC                             EnglishXXXXXXXXX
//  RTL                      DFC                             LEFT                            XXXXXXXXXEnglish
//  RTL                      DFC                             RIGHT                          EnglishXXXXXXXXX
//  RTL                      UseFlowDirection            DFC                             EnglishXXXXXXXXX
//  RTL                      UseFlowDirection            LEFT                            XXXXXXXXXEnglish
//  RTL                      UseFlowDirection            RIGHT                          EnglishXXXXXXXXX
//------------------------------------------------------------------------
_Check_return_
HRESULT
CTextBlock::DetermineTextReadingOrderAndAlignment(
    _In_ IDWriteFactory *pDWriteFactory,
    _In_ const TextFormatting* pTextFormatting,
    _Out_ DWRITE_READING_DIRECTION* dwriteReadingDirection,
    _Out_ DWRITE_TEXT_ALIGNMENT* dwriteTextAlignment
    )
{
    BOOL isAmbiguousReadingDirection = false;
    BOOL shouldFlipTextAlignment = false;
    DWRITE_READING_DIRECTION flowDirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    DWRITE_READING_DIRECTION setReadingDirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    DWRITE_READING_DIRECTION contentReadingDirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    DirectUI::TextAlignment detectedTextAlignment = m_textAlignment;
    *dwriteReadingDirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    *dwriteTextAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;

    if (pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
    {
        flowDirection = DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
        setReadingDirection = DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
        contentReadingDirection = DWRITE_READING_DIRECTION_RIGHT_TO_LEFT;
        // if fow direction is RTL, set ReadingDirection accordingly. This could be overwritten in code below if textReadingOrder is set to DFC
        *dwriteReadingDirection = flowDirection;
    }

    // Analyze content reading direction when either textReadingOrder is DFC or textAlignment is DFC
    if ((m_textReadingOrder == DirectUI::TextReadingOrder::DetectFromContent) || (m_textAlignment == DirectUI::TextAlignment::DetectFromContent))
    {
        DWriteFontAndScriptServices *dwriteFontServices = nullptr;
        IFC_RETURN(GetDWriteFontAndScriptServices(&dwriteFontServices));
        Microsoft::WRL::ComPtr<IDWriteTextAnalyzer> dwriteTextAnalyzer;
        IFC_RETURN(dwriteFontServices->GetDWriteTextAnalyzer(&dwriteTextAnalyzer));

        RichTextServices::Internal::StackTextAnalysisSource analysisSource(m_strText.GetBuffer(),m_strText.GetCount(),nullptr, flowDirection, nullptr);

        IFC_RETURN(::TextAnalysis_GetContentReadingDirection(
             dwriteTextAnalyzer.Get(),
             &analysisSource,
             0,
             m_strText.GetCount(),
             &contentReadingDirection,
             &isAmbiguousReadingDirection));

        if (!isAmbiguousReadingDirection)
        {
            if (m_textReadingOrder == DirectUI::TextReadingOrder::DetectFromContent)
            {
                *dwriteReadingDirection = contentReadingDirection;
                // update setReadingDirection, to be used to check if alignment needs to be fliped when text alignment is DFC
                setReadingDirection = contentReadingDirection;
                if (contentReadingDirection != flowDirection)
                {
                    shouldFlipTextAlignment = TRUE;
                }
            }
        }

        if (m_textAlignment == DirectUI::TextAlignment::DetectFromContent)
        {
            // set text alignment to what is detected from content, but we may need to flip it if set reading direction is RTL.
            if (contentReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT)
            {
                detectedTextAlignment = DirectUI::TextAlignment::Right;
            }
            else
            {
                detectedTextAlignment = DirectUI::TextAlignment::Left;
            }

            if (setReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT)
            {
                shouldFlipTextAlignment = TRUE;
            }
            else
            {
                shouldFlipTextAlignment = FALSE;
            }
        }
    }


    DWRITE_TEXT_ALIGNMENT textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
    switch (detectedTextAlignment)
    {
        case DirectUI::TextAlignment::Center:
        {
            textAlignment = DWRITE_TEXT_ALIGNMENT_CENTER;
            break;
        }
        case DirectUI::TextAlignment::Left:
        {
            if (shouldFlipTextAlignment)
            {
                textAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
            }
            else
            {
                textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
            }
            break;
        }
        case DirectUI::TextAlignment::Right:
        {
            if (shouldFlipTextAlignment)
            {
                textAlignment = DWRITE_TEXT_ALIGNMENT_LEADING;
            }
            else
            {
                textAlignment = DWRITE_TEXT_ALIGNMENT_TRAILING;
            }
            break;
        }
        case DirectUI::TextAlignment::Justify:
        {
            textAlignment = DWRITE_TEXT_ALIGNMENT_JUSTIFIED;
            break;
        }
    }
    *dwriteTextAlignment = textAlignment;
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetLineHeight(_Out_ float* baseline, _Out_ float* lineAdvance)
{
    *baseline = 0;
    *lineAdvance = 0;

    // Get line bounds
    float fontBaseline;
    float fontLineAdvance;
    IFC_RETURN(EnsureFontContext());
    const TextFormatting* pTextFormatting = nullptr;
    IFC_RETURN(GetTextFormatting(&pTextFormatting));
    IFC_RETURN(pTextFormatting->m_pFontFamily->GetTextLineBoundsMetrics(GetFontContext(), m_textLineBounds, &fontBaseline, &fontLineAdvance));
    ASSERT(fontLineAdvance > 0);

    float scaledFontSize = pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale());

    // When LineHeight is 0, all Linestackstrategies behave identical.
    if (m_eLineHeight <= 0)
    {
        // For single text string, all three line stacking strategies have uniformed line spacing. This is the behavior of LS path.
        *baseline = scaledFontSize*fontBaseline;
        *lineAdvance = scaledFontSize*fontLineAdvance;
    }
    else
    {
        switch (m_lineStackingStrategy)
        {
            case DirectUI::LineStackingStrategy::BlockLineHeight:
            {
                float fontBaselineRatio = fontBaseline / fontLineAdvance;
                *baseline = fontBaselineRatio * m_eLineHeight;
                *lineAdvance = m_eLineHeight;
                break;
            }
            case DirectUI::LineStackingStrategy::MaxHeight:
            {
                *baseline = scaledFontSize*fontBaseline;
                *lineAdvance = std::max(m_eLineHeight, scaledFontSize*fontLineAdvance);
                break;
            }
            case DirectUI::LineStackingStrategy::BaselineToBaseline:
            {
                *baseline = scaledFontSize*fontBaseline;
                *lineAdvance = m_eLineHeight;
                break;
            }
            default:
            {
                ASSERT(FALSE);
                break;
            }
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   CTextBlock::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::MeasureOverride(
    XSIZEF availableSize,
    XSIZEF& desiredSize
    )
{
    m_hasBeenMeasured = TRUE;
    CValue highContrastAdjustment;

    IFC_RETURN(ResolveTextMode());

    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_isDWriteTextLayoutDirty == TRUE)
        {
            m_pTextLayout = nullptr;
            m_isDWriteTextLayoutDirty = FALSE;
        }

        // Get line bounds
        float baseline;
        float lineAdvance;
        float lineStackingOffset;
        IFC_RETURN(GetLineHeight(&baseline, &lineAdvance));

        // Fast path for empty TextBlock, always return the line height.
        if (m_strText.GetCount() == 0)
        {
            IFC_RETURN(GetLineStackingOffset(1, &lineStackingOffset)); // Empty TextBlock is always formatted into 1 line.
            desiredSize.width = m_padding.left + m_padding.right;
            desiredSize.height = lineAdvance - lineStackingOffset + m_padding.top + m_padding.bottom;
            return S_OK;
        }

        availableSize.width -= m_padding.left + m_padding.right;
        availableSize.height -= m_padding.top + m_padding.bottom;
        availableSize.width  = MAX(availableSize.width, 0);
        availableSize.height = MAX(availableSize.height, 0);

        IFC_RETURN(ConfigureDWriteTextLayout(availableSize, baseline, lineAdvance));

        ASSERT(m_pTextLayout);

        DWRITE_TEXT_METRICS textMetrics = {};
        IFC_RETURN(m_pTextLayout->GetMetrics(&textMetrics));

        // hack for MaxLines Property. Calcuate total line height for m_maxLines lines, then trim it.
        if (m_maxLines != 0 && m_maxLines < textMetrics.lineCount)
        {
            uint32_t actualLineCount = 0;
            float maxHeight = 0;

            std::vector<DWRITE_LINE_METRICS> lineInformation(textMetrics.lineCount);

            IFC_RETURN(m_pTextLayout->GetLineMetrics(lineInformation.data(), textMetrics.lineCount, &actualLineCount));
            for (uint32_t index = 0; index < m_maxLines; index++)
            {
                maxHeight += lineInformation[index].height;
            }

            IFC_RETURN(m_pTextLayout->SetMaxHeight(maxHeight));

            // When MaxLines is smaller than the actual line count, we need to ask DWriteTextLayout to trim the text if
            // TextTrimming == DirectUI::TextTrimming::None. The default granularity is word.
            if (m_textTrimming == DirectUI::TextTrimming::None)
            {
                DWRITE_TRIMMING trimmingOptions;
                trimmingOptions = {
                    DWRITE_TRIMMING_GRANULARITY_CHARACTER,
                    0, // delimiter
                    0  // delimiter occurrence
                };

                IFC_RETURN(m_pTextLayout->SetTrimming(&trimmingOptions, nullptr));
            }

            IFC_RETURN(m_pTextLayout->GetMetrics(&textMetrics));
        }

        // Calculate the bottom adjustment for LineStackingStrategy.BaselineToBaseline && LineHeight  > 0.
        IFC_RETURN(GetLineStackingOffset(textMetrics.lineCount, &lineStackingOffset));
        desiredSize.width = textMetrics.width + m_padding.left + m_padding.right;
        desiredSize.height = textMetrics.height -lineStackingOffset + m_padding.top + m_padding.bottom;
    }
    else
    {
        ASSERT(m_textMode == TextMode::Normal);
        IFC_RETURN(EnsureBlockLayout());
        if (m_pPageNode != nullptr)
        {
            // TextBlock measure options:
            //      1. AllowEmptyContent is always FALSE.
            //      2. MeasureBottomless is always TRUE.
            //      3. SuppressTopMargin is always FALSE.
            //      4. Previous break is always nullptr.
            IFC_RETURN(m_pPageNode->Measure(availableSize, m_maxLines, 0.0f, FALSE, TRUE, FALSE, nullptr, nullptr));
            desiredSize = m_pPageNode->GetDesiredSize();
        }
    }

    if (GetUseLayoutRounding())
    {
        // In order to prevent text clipping as a result of layout rounding at
        // scales other than 1.0x, the ceiling of the rescaled size is used.
        const float plateauScale = RootScale::GetRasterizationScaleForElement(this);
        XSIZEF pageNodeSize = desiredSize;
        desiredSize.width = XcpCeiling(pageNodeSize.width * plateauScale) / plateauScale;
        desiredSize.height = XcpCeiling(pageNodeSize.height * plateauScale) / plateauScale;

        // LsTextLine is not aware of layoutround and uses baseline height to place the rendered text.
        // However, because the height of the *block is potentionally layoutround-ed up, we should adjust the
        // placement of text by the difference.  Horizontal adjustment is not of concern since
        // LsTextLine uses arranged size which is already layoutround-ed.
        m_layoutRoundingHeightAdjustment = desiredSize.height - pageNodeSize.height;
    }

    // ElementHighContrastAdjustment is an inherited property so we don't get a Property change when the element is added to the tree, we
    // check the property here to make sure it is current.
    IFC_RETURN(GetValueByIndex(KnownPropertyIndex::UIElement_HighContrastAdjustment, &highContrastAdjustment));
    IFC_RETURN(OnHighContrastAdjustmentChanged(static_cast<DirectUI::ElementHighContrastAdjustment>(highContrastAdjustment.AsEnum())));

    // Measure the CaretBrowsingCaret if present
    auto children = GetUnsortedChildren();
    UINT32 childrenCount = children.GetCount();

    for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
    {
        CUIElement* currentChild = children[childIndex];
        if (currentChild->GetRequiresMeasure() && currentChild->OfTypeByIndex<KnownTypeIndex::CaretBrowsingCaret>())
        {
            IFC_RETURN(currentChild->EnsureLayoutStorage());
            IFC_RETURN(currentChild->Measure(availableSize));
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::ArrangeOverride
//
//  Synopsis: Returns the final render size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::ArrangeOverride(
    XSIZEF finalSize,
    XSIZEF& newFinalSize
    )
{
    XSIZEF renderSize = {0.0f, 0.0f};

    if (m_textMode == TextMode::DWriteLayout)
    {
        if (!GetIsMeasureDirty())
        {
            float lineStackingOffset;

            // Fast path for empty TextBlock.
            if (m_strText.GetCount() == 0)
            {
                float baseline;
                float lineAdvance;
                IFC_RETURN(GetLineHeight(&baseline, &lineAdvance));

                // Calculate the bottom adjustment for LineStackingStrategy.BaselineToBaseline && LineHeight  > 0.
                IFC_RETURN(GetLineStackingOffset(1, &lineStackingOffset)); // Empty TextBlock is always formatted into 1 line.
                renderSize.height = lineAdvance - lineStackingOffset;

                IFC_RETURN(UpdateIsTextTrimmedFastPath(nullptr));
            }
            else if (m_pTextLayout != nullptr)
            {
                std::shared_ptr<HighlightRegion> selection;
                if (m_pTextView != nullptr &&
                    IsSelectionEnabled() &&
                    m_pSelectionManager->IsSelectionVisible())
                {
                    m_pSelectionManager->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection);
                }

                const TextFormatting* pTextFormatting = nullptr;
                IFC_RETURN(GetTextFormatting(&pTextFormatting));

                // For following cases we need to re-measure the text based on the finalSize.
                if (m_textTrimming != DirectUI::TextTrimming::None
                    || m_textWrapping != DirectUI::TextWrapping::NoWrap
                    || m_textAlignment != DirectUI::TextAlignment::Left
                    || pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
                {
                    IFC_RETURN(m_pTextLayout->SetMaxWidth(finalSize.width - m_padding.left - m_padding.right));
                }

                DWRITE_TEXT_METRICS m = {};
                if (SUCCEEDED(m_pTextLayout->GetMetrics(&m)))
                {
                    // Calculate the bottom adjustment for LineStackingStrategy.BaselineToBaseline && LineHeight  > 0.
                    IFC_RETURN(GetLineStackingOffset(m.lineCount, &lineStackingOffset));

                    renderSize.width = MIN(m.width, m.layoutWidth);
                    renderSize.height = MIN(m.height - lineStackingOffset, m.layoutHeight);
                }

                IFC_RETURN(EnsureTextDrawingContext());
                xref::weakref_ptr<CTextBlock> pBrushSource = xref::get_weakref(this);

                m_pTextDrawingContext->SetIsColorFontEnabled(m_isColorFontEnabled);

                // If no selection, no additional processing needs to be done for
                // selection rendering. We can pre-draw it here.
                if (!selection)
                {
                    DWriteTextRenderer dWriteRenderer(m_pTextDrawingContext, pBrushSource);
                    m_pTextDrawingContext->Clear();
                    IFC_RETURN(m_pTextLayout->Draw(nullptr, &dWriteRenderer, 0, 0));
                    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
                }

                IFC_RETURN(UpdateIsTextTrimmedFastPath(&m));
            }
        }
    }
    else
    {
        ASSERT(m_textMode == TextMode::Normal);
        if (m_pPageNode != nullptr &&
            !m_pPageNode->IsMeasureDirty())
        {
            // If final size is infinite, Arrange at DesiredSize.
            ASSERT(!IsInfiniteF(finalSize.width));
            ASSERT(!IsInfiniteF(finalSize.height));

            IFC_RETURN(m_pPageNode->Arrange(finalSize));
            renderSize = m_pPageNode->GetRenderSize();

            std::shared_ptr<HighlightRegion> selection;
            if (m_pTextView != nullptr &&
                IsSelectionEnabled() &&
                m_pSelectionManager->IsSelectionVisible())
            {
                m_pSelectionManager->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection);
            }

            // Set DrawingContext properties to handle foreground color when BackPlate is enabled. This must be set every ArrangeOverride because
            // elements inside the DrawingContext can change.
            m_pPageNode->GetDrawingContext()->SetControlEnabled(IsEnabled());
            m_pPageNode->GetDrawingContext()->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), m_useHyperlinkForegroundOnBackPlate);

            // If no selection, no additional processing needs to be done for
            // selection rendering. Proceed with regular text rendering.
            if (!selection)
            {
                // Render page node which will communicate render data for text to the drawing context.
                // There's no need to force re-rendering here, since Arrange was just called on PageNode and Arrange bypass
                // rules apply to render in this case.
                IFC_RETURN(m_pPageNode->Draw(FALSE));
                CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
            }
            else
            {
                m_redrawForArrange = true;  // Force redraw during render pass
            }

            IFC_RETURN(UpdateIsTextTrimmedSlowPath());
        }
    }

    // We want to return the larger of render size returned by Arrange and
    // finalSize passed by the parent. This is done to make sure that content is
    // positioned correctely if the actual width/height is smaller than finalSize,
    // or clipped correctly if the width is greater than finalSize.
    newFinalSize.width  = MAX(finalSize.width, renderSize.width);
    newFinalSize.height = MAX(finalSize.height, renderSize.height);


    { // Forcefully arrange the CaretBrowsingCaret at (0,0) as we'll use a Translation to position it
        auto children = GetUnsortedChildren();
        UINT32 childrenCount = children.GetCount();

        for (XUINT32 childIndex = 0; childIndex < childrenCount; childIndex++)
        {
            CUIElement* currentChild = children[childIndex];
            if (currentChild->OfTypeByIndex<KnownTypeIndex::CaretBrowsingCaret>())
            {
                XRECTF arrangeRect;
                arrangeRect.X = 0;
                arrangeRect.Y = 0;
                arrangeRect.Width = currentChild->GetLayoutStorage()->m_desiredSize.width;
                arrangeRect.Height = currentChild->GetLayoutStorage()->m_desiredSize.height;
                IFC_RETURN(currentChild->Arrange(arrangeRect));
            }
        }
    }


    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  UpdateIsTextTrimmedFastPath
//
//  Synopsis: Update whether the text of the block has been trimmed or
//            not and raise an event if the state of trimming has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::UpdateIsTextTrimmedFastPath(_In_opt_ DWRITE_TEXT_METRICS *m)
{
    auto last_isTextTrimmed = m_isTextTrimmed;
    if (!m)
    {
        // Empty text block cannot be trimmed
        m_isTextTrimmed = false;
    }
    else
    {
        std::vector<DWRITE_LINE_METRICS> lineInformation(m->lineCount);
        uint32_t actualLineCount = 0;
        IFC_RETURN(m_pTextLayout->GetLineMetrics(lineInformation.data(), m->lineCount, &actualLineCount));

        ASSERT(actualLineCount != 0); // Line count formatted by DWrite will never be 0. Even an empty TextBlock will have 1 line.
        m_isTextTrimmed = !!lineInformation[actualLineCount - 1].isTrimmed;
    }
    if (last_isTextTrimmed != m_isTextTrimmed)
    {
        RaiseIsTextTrimmedChangedEvent();

        CValue oldValue;
        oldValue.Set<valueBool>(last_isTextTrimmed);
        CValue newValue;
        newValue.Set<valueBool>(m_isTextTrimmed);
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_IsTextTrimmed),
                oldValue,
                newValue
            )));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  UpdateIsTextTrimmedSlowPath
//
//  Synopsis: Update whether the text of the block has been trimmed or
//            not and raise an event if the state of trimming has changed.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::UpdateIsTextTrimmedSlowPath()
{
    bool isTrimmed = false;

    ParagraphNode *blockChild = static_cast<ParagraphNode *>(m_pPageNode->GetFirstChild());
    while (blockChild != nullptr)
    {
        isTrimmed = blockChild->GetHasTrimmedLine();
        if (isTrimmed)
        {
            break;
        }
        blockChild = static_cast<ParagraphNode *>(blockChild->GetNext());
    }

    auto last_isTextTrimmed = m_isTextTrimmed;
    m_isTextTrimmed = isTrimmed;
    if (last_isTextTrimmed != m_isTextTrimmed)
    {
        RaiseIsTextTrimmedChangedEvent();

        CValue oldValue;
        oldValue.Set<valueBool>(last_isTextTrimmed);
        CValue newValue;
        newValue.Set<valueBool>(m_isTextTrimmed);
        IFC_RETURN(NotifyPropertyChanged(
            PropertyChangedParams(
                DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_IsTextTrimmed),
                oldValue,
                newValue
            )));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::RaiseIsTextTrimmedChangedEvent
//
//  Synopsis: Raises event for IsTextTrimmedChanged.
//
//------------------------------------------------------------------------
void CTextBlock::RaiseIsTextTrimmedChangedEvent()
{
    CEventManager *const eventManager = GetContext()->GetEventManager();
    // Create the DO that represents the event args.
    xref_ptr<CIsTextTrimmedChangedEventArgs> args;
    args.init(new CIsTextTrimmedChangedEventArgs());
    // Raise event.
    eventManager->Raise(EventHandle(KnownEventIndex::TextBlock_IsTextTrimmedChanged), true, this, args);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Mark state as dirty when an inherited property changes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::MarkInheritedPropertyDirty(
    _In_ const CDependencyProperty* pdp,
    _In_ const CValue* pValue)
{
    // Language list cannot be updated on the existing dwrite text layout, we have to create a new one in the next Measure call.
    if (pdp->GetIndex() == KnownPropertyIndex::FrameworkElement_Language)
    {
        m_isDWriteTextLayoutDirty = TRUE;
    }

    IFC_RETURN(CFrameworkElement::MarkInheritedPropertyDirty(pdp, pValue));

    if (!IsForegroundPropertyIndex(pdp->GetIndex()))
    {
        InvalidateContent();  //method does not return a value
    }
    else
    {
        InvalidateRender();
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Notify collection and its brush that the theme has changed.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBlock::NotifyThemeChangedCore(
    _In_ Theming::Theme theme,
    _In_ bool fForceRefresh)
{
    IFC_RETURN(CFrameworkElement::NotifyThemeChangedCore(theme, fForceRefresh));

    // Theme change may have changed high contrast mode and we disable color fonts
    // in high contrast.  Translation of color fonts happens during arrange so we
    // need to re-run arrange to re-apply that decision.
    if (m_isColorFontEnabled)
    {
        InvalidateContentArrange();
    }

    IFC_RETURN(EnsureBackPlateDependencies());
    UpdateBackPlateForegroundOverride();

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::NotifyApplicationHighContrastAdjustmentChanged()
{
    DirectUI::ApplicationHighContrastAdjustment applicationHighContrastAdjustment;
    IFC_RETURN(CApplication::GetApplicationHighContrastAdjustment(&applicationHighContrastAdjustment));

    IFC_RETURN(OnApplicationHighContrastAdjustmentChanged(applicationHighContrastAdjustment));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::EnsureFontContext
//
//  Synopsis: Makes sure the m_pFontContext is both present and up to date.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTextBlock::EnsureFontContext()
{
    CTextCore           *pTextCore   = nullptr;
    xref_ptr<IFontCollection> fontCollection;

    IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
    IFontAndScriptServices *pFontAndScriptServices = nullptr;
    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
    IFC_RETURN(pFontAndScriptServices->GetSystemFontCollection(fontCollection.ReleaseAndGetAddressOf()));

    if ((m_pFontContext != nullptr) && (m_pFontContext->GetBaseUri() == nullptr))
    {
        // our font context doesn't have a BaseUri, it might have been prepared while the element was not yet in the tree
        m_pFontContext->SetBaseUri(GetBaseUri());
    }

    if (m_pFontContext == nullptr)
    {
        m_pFontContext = new CFontContext(fontCollection.get(), GetBaseUri());
    }
    else
    {
        m_pFontContext->SetFontSource(fontCollection.get());
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::EnsureBlockLayout()
{
    HRESULT hr = S_OK;
    BlockNode *pBlockNode = nullptr;

    // this function should only be called when TextMode is normal.
    ASSERT(m_textMode == TextMode::Normal);

    // TextBlock should format one line of content  at its default properties even if there are no inlines in the collection.
    if (m_pInlines == nullptr)
    {
        IFC(CreateInlines());
    }

    if (m_pBlockLayout == nullptr)
    {
        IFC(EnsureFontContext());

        m_pBlockLayout = new BlockLayoutEngine(this);
    }

    if (m_pPageNode == nullptr)
    {
        IFC(m_pBlockLayout->CreatePageNode(nullptr, this, &pBlockNode));
        m_pPageNode = pBlockNode;
        pBlockNode = nullptr;
    }

    // If there is a valid PageNode, there is content that will be laid out. Create a TextView.
    if (m_pPageNode != nullptr)
    {
        EnsureTextBlockView();
    }

Cleanup:
    delete pBlockNode;
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::EnsureTextSelectionManager
//
//  Synopsis: Used to lazily initialize TextSelectionManager.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::EnsureTextSelectionManager()
{
    if (m_pSelectionManager == nullptr && (m_isTextSelectionEnabled || IsHighContrastAdjustmentActive()))
    {
        ASSERT(m_pInlines != nullptr && m_pTextView != nullptr);
        IFC_RETURN(TextSelectionManager::Create(this, m_pInlines, &m_pSelectionManager));
        // if we create a new text selection manager, we need to update its
        // selection highlight color.
        UpdateSelectionHighlightColor();

        IFC_RETURN(m_pSelectionManager->TextViewChanged(nullptr, m_pTextView));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::CreateInlines
//
//  Synopsis: Creates the inlines property, using SetValue to ensure
//            that the property system knows the property is not defaulted.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::CreateInlines()
{
    HRESULT              hr                  = S_OK;
    CInlineCollection   *pInlines            = nullptr;
    CREATEPARAMETERS     createParameters(GetContext());

    IFCEXPECT(m_pInlines == nullptr);

    IFC(CreateDO(&pInlines, &createParameters));
    IFC(CFrameworkElement::SetValueByKnownIndex(KnownPropertyIndex::TextBlock_Inlines, pInlines));

Cleanup:
    ReleaseInterface(pInlines);
    return hr;
}

_Check_return_ HRESULT CTextBlock::CreateTextHighlighters()
{
    ASSERT(m_textHighlighters == nullptr);

    xref_ptr<CTextHighlighterCollection> textHighlighterCollection;

    CREATEPARAMETERS createParameters(GetContext());
    IFC_RETURN(CreateDO(textHighlighterCollection.ReleaseAndGetAddressOf(), &createParameters));
    IFC_RETURN(textHighlighterCollection->SetOwner(this));

    CValue value;
    value.WrapObjectNoRef(textHighlighterCollection.get());

    // This will also set the m_textHighlighters property via metadata table offset
    IFC_RETURN(SetValueByKnownIndex(KnownPropertyIndex::TextBlock_TextHighlighters, value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//
//  Synopsis: Creates the SelectionHighlightColor property, using SetValue to ensure
//            that the property system knows the property is not defaulted.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::CreateSelectionHighlightColor()
{
    CValue value;

    IFCEXPECT_RETURN(IsPropertyDefaultByIndex(KnownPropertyIndex::TextBlock_SelectionHighlightColor));

    value.SetColor(GetDefaultSelectionHighlightColor());
    IFC_RETURN(CFrameworkElement::SetValueByKnownIndex(KnownPropertyIndex::TextBlock_SelectionHighlightColor, value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//
//  Synopsis: Update selection manager's highlight selection color,
//  use the default color if the SelectionHighlightColor property is set to nullptr
//
//------------------------------------------------------------------------
void CTextBlock::UpdateSelectionHighlightColor()
{
    uint32_t selectionHighlightColor;
    if (m_pSelectionManager)
    {
        auto highlightBrush = GetSelectionHighlightColor();
        if (highlightBrush != nullptr)
        {
            selectionHighlightColor = GetSelectionHighlightColor()->m_rgb;
        }
        else
        {
            selectionHighlightColor = GetDefaultSelectionHighlightColor();
        }
        VERIFYHR(m_pSelectionManager->SetSelectionHighlightColor(selectionHighlightColor));
        InvalidateRender();
    }
}

_Check_return_ HRESULT CTextBlock::InvalidateFontSize()
{
    const CDependencyProperty *pFontSizeProperty = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::TextBlock_FontSize);
    CValue value;

    IFC_RETURN(GetValueInherited(pFontSizeProperty, &value));
    IFC_RETURN(MarkInheritedPropertyDirty(pFontSizeProperty, &value));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::InvalidateContent
//
//  Synopsis: Invalidates layout and stored information about content,
//            e.g. run caches.
//
//------------------------------------------------------------------------
void CTextBlock::InvalidateContent()
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateContent();
    }
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->Clear();
    }
    InvalidateMeasure();
    InvalidateRender();
}

bool CTextBlock::BaseRealizationHasSubPixelOffsets() const
{
    return ((m_pTextDrawingContext != nullptr) && m_pTextDrawingContext->BaseRealizationHasSubPixelOffsets());
}

void CTextBlock::ClearBaseRealization()
{
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->ClearBaseRealization();
    }
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::InvalidateContentMeasure
//
//  Synopsis: Invalidate layout for all content, including
//            arrange/render info.
//
//------------------------------------------------------------------------
void CTextBlock::InvalidateContentMeasure()
{
    // Invalidate measure for the block layout engine, which invalidates arrange as well.
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateMeasure();
    }

    InvalidateMeasure();
    InvalidateRender();
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::InvalidateContentArrange
//
//  Synopsis: Invalidate arrange and render data for content.
//
//------------------------------------------------------------------------
void CTextBlock::InvalidateContentArrange()
{
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->InvalidateArrange();
    }

    InvalidateArrange();
    InvalidateRender();
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::InvalidateRender
//
//  Synopsis: Invalidates rendering data and releases render cache.
//
//------------------------------------------------------------------------
void CTextBlock::InvalidateRender()
{
    CUIElement::NWSetContentDirty(this, DirtyFlags::Render);
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::OnSelectionEnabledChanged
//
//  Synopsis: Creates/deletes TextSelectionManager if selection is enabled/
//            disabled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::OnSelectionEnabledChanged(bool oldValue)
{
    if (oldValue != m_isTextSelectionEnabled)
    {
        if (oldValue)
        {
            IFC_RETURN(SetCursor(MouseCursorDefault));

            if (m_pSelectionManager != nullptr)
            {
                IFC_RETURN(FxCallbacks::TextControlFlyout_CloseIfOpen(GetSelectionFlyoutNoRef()));

                // Selection went from enabled to disabled. Delete selection manager if BackPlate doesn't need it.
                if (!IsHighContrastAdjustmentEnabled())
                {
                    IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));
                }
                else
                {
                    IFC_RETURN(m_pSelectionManager->ReleaseGrippers());
                }
            }
        }
        else
        {
            IFC_RETURN(SetCursor(MouseCursorIBeam));
        }

        InvalidateRender();
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnHighContrastAdjustmentChanged(
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

_Check_return_ HRESULT CTextBlock::OnApplicationHighContrastAdjustmentChanged(
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

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::UpdateHighContrastAdjustments
//
//  Synopsis: Creates/deletes TextSelectionManager if HighContrastAdjustments are enabled/
//            disabled.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::UpdateHighContrastAdjustments()
{
    if (!IsHighContrastAdjustmentEnabled())
    {
        // BackPlate was set to disabled. Delete selection manager if selection doesn't need it.
        if (m_pSelectionManager && !m_isTextSelectionEnabled)
        {
            IFC_RETURN(TextSelectionManager::Destroy(&m_pSelectionManager));
        }
    }
    else
    {
        IFC_RETURN(EnsureBackPlateDependencies());
    }

    UpdateBackPlateForegroundOverride();
    InvalidateContentArrange();

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CTextBlock::RaiseSelectionChangedEvent
//
//  Synopsis: Raises routed event for SelectionChanged.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::RaiseSelectionChangedEvent()
{
    CEventManager *pEventManager = nullptr;
    xref_ptr<CRoutedEventArgs> pArgs;

    // Parser setting selection properties should not fire SelectionChanged event.
    if (!ParserOwnsParent())
    {
        auto core = GetContext();
        IFCEXPECT_ASSERT_RETURN(core);

        pEventManager = core->GetEventManager();
        if (pEventManager)
        {
            // Create the DO that represents the event args.
            pArgs = make_xref<CRoutedEventArgs>();

            // Raise event.
            pEventManager->RaiseRoutedEvent(EventHandle(KnownEventIndex::TextBlock_SelectionChanged), this, pArgs.get());

            if (core->UIAClientsAreListening(UIAXcp::AETextPatternOnTextSelectionChanged) == S_OK)
            {
                if(m_pAP == nullptr)
                {
                    OnCreateAutomationPeer();
                }
                else
                {
                    m_pAP->RaiseAutomationEvent(UIAXcp::AETextPatternOnTextSelectionChanged);
                }
            }
        }
    }

    return S_OK;
}

CTextElement* CTextBlock::GetTextElementForFocusRect()
{
    if (m_pTextView != nullptr
        && !CFocusRectManager::AreHighVisibilityFocusRectsEnabled())
    {
        CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
        if (pFocusManager)
        {
            CTextElement* pTextElement = pFocusManager->GetTextElementForFocusRectCandidate();

            if (pTextElement && pTextElement->GetContainingFrameworkElement() == this)
            {
                return pTextElement;
            }
        }
    }
    return nullptr;
}


//------------------------------------------------------------------------
//
//  Method: D2DPreChildrenRenderVirtual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::D2DPreChildrenRenderVirtual(
    _In_ const SharedRenderParams& sharedRP,
    _In_ const D2DRenderParams& d2dRP
    )
{
    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_pTextLayout != nullptr &&
            !GetIsMeasureDirty() &&
            !GetIsArrangeDirty())
        {

            CMILMatrix contentRenderTransform;
            IFC_RETURN(GetContentRenderTransform(&contentRenderTransform));

            CMILMatrix localTransformToRoot = *sharedRP.pCurrentTransform;
            localTransformToRoot.Prepend(contentRenderTransform);

            SharedRenderParams localSharedRP = sharedRP;
            localSharedRP.pCurrentTransform = &localTransformToRoot;

            if (IsSelectionEnabled() && m_pTextView)
             {
                IFC_RETURN(m_pSelectionManager->D2DRender(
                     d2dRP,
                     m_pTextView,
                     UseHighContrastSelection(GetContext()),
                     0,
                     nullptr));
             }

            IFC_RETURN(EnsureTextDrawingContext());

            IFC_RETURN(m_pTextDrawingContext->D2DRender(localSharedRP, d2dRP, d2dRP.m_forceOpaque ? 1.0f : GetOpacityCombined()));
        }
    }
    else
    {
        if (m_pPageNode != nullptr &&
            !m_pPageNode->IsMeasureDirty() &&
            !m_pPageNode->IsArrangeDirty())
        {
            ASSERT(m_pPageNode->GetDrawingContext() != nullptr);
            if (IsSelectionEnabled() && m_pTextView)
            {
                IFC_RETURN(m_pSelectionManager->D2DRender(
                    d2dRP,
                    m_pTextView,
                    UseHighContrastSelection(GetContext()),
                    0,
                    nullptr));
            }

            IFC_RETURN(m_pPageNode->GetDrawingContext()->D2DRenderContent(
                sharedRP,
                d2dRP,
                d2dRP.m_forceOpaque ? 1.0f : GetOpacityCombined()));
        }
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Overridden by various elements to create any D2D resources
//      needed to render this element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::D2DEnsureResources(
    _In_ const D2DPrecomputeParams &cp,
    _In_ const CMILMatrix *pMyAccumulatedTransform
    )
{
    if (m_textMode == TextMode::DWriteLayout)
    {
        IFC_RETURN(EnsureTextDrawingContext());
        if (NWIsContentDirty())
        {
            m_pTextDrawingContext->ClearGlyphRunCaches();
        }
        IFC_RETURN(m_pTextDrawingContext->D2DEnsureResources(cp, pMyAccumulatedTransform));
    }

    else
    {
        if (m_pPageNode != nullptr &&
            !m_pPageNode->IsMeasureDirty() &&
            !m_pPageNode->IsArrangeDirty())
        {
            ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

            // Render page node which will communicate render data for text to the drawing context.
            // The Draw call was skipped in Arrange.
            // If element is in the live tree, we use whatever render data we already have.
            IFC_RETURN(m_pPageNode->Draw(FALSE));

            // If UIElement is marked dirty for rendering, invalidate render walk caches accumulated
            // in the drawing context.
            // Example of such situation is when Brush is changed inside, without replacing entire Brush object.
            // In such case all the render walk caches (textures/edge stores) need to be regenerated.
            if (NWIsContentDirty())
            {
                m_pPageNode->GetDrawingContext()->InvalidateRenderCache();
            }

            IFC_RETURN(m_pPageNode->GetDrawingContext()->D2DEnsureResources(cp, pMyAccumulatedTransform));
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::UpdateSelectionAndHighlightRegions(_Out_ bool *redrawForHighlightRegions)
{
    *redrawForHighlightRegions = false;
    std::shared_ptr<HighlightRegion> selection = nullptr;
    m_highlightRegions.clear();

    // Find text selection.
    if (m_pSelectionManager != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->GetSelectionHighlightRegion(UseHighContrastSelection(GetContext()), selection));
        if (selection != nullptr)
        {
            m_highlightRegions.push_back(*selection);
        }
    }

    if (!TextSelectionManager::AreSelectionHighlightOffsetsEqual(m_selectionHighlight, selection))
    {
        // Redraw is needed
        // Save offsets, so 'this' will maintain selection range memory ownership
        m_selectionHighlight = selection;
        // If selection has changed we must force re-rendering even if Arrange was bypassed.
        *redrawForHighlightRegions = true;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::NotifyRenderContent(
    HWRenderVisibility visibility
    )
{
    if (visibility == HWRenderVisibility::Invisible)
    {
        m_alphaMask.Hide();
    }
    else
    {
        // Update the alpha mask.  This will check if there is one created and only update it if necessary.
        IFC_RETURN(m_alphaMask.UpdateIfAvailable(this));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: PrintPreChildrenPrintVirtual
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::PreChildrenPrintVirtual(
    _In_ const SharedRenderParams& sharedPrintParams,
    _In_ const D2DPrecomputeParams& cp,
    _In_ const D2DRenderParams &printParams
    )
{
    IFC_RETURN(D2DEnsureResources(cp, sharedPrintParams.pCurrentTransform));
    IFC_RETURN(D2DPreChildrenRenderVirtual(sharedPrintParams, printParams));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    pBounds->left = 0.0f;
    pBounds->top = 0.0f;
    pBounds->right = CFrameworkElement::GetActualWidth();
    pBounds->bottom = CFrameworkElement::GetActualHeight();

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
_Check_return_ HRESULT CTextBlock::HitTestLocalInternal(
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
_Check_return_ HRESULT CTextBlock::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds =  { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = target.IntersectsRect(innerBounds);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:  CTextBlock::GetTextElementBoundRect
//
//  Summary: Calculate bounding rectangles for the specifed TextElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GetTextElementBoundRect(
    _In_ CTextElement *pElement,
    _Out_ XRECTF *pRectFocus,
    bool ignoreClip
    )
{
    ASSERT(m_textMode == TextMode::Normal);
    RRETURN(CRichTextBlock::GetTextElementBoundRect(pElement, m_pTextView, this, pRectFocus, ignoreClip));
}

//------------------------------------------------------------------------
//
//  Method:  CTextBlock::HitTestLink
//
//  Summary: Hit test Links.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::HitTestLink(
    _In_ XPOINTF *ptPointer,
    _Outptr_ CHyperlink **ppLink
    )
{
    CHyperlink *pLink = nullptr;

    *ppLink = nullptr;

    if (m_pInlines != nullptr && m_pTextView != nullptr)
    {
        IFC_RETURN(CRichTextBlock::HitTestLink(
            m_pInlines->GetTextContainer(),
            this,
            ptPointer,
            m_pTextView,
            &pLink));
    }

    *ppLink = pLink;

    return S_OK;
}

_Check_return_ XUINT32 CTextBlock::GetLinkAPChildrenCount()
{
    return CRichTextBlock::GetLinkAPChildrenCountHelper(m_pInlines);
}

//------------------------------------------------------------------------
//
//  Method:  GetFocusableChildrenHelper
//
//  Summary: Navigates through the TextElement collection and retrieves
//           list of focusable element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GetFocusableChildrenHelper(
    _In_ CDOCollection *pFocusChildren,
    _In_ CTextElementCollection *pCollection
    )
{
    HRESULT hr = S_OK;
    CDependencyObject *pObject = nullptr;

    for (uint32_t i = 0, count = pCollection->GetCount(); i < count; i++)
    {
        pObject = pCollection->GetItemDOWithAddRef(i);
        if (pObject)
        {
            if (CFocusableHelper::IsFocusableDO(pObject))
            {
                IFC(pFocusChildren->Append(pObject));
            }
            else if (pObject->OfTypeByIndex<KnownTypeIndex::Span>())
            {
                CSpan *pSpan = static_cast<CSpan *>(pObject);
                if (pSpan->m_pInlines != nullptr)
                {
                    IFC(GetFocusableChildrenHelper(pFocusChildren, pSpan->m_pInlines));
                }
            }
        }
        ReleaseInterface(pObject);
    }

Cleanup:
    ReleaseInterface(pObject);
    return hr;
}

//------------------------------------------------------------------------
//
//  Method:  GetFocusableChildren
//
//  Summary: Builds a collection of elements for the focus manager to
//           discover and work with for tab navigation.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::GetFocusableChildren(
    _Outptr_result_maybenull_ CDOCollection **ppFocusableChildren
    )
{
    *ppFocusableChildren = nullptr;

    if (m_pInlines != nullptr)
    {
        if (m_focusableChildrenCollection == nullptr)
        {
            m_focusableChildrenCollection = make_xref<TextBlockFocusableChildrenCollection>(GetContext());
            IFC_RETURN(GetFocusableChildrenHelper(m_focusableChildrenCollection.get(), m_pInlines));
        }

        if (m_focusableChildrenCollection->GetCount() != 0)
        {
            // NOTE: Caller doesn't expect us to do an AddRef, so we don't use detach here.
            *ppFocusableChildren = m_focusableChildrenCollection.get();
        }
    }

    return S_OK;
}

void CTextBlock::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    if (m_pPageNode != nullptr)
    {
        m_pPageNode->CleanupRealizations();
    }

    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->ClearGlyphRunCaches();
    }

    m_alphaMask.Hide();
}

void CTextBlock::ClearPCRenderData()
{
    ASSERT(IsInPCScene_IncludingDeviceLost());

    __super::ClearPCRenderData();

    // Hide the alpha mask so that the composition surface is released.
    m_alphaMask.Hide();
}

//-----------------------------------------------------------------------------
//
//  Synopsis:
//      CUIElement override to update the gripper positions. This is required
//      when the rearrage occurs, for example upon a screen orientation change.
//
//-----------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::ArrangeCore(XRECTF finalRect)
{
    HRESULT hr = CFrameworkElement::ArrangeCore(finalRect);
    if (m_pSelectionManager != nullptr)
    {
        IFC(m_pSelectionManager->UpdateGripperPositions());
    }
Cleanup:
    return hr;
}

void CTextBlock::SetFastPathOptOutConditions (_In_ FastPathOptOutConditions mask)
{
    m_fFastPathOptOutConditions |= static_cast<uint32_t>(mask);

}

void CTextBlock::ClearFastPathOptOutConditions (_In_ FastPathOptOutConditions mask)
{
    m_fFastPathOptOutConditions &= ~static_cast<uint32_t>(mask);

}

void CTextBlock::SetFastPathOptOutConditions (_In_ FastPathOptOutConditions mask, _In_ bool isSet)
{
    if (isSet)
    {
        SetFastPathOptOutConditions(mask);
    }
    else
    {
        ClearFastPathOptOutConditions(mask);
    }
}

// Determine the layout mode, either Normal(LS) or DWriteTextLayout.
_Check_return_ HRESULT CTextBlock::ResolveTextMode()
{
    IFC_RETURN(EnsureTextFormattingForRead());
    IFC_RETURN(EnsureInheritedPropertiesForRead());

    BOOL canUseDWriteLayout = false;

    if (m_fFastPathOptOutConditions == 0)
    {
        // If all the above tests passed, we also need to make sure the language does not need number substitution,
        // because there is currently a behavior difference in DWTL for number substitution.
        // We need to check here rather than in UpdateFastPathOptOutConditions because the Language is an inherited property,
        // and we cannot rely on OnPropertyChanged notifications when the inherited value changes.
        BOOL needNumberSubstitution;
        IFC_RETURN(NeedNumberSubsitution(&needNumberSubstitution));
        canUseDWriteLayout = !needNumberSubstitution;
    }

    if (m_textMode == TextMode::Normal)
    {
        if (canUseDWriteLayout)
        {
            m_textMode = TextMode::DWriteLayout;
        }
    }
    else
    {
        if (!canUseDWriteLayout)
        {
            IFC_RETURN(DeferredCreateInlineCollection());
            m_textMode = TextMode::Normal;
            m_pTextLayout = nullptr;
            m_pTextDrawingContext.reset();
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::TextRangeAdapterAssociated()
{
    if (m_textMode == TextMode::DWriteLayout)
    {
        IFC_RETURN(DeferredCreateInlineCollection());
        EnsureTextBlockView();
        IFC_RETURN(UpdateLayout());
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::NeedNumberSubsitution(_Out_ BOOL* pIsNeeded)
{
    *pIsNeeded = true;
    DWriteFontAndScriptServices *pDWriteFontServices = nullptr;
    const TextFormatting* pTextFormatting = nullptr;
    Microsoft::WRL::ComPtr<IDWriteNumberSubstitution> spDWriteNumberSubstitution;

    IFC_RETURN(GetTextFormatting(&pTextFormatting));
    IFC_RETURN(GetDWriteFontAndScriptServices(&pDWriteFontServices));

    IFC_RETURN(pDWriteFontServices->GetNumberSubstitution(pTextFormatting->m_strLanguageString, nullptr, &spDWriteNumberSubstitution));

    if (spDWriteNumberSubstitution == nullptr)
    {
        *pIsNeeded = false;
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetDWriteFontAndScriptServices(_Outptr_ DWriteFontAndScriptServices **ppDWriteFontServices)
{
    CTextCore *pTextCore = nullptr;
    IFontAndScriptServices *pFontServices = nullptr;
    IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
    IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontServices));

    *ppDWriteFontServices =
        static_cast<DWriteFontAndScriptServices*>(
            static_cast<PALFontAndScriptServices*>(pFontServices)->GetPALFontAndScriptServices());

    return S_OK;
}

XPOINTF CTextBlock::GetContentRenderingOffset(_In_ DirectUI::FlowDirection flowDirection) const
{
    XPOINTF offset = {0.0f, 0.0f};

    if (flowDirection == DirectUI::FlowDirection::RightToLeft)
    {
        offset.x = -m_padding.left;
    }
    else
    {
        offset.x = m_padding.left;
    }
    offset.y = m_padding.top;
    offset.y += m_layoutRoundingHeightAdjustment;

    return offset;
}

_Check_return_ HRESULT CTextBlock::GetContentRenderTransform(_Out_ CMILMatrix* pContentRenderTransform)
{
    pContentRenderTransform->SetToIdentity();

    const TextFormatting* pTextFormatting = nullptr;
    IFC_RETURN(GetTextFormatting(&pTextFormatting));

    // For FlowDirection == RTL, we need to undo a mirroring transform set from a higher node.
    if (pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::RightToLeft)
    {
        DWRITE_TEXT_METRICS m = {};
        IFC_RETURN(m_pTextLayout->GetMetrics(&m));
        pContentRenderTransform->SetM11(-1);
        pContentRenderTransform->SetDx(m.layoutWidth);
    }

    const XPOINTF contentOffset = GetContentRenderingOffset(pTextFormatting->m_nFlowDirection);
    CMILMatrix contentOffsetTransform(TRUE);
    contentOffsetTransform.SetDx(contentOffset.x);
    contentOffsetTransform.SetDy(contentOffset.y);
    pContentRenderTransform->Prepend(contentOffsetTransform);
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::DeferredCreateInlineCollection()
{
    if (m_pInlines == nullptr)
    {
        if (!m_shouldAddTextOnDeferredInlineCollectionCreation)
        {
            IFC_RETURN(CreateInlines());
        }
        else
        {
            IFC_RETURN(SetTextToInlineCollection());
            // Since we just created the inlines, we know there is no complex content in them yet.
            ClearFastPathOptOutConditions(FastPathOptOutConditions::HasComplexContent);
        }
        m_shouldAddTextOnDeferredInlineCollectionCreation = FALSE;
    }

    return S_OK;
}

bool CTextBlock::GetUseLayoutRounding() const
{
    return !!m_fUseLayoutRounding;
}

_Check_return_ HRESULT CTextBlock::OnPointerCaptureLost(_In_ CEventArgs* pEventArgs)
{
    IFC_RETURN(__super::OnPointerCaptureLost(pEventArgs));
    if (m_pressedHyperlink)
    {
        IFC_RETURN(m_pressedHyperlink->UpdateForegroundColor(HYPERLINK_NORMAL));
    }

    if (m_currentLink)
    {
        IFC_RETURN(m_currentLink->UpdateForegroundColor(HYPERLINK_NORMAL));
    }
    return S_OK;
}

// Layout offset for LineStackingStrategy::BaselineToBaseline and line height > 0.
// This function adjusts the first/last line offsets for the fast path(DWTL).
// Please see ApplyLineStackingStrategy() for the slow path peer.
_Check_return_ HRESULT CTextBlock::GetLineStackingOffset(_In_ uint32_t lineCount, _Out_ float* offset)
{
    ASSERT(m_textMode == TextMode::DWriteLayout);
    ASSERT(lineCount != 0); // Line count formatted by DWrite will never be 0.Even an empty TextBlock will have 1 line.
    *offset = 0.0f;

    if (m_lineStackingStrategy == DirectUI::LineStackingStrategy::BaselineToBaseline && m_eLineHeight > 0)
    {
        float fontBaseline;
        float fontLineAdvance;
        const TextFormatting* pTextFormatting = nullptr;
        IFC_RETURN(GetTextFormatting(&pTextFormatting));
        IFC_RETURN(pTextFormatting->m_pFontFamily->GetTextLineBoundsMetrics(GetFontContext(), m_textLineBounds, &fontBaseline, &fontLineAdvance));
        ASSERT(fontLineAdvance > 0);
        float scaledFontSize = pTextFormatting->GetScaledFontSize(GetContext()->GetFontScale());

        if (lineCount == 1)
        {
            float fontBaselineRatio = fontBaseline / fontLineAdvance;
            *offset = (m_eLineHeight - scaledFontSize*fontLineAdvance) * fontBaselineRatio;
        }
        else
        {
            *offset = m_eLineHeight - scaledFontSize*fontLineAdvance;
        }
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetDWriteTextLayout(_Out_ IDWriteTextLayout** textLayout) const
{
    IFC_RETURN(m_pTextLayout.CopyTo(textLayout));
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetFlowDirection(_Out_ DirectUI::FlowDirection* flowDirection)
{
    const TextFormatting* pTextFormatting = nullptr;
    IFC_RETURN(GetTextFormatting(&pTextFormatting));
    *flowDirection = pTextFormatting->m_nFlowDirection;
    return S_OK;
}

void CTextBlock::EnsureTextBlockView()
{
    //We don't need to always create the view. But for the below cases we need to create it:
    // 1) Hit-testing/text adapter associated.
    // 2) selection enabled.
    // 3) GetContentStart, a text pointer will be returned, users could call GetCharacterRect() which requires hit-testing.
    // 4) GetContentEnd, same as 3.
    // 5) BackPlate active
    // 6) TextHighlighting
    if (m_pTextView == nullptr)
    {
        m_pTextView = new TextBlockView(this);
    }
}

_Check_return_ HRESULT CTextBlock::GetAlphaMask(
    _Outptr_ WUComp::ICompositionBrush** ppReturnValue)
{
    IFC_RETURN(m_alphaMask.Ensure(this));
    *ppReturnValue = m_alphaMask.GetCompositionBrush().Detach();

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetDWriteTextMetricsOffset(_Out_ XPOINTF* offset)
{
    *offset = {0,0};
    ASSERT(m_textMode == TextMode::DWriteLayout);
    if (m_pTextLayout)
    {
        DWRITE_TEXT_METRICS m = {};
        IFC_RETURN(m_pTextLayout->GetMetrics(&m));
        offset->x = m.left;
        offset->y = m.top;
    }
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetTightInnerBounds(
    _Out_ XRECTF_RB* pBounds)
{
    float width;
    float height;
    XPOINTF textMetricsOffset; // Account for offsets caused by FlowDirection, TextAlignment.
    XPOINTF contentRenderingOffset; // Account for offsets cuased by Padding.
    DirectUI::FlowDirection flowDirection;
    IFC_RETURN(GetActualWidth(&width));
    IFC_RETURN(GetActualHeight(&height));
    IFC_RETURN(GetFlowDirection(&flowDirection));
    contentRenderingOffset = GetContentRenderingOffset(flowDirection);

    if (m_textMode == TextMode::DWriteLayout)
    {
        IFC_RETURN(GetDWriteTextMetricsOffset(&textMetricsOffset));
    }
    else
    {
        // Do hit-testing for slow path.
        uint32_t cBoundRects = 0;
        XRECTF *pBoundRects = nullptr;
        xref_ptr<CTextPointerWrapper> contentStart;
        xref_ptr<CTextPointerWrapper> contentEnd;
        int32_t contentStartOffset;
        int32_t contentEndOffset;
        XRECTF textMetricsBound;

        IFC_RETURN(GetContentStart(contentStart.ReleaseAndGetAddressOf()));
        IFC_RETURN(GetContentEnd(contentEnd.ReleaseAndGetAddressOf()));
        IFC_RETURN(contentStart->GetOffset(&contentStartOffset));
        IFC_RETURN(contentEnd->GetOffset(&contentEndOffset));

        EnsureTextBlockView();
        IFC_RETURN(m_pTextView->TextRangeToTextBounds(
            contentStartOffset,
            contentEndOffset,
            &cBoundRects,
            &pBoundRects));

        if (cBoundRects > 0)
        {
            //  since there can be multiple bounding rects as they wrap we need to unioning them together
            textMetricsBound = pBoundRects[0];
            for (uint32_t i = 1; i < cBoundRects; i++)
            {
                UnionRectF(&textMetricsBound, &pBoundRects[i]);
            }
        }
        delete [] pBoundRects;
        textMetricsOffset.x = textMetricsBound.X;
        textMetricsOffset.y = textMetricsBound.Y;
    }

    pBounds->left = textMetricsOffset.x + contentRenderingOffset.x;
    pBounds->top = textMetricsOffset.y + contentRenderingOffset.y;
    pBounds->right = pBounds->left + width;
    pBounds->bottom = pBounds->top + height;
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::GetTightGlobalBounds(
    _Out_ XRECTF_RB* pBounds,
    _In_ bool ignoreClipping,
    _In_ bool useTargetInformation  // Attempt to use target values for animations (e.g. manipulations/LTEs) instead of current values
)
{
    XRECTF_RB innerBounds;

    IFC_RETURN(GetTightInnerBounds(&innerBounds));
    IFC_RETURN(TransformToWorldSpace(&innerBounds, pBounds, ignoreClipping, false /* ignoreClippingOnScrollContentPresenters */, useTargetInformation));

    return S_OK;
}

// Checks is HighContrast Theme is active and HighContrastAdjustment is enabled.
bool CTextBlock::IsHighContrastAdjustmentActive() const
{
    return UseHighContrastSelection(GetContext()) && IsHighContrastAdjustmentEnabled();
}

bool CTextBlock::IsHighContrastAdjustmentEnabled() const
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

// Sets DrawingContext properties to handle foreground color when BackPlate is enabled.
void CTextBlock::UpdateBackPlateForegroundOverride()
{
    if (m_pTextDrawingContext)
    {
        m_pTextDrawingContext->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), m_useHyperlinkForegroundOnBackPlate);
    }

    if (m_pPageNode)
    {
        if (m_pPageNode->GetDrawingContext() != nullptr)
        {
            m_pPageNode->GetDrawingContext()->SetBackPlateConfiguration(IsHighContrastAdjustmentActive(), m_useHyperlinkForegroundOnBackPlate);
        }
    }
}

void CTextBlock::SetUseHyperlinkForegroundOnBackPlate(bool useHyperlinkForegroundOnBackPlate)
{
    if (m_useHyperlinkForegroundOnBackPlate != useHyperlinkForegroundOnBackPlate)
    {
        m_useHyperlinkForegroundOnBackPlate = useHyperlinkForegroundOnBackPlate;
        UpdateBackPlateForegroundOverride();
    }
}

void CTextBlock::SetBackPlateVisibility(bool visible)
{
    if (m_drawBackPlate != visible)
    {
        m_drawBackPlate = visible;
        InvalidateRender();
    }
}

bool CTextBlock::IsSelectionEnabled() const
{
    return m_pSelectionManager != nullptr && m_isTextSelectionEnabled;
}

_Check_return_ HRESULT CTextBlock::RenderHighlighterForegroundCallback(
    _In_ CSolidColorBrush* foregroundBrush,
    _In_ uint32_t highlightRectCount,
    _In_reads_(highlightRectCount) XRECTF* highlightRects
    )
{
    if (m_textMode == TextMode::DWriteLayout)
    {
        XPOINTF pageOffset = { m_padding.left, m_padding.top };
        m_pTextDrawingContext->AppendForegroundHighlightInfo(
            highlightRectCount,
            highlightRects,
            foregroundBrush,
            0, // Index in rect array at which DrawingContext should start processing rects.
            highlightRectCount,// Number of rectangles processed by the page.
            pageOffset);
    }
    else
    {
        XPOINTF pageOffset = { 0.0f, 0.0f };
        uint32_t rectsProcessed = 0;
        m_pPageNode->GetDrawingContext()->AppendForegroundHighlightInfo(
            highlightRectCount,
            highlightRects,
            foregroundBrush,
            0, // Index in rect array at which DrawingContext should start processing rects.
            pageOffset, // Offset of page in element's space.
            &rectsProcessed); // Number of rectangles processed by the page.

        // All highlight rects within local text view should be within the page.
        ASSERT(rectsProcessed == highlightRectCount);
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::RedrawText()
{
    if (m_textMode == TextMode::DWriteLayout)
    {
        // If selection has changed we must force re-rendering even if Arrange was bypassed.
        xref::weakref_ptr<CTextBlock> pBrushSource = xref::get_weakref(this);

        DWriteTextRenderer dWriteRenderer(m_pTextDrawingContext, pBrushSource);

        m_pTextDrawingContext->Clear();
        DirectUI::FlowDirection flowDirection;
        IFC_RETURN(GetFlowDirection(&flowDirection));
        m_pTextDrawingContext->SetFlipSelectionAlongHorizontalAxis(flowDirection == DirectUI::FlowDirection::RightToLeft);
        IFC_RETURN(m_pTextLayout->Draw(nullptr, &dWriteRenderer, 0, 0));
    }
    else
    {
        // Render page node which will communicate render data for text to the drawing context.
        // If selection has changed we must force re-rendering even if Arrange was bypassed.
        IFC_RETURN(m_pPageNode->Draw(TRUE));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
// Event handlers
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::OnPointerMoved(
    _In_ CEventArgs* pEventArgs
)
{
    // check whether the pointer is over a Link
    if (m_pInlines != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pInlines->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));
            }

            auto currentLink = GetCurrentLinkNoRef();
            if (currentLink != pLink)
            {
                if (currentLink != nullptr)
                {
                    // If we go from one hyperlink to another, we need to reset the old one to normal color.
                    IFC_RETURN(currentLink->OnPointerExited(pPointerArgs));
                }

                if (pLink != nullptr)
                {
                    IFC_RETURN(pLink->OnPointerEntered(pPointerArgs));
                }
            }

            if (pLink != nullptr)
            {
                m_currentLink = pLink;
                // There is a hyperlink, we need to update the cursor to hand shape.
                VERIFYHR(SetCursor(MouseCursorHand));

                // Moved but pressed.
                auto currentHyperlink = m_currentLink.get();
                if (pPointerArgs->m_pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse ||
                    (pPointerArgs->m_pPointer->m_bLeftButtonPressed && (currentHyperlink == GetPressedHyperlink())))
                {
                    IFC_RETURN(currentHyperlink->UpdateForegroundColor(HYPERLINK_PRESSED));
                }
                // Just point over.
                else
                {
                    IFC_RETURN(currentHyperlink->UpdateForegroundColor(HYPERLINK_POINTOVER));
                }
            }
            else
            {
                if (m_isTextSelectionEnabled)
                {
                    VERIFYHR(SetCursor(MouseCursorIBeam));
                }
                else
                {
                    VERIFYHR(SetCursor(MouseCursorDefault));
                }
            }
        }
    }

    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnPointerMoved(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnPointerExited(
    _In_ CEventArgs* pEventArgs
)
{
    if (auto currentLink = GetCurrentLinkNoRef())
    {
        IFC_RETURN(currentLink->OnPointerExited(static_cast<CPointerEventArgs *>(pEventArgs)));
        m_currentLink.reset();
    }

    m_pressedHyperlink.reset();
    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnPointerPressed(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (m_pInlines != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        CHyperlink *pLink = nullptr;

        if (!pPointerArgs->m_bHandled)
        {
            // Hyperlink clicks are prioritized above selection.
            if (pPointerArgs->m_pPointer->m_pointerDeviceType != DirectUI::PointerDeviceType::Mouse ||
                pPointerArgs->m_pPointer->m_bLeftButtonPressed)
            {
                // If the event doesn't come from the sender, we shouldn't be doing anything.
                if (pPointerArgs->m_pSource == this)
                {
                    IFC_RETURN(CRichTextBlock::HitTestLink(
                        m_pInlines->GetTextContainer(),
                        this,
                        &pPointerArgs->GetGlobalPoint(),
                        m_pTextView,
                        &pLink));
                }

                if (pLink != nullptr)
                {
                    CFocusManager *pFocusManager = VisualTree::GetFocusManagerForElement(this);
                    const FocusMovementResult result = pFocusManager->SetFocusedElement(FocusMovement(pLink, DirectUI::FocusNavigationDirection::None, DirectUI::FocusState::Pointer));
                    IFC_RETURN(result.GetHResult());
                    m_pressedHyperlink = pLink;

                    IFC_RETURN(pLink->UpdateForegroundColor(HYPERLINK_PRESSED));
                }

                if (pLink != nullptr)
                {
                    m_currentLink = pLink;
                }
            }
        }
        if (IsSelectionEnabled())
        {
            IFC_RETURN(m_pSelectionManager->OnPointerPressed(
                this,
                pEventArgs,
                m_pTextView));
        }
        if (pLink != nullptr)
        {
            pPointerArgs->m_bHandled = TRUE;
        }
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnPointerReleased(
    _In_ CEventArgs* pEventArgs
)
{
    if (m_pInlines != nullptr && m_pTextView != nullptr)
    {
        CPointerEventArgs *pPointerArgs = static_cast<CPointerEventArgs*>(pEventArgs);
        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;
            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                IFC_RETURN(CRichTextBlock::HitTestLink(
                    m_pInlines->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));

                // If we go from one link to another, we need to reset the old one to normal color.
                auto currentLink = GetCurrentLinkNoRef();
                if (currentLink != pLink)
                {
                    if (currentLink != nullptr)
                    {
                        // If we go from one hyperlink to another, we need to reset the old one to normal color.
                        IFC_RETURN(currentLink->OnPointerExited(pPointerArgs));
                    }

                    if (pLink != nullptr)
                    {
                        IFC_RETURN(pLink->OnPointerEntered(pPointerArgs));
                    }
                }

                if (pLink != nullptr)
                {
                    m_currentLink = pLink;

                    // Not pressed, point over for mouse.
                    if (pPointerArgs->m_pPointer->m_pointerDeviceType == DirectUI::PointerDeviceType::Mouse
                        && !pPointerArgs->m_pPointer->m_bLeftButtonPressed)
                    {
                        IFC_RETURN(pLink->UpdateForegroundColor(HYPERLINK_POINTOVER));
                    }
                    // When touch release, set color to normal.
                    else
                    {
                        IFC_RETURN(pLink->OnPointerExited(pPointerArgs));
                    }
                }
                else
                {
                    m_currentLink.reset();
                }
            }
        }
    }
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnPointerReleased(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnGotFocus(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnGotFocus(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnLostFocus(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnLostFocus(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnHolding(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnHolding(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnTapped(
    _In_ CEventArgs* pEventArgs
)
{
    HRESULT hr = S_OK;

    IFC(EnsureTextSelectionManager());

    if (m_pInlines != nullptr && m_pTextView != nullptr)
    {
        CTappedEventArgs *pPointerArgs = static_cast<CTappedEventArgs*>(pEventArgs);
        bool processSelection = true;

        if (!pPointerArgs->m_bHandled)
        {
            CHyperlink *pLink = nullptr;

            // If the event doesn't come from the sender, we shouldn't be doing anything.
            if (pPointerArgs->m_pSource == this)
            {
                // Hittest the content to find out if we are over a Hyperlink element.
                IFC(CRichTextBlock::HitTestLink(
                    m_pInlines->GetTextContainer(),
                    this,
                    &pPointerArgs->GetGlobalPoint(),
                    m_pTextView,
                    &pLink));
            }

            // Hyperlink clicks are prioritized above selection.
            // Do Hyperlink navigation only if the Hyperlink is the same as
            // the one for which press was initiated.
            if (pLink != nullptr && GetPressedHyperlink() == pLink)
            {
                processSelection = false;
                IFC(pLink->Navigate());
                pPointerArgs->m_bHandled = TRUE;
            }
        }

        if (IsSelectionEnabled() && processSelection)
        {
            IFC(m_pSelectionManager->OnTapped(
                this,
                pEventArgs,
                m_pTextView));
        }
    }

Cleanup:
    m_pressedHyperlink.reset();
    return hr;
}

_Check_return_ HRESULT CTextBlock::OnRightTapped(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnRightTapped(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnDoubleTapped(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnDoubleTapped(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnKeyUp(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnKeyUp(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::OnKeyDown(
    _In_ CEventArgs* pEventArgs
)
{
    IFC_RETURN(EnsureTextSelectionManager());
    if (IsSelectionEnabled() && m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->OnKeyDown(
            this,
            pEventArgs,
            m_pTextView));
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   HWRenderSelection
//
//  Synopsis:
//      Calls TextSelectionManager to render selection, indicating if
//      current theme is high contrast so appropriate highlight color
//      can be used.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::HWRenderSelection(
    _In_ IContentRenderer* pContentRenderer,
    _In_ bool isHighContrast,
    _In_ uint32_t highlightRectCount,
    _In_reads_opt_(highlightRectCount) XRECTF *pHighlightRects,
    _In_ bool isBackPlate
    )
{
    if (m_pSelectionManager != nullptr &&
        m_pTextView != nullptr)
    {
        IFC_RETURN(m_pSelectionManager->HWRender(pContentRenderer, m_pTextView, isHighContrast, highlightRectCount, pHighlightRects, isBackPlate));
    }

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::RenderBackplateIfRequired(_In_opt_ IContentRenderer* pContentRenderer)
{
    HRESULT hr = S_OK;
    XRECTF *pBackPlateHighlightRects = nullptr;
    uint32_t cBackPlateHighlightRects = 0;

    if(m_pInlines == nullptr || m_pTextView == nullptr)
    {
        goto Cleanup;
    }

    IFC(EnsureTextSelectionManager());

    if (m_pSelectionManager != nullptr)
    {
        // Find the BackPlate selection rectangles.
        if (pBackPlateHighlightRects == nullptr)
        {
            if (m_drawBackPlate && IsHighContrastAdjustmentActive())
            {
                IFC(m_pSelectionManager->GetBackPlateSelectionHighlightRects(
                    m_pTextView,
                    &cBackPlateHighlightRects,
                    &pBackPlateHighlightRects));
            }
        }
    }

    // Render BackPlate.
    if (m_drawBackPlate && IsHighContrastAdjustmentActive() &&
        cBackPlateHighlightRects > 0)
    {
        ASSERT(pContentRenderer != nullptr);
        IFC(HWRenderSelection(
            pContentRenderer,
            UseHighContrastSelection(GetContext()), // Is high contrast.
            cBackPlateHighlightRects,
            pBackPlateHighlightRects,
            true));
    }

Cleanup:
    delete[] pBackPlateHighlightRects;
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders content in PC rendering walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::HWRenderContent(
    _In_ IContentRenderer* pContentRenderer
)
{
    HRESULT hr = S_OK;

    if (m_textMode == TextMode::DWriteLayout)
    {
        if (m_pTextLayout != nullptr &&
            m_pTextDrawingContext != nullptr &&
            !GetIsMeasureDirty() &&
            !GetIsArrangeDirty())
        {
            IFC(RenderBackplateIfRequired(pContentRenderer));

            auto foregroundRenderingCallback = std::bind(
                &CTextBlock::RenderHighlighterForegroundCallback,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

            bool redrawForHighlightRegions = false;
            IFC(UpdateSelectionAndHighlightRegions(&redrawForHighlightRegions));

            //Render the highlight selection rectangles and redraw if necessary.
            IFC(TextHighlightRenderer::HWRenderCollection(
                GetContext(),
                m_textHighlighters,
                m_highlightRegions,
                m_pTextView,
                pContentRenderer,
                foregroundRenderingCallback));

            // TODO: Potential optimization for text highlighters here and elsewhere to set a collection
            //                 dirty flag when the collection is changed so that text only needs to be redrawn when
            //                 necessary.  A similar change also needs to be done elsewhere in this file and in
            //                 RichTextBlock/RichTextBlockOverflow.  Search for "redrawForHighlightRegions" to find locations.
            if (redrawForHighlightRegions || m_redrawForArrange || (m_textHighlighters != nullptr))
            {
                // Text must be redrawn due to highlight rects used for glyph run foreground coloring.
                IFC(RedrawText());
                m_redrawForArrange = false;
            }

            const HWRenderParams& rp = *(pContentRenderer->GetRenderParams());
            HWRenderParams localRP = rp;
            HWRenderParamsOverride hwrpOverride(pContentRenderer, &localRP);

            CMILMatrix contentRenderTransform;
            IFC(GetContentRenderTransform(&contentRenderTransform));

            CTransformToRoot localTransformToRoot = *(pContentRenderer->GetTransformToRoot());
            localTransformToRoot.Prepend(contentRenderTransform);

            TransformAndClipStack transformsAndClips;
            IFC(transformsAndClips.Set(rp.pTransformsAndClipsToCompNode));
            transformsAndClips.PrependTransform(contentRenderTransform);

            TransformToRoot2DOverride transform2DOverride(pContentRenderer, &localTransformToRoot);
            localRP.pTransformsAndClipsToCompNode = &transformsAndClips;

            if (NWIsContentDirty())
            {
                m_pTextDrawingContext->ClearGlyphRunCaches();
            }

            IFC(m_pTextDrawingContext->HWRender(pContentRenderer));
        }
    }
    else if (m_textMode == TextMode::Normal)
    {
        if (m_pPageNode != nullptr &&
            !m_pPageNode->IsMeasureDirty() &&
            !m_pPageNode->IsArrangeDirty())
        {
            ASSERT(m_pPageNode->GetDrawingContext() != nullptr);

            // Page node should be rendered at this time so that glyph runs can be split to show selection
            // foreground per theme colors.
            IFC(RenderBackplateIfRequired(pContentRenderer));

            auto foregroundRenderingCallback = std::bind(
                &CTextBlock::RenderHighlighterForegroundCallback,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3);

            bool redrawForHighlightRegions = false;
            IFC(UpdateSelectionAndHighlightRegions(&redrawForHighlightRegions));

            // Render the highlight selection rectangles and redraw if necessary.
            IFC(TextHighlightRenderer::HWRenderCollection(
                GetContext(),
                m_textHighlighters,
                m_highlightRegions,
                m_pTextView,
                pContentRenderer,
                foregroundRenderingCallback));

            if (redrawForHighlightRegions || m_redrawForArrange || (m_textHighlighters != nullptr))
            {
                // Text must be redrawn due to highlight rects used for glyph run foreground coloring.
                IFC(RedrawText());
                m_redrawForArrange = false;
            }

            // If UIElement is marked dirty for rendering, invalidate render walk caches accumulated
            // in the drawing context.
            // Example of such situation is when Brush is changed inside, without replacing entire Brush object.
            // In such case all the render walk caches (textures/edge stores) need to be regenerated.
            if (NWIsContentDirty())
            {
                m_pPageNode->GetDrawingContext()->InvalidateRenderCache();
            }

            IFC(m_pPageNode->GetDrawingContext()->HWRenderContent(pContentRenderer));
        }
    }

    IFC(NotifyRenderContent(HWRenderVisibility::Visible));

Cleanup:
    if (m_pTextDrawingContext)
    {
        m_pTextDrawingContext->ClearForegroundHighlightInfo();
    }

    if (m_pPageNode != nullptr)
    {
        if (m_pPageNode->GetDrawingContext() != nullptr)
        {
            m_pPageNode->GetDrawingContext()->ClearForegroundHighlightInfo();
        }
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Renders post children content in PC rendering walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextBlock::HWPostChildrenRender(
    _In_ IContentRenderer* pContentRenderer
    )
{
    CTextElement *pFocusedElement = GetTextElementForFocusRect();
    if (pFocusedElement)
    {
        IFC_RETURN(CRichTextBlock::HWRenderFocusRects(pContentRenderer, pFocusedElement, m_pTextView));
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns independently animated brushes for rendering with PC.
//      The fill brush is the foreground brush. The stroke brush is the
//      selection brush.
//
//------------------------------------------------------------------------
void CTextBlock::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush
    )
{
    if (m_pTextFormatting->m_pForeground != nullptr &&
        m_pTextFormatting->m_pForeground->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(m_pTextFormatting->m_pForeground));
    }
    if (m_pSelectionManager != nullptr && m_pSelectionManager->GetSelectionBackgroundBrush() != nullptr)
    {
        SetInterface(*ppStrokeBrush, m_pSelectionManager->GetSelectionBackgroundBrush());
    }
}

CFlyoutBase* CTextBlock::GetSelectionFlyoutNoRef() const
{
    CValue result;
    VERIFYHR(GetValueByIndex(KnownPropertyIndex::TextBlock_SelectionFlyout, &result));
    return do_pointer_cast<CFlyoutBase>(result.AsObject());
}

_Check_return_ HRESULT CTextBlock::FireContextMenuOpeningEventSynchronously(_Out_ bool& handled, const wf::Point& point)
{
    handled = false;

    XPOINTF pointerPosition = { point.X, point.Y };

    xref_ptr<ITransformer> spTransformer;
    IFC_RETURN(TransformToRoot(spTransformer.ReleaseAndGetAddressOf()));
    IFC_RETURN(spTransformer->Transform(&pointerPosition, &pointerPosition, 1 /* count */));

    // Convert to DIPS by adjusting for scale
    const float zoomScale = RootScale::GetRasterizationScaleForElement(this);
    pointerPosition /= zoomScale;

    // We cannot use EventManager here due to reentrancy so raising through the peer directly
    IFC_RETURN(FxCallbacks::TextBlock_OnContextMenuOpeningHandler(static_cast<CDependencyObject*>(this), pointerPosition.x, pointerPosition.y, handled));

    return S_OK;
}

_Check_return_ HRESULT CTextBlock::UpdateSelectionFlyoutVisibility()
{
    IFC_RETURN(m_pSelectionManager->UpdateSelectionFlyoutVisibility());
    return S_OK;
}
