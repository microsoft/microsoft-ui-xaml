// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParagraphNode.h"
#include "PageHostedObjectRun.h"
#include "BlockLayoutHelpers.h"

using namespace RichTextServices;
using namespace DirectUI;

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::ParagraphTextSource
//
//---------------------------------------------------------------------------
ParagraphTextSource::ParagraphTextSource(
        _In_opt_ CParagraph *pParagraph,
        _In_ CFrameworkElement *pContentOwner
    ) :
    m_pParagraph(pParagraph),
    m_pContentOwner(pContentOwner),
    m_pParagraphNode(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::~ParagraphTextSource
//
//---------------------------------------------------------------------------
ParagraphTextSource::~ParagraphTextSource()
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::GetTextRun
//
//  Synopsis:
//      Fetches one run of text.
//
//---------------------------------------------------------------------------
Result::Enum ParagraphTextSource::GetTextRun(
    _In_ XUINT32 characterIndex,
    _Outptr_ TextRun **ppTextRun
    )
{
    Result::Enum txhr = Result::Success;
    const WCHAR *pCharacters = NULL;
    const TextFormatting *pFormatting = NULL;
    const InheritedProperties *pInheritedAttachedProperties = NULL;
    XUINT32 cCharacters = 0;
    TextNestingType nestingType = NestedContent;
    TextRunProperties *pTextProperties = NULL;
    TextRun *pTextRun = NULL;
    XUINT32 paragraphLength = 0;
    CTextElement *pNestedElement = NULL;
    DirectionalControl::Enum directionalControl = DirectionalControl::None;
    CFontContext *pFontContext = NULL;
    CFontTypeface *pFontTypeface = NULL;
    CInlineUIContainer *pUIContainer = NULL;
    XUINT32 length = 0;
    CInlineCollection *pInlines = NULL;

    // m_pParagraph can be NULL for a content-free RichTextBlock or for a TextBlock.
    // For the TextBlock case, use the TextBlock's InlineCollection. For empty RichTextBlock, it's correct to
    // calculation paragraph length as 0.
    if (m_pParagraph != NULL)
    {
        m_pParagraph->GetPositionCount(&paragraphLength);
    }
    else if (CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(m_pContentOwner))
    {
        pInlines = pTextBlock->m_pInlines;
        pInlines->GetPositionCount(&paragraphLength);
    }

    if (paragraphLength == 0 ||
        characterIndex == (paragraphLength - 1))
    {
        // Last character in the paragraph counts as </Paragraph> - return EOP run.
        // We can also check for close nesting and Paragraph type for this.
        pTextRun = new EndOfParagraphRun(1, characterIndex);
    }
    else
    {
        if (m_pParagraph != NULL)
        {
            IFC_FROM_HRESULT_RTS(m_pParagraph->GetRun(
                characterIndex,
               &pFormatting,
               &pInheritedAttachedProperties,
               &nestingType,
               &pNestedElement,
               &pCharacters,
               &cCharacters
            ));
        }
        else if (pInlines != NULL)
        {
            IFC_FROM_HRESULT_RTS(pInlines->GetRun(
                characterIndex,
               &pFormatting,
               &pInheritedAttachedProperties,
               &nestingType,
               &pNestedElement,
               &pCharacters,
               &cCharacters
            ));
        }


        if (pCharacters == NULL && cCharacters == 1)
        {
            // If there is a nested element, examine for InlineUIContainer or direction change.
            if (pNestedElement != NULL &&
                (nestingType == OpenNesting ||
                 nestingType == CloseNesting))
            {
                if (pNestedElement->OfTypeByIndex<KnownTypeIndex::InlineUIContainer>())
                {
                    if (nestingType == OpenNesting)
                    {
                        // Object run.
                        IFC_FROM_HRESULT_RTS(DoPointerCast(pUIContainer, pNestedElement));

                        // Create content properties from CFormatting
                        pFontContext = GetFontContext();


                        IFC_FROM_HRESULT_RTS(pFormatting->m_pFontFamily->GetFontTypeface(
                            pFontContext,
                            CWeightStyleStretch(static_cast<XUINT32>(pFormatting->m_nFontWeight),
                                                static_cast<XUINT32>(pFormatting->m_nFontStyle),
                                                static_cast<XUINT32>(pFormatting->m_nFontStretch)),
                            &pFontTypeface));

                        pTextProperties = new TextRunProperties(
                                pFontTypeface,
                                pFormatting->GetScaledFontSize(m_pContentOwner->GetContext()->GetFontScale()),
                                (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Underline)) > 0,
                                (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Strikethrough)) > 0,
                                pFormatting->m_nCharacterSpacing,
                                xref::get_weakref(static_cast<CDependencyObject*>(pNestedElement)),
                                pFormatting->GetResolvedLanguageStringNoRef(),
                                pFormatting->GetResolvedLanguageListStringNoRef(),
                                pInheritedAttachedProperties);

                        pTextRun = new PageHostedObjectRun(
                            pUIContainer,
                            characterIndex,
                            pTextProperties
                        );
                    }
                    else
                    {
                        // No reason to break inside an InlineUIContainer. This code path will just be processed
                        // as a HiddenRun, which may be OK, but no reason this should happen.
                        ASSERT(FALSE);
                    }
                }
                else if (pNestedElement->OfTypeByIndex<KnownTypeIndex::LineBreak>())
                {
                    if (nestingType == OpenNesting)
                    {
                        // LineBreak open is processed as a HiddenRun.
                        pTextRun = new HiddenRun(characterIndex);
                    }
                    else
                    {
                        // LineBreak close is processed as EndOfLine run.
                        pTextRun = new EndOfLineRun(characterIndex);
                    }
                }
                else
                {
                    IFC_FROM_HRESULT_RTS(GetDirectionalControl(
                        pNestedElement,
                        nestingType == CloseNesting ? true : false,
                        &directionalControl));
                }
            }

            if (pTextRun == NULL)
            {
                // No embedded object run was created, so this is either Hidden or DirectionalControl.
                if (directionalControl == DirectionalControl::None)
                {
                    // No directional change, hidden run.
                    pTextRun = new HiddenRun(characterIndex);
                }
                else
                {
                    // DirectionalControl run.
                    pTextRun = new DirectionalControlRun(characterIndex, directionalControl);
                }
            }
        }
        else
        {
            ASSERT(nestingType == NestedContent);
            ASSERT(pCharacters != NULL);

            if (!IsXamlNewline(pCharacters[0]))
            {
                xref::weakref_ptr<CDependencyObject> pBrushSource;
                // New lines don't require properties, but text and control charatcers do.
                // Create properties from CFormatting.
                pFontContext = GetFontContext();

                IFC_FROM_HRESULT_RTS(pFormatting->m_pFontFamily->GetFontTypeface(
                    pFontContext,
                    CWeightStyleStretch(static_cast<XUINT32>(pFormatting->m_nFontWeight),
                                        static_cast<XUINT32>(pFormatting->m_nFontStyle),
                                        static_cast<XUINT32>(pFormatting->m_nFontStretch)),
                    &pFontTypeface));

                pBrushSource = xref::get_weakref(pNestedElement != nullptr ? static_cast<CDependencyObject*>(pNestedElement) :
                    static_cast<CDependencyObject*>(m_pContentOwner));

                pTextProperties = new TextRunProperties(
                    pFontTypeface,
                    pFormatting->GetScaledFontSize(m_pContentOwner->GetContext()->GetFontScale()),
                    (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Underline)) > 0,
                    (static_cast<XUINT32>(pFormatting->m_nTextDecorations) & static_cast<XUINT32>(DirectUI::TextDecorations::Strikethrough)) > 0,
                    pFormatting->m_nCharacterSpacing,
                    std::move(pBrushSource),
                    pFormatting->GetResolvedLanguageStringNoRef(),
                    pFormatting->GetResolvedLanguageListStringNoRef(),
                    pInheritedAttachedProperties);
            }

            // Check for control characters requiring special runs - CR/LF, tab, etc.
            IFC_FROM_HRESULT_RTS(SplitAtFormatControlCharacter(
                characterIndex,
                pCharacters,
                cCharacters,
                pTextProperties,
                &pTextRun));

            // If no control characters detected, process regular TextCharactersRun.
            if (pTextRun == NULL)
            {
                // Limit TextCharactersRun at any format control character.
                while (length < cCharacters &&  !IsFormatControlCharacter(pCharacters[length]))
                {
                    length++;
                }

                pTextRun = new TextCharactersRun(
                        pCharacters,
                        length,
                        characterIndex,
                        pTextProperties);

            }
        }
    }

    // Success. Return results to caller.
    *ppTextRun = pTextRun;
    pTextRun = NULL;

Cleanup:
    ReleaseInterface(pTextProperties);
    ReleaseInterface(pTextRun);
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::GetEmbeddedElementHost
//
//  Synopsis:
//      Get the embedded element host for TextSource in the current
//      formatting context.
//
//---------------------------------------------------------------------------
IEmbeddedElementHost *ParagraphTextSource::GetEmbeddedElementHost() const
{
    return m_pParagraphNode->GetEmbeddedElementHost();
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::SetParagraphNode
//
//  Synopsis:
//      Sets the embedded element host for TextSource in the current
//      formatting context.
//
//---------------------------------------------------------------------------
void ParagraphTextSource::SetParagraphNode(_In_ ParagraphNode *pNode)
{
    m_pParagraphNode = pNode;
}


_Check_return_ HRESULT ParagraphTextSource::IsInSurrogateCRLF(
    _In_ CInlineCollection* inlines,
    _In_ XUINT32 characterIndex,
    _Out_ bool *pIsInSurrogateCRLF
    )
{
    XUINT32 length = 0;
    const WCHAR *pCharacters = nullptr;
    XUINT32 runLength = 0;
    *pIsInSurrogateCRLF  = FALSE;

    inlines->GetPositionCount(&length);
    IFCEXPECT_RETURN(characterIndex <= length);

    IFC_RETURN(inlines->GetRun(characterIndex, nullptr, nullptr, nullptr, nullptr, &pCharacters, &runLength));
    if (pCharacters &&
        runLength > 0)
    {
        if (IS_TRAILING_SURROGATE(pCharacters[0]))
        {
            *pIsInSurrogateCRLF = TRUE;
        }
        else if (pCharacters[0] == UNICODE_LINE_FEED && characterIndex > 0)
        {
            // There's a LF here.  Go back one character and look for CR.
            const WCHAR *pLookForCR = NULL;
            XUINT32 cLookForCR = 0;

            IFC_RETURN(inlines->GetRun(characterIndex - 1, nullptr, nullptr, nullptr, nullptr, &pLookForCR, &cLookForCR));
            if (cLookForCR > 0
                && pLookForCR != NULL
                && pLookForCR[0] == UNICODE_CARRIAGE_RETURN)
            {
                // We are in the middle of a CR+LF sequence.
                *pIsInSurrogateCRLF = TRUE;
            }
        }
    }
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::IsInSurrogateCRLF
//
//  Synopsis:
//      Checks if a position is within a surrogate pair or CRLF sequence.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphTextSource::IsInSurrogateCRLF(
    _In_ XUINT32 characterIndex,
    _Out_ bool *pIsInSurrogateCRLF
    ) const
{
    CInlineCollection *pInlines = nullptr;
    *pIsInSurrogateCRLF  = FALSE;

    // m_pParagraph can be NULL for a content-free RichTextBlock or for a TextBlock.
    // For the TextBlock case, use the TextBlock's InlineCollection. For empty RichTextBlock, inlines should be considered NULL for this logic.
    if (m_pParagraph != nullptr)
    {
        pInlines = m_pParagraph->m_pInlines;
    }
    else if (CTextBlock *pTextBlock = do_pointer_cast<CTextBlock>(m_pContentOwner))
    {
        pInlines = pTextBlock->m_pInlines;
    }

    if (pInlines != nullptr)
    {
        IFC_RETURN(ParagraphTextSource::IsInSurrogateCRLF(pInlines, characterIndex, pIsInSurrogateCRLF));
    }
    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::SplitAtFormatControlCharacter
//
//  Synopsis:
//      Checks for formatting control characters requiring special runs:
//      CR/LF/Tab and splits the content, creating a run for the control
//      character.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphTextSource::SplitAtFormatControlCharacter(
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run.
    _In_reads_(length) const WCHAR *pCharacters,
        // Character buffer containing the run's character data.
    _In_ XUINT32 length,
        // Number of characters in the run.
    _In_opt_ TextRunProperties *pTextProperties,
    _Outptr_ TextRun **ppTextRun
        // Content terminating run created if terminating characters are found.
    )
{
    HRESULT hr = S_OK;
    TextRun *pTextRun = NULL;

    if (IsFormatControlCharacter(pCharacters[0]))
    {
        if (IsXamlNewline(pCharacters[0]))
        {
            // Change the run type to end of line or end of paragraph
            // Check for CR+LF - the only two character end of line case
            if (length > 1 &&
                pCharacters[0] == UNICODE_CARRIAGE_RETURN &&
                pCharacters[1] == UNICODE_LINE_FEED)
            {
                // Special case. Return the initial 'CR' as a hidden character. The following 'LF' will
                // get picked up on the next call by the  else clause beow and returned as an end of line.
                pTextRun = new HiddenRun(characterIndex);
            }
            else if (pCharacters[0] == UNICODE_PARAGRAPH_SEPARATOR)
            {
                // Treat EOP as EOL when it's encountered as a character in text.
                // Only EOPs we should process as end of paragraph are at Paragraph element end
                // and content end for TextBlock/TextBox.
                pTextRun = new EndOfLineRun(characterIndex);
            }
            else
            {
                pTextRun = new EndOfLineRun(characterIndex);
            }
        }
        else if (pCharacters[0] == UNICODE_CHARACTER_TABULATION)
        {
            pTextRun = new TextCharactersRun(pCharacters, 1, characterIndex, pTextProperties);
        }
        else
        {
            // No other format control currently supported.
            ASSERT(FALSE);
        }
    }

    *ppTextRun = pTextRun;
    pTextRun = NULL;

    RRETURN(hr);//RRETURN_REMOVAL
}


//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::GetDirectionalControl
//
//  Synopsis:
//      Gets DirectionalControl change caused by a TextElement if it has a flow direction different than
//      its parent TextElement.
//
//---------------------------------------------------------------------------
_Check_return_ HRESULT ParagraphTextSource::GetDirectionalControl(
    _In_ CTextElement* pTextElement,
        // A pointer to the TextElement object.
    _In_ bool isCloseNesting,
        // Flag indicating whether the element is closing, i.e. whether directional control
        // should begin or end.
    _Out_ DirectionalControl::Enum *pControl
        // Receives the DirectionalControl::Enum value indicating any directional
        // control change caused by this element.
    )
{
    *pControl = DirectionalControl::None;

    // If FlowDirection property has not been set on the element, there is no directional control change.
    // Only Run can accept FlowDirection property. Limit the check just to Runs.
    if (pTextElement->OfTypeByIndex<KnownTypeIndex::Run>() &&
        !pTextElement->IsPropertyDefaultByIndex(KnownPropertyIndex::Run_FlowDirection))
    {
        DirectUI::FlowDirection parentFlowDirection;
        CTextElement *pParentTextElement = do_pointer_cast<CTextElement>(pTextElement->GetParentInternal(false));

        IFC_RETURN(pTextElement->EnsureTextFormattingForRead());

        if (!pParentTextElement)
        {
            CTextElementCollection *pParentTextElementCollection = do_pointer_cast<CTextElementCollection>(pTextElement->GetParentInternal(false));
            pParentTextElementCollection = do_pointer_cast<CTextElementCollection>(pTextElement->GetParentInternal(false));
            if (pParentTextElementCollection)
            {
                pParentTextElement = do_pointer_cast<CTextElement>(pParentTextElementCollection->GetParentInternal(false));
            }
        }

        if (pParentTextElement)
        {
            IFC_RETURN(pParentTextElement->EnsureTextFormattingForRead());
            parentFlowDirection = pParentTextElement->m_pTextFormatting->m_nFlowDirection;
        }
        else
        {
            // No parent TextElement found. Per OM, TextElements must be parented by other TextElements or TextElementCollections.
            // This must be the top-level TextElement, so use the owning control's FlowDirection to compare.
            parentFlowDirection = m_pContentOwner->IsRightToLeft() ? DirectUI::FlowDirection::RightToLeft : DirectUI::FlowDirection::LeftToRight;
        }

        if (parentFlowDirection != pTextElement->m_pTextFormatting->m_nFlowDirection)
        {
            if (isCloseNesting)
            {
                *pControl = DirectionalControl::PopDirectionalFormatting;
            }
            else if (pTextElement->m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::LeftToRight)
            {
                *pControl = DirectionalControl::LeftToRightEmbedding;
            }
            else
            {
                *pControl = DirectionalControl::RightToLeftEmbedding;
            }
        }
        else
        {
            if (pTextElement->m_pTextFormatting->m_nFlowDirection == DirectUI::FlowDirection::LeftToRight)
            {
                *pControl = DirectionalControl::LeftToRightMark;
            }
            else
            {
                *pControl = DirectionalControl::RightToLeftMark;
            }
        }
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      ParagraphTextSource::GetFontContext
//
//---------------------------------------------------------------------------
CFontContext* ParagraphTextSource::GetFontContext()
{
    return BlockLayoutHelpers::GetFontContext(m_pContentOwner);
}
