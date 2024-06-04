// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "BlockLayoutHelpers.h"
#include <MetadataAPI.h>

using namespace DirectUI;
using namespace RichTextServices;

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextFormatter
//
// Synopsis:
//      Gets the TextFormatter instance from the layout engine owner.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetTextFormatter(
    _In_ CDependencyObject *pLayoutOwner,
    _Outptr_ RichTextServices::TextFormatter **ppTextFormatter
)
{
    TextFormatter *pTextFormatter = NULL;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        IFC_RETURN(pRichTextBlock->GetTextFormatter(&pTextFormatter));
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        IFC_RETURN(pTextBlock->GetTextFormatter(&pTextFormatter));
    }
    else
    {
        ASSERT(FALSE);
    }

    *ppTextFormatter = pTextFormatter;
    pTextFormatter = NULL;

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::ReleaseTextFormatter
//
// Synopsis:
//      Releases the TextFormatter instance.
//
//---------------------------------------------------------------------------
void BlockLayoutHelpers::ReleaseTextFormatter(
    _In_ CDependencyObject *pLayoutOwner,
    _In_ RichTextServices::TextFormatter *pTextFormatter
    )
{
    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        pRichTextBlock->ReleaseTextFormatter(pTextFormatter);
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        pTextBlock->ReleaseTextFormatter(pTextFormatter);
    }
    else
    {
        ASSERT(FALSE);
    }
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetParagraphProperties
//
// Synopsis:
//      Resolves paragraph properties from paragraph owner and control owner.
//      Both are needed because certain properties e.g. TextWrapping
//      are not available on CParagraph. Both owners will be the same for
//      non-paragraph content e.g. TextBlock.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetParagraphProperties(
    _In_opt_ CDependencyObject *pParagraph,
    _In_ CDependencyObject *pLayoutOwner,
    _Outptr_ TextParagraphProperties **ppTextParagraphProperties
)
{
    HRESULT                    hr                        = S_OK;
    TextFormatting const      *pFormatting               = NULL;
    InheritedProperties const *pInheritedProperties      = NULL;
    CFontContext              *pFontContext              = NULL;
    CFontTypeface             *pFontTypeface             = NULL;
    TextRunProperties         *pDefaultTextRunProperties = NULL;
    TextParagraphProperties   *pTextParagraphProperties  = NULL;
    DirectUI::TextAlignment    textAlignment;
    CDependencyObject         *pEffectiveParagraph       = pParagraph;

    ASSERT(pLayoutOwner);
    ASSERT(ppTextParagraphProperties);

    if (pEffectiveParagraph == NULL)
    {
        // Use the layout owner as the effective paragraph.
        pEffectiveParagraph = pLayoutOwner;
    }

    // Since GetFormatting and GetInheritedAttachedProperties expect one "paragraph" element, use
    // the effective paragraph. GetTextAlignment expects both paragraph and layout owner and will handle the
    // NULL paragraph case, since it is public and called from other places in ParagraphNode. So use original values.
    pFontContext = GetFontContext(pLayoutOwner);
    IFC(GetFormatting(pEffectiveParagraph, &pFormatting));
    IFC(GetInheritedAttachedProperties(pEffectiveParagraph, &pInheritedProperties));
    IFC(GetTextAlignment(pParagraph, pLayoutOwner, &textAlignment));

    IFC(pFormatting->m_pFontFamily->GetFontTypeface(
        pFontContext,
        CFontFaceCriteria(
            static_cast<XUINT32>(pFormatting->m_nFontWeight),
            static_cast<XUINT32>(pFormatting->m_nFontStyle),
            static_cast<XUINT32>(pFormatting->m_nFontStretch),
            pFormatting->m_eFontSize),
        &pFontTypeface
    ));

    pDefaultTextRunProperties = new TextRunProperties(
        pFontTypeface,
        pFormatting->GetScaledFontSize(pLayoutOwner->GetContext()->GetFontScale()),
        (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Underline)) > 0,
        (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Strikethrough)) > 0,
        pFormatting->m_nCharacterSpacing,
        xref::get_weakref(pEffectiveParagraph),
        pFormatting->GetResolvedLanguageStringNoRef(),
        pFormatting->GetResolvedLanguageListStringNoRef(),
        pInheritedProperties
    );

    pTextParagraphProperties = new TextParagraphProperties(
        GetFlowDirection(pLayoutOwner),
        pDefaultTextRunProperties,
        GetParagraphIndent(pParagraph, pLayoutOwner),
        GetTextWrapping(pLayoutOwner),
        GetTextLineBounds(pLayoutOwner),
        GetTextAlignment(pLayoutOwner)
    );

    pTextParagraphProperties->SetFlags(TextParagraphProperties::Flags::Justify, textAlignment == DirectUI::TextAlignment::Justify);
    pTextParagraphProperties->SetFlags(TextParagraphProperties::Flags::TrimSideBearings, GetOpticalMarginAlignment(pLayoutOwner) == DirectUI::OpticalMarginAlignment::TrimSideBearings);
    pTextParagraphProperties->SetFlags(TextParagraphProperties::Flags::DetermineTextReadingOrderFromContent, GetTextReadingOrder(pLayoutOwner) == DirectUI::TextReadingOrder::DetectFromContent);
    pTextParagraphProperties->SetFlags(TextParagraphProperties::Flags::DetermineAlignmentFromContent, textAlignment == DirectUI::TextAlignment::DetectFromContent);

    *ppTextParagraphProperties = pTextParagraphProperties;
    pTextParagraphProperties = NULL;

Cleanup:
    ReleaseInterface(pDefaultTextRunProperties);
    ReleaseInterface(pTextParagraphProperties);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::CreateCollapsingSymbol
//
// Synopsis:
//      Resolves collapsing properties from paragraph owner and control owner,
//      and creates collapsing symbol.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::CreateCollapsingSymbol(
    _In_ CDependencyObject *pParagraph,
    _In_ CDependencyObject *pLayoutOwner,
    _Outptr_ RichTextServices::TextCollapsingSymbol **ppCollapsingSymbol
    )
{
    HRESULT hr = S_OK;
    TextFormatting const *pTextFormatting = NULL;
    InheritedProperties const *pInheritedProperties = NULL;
    CFontContext *pFontContext;
    CFontTypeface *pFontTypeface;
    TextRunProperties *pDefaultTextRunProperties = NULL;
    TextCollapsingCharacters *pCollapsingCharacters = NULL;
    IFssFontFace *pFontFace = NULL;
    WCHAR *pCollapsingChar = NULL;
    XFLOAT *pCollapsingWidths = NULL;
    XFLOAT collapsingCharWidth = 0.0f;
    CDependencyObject *pEffectiveParagraph = pParagraph;

    ASSERT(pLayoutOwner);

    if (pEffectiveParagraph == NULL)
    {
        // Use the layout owner as the effective paragraph.
        pEffectiveParagraph = pLayoutOwner;
    }

    pCollapsingChar = new WCHAR(UNICODE_ELLIPSIS);

    pFontContext = GetFontContext(pLayoutOwner);
    IFC(GetFormatting(pEffectiveParagraph, &pTextFormatting));
    IFC(GetInheritedAttachedProperties(pEffectiveParagraph, &pInheritedProperties));

    IFC(pTextFormatting->m_pFontFamily->GetFontTypeface(
        pFontContext,
        CFontFaceCriteria(
            static_cast<XUINT32>(pTextFormatting->m_nFontWeight),
            static_cast<XUINT32>(pTextFormatting->m_nFontStyle),
            static_cast<XUINT32>(pTextFormatting->m_nFontStretch),
            pTextFormatting->m_eFontSize),
        &pFontTypeface
        ));

    pDefaultTextRunProperties = new TextRunProperties(
        pFontTypeface,
        pTextFormatting->GetScaledFontSize(pLayoutOwner->GetContext()->GetFontScale()),
        (static_cast<XUINT32>(pTextFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Underline)) > 0,
        (static_cast<XUINT32>(pTextFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Strikethrough)) > 0,
        pTextFormatting->m_nCharacterSpacing,
        xref::get_weakref(pEffectiveParagraph),
        pTextFormatting->GetResolvedLanguageStringNoRef(),
        pTextFormatting->GetResolvedLanguageListStringNoRef(),
        pInheritedProperties
        );

    XUINT32 mappedLength;
    XFLOAT mappedScale;
    IFC(pFontTypeface->MapCharacters(
            pCollapsingChar,
            1,
            pTextFormatting->GetResolvedLanguageStringNoRef().GetBuffer(),
            pTextFormatting->GetResolvedLanguageListStringNoRef().GetBuffer(),
            NULL,
            &pFontFace,
            &mappedLength,
            &mappedScale));

    ASSERT(mappedLength == 1);
    IFC(ComputeCollapsingCharacterWidth(pFontFace, pCollapsingChar, pDefaultTextRunProperties, &collapsingCharWidth))

    pCollapsingWidths = new XFLOAT(collapsingCharWidth);
    pCollapsingCharacters = new TextCollapsingCharacters(
        pCollapsingChar,
        1,
        pCollapsingWidths,
        collapsingCharWidth,
        RichTextServices::FlowDirection::LeftToRight,
        pDefaultTextRunProperties,
        pFontFace);

    *ppCollapsingSymbol = pCollapsingCharacters;
    pCollapsingChar = NULL;
    pCollapsingWidths = NULL;
    pCollapsingCharacters = NULL;

Cleanup:
    delete pCollapsingChar;
    delete pCollapsingWidths;
    ReleaseInterface(pCollapsingCharacters);
    ReleaseInterface(pDefaultTextRunProperties);
    ReleaseInterface(pFontFace);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextAlignment
//
// Synopsis:
//      Reads TextAlignment value from paragraph/control.
//      TextAlignment is not inherited, so if it's not set on the paragraph,
//      we have to read from the control anyway.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetTextAlignment(
    _In_ CDependencyObject *pParagraph,
    _In_ CDependencyObject *pLayoutOwner,
    _Out_ DirectUI::TextAlignment *pTextAlignment
    )
{
    const CDependencyProperty *pdp = NULL;
    CValue value;
    DirectUI::TextAlignment textAlignment = DirectUI::TextAlignment::Left;
    bool paragraphValueSet = false;

    ASSERT(pLayoutOwner);

    // NULL paragraph may be passed in for TextBlock or empty RichTextBlock scenario. In that case look up this value from layout owner directly.
    if (NULL != pParagraph)
    {
        ASSERT(pParagraph->OfTypeByIndex<KnownTypeIndex::Paragraph>());
        CParagraph *pParagraphLocal = static_cast<CParagraph *>(pParagraph);
        pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Block_TextAlignment);
        if (pParagraphLocal->HasLocalOrModifierValue(pdp))
        {
            // Paragraph has a local value for this property, so use that.
            IFC_RETURN(pParagraphLocal->GetValue(pdp, &value));
            IFCEXPECT_RETURN(value.IsEnum());
            textAlignment = static_cast<DirectUI::TextAlignment>(value.AsEnum());
            paragraphValueSet = TRUE;
        }
    }

    if (!paragraphValueSet)
    {
        if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
            textAlignment = pRichTextBlock->m_textAlignment;
        }
        else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
            textAlignment = pTextBlock->m_textAlignment;
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    *pTextAlignment = textAlignment;

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextTrimming
//
// Synopsis:
//      Reads TextTrimming value from control owner. This property is
//      not exposed at paragraph level.
//
//---------------------------------------------------------------------------
TextTrimming BlockLayoutHelpers::GetTextTrimming(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    DirectUI::TextTrimming textTrimming = DirectUI::TextTrimming::None;
    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        textTrimming = pRichTextBlock->m_textTrimming;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        textTrimming = pTextBlock->m_textTrimming;
    }
    else
    {
        ASSERT(FALSE);
    }

    return textTrimming;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::ShowParagraphEllipsisOnPage
//
// Synopsis:
//      Indicates whether paragraph ellipsis should be displayed on a
//      particular page.
//
//  Notes:
//      1. There are no checks for TextTrimming here. If TextTrimming
//         is necessary for paragraph ellipsis to be shown callers should read
//         the value separately.
//      2. Check for ParagraphEllipsis is necessary because the page owner may
//         want text to be trimmed if it overflows horizontally but not if it
//         doesn't fit vertically - the page may handle this in other ways
//         e.g. by breaking.
//
//  TODO: Remove this - this should be an option passed to
//  Arrange.
//
//---------------------------------------------------------------------------
bool BlockLayoutHelpers::ShowParagraphEllipsisOnPage(
    _In_ CDependencyObject *pPageOwner
    )
{
    bool showParagraphEllipsis = false;
    ASSERT(pPageOwner);

    if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        // RichTextBlock will only show paragraph ellipsis if it has no overflow
        // target, otherwise, overflow will go to the target.
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pPageOwner);
        showParagraphEllipsis = (pRichTextBlock->m_pOverflowTarget == NULL);
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        // RichTextBlockOverflow will only show paragraph ellipsis if it has no overflow
        // target, otherwise, overflow will go to the target.
        CRichTextBlockOverflow *pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pPageOwner);
        showParagraphEllipsis = (pRichTextBlockOverflow->m_pOverflowTarget == NULL);
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        // TextBlock will always show paragraph ellipsis if trimming is on.
        showParagraphEllipsis = TRUE;
    }
    else
    {
        ASSERT(FALSE);
    }

    return showParagraphEllipsis;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetPagePadding
//
// Synopsis:
//      Gets the Padding value from the page owner.
//
//---------------------------------------------------------------------------
void BlockLayoutHelpers::GetPagePadding(
    _In_ CDependencyObject *pPageOwner,
    _Out_ XTHICKNESS *pPadding
)
{
    XTHICKNESS padding = {0.0f, 0.0f, 0.0f, 0.0f};

    ASSERT(pPageOwner);

    if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pPageOwner);
        padding = pRichTextBlock->m_padding;
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        CRichTextBlockOverflow *pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pPageOwner);
        padding = pRichTextBlockOverflow->m_padding;
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pPageOwner);
        padding = pTextBlock->m_padding;
    }
    else
    {
        ASSERT(FALSE);
    }

    *pPadding = padding;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetLayoutRoundingHeightAdjustment
//
// Synopsis:
//      Gets the height adjustment value from the page owner.
//
//---------------------------------------------------------------------------

void BlockLayoutHelpers::GetLayoutRoundingHeightAdjustment(
    _In_ CDependencyObject *pPageOwner,
    _Out_ XFLOAT *pLayoutRoundingHeightAdjustment
    )
{
    FLOAT layoutRoundingHeightAdjustment = 0.0f;

    ASSERT(pPageOwner);

    if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pPageOwner);
        layoutRoundingHeightAdjustment = pRichTextBlock->m_layoutRoundingHeightAdjustment;
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        CRichTextBlockOverflow *pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pPageOwner);
        layoutRoundingHeightAdjustment = pRichTextBlockOverflow->m_layoutRoundingHeightAdjustment;
    }
    else if (pPageOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pPageOwner);
        layoutRoundingHeightAdjustment = pTextBlock->m_layoutRoundingHeightAdjustment;
    }
    else
    {
        ASSERT(FALSE);
    }

    *pLayoutRoundingHeightAdjustment = layoutRoundingHeightAdjustment;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetBlockMargin
//
// Synopsis:
//      Gets the Margin value from the block element.
//
//---------------------------------------------------------------------------
void BlockLayoutHelpers::GetBlockMargin(
    _In_opt_ CDependencyObject *pBlock,
    _Out_ XTHICKNESS *pMargin
)
{
    XTHICKNESS margin = {0.0f, 0.0f, 0.0f, 0.0f};

    // NULL pBlock may be passed in for TextBlock or empty RichTextBlock scenario.
    if (pBlock != NULL &&
        pBlock->OfTypeByIndex<KnownTypeIndex::Block>())
    {
        CBlock *pBlockLocal = static_cast<CBlock *>(pBlock);
        margin = pBlockLocal->m_margin;
    }

    *pMargin = margin;
}

//------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetElementBaseline
//
//  Synopsis:
//      Retrieves the baseline offset of a given UIElement.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetElementBaseline(
    _In_ CUIElement *pElement,
    _Out_ XFLOAT *pBaseline)
{
    ASSERT(pElement);

    if (pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pElement);
        IFC_RETURN(pRichTextBlock->GetBaselineOffset(pBaseline));
    }
    else if (pElement->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>())
    {
        CRichTextBlockOverflow *pRichTextBlockOverflow = static_cast<CRichTextBlockOverflow *>(pElement);
        IFC_RETURN(pRichTextBlockOverflow->GetBaselineOffset(pBaseline));
    }
    else if (pElement->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pElement);
        IFC_RETURN(pTextBlock->GetBaselineOffset(pBaseline));
    }
    else
    {
        *pBaseline = pElement->DesiredSize.height;
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetLineStackingInfo
//
// Synopsis:
//      Reads LineHeight/ LineStacking info value from paragraph/control.
//      These properties are not inherited, so if not set on the paragraph,
//      we have to read from the control anyway.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetLineStackingInfo(
    _In_ CDependencyObject *pParagraph,
    _In_ CDependencyObject *pLayoutOwner,
    _Out_ LineStackingStrategy *pLineStackingStrategy,
    _Out_ XFLOAT *pDefaultFontBaseline,
    _Out_ XFLOAT *pDefaultFontLineAdvance,
    _Out_ XFLOAT *pLineHeight
    )
{
    CFontContext *pFontContext = NULL;
    TextFormatting const *pTextFormatting = NULL;
    const CDependencyProperty *pdp = NULL;
    CValue value;
    bool paragraphLineHeightSet = false;
    bool paragraphLineStackingStrategySet = false;
    CDependencyObject *pEffectiveParagraph = pParagraph;

    ASSERT(pLayoutOwner);

    if (pEffectiveParagraph == NULL)
    {
        // Use the layout owner as the effective paragraph.
        pEffectiveParagraph = pLayoutOwner;
    }

    // Formatting can always be read from the paragraph, and font context from the control.
    pFontContext = GetFontContext(pLayoutOwner);
    IFC_RETURN(GetFormatting(pEffectiveParagraph, &pTextFormatting));
    IFC_RETURN(pTextFormatting->m_pFontFamily->GetTextLineBoundsMetrics(pFontContext, GetTextLineBounds(pLayoutOwner), pDefaultFontBaseline, pDefaultFontLineAdvance));

    // LineHeight and LineStackingStrategy must be read from the paragraph if set locally, the control otherwise.
    // NULL paragraph may be passed in for TextBlock or empty RichTextBlock scenario. In that case look up this value from layout owner directly.
    if (NULL != pParagraph)
    {
        ASSERT(pParagraph->OfTypeByIndex<KnownTypeIndex::Paragraph>());
        CParagraph *pParagraphLocal = static_cast<CParagraph *>(pParagraph);

        pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Block_LineHeight);
        if (pParagraphLocal->HasLocalOrModifierValue(pdp))
        {
            // Paragraph has a local value for this property, so use that.
            IFC_RETURN(pParagraphLocal->GetValue(pdp, &value));
            IFCEXPECT_RETURN(value.GetType() == valueFloat);
            *pLineHeight = value.AsFloat();
            paragraphLineHeightSet = TRUE;
        }

        pdp = MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Block_LineStackingStrategy);
        if (pParagraph->HasLocalOrModifierValue(pdp))
        {
            IFC_RETURN(pParagraphLocal->GetValue(pdp, &value));
            IFCEXPECT_RETURN(value.IsEnum());
            *pLineStackingStrategy = static_cast<LineStackingStrategy>(value.AsEnum());
            paragraphLineStackingStrategySet = TRUE;
        }
    }

    // If values are not set on paragraph, read them from the control. CRichTextBlock is the only
    // control where this is currently possible.
    if (!(paragraphLineStackingStrategySet) ||
        !(paragraphLineHeightSet))
    {
        if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
            if (!paragraphLineStackingStrategySet)
            {
                *pLineStackingStrategy = pRichTextBlock->m_lineStackingStrategy;
            }
            if (!paragraphLineHeightSet)
            {
                *pLineHeight = pRichTextBlock->m_eLineHeight;
            }
        }
        else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
        {
            CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
            if (!paragraphLineStackingStrategySet)
            {
                *pLineStackingStrategy = pTextBlock->m_lineStackingStrategy;
            }
            if (!paragraphLineHeightSet)
            {
                *pLineHeight = pTextBlock->m_eLineHeight;
            }
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetFontContext
//
// Synopsis:
//      Rerieves font context value from control owner. Font context is
//      not available at paragraph level.
//
//---------------------------------------------------------------------------
CFontContext * BlockLayoutHelpers::GetFontContext(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    CFontContext *pFontContext = NULL;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        pFontContext = pRichTextBlock->GetFontContext();
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        pFontContext = pTextBlock->GetFontContext();
    }
    else
    {
        ASSERT(FALSE);
    }

    return pFontContext;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetFormatting
//
// Synopsis:
//      Reads formatting properties from paragraph owner. All properties in
//      CFormatting are inherited by TextElement so they are always available
//      at the paragraph level.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetFormatting(
    _In_        CDependencyObject     *pParagraph,
    _Outptr_ TextFormatting const **ppFormatting
)
{
    ASSERT(pParagraph);
    ASSERT(pParagraph->OfTypeByIndex<KnownTypeIndex::Paragraph>() ||
        pParagraph->OfTypeByIndex<KnownTypeIndex::TextBlock>() ||
        pParagraph->OfTypeByIndex<KnownTypeIndex::RichTextBlock>());

    IFC_RETURN(pParagraph->GetTextFormatting(ppFormatting));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetInheritedAttachedProperties
//
// Synopsis:
//      Reads formatting properties from paragraph owner. All properties in
//      CFormatting are inherited by TextElement so they are always available
//      at the paragraph level.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT BlockLayoutHelpers::GetInheritedAttachedProperties(
    _In_        CDependencyObject          *pParagraph,
    _Outptr_ InheritedProperties const **ppInheritedAttachedProperties
)
{
    ASSERT(pParagraph);
    ASSERT(pParagraph->OfTypeByIndex<KnownTypeIndex::Paragraph>() ||
        pParagraph->OfTypeByIndex<KnownTypeIndex::TextBlock>() ||
        pParagraph->OfTypeByIndex<KnownTypeIndex::RichTextBlock>());

    IFC_RETURN(pParagraph->GetInheritedProperties(ppInheritedAttachedProperties));

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetFlowDirection
//
//---------------------------------------------------------------------------
RichTextServices::FlowDirection::Enum BlockLayoutHelpers::GetFlowDirection(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    RichTextServices::FlowDirection::Enum flowDirection = RichTextServices::FlowDirection::LeftToRight;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        ASSERT(pRichTextBlock->EnsureTextFormattingForRead() == S_OK);
        if (pRichTextBlock->m_pTextFormatting != NULL)
        {
            flowDirection = static_cast<RichTextServices::FlowDirection::Enum>(
                pRichTextBlock->m_pTextFormatting->m_nFlowDirection
            );
        }
        else
        {
            flowDirection = RichTextServices::FlowDirection::LeftToRight;
        }
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        ASSERT(pTextBlock->EnsureTextFormattingForRead() == S_OK);
        if (pTextBlock->m_pTextFormatting != NULL)
        {
            flowDirection = static_cast<RichTextServices::FlowDirection::Enum>(
                pTextBlock->m_pTextFormatting->m_nFlowDirection
            );
        }
        else
        {
            flowDirection = RichTextServices::FlowDirection::LeftToRight;
        }
    }
    else
    {
        ASSERT(FALSE);
    }

    return flowDirection;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextWrapping
//
//---------------------------------------------------------------------------
TextWrapping BlockLayoutHelpers::GetTextWrapping(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    DirectUI::TextWrapping textWrapping = DirectUI::TextWrapping::NoWrap;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        textWrapping = pRichTextBlock->m_textWrapping;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        textWrapping = pTextBlock->m_textWrapping;
    }
    else
    {
        ASSERT(FALSE);
    }

    return textWrapping;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetParagraphIndent
//
//---------------------------------------------------------------------------
XFLOAT BlockLayoutHelpers::GetParagraphIndent(
    _In_opt_ CDependencyObject *pParagraph,
    _In_ CDependencyObject *pLayoutOwner
    )
{
    XFLOAT indent = 0.0f;
    bool paragraphValueSet = false;

    ASSERT(pLayoutOwner);

    // NULL paragraph may be passed in for TextBlock or empty RichTextBlock scenario. In that case look up this value from layout owner directly.
    if (pParagraph != NULL)
    {
        ASSERT(pParagraph->OfTypeByIndex<KnownTypeIndex::Paragraph>());
        CParagraph *pParagraphLocal = static_cast<CParagraph *>(pParagraph);
        if (pParagraphLocal->HasLocalOrModifierValue(MetadataAPI::GetDependencyPropertyByIndex(KnownPropertyIndex::Paragraph_TextIndent)))
        {
            // Paragraph has a local value for this property, so use that.
            indent = pParagraphLocal->m_textIndent;
            paragraphValueSet = TRUE;
        }
    }

    if (!paragraphValueSet)
    {
        // TextBlock does not support TextiNDENT
        if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
        {
            CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
            indent = pRichTextBlock->m_textIndent;
        }
    }

    return indent;
}


//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetIsColorFontEnabled
//
//---------------------------------------------------------------------------
bool BlockLayoutHelpers::GetIsColorFontEnabled(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    bool isColorFontEnabled = false;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        isColorFontEnabled = pRichTextBlock->m_isColorFontEnabled;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        isColorFontEnabled = pTextBlock->m_isColorFontEnabled;
    }
    else
    {
        ASSERT(FALSE);
    }

    return isColorFontEnabled;
}


_Check_return_ HRESULT BlockLayoutHelpers::ComputeCollapsingCharacterWidth(
    _In_  IFssFontFace                *pFontFace,
    _In_  WCHAR                       *pCollapsingChar,
    _In_  TextRunProperties           *pTextRunProperties,
    _Out_ XFLOAT                      *pCollapsingCharWidth
    )
{
    XUINT16               uGlyphId        = 0;
    XFLOAT                eAdvance        = 0.0f;
    XUINT32               character       = *pCollapsingChar;
    FssGlyphMetrics       glyphMetrics;
    FssFontMetrics        fssFontMetrics;

    if (pFontFace)
    {
        // Determine the nominal advance width of the collapsing character

        IFC_RETURN(pFontFace->GetGlyphIndices(&character, 1, &uGlyphId));
        IFC_RETURN(pFontFace->GetDesignGlyphMetrics(&uGlyphId, 1, &glyphMetrics, FALSE /*IsSideways*/));


        // Convert the advance width from size independent font units to
        // 96ths of an inch at the specified font size.

        pFontFace->GetMetrics(&fssFontMetrics);

        eAdvance = ((XFLOAT)(glyphMetrics.AdvanceWidth) * pTextRunProperties->GetFontSize())
            / ((XFLOAT)(fssFontMetrics.DesignUnitsPerEm));
    }

    // All done. Return the advance width of the collapsing character.
    *pCollapsingCharWidth = eAdvance;

    return S_OK;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetOpticalMarginAlignment
//
// Synopsis:
//      Reads OpticalMarginAlignment value from control owner.
//
//---------------------------------------------------------------------------
OpticalMarginAlignment BlockLayoutHelpers::GetOpticalMarginAlignment(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    DirectUI::OpticalMarginAlignment opticalMarginAlignment = DirectUI::OpticalMarginAlignment::None;
    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        opticalMarginAlignment = pRichTextBlock->m_opticalMarginAlignment;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        opticalMarginAlignment = pTextBlock->m_opticalMarginAlignment;
    }
    else
    {
        ASSERT(FALSE);
    }

    return opticalMarginAlignment;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextLineBounds
//
//---------------------------------------------------------------------------
TextLineBounds BlockLayoutHelpers::GetTextLineBounds(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    DirectUI::TextLineBounds textLineBounds = DirectUI::TextLineBounds::Full;

    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        textLineBounds = pRichTextBlock->m_textLineBounds;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        textLineBounds = pTextBlock->m_textLineBounds;
    }
    else
    {
        ASSERT(FALSE);
    }

    return textLineBounds;
}

//---------------------------------------------------------------------------
//
// BlockLayoutHelpers::GetTextReadingOrder
//
// Synopsis:
//      Reads TextReadingOrder value from control owner.
//
//---------------------------------------------------------------------------
TextReadingOrder BlockLayoutHelpers::GetTextReadingOrder(
    _In_ CDependencyObject *pLayoutOwner
    )
{
    DirectUI::TextReadingOrder textReadingOrder = DirectUI::TextReadingOrder::Default;
    ASSERT(pLayoutOwner);

    if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        CRichTextBlock *pRichTextBlock = static_cast<CRichTextBlock *>(pLayoutOwner);
        textReadingOrder = pRichTextBlock->m_textReadingOrder;
    }
    else if (pLayoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        CTextBlock *pTextBlock = static_cast<CTextBlock *>(pLayoutOwner);
        textReadingOrder = pTextBlock->m_textReadingOrder;
    }
    else
    {
        ASSERT(FALSE);
    }

    return textReadingOrder;
}

DirectUI::TextAlignment BlockLayoutHelpers::GetTextAlignment(_In_ CDependencyObject* layoutOwner)
{
    DirectUI::TextAlignment textAlignment = DirectUI::TextAlignment::Left;

    if (layoutOwner->OfTypeByIndex<KnownTypeIndex::RichTextBlock>())
    {
        textAlignment = static_cast<CRichTextBlock*>(layoutOwner)->m_textAlignment;
    }
    else if (layoutOwner->OfTypeByIndex<KnownTypeIndex::TextBlock>())
    {
        textAlignment = static_cast<CTextBlock*>(layoutOwner)->m_textAlignment;
    }
    else
    {
        ASSERT(FALSE);
    }

    return textAlignment;
}
