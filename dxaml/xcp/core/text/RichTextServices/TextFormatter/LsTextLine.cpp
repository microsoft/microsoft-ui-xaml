// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsTextLine.h"
#include "LsTextFormatter.h"
#include "LsTextLineBreak.h"
#include "LsRun.h"
#include "LsSpan.h"
#include "TextStore.h"
#include "TxMath.h"
#include "TextDpi.h"

using namespace DirectUI;
using namespace Ptls6;
using namespace RichTextServices;
using namespace RichTextServices::Internal;

const LSLINERESTR s_lineRestrictions = {
    0,      // Left area reserved for obstacles
    0,      // Right area reserved for obstacles
    FALSE   // Hyphenation disallowed for this line
};

const LSDEVINFO s_deviceInfo = {
    TRUE,       // preview device and reference device are the same
                // LSDEVRES
    TRUE,
    {
        TEXTDPI_DPI, // resolution of preview device
        TEXTDPI_DPI, // resolution of preview device
        TEXTDPI_DPI, // resolution of reference device
        TEXTDPI_DPI  // resolution of reference device
    }
};

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::Create
//
//  Synopsis:
//      Creates a new instance of the LsTextLine class.
//
//  Remarks:
//      The formattingWidth parameter is a hard constraint that the line cannot
//      exceed. At formattingWidth, the line must break.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::Create(
    _In_ XUINT32 startIndex,
        // Line's start index.
    _In_ XFLOAT formattingWidth,
        // Maximum allowed width during formatting.
    _In_opt_ LsTextLineBreak *pPreviousLineBreak,
        // Break record for previous line's formatting.
    _In_ LsTextFormatter *pTextFormatter,
        // TextFormatter that initiates the Create request.
    _Outptr_  LsTextLine **ppTextLine
        // Pointer to address of line created/
    )
{
    Result::Enum txhr = Result::Success;
    LsTextLine *pLine = NULL;

    IFC_EXPECT_RTS(pTextFormatter);
    pLine = new LsTextLine(pTextFormatter);

    IFC_OOM_RTS(pLine);
    IFCTEXT(pLine->Format(startIndex, formattingWidth, pPreviousLineBreak));

    *ppTextLine = pLine;
    pLine = NULL;

Cleanup:
    ReleaseInterface(pLine);
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::LsTextLine
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsTextLine::LsTextLine(
    _In_ LsTextFormatter *pTextFormatter
        // TextFormatter that initiates the Create request.
    )
{
    m_width = 0;
    m_widthIncludingTrailingWhitespace = 0;
    m_start = 0;
    m_height = 0;
    m_textHeight = 0;
    m_baseline = 0;
    m_textBaseline = 0;
    m_overhangLeading = 0;
    m_overhangTrailing = 0;
    m_length = 0;
    m_trailingWhitespaceLength = 0;
    m_newlineLength = 0;
    m_dependentLength = 0;
    m_pTextLineBreak = NULL;
    m_pLsLine = NULL;
    m_startIndex = 0;
    m_depthQueryMax = 0;
    m_formattingWidth = 0;
    m_viewportWidth = 0;
    m_flags = 0;
    m_pCollapsingSymbol = NULL;
    m_pPreviousLineBreak = NULL;

    m_pTextFormatter = pTextFormatter;
    m_pTextSource = pTextFormatter->m_lsHostContext.pTextSource;
    m_pTextParagraphProperties = pTextFormatter->m_lsHostContext.pParagraphProperties;
    m_pTextStore = pTextFormatter->m_lsHostContext.pTextStore;

    // Compare text reading order determined by TextStore to the FlowDirection set on the Block.
    // The flag will indicate during Block::ArrangeCore() to flip TextAlignment to preserve the layout
    // set by FlowDirection and TextAlignment on the BlockNode.
    m_paraFlowDirection = m_pTextStore->GetParagraphFlowDirection();
    m_detectedParaFlowDirection = m_pTextStore->GetDetectedParagraphFlowDirection();
    m_alignmentFollowsReadingOrder = m_pTextStore->GetParagraphFlowDirection() == m_pTextParagraphProperties->GetFlowDirection();

    m_pTextFormatter->AddRef();
    m_pTextParagraphProperties->AddRef();
    m_pTextStore->AddRef();
    m_pTextSource->AddRef();
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::~LsTextLine
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
LsTextLine::~LsTextLine()
{
    ReleaseInterface(m_pPreviousLineBreak);
    ReleaseInterface(m_pTextLineBreak);
    ReleaseInterface(m_pCollapsingSymbol);
    if (m_pLsLine != NULL)
    {
        LsDestroyLine(m_pTextFormatter->m_pLsContext, m_pLsLine);
    }
    m_pTextParagraphProperties->Release();
    m_pTextStore->Release();
    m_pTextSource->Release();
    m_pTextFormatter->Release();
}


//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::Arrange
//
//  Synopsis:
//      Arranges the content of the line.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::Arrange(
    _In_ const XRECTF &bounds
        // Bounds of the line within its container.
    )
{
    Result::Enum txhr = Result::Success;
    XUINT32 runPosition = 0;
    LsRun *pLsRun;
    LSQSUBINFO* pSublineInfo = NULL;
    XUINT32 partialRunOffset;

    pLsRun = m_pTextStore->GetRunCache()->GetLsRun(m_startIndex, NULL);
    partialRunOffset = m_startIndex - pLsRun->GetCharacterIndex();
    while (pLsRun != NULL && runPosition < GetLength())
    {
        const TextRun *pTextRun = pLsRun->GetTextRun();
        if (pTextRun != NULL && pTextRun->GetType() == TextRunType::Object)
        {
            LSTEXTCELL lsTextCell = {0};
            LONG sublineCount = 0;
            XPOINTF objectPosition;
            ObjectRun *pObjectRun = const_cast<ObjectRun*>(static_cast<const ObjectRun*>(pTextRun));

            IFC_OOM_RTS(pSublineInfo = new LSQSUBINFO[m_depthQueryMax]);
            IFCTEXT(ResultFromLSErr(LsQueryLineCpPpoint(
                m_pLsLine,
                runPosition + m_startIndex,
                m_depthQueryMax,
                pSublineInfo,
                &sublineCount,
                &lsTextCell)));

            // Only empty lines have no sub-lines.
            ASSERT(sublineCount > 0);

            // When querying for information about inline objects, LS will provide Object related metrics.
            // Use those to calculate the position of the object.
            // Since pointUvStartObj.v refers to the line top, need to use ascent information to
            // calculate actual position of the Object relative to the baseline.
            objectPosition.x = bounds.X + TextDpi::FromTextDpi(
                pSublineInfo[sublineCount-1].pointUvStartObj.u);
            objectPosition.y = bounds.Y + TextDpi::FromTextDpi(
                pSublineInfo[sublineCount-1].pointUvStartSubline.v +
                pSublineInfo[sublineCount-1].heightsPresSubline.dvAscent -
                pSublineInfo[sublineCount-1].heightsPresObj.dvAscent);

            // LS reports the position of the Object in coordinates space of the last subline,
            // which might be different than the main line. If there are sublines and the flow direction
            // is different, extract the width of the object to get the top-left in lines coordinate space.
            if (sublineCount > 1 && pSublineInfo[0].lstflowSubline != pSublineInfo[sublineCount-1].lstflowSubline)
            {
                objectPosition.x -= TextDpi::FromTextDpi(pSublineInfo[sublineCount-1].dupObj);
            }
            // TH2 bug 5038422: Inline images overlap text in RichTextBlock in right-to-left languages.
            // When FlowDirection is not equal to detected content reading order,
            // we flipped the line alignment in ParagraphNode::CalculateLineOffset and updated the line offset,
            // And this line offset was used to place the inline element in LsTextLine::Arrange.
            // Later when the line is being drawn, another offset is being pushed in LsTextLine::Draw() content that is detected with RTL reading direction.
            // This offset is only applied on the text (D2DTextDraingContext only draws the text),
            // not the inline element(UIElement that is being rendered during the noral render walk.), so the inline offset is misplaced.
            // We should adjust the inline element's offset, when the content reading order is not equal to the flow direction.
            if (!m_alignmentFollowsReadingOrder)
            {
                objectPosition.x = bounds.X + bounds.Width - objectPosition.x - TextDpi::FromTextDpi(pSublineInfo[sublineCount - 1].dupObj);

                // RS5 Bug #17528972:  The above fix for bug 5038422 doesn't account for the TextAlignment property, which shifts the entire line around.
                // The tactical fix for RS5 is to handle right alignment, by detecting this and shifting the InlineUIContainer by the appropriate amount.
                // Note that when TextAlignment == DetectFromContent, this effectively works out to Right in both LTR and RTL scenarios, which may
                // seem counter-intuitive.  To spell out the two cases:
                // 1) If the text is LTR, FlowDirection = RTL, the computed text alignment is Right.  This renders left-aligned on screen.
                // 2) If the text is RTL, FlowDirection = LTR, the computed text alignment is also Right.  This renders right-aligned on screen.
                if (m_pTextParagraphProperties->GetTextAlignment() == DirectUI::TextAlignment::Right ||
                    m_pTextParagraphProperties->GetTextAlignment() == DirectUI::TextAlignment::DetectFromContent)
                {
                    objectPosition.x += (m_formattingWidth - bounds.Width);
                }
            }

            pObjectRun->Arrange(objectPosition);

            delete [] pSublineInfo;
            pSublineInfo = NULL;
        }

        runPosition += pLsRun->GetLength() - partialRunOffset;
        partialRunOffset = 0;
        pLsRun = pLsRun->GetNext();
    }

Cleanup:
    delete [] pSublineInfo;
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::Draw
//
//  Synopsis:
//      Creates rendering data for the line's contents.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::Draw(
    _In_ TextDrawingContext *pDrawingContext,
        // The TextDrawingContext object onto which the TextLine is drawn.
    _In_ const XPOINTF &origin,
        // A point value that represents the drawing origin.
    _In_ XFLOAT viewportWidth
        // A value that represents the viewport width available for rendering.
    )
{
    Result::Enum txhr = Result::Success;
    LSPOINT lsLineOrigin;
    XFLOAT originX;
    XFLOAT originY;
    LSRECT lsClipRect = { XINT32_MIN, XINT32_MIN, XINT32_MAX, XINT32_MAX };
    FlowDirection::Enum flowDirection = m_pTextStore->GetParagraphFlowDirection();

    IFC_EXPECT_RTS(pDrawingContext);

    m_pTextFormatter->m_lsHostContext.pDrawingContext = pDrawingContext;
    m_viewportWidth = viewportWidth;

    if (flowDirection == FlowDirection::RightToLeft)
    {
        // Adjust text offset by the viewport constraint.
        originX = m_viewportWidth - origin.x;
    }
    else
    {
        originX = origin.x;
    }
    originY = origin.y + m_baseline;

    lsLineOrigin.x = TextDpi::ToTextDpi(originX);
    lsLineOrigin.y = TextDpi::ToTextDpi(originY);

    IFCTEXT(ResultFromLSErr(LsDisplayLine(m_pLsLine, &lsLineOrigin, 1, &lsClipRect)));

    if (GetFlags(Flags::Collapsed) && (m_pCollapsingSymbol != NULL))
    {
        XPOINTF collapsingSymbolOrigin;
        collapsingSymbolOrigin.x = origin.x + m_width - m_pCollapsingSymbol->GetWidth();
        collapsingSymbolOrigin.y = originY;

        IFCTEXT(m_pCollapsingSymbol->Draw(
            pDrawingContext,
            collapsingSymbolOrigin,
            viewportWidth,
            flowDirection));
    }

Cleanup:
    m_pTextFormatter->m_lsHostContext.pDrawingContext = NULL;

    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      TextLine::Collapse
//
//  Synopsis:
//      Creates a new TextLine shortened to the specified constraining width.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::Collapse(
    _In_ XFLOAT collapsingWidth,
        // Width of collapsed line.
    _In_ TextTrimming collapsingStyle,
        // Collapsing style - trailing character or word.
    _In_opt_ TextCollapsingSymbol *pCollapsingSymbol,
        // Collapsing symbol to be displayed.
    _Outptr_ TextLine **ppCollapsedLine
        // Collapsed TextLine.
    )
{
    Result::Enum txhr = Result::Success;
    LsTextLine *pCollapsedLine = NULL;
    *ppCollapsedLine = NULL;

    // A line can only be collapsed after it has been formatted normally first.
    ASSERT_EXPECT_RTS(m_pLsLine != NULL);

    // Set up formatter host context.
    m_pTextFormatter->m_lsHostContext.pTextSource = m_pTextSource;
    m_pTextFormatter->m_lsHostContext.pParagraphProperties = m_pTextParagraphProperties;
    m_pTextFormatter->m_lsHostContext.pTextStore = m_pTextStore;
    m_pTextFormatter->m_lsHostContext.hasMultiCharacterClusters = FALSE;
    m_pTextFormatter->m_lsHostContext.lineStartIndex = m_startIndex;
    m_pTextFormatter->m_lsHostContext.collapsedLine = TRUE;
    m_pTextFormatter->m_lsHostContext.useEmergencyBreaking = (collapsingStyle == DirectUI::TextTrimming::CharacterEllipsis);
    m_pTextFormatter->m_lsHostContext.clipLastWordOnLine = (collapsingStyle == DirectUI::TextTrimming::Clip);

    pCollapsedLine = new LsTextLine(m_pTextFormatter);
    IFC_OOM_RTS(pCollapsedLine);
    IFCTEXT(pCollapsedLine->FormatCollapsed(this, m_startIndex, collapsingWidth, pCollapsingSymbol, m_pPreviousLineBreak));
    *ppCollapsedLine = pCollapsedLine;
    pCollapsedLine = NULL;

Cleanup:
    ReleaseInterface(pCollapsedLine);
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      TextLine::HasCollapsed
//
//  Synopsis:
//      Returns TRUE if the visual width of this line will be smaller than its logical width,
//              FALSE otherwise.
//
//-----------------------------------------------------------------------
bool LsTextLine::HasCollapsed() const
{
    return GetFlags(Flags::Collapsed);
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::HasMultiCharacterClusters
//
//  Synopsis:
//      Returns FALSE if the caller can rely on all unicode code points
//      in the line mapping to independent glyphs runs.  I.e., glyph
//      clusters always map to single characters, never runs of combined
//      characters such as surrogate pairs or base characters with
//      combining marks.
//
//  Notes:
//      This method is useful for callers who may be able to avoid
//      reformatting a line in the future if it has not complex content.
//      In particular, caret navigation can be implemented without a line
//      format by caching this value.
//
//-----------------------------------------------------------------------
bool LsTextLine::HasMultiCharacterClusters() const
{
    return GetFlags(Flags::HasMultiCharacterClusters);
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetCharacterHitFromDistance
//
//  Synopsis:
//      Gets the character hit corresponding to the specified distance
//      from the beginning of the line.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::GetCharacterHitFromDistance(
    _In_ XFLOAT distance,
        // A value that represents the distance from the beginning of the line.
    _Out_ CharacterHit *pCharacterHit
        // The CharacterHit object at the specified distance from the beginning of the line.
    ) const
{
    // TODO: add logic to check collapsed area when collapsing is enabled.
    Result::Enum txhr = Result::Success;
    ASSERT(pCharacterHit != NULL);
    pCharacterHit->firstCharacterIndex = m_startIndex;
    pCharacterHit->trailingLength = 0;
    LSPOINTUV queryPoint = {0, 0};
    LSTEXTCELL lsTextCell = {0};
    PLSQSUBINFO pSublineInfo = NULL;
    LONG sublineCount;
    bool match = false;

    XINT32 hitTestDistance = TextDpi::ToTextDpi(distance);
    IFC_OOM_RTS(pSublineInfo = new LSQSUBINFO[m_depthQueryMax]);
    queryPoint.u = hitTestDistance;

    IFCTEXT(ResultFromLSErr(LsQueryLinePointPcp(m_pLsLine,
                                        &queryPoint,
                                         m_depthQueryMax,
                                         pSublineInfo,
                                         &sublineCount,
                                         &lsTextCell
                                         )));

    if (lsTextCell.cpEndCell < lsTextCell.cpStartCell)
    {
        // Workaround for LS bug 1005 in cases of hyphenated words.
        lsTextCell.cpEndCell = lsTextCell.cpStartCell;
    }

    if (sublineCount > 0 && lsTextCell.dupCell > 0)
    {
        bool stopAtEveryCharacter = true;
        XUINT32 caretStopCount = lsTextCell.cpEndCell + 1 - lsTextCell.cpStartCell;
        XUINT32 codepointsToNextCaretStop = 1;

        // Determine how many caret stops are in the cell.
        IFCTEXT(StopAtEachCharacterInCell(pSublineInfo, sublineCount, lsTextCell, m_pTextFormatter->m_lsHostContext.pFontAndScriptServices, &stopAtEveryCharacter));

        if (!stopAtEveryCharacter)
        {
            // This is a non divisible cluster, stop only at cell boundary.
            codepointsToNextCaretStop = caretStopCount;
            caretStopCount = 1;
        }

        XUINT32 direction = (pSublineInfo[sublineCount - 1].lstflowSubline == pSublineInfo[0].lstflowSubline) ? 1 : -1;
        XUINT32 wholeAdvance = lsTextCell.dupCell / caretStopCount;
        XUINT32 remainingAdvance = lsTextCell.dupCell % caretStopCount;
        hitTestDistance = (hitTestDistance - lsTextCell.pointUvStartCell.u) * direction;

        for (XUINT32 i = 0; i < caretStopCount; i++)
        {
            XINT32 caretAdvance = wholeAdvance;
            if (remainingAdvance > 0)
            {
                caretAdvance++;
                remainingAdvance--;
            }

            if (hitTestDistance <= caretAdvance)
            {
                match = true;
                pCharacterHit->firstCharacterIndex = lsTextCell.cpStartCell + i;
                if (hitTestDistance > caretAdvance / 2)
                {
                    // Hittest at the trailing edge of the current caret stop
                     pCharacterHit->trailingLength = codepointsToNextCaretStop;
                }
                else
                {
                    // Hittest at the leading edge of the current caret stop
                     pCharacterHit->trailingLength = 0;
                }
                break;
            }
            hitTestDistance -= caretAdvance;
        }

       if (!match)
       {
           // hittest beyond the last caret stop, return the trailing edge of the last caret stop
           pCharacterHit->firstCharacterIndex = lsTextCell.cpStartCell + caretStopCount - 1;
           pCharacterHit->trailingLength = codepointsToNextCaretStop;
       }
    }

Cleanup:
    delete [] pSublineInfo;
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetDistanceFromCharacterHit
//
//  Synopsis:
//      Gets the distance from the beginning of the line to the specified
//      character hit.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::GetDistanceFromCharacterHit(
    _In_ const CharacterHit &characterHit,
        // The CharacterHit object whose distance you want to query.
    _Out_ XFLOAT *pDistance
        // A value that represents the distance from the beginning of the line.
    ) const
{
    ASSERT(pDistance != NULL);
    Result::Enum txhr = Result::Success;
    XINT32 hitTestDistance = 0;
    *pDistance = 0;
    LONG sublineCount;
    LSTEXTCELL lsTextCell = {0};
    LSQSUBINFO* pSublineInfo = NULL;

    if (static_cast<XUINT32>(characterHit.firstCharacterIndex) >= (m_startIndex + m_length))
    {
        // Returning line width for character hit beyond the last caret stop
        *pDistance = m_widthIncludingTrailingWhitespace;
    }
    else
    {
        IFC_OOM_RTS(pSublineInfo = new LSQSUBINFO[m_depthQueryMax]);

        // TODO: take collapsing into account.
        XINT32 lscpCurrent = characterHit.firstCharacterIndex;

        IFCTEXT(ResultFromLSErr(LsQueryLineCpPpoint(m_pLsLine,
                                               lscpCurrent,
                                               m_depthQueryMax,
                                               pSublineInfo,
                                              &sublineCount,
                                              &lsTextCell
                                              )));

        if (lsTextCell.cpEndCell < lsTextCell.cpStartCell)
        {
            // Workaround for LS bug 1005 in cases of hyphenated words.
            lsTextCell.cpEndCell = lsTextCell.cpStartCell;
        }

        if (sublineCount > 0)
        {
            IFCTEXT(GetDistanceInsideTextCell(lscpCurrent - lsTextCell.cpStartCell,
                                          characterHit.trailingLength != 0,
                                          pSublineInfo,
                                          sublineCount,
                                          lsTextCell,
                                         &hitTestDistance
                                         ));

            hitTestDistance += lsTextCell.pointUvStartCell.u;
            *pDistance = TextDpi::FromTextDpi(hitTestDistance);
        }
    }

Cleanup:
    delete [] pSublineInfo;
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetPreviousCaretCharacterHit
//
//  Synopsis:
//      Gets the previous character hit for caret navigation.
//
//  Notes:
//      Returns the leading edge of the nearest preceding visible character.
//
//      If there is no previous character, the return value is exactly
//      equal to characterHit.
//
//      Collapsed lines are not supported.  Returns Result::InvalidOperation when
//      HasCollapsed() is TRUE.
//
//      On input, characterHit.trailingLength is treated as a flag.
//      If 0, characterHit references the leading edge of the indicated
//      character cluster, otherwise it references the trailing edge.
//
//      On output, pPreviousCharacterHit->trailingLength always points
//      exactly at the trailing edge of the returned cluster.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::GetPreviousCaretCharacterHit(
    _In_ const CharacterHit &characterHit,
    _Out_ CharacterHit *pPreviousCharacterHit) const
{
    Result::Enum txhr = Result::Success;

    if (NULL == pPreviousCharacterHit)
    {
        IFCTEXT(Result::InvalidParameter);
    }
    if (GetFlags(Flags::Collapsed))
    {
        IFCTEXT(Result::InvalidOperation);
    }

    // Default the out value for the case where there is no previous caret position.
    *pPreviousCharacterHit = characterHit;

    // Find the caret positions surrounding the leading edge of the input char.
    XINT32 leftCaretIndex;
    XINT32 offsetToNextCaretIndex;
    bool foundCaretIndex;
    IFCTEXT(GetClosestCaretIndex(characterHit.firstCharacterIndex, &leftCaretIndex, &offsetToNextCaretIndex, &foundCaretIndex));

    // If we started on a leading edge, move to the leading edge of the preceding char, if any.
    // We only do this if there's space left on the line and the input position is exactly at
    // a caret stop.
    if (foundCaretIndex &&
        0 == characterHit.trailingLength &&
        m_startIndex < static_cast<XUINT32>(leftCaretIndex) &&
        leftCaretIndex == characterHit.firstCharacterIndex)
    {
        IFCTEXT(GetClosestCaretIndex(characterHit.firstCharacterIndex - 1, &leftCaretIndex, &offsetToNextCaretIndex, &foundCaretIndex));
    }

    if (foundCaretIndex)
    {
        // The output position is always on the leading edge of the character we just moved over.
        pPreviousCharacterHit->firstCharacterIndex = leftCaretIndex;
        pPreviousCharacterHit->trailingLength = 0;
    }

Cleanup:
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetNextCaretCharacterHit
//
//  Synopsis:
//      Gets the next character hit for caret navigation.
//
//  Notes:
//      Returns the trailing edge of the nearest following visible character.
//
//      If there is no next character, the return value is exactly
//      equal to characterHit.
//
//      Collapsed lines are not supported.  Returns Result::InvalidOperation when
//      HasCollapsed() is TRUE.
//
//      On input, characterHit.trailingLength is treated as a flag.
//      If 0, characterHit references the leading edge of the indicated
//      character cluster, otherwise it references the trailing edge.
//
//      On output, pNextCharacterHit->trailingLength always points
//      exactly at the trailing edge of the returned cluster.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::GetNextCaretCharacterHit(
    _In_ const CharacterHit &characterHit,
    _Out_ CharacterHit *pNextCharacterHit) const
{
    Result::Enum txhr = Result::Success;

    if (NULL == pNextCharacterHit)
    {
        IFCTEXT(Result::InvalidParameter);
    }
    if (GetFlags(Flags::Collapsed))
    {
        IFCTEXT(Result::InvalidOperation);
    }

    // Default the out value for the case where there is no next caret position.
    *pNextCharacterHit = characterHit;

    // Advance to the next visible run of text.
    XINT32 visibleIndex;
    bool foundIndex;
    IFCTEXT(GetNextVisibleIndex(characterHit.firstCharacterIndex, &visibleIndex, &foundIndex));

    if (foundIndex)
    {
        // Find the caret positions surrounding the leading edge of the input char.
        XINT32 leftCaretIndex;
        XINT32 offsetToNextCaretIndex;
        IFCTEXT(GetClosestCaretIndex(visibleIndex, &leftCaretIndex, &offsetToNextCaretIndex, &foundIndex));
        ASSERT(foundIndex);

        // If we started on a trailing edge, move to the trailing edge of the following char, if any.
        // But only do this if there's space left on the line and the input position is exactly at
        // a caret stop.
        if (0 != characterHit.trailingLength &&
            characterHit.firstCharacterIndex + characterHit.trailingLength == leftCaretIndex + offsetToNextCaretIndex)
        {
            IFCTEXT(GetNextVisibleIndex(leftCaretIndex + offsetToNextCaretIndex, &visibleIndex, &foundIndex));

            if (foundIndex)
            {
                IFCTEXT(GetClosestCaretIndex(visibleIndex, &leftCaretIndex, &offsetToNextCaretIndex, &foundIndex));
            }
        }

        if (foundIndex)
        {
            // The output position is always on the trailing edge of the character we just moved over.
            pNextCharacterHit->firstCharacterIndex = leftCaretIndex;
            pNextCharacterHit->trailingLength = offsetToNextCaretIndex;
        }
    }

Cleanup:
    return txhr;
}

//-----------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetTextBounds
//
//  Synopsis:
//      Gets an array of bounding rectangles that represent the range of
//      characters within a text line.
//
//-----------------------------------------------------------------------
Result::Enum LsTextLine::GetTextBounds(
    _In_ XINT32 firstCharacterIndex,
        // A value that represents the index of first character of specified range.
    _In_ XINT32 textLength,
        // A value that represents the number of characters of the specified range.
    _Out_ XUINT32 *pcBounds,
        // The size of array of text bounds.
    _Outptr_result_buffer_(*pcBounds) TextBounds **ppBounds
        // An array of text bounds that represent the range of characters within a text line.
    ) const
{
    ASSERT(pcBounds != NULL);
    ASSERT(ppBounds != NULL);
    ASSERT(textLength != 0);
    ASSERT(firstCharacterIndex >= 0);
    LSQSUBINFO *pLastSublines = NULL;
    LSQSUBINFO *pFirstSublines = NULL;
    XUINT32 cBoundsFromBaseLevel = 0;
    XUINT32 cBoundsToBaseLevel = 0;
    TextBounds *pBoundsToBaseLevel = NULL;
    TextBounds *pBoundsFromBaseLevel = NULL;
    XINT32 firstCellDistance;
    XINT32 cpFirst = firstCharacterIndex;
    // By default, if the hittested CP is visible, then we want cpFirst to hit
    // on the leading edge of the first visible cp, and cpEnd to hit on the trailing edge of the
    // last visible cp.
    bool isCpFirstTrailing = false;
    bool isCpLastTrailing = true;

    LONG firstDepth;
    LONG lastDepth;
    LSTEXTCELL firstTextCell = {0};
    LSTEXTCELL lastTextCell = {0};

    Result::Enum txhr = Result::Success;

    *pcBounds = 0;
    *ppBounds = NULL;

    // Reverse start and end positions for negative length.
    if (textLength < 0)
    {
        cpFirst += textLength;
        textLength = -textLength;
    }

    if (cpFirst < static_cast<XINT32>(m_startIndex))
    {
        textLength += (cpFirst - m_startIndex);
        cpFirst = static_cast<XINT32>(m_startIndex);
    }

    if (cpFirst > static_cast<XINT32>(m_startIndex + m_length - textLength))
    {
        textLength = static_cast<XINT32>(m_startIndex + m_length - cpFirst);
    }

    XINT32 cpLast = cpFirst + textLength - 1;

    // Get first cp sublines & text cell.
    IFC_OOM_RTS(pFirstSublines = new LSQSUBINFO[m_depthQueryMax]);

    IFCTEXT(ResultFromLSErr(LsQueryLineCpPpoint(m_pLsLine,
                                           cpFirst,
                                           m_depthQueryMax,
                                           pFirstSublines,
                                          &firstDepth,
                                          &firstTextCell
                                           )));

    if (firstDepth <= 0)
    {
        // This happens for empty line (line containing only EOP).
        IFCTEXT(CreateEmptyBounds(pcBounds,
                              ppBounds
                              ));
        goto Cleanup;
    }

    // Get last cp sublines & text cell.
    IFC_OOM_RTS(pLastSublines = new LSQSUBINFO[m_depthQueryMax]);

    IFCTEXT(ResultFromLSErr(LsQueryLineCpPpoint(m_pLsLine,
                                           cpLast,
                                           m_depthQueryMax,
                                           pLastSublines,
                                          &lastDepth,
                                          &lastTextCell
                                           )));

    if(lastDepth <= 0)
    {
        // This should never happen but if it does, we still cant throw here.
        // We must return something even though it's a degenerate bounds or
        // client hittesting code will just crash.
        ASSERT(FALSE);
        IFCTEXT(CreateEmptyBounds(pcBounds,
                              ppBounds
                              ));
        goto Cleanup;
    }

    if (cpFirst > firstTextCell.cpEndCell)
    {
        // When cpFirst is after the last visible cp, then it hits the trailing edge of that cp.
        isCpFirstTrailing = true;
    }

    if (cpLast < lastTextCell.cpStartCell)
    {
        // When cpEnd is before the first visible cp, then it hits the leading edge of that cp.
        isCpLastTrailing = false;
    }

    if (firstDepth == lastDepth && pFirstSublines[firstDepth - 1].cpFirstSubline == pLastSublines[lastDepth - 1].cpFirstSubline)
    {
        // First and last cp are within the same subline.
        TextBounds bounds;
        XINT32 lastCellDistance = 0;

        IFCTEXT(GetDistanceInsideTextCell(cpFirst - firstTextCell.cpStartCell,
                                      isCpFirstTrailing!=0,
                                      pFirstSublines,
                                      firstDepth,
                                      firstTextCell,
                                     &firstCellDistance
                                     ));
        firstCellDistance += firstTextCell.pointUvStartCell.u;

        IFCTEXT(GetDistanceInsideTextCell(cpLast - lastTextCell.cpStartCell,
                                      isCpLastTrailing!=0,
                                      pLastSublines,
                                      lastDepth,
                                      lastTextCell,
                                     &lastCellDistance
                                     ));
        lastCellDistance += lastTextCell.pointUvStartCell.u;

        if (CalculateValidTextBounds(firstCellDistance,
                                     lastCellDistance,
                                     0,
                                     m_height,
                                     pFirstSublines[firstDepth - 1].lstflowSubline,
                                     bounds
                                    ))
        {
            (*pcBounds) = 1;
        }

        if ((*pcBounds) > 0)
        {
            IFC_OOM_RTS(*ppBounds = new TextBounds[1]);
            CopyBounds(bounds, (*ppBounds)[0]);
        }
    }
    else
    {
        // First and last cp are not in the same subline.

        // The hittested cp can be outside of the returned sublines when it is a hidden cp.
        // We should not pass beyond the end of the returned sublines.
        XINT32 cpCurrent = cpFirst;
        XINT32 cpEndInSubline = MIN(cpLast, pLastSublines[lastDepth - 1].cpFirstSubline + pLastSublines[lastDepth - 1].dcpSubline - 1);
        XINT32 currentDistance = 0;
        LONG baseLevelDepth = 0;
        IFCTEXT(GetDistanceInsideTextCell(cpCurrent - firstTextCell.cpStartCell,
                                      isCpFirstTrailing!=0,
                                      pFirstSublines,
                                      firstDepth,
                                      firstTextCell,
                                     &currentDistance
                                     ));
        currentDistance += firstTextCell.pointUvStartCell.u;

        IFCTEXT(CollectTextBoundsToBaseLevel(
                                        &cpCurrent,
                                        &currentDistance,
                                         pFirstSublines,
                                         firstDepth,
                                         cpEndInSubline,
                                        &baseLevelDepth,
                                        &cBoundsToBaseLevel,
                                        &pBoundsToBaseLevel
                                        ));

       if (baseLevelDepth < lastDepth)
       {
           IFCTEXT(CollectTextBoundsFromBaseLevel(
                                              &cpCurrent,
                                              &currentDistance,
                                               pLastSublines,
                                               lastDepth,
                                               baseLevelDepth,
                                              &cBoundsFromBaseLevel,
                                              &pBoundsFromBaseLevel
                                              ));
        }

        (*pcBounds) = cBoundsToBaseLevel + cBoundsFromBaseLevel;

        // Collect the bounds from the start of the immediate enclosing subline of the last LSCP
        // to the hittested text cell.
        XINT32 lastCellDistance = 0;
        TextBounds lastSublineBounds = {0};
        IFCTEXT(GetDistanceInsideTextCell(cpLast - lastTextCell.cpStartCell,
                                      isCpLastTrailing!=0,
                                      pLastSublines,
                                      lastDepth,
                                      lastTextCell,
                                     &lastCellDistance
                                     ));
        lastCellDistance += lastTextCell.pointUvStartCell.u;

        if (CalculateValidTextBounds(currentDistance,
                                     lastCellDistance,
                                     0,
                                     m_height,
                                     pLastSublines[lastDepth - 1].lstflowSubline,
                                     lastSublineBounds
                                    ))
        {
            (*pcBounds)++;
        }

        IFC_OOM_RTS(*ppBounds = new TextBounds[*pcBounds]);

        // Copy text bounds from and to base level depth.
        for (XUINT32 i = 0; i < cBoundsToBaseLevel; i++)
        {
            CopyBounds(pBoundsToBaseLevel[i], (*ppBounds)[i]);
        }

        for (XUINT32 i = 0; i < cBoundsFromBaseLevel; i++)
        {
            CopyBounds(pBoundsFromBaseLevel[i], (*ppBounds)[i + cBoundsToBaseLevel]);
        }

        if (cBoundsToBaseLevel + cBoundsFromBaseLevel < (*pcBounds))
        {
            ASSERT(((*pcBounds) - (cBoundsToBaseLevel + cBoundsFromBaseLevel)) == 1);
            CopyBounds(lastSublineBounds, (*ppBounds)[(*pcBounds) - 1]);
        }
    }

    if ((*pcBounds) == 0)
    {

        // No non-zerowidth bounds detected, fallback to the position of first cp
        // This can happen if hidden run is hittest'd.

        // Clear any memory allocated for bounds.
        delete [] (*ppBounds);
        IFC_OOM_RTS(*ppBounds = new TextBounds[1]);
        (*pcBounds) = 1;

        IFCTEXT(GetDistanceInsideTextCell(cpFirst - firstTextCell.cpStartCell,
                                      isCpFirstTrailing!=0,
                                      pFirstSublines,
                                      firstDepth,
                                      firstTextCell,
                                     &firstCellDistance
                                     ));
        firstCellDistance += firstTextCell.pointUvStartCell.u;

        // Create 0-width bounds at the first cp.
        (*ppBounds)[0].rect.X = TextDpi::FromTextDpi(firstCellDistance);
        (*ppBounds)[0].rect.Width = 0.0f;
        (*ppBounds)[0].rect.Y = 0.0f;
        (*ppBounds)[0].rect.Height = m_height;
        (*ppBounds)[0].flowDirection = (pFirstSublines[firstDepth - 1].lstflowSubline == lstflowWS
                                        || pFirstSublines[firstDepth - 1].lstflowSubline == lstflowWN)
                                        ? FlowDirection::RightToLeft
                                        : FlowDirection::LeftToRight;
    }

Cleanup:
   delete [] pFirstSublines;
   delete [] pLastSublines;
   delete [] pBoundsToBaseLevel;
   delete [] pBoundsFromBaseLevel;
   pBoundsToBaseLevel = NULL;
   pBoundsFromBaseLevel = NULL;
   return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::Format
//
//  Synopsis:
//      Formats the line and stores formatting results.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::Format(
    _In_ XUINT32 startIndex,
        // Start index.
    _In_ XFLOAT formattingWidth,
        // Maximum allowed width during formatting.
    _In_opt_ LsTextLineBreak *pPreviousLineBreak
        // Line breaking information for the previous line, if any exists.
    )
{
    Result::Enum txhr = Result::Success;
    PLSBREAKRECLINE pLsPreviousLineBreak = NULL;
    PLSBREAKRECLINE pLsLineBreak;
    LSPAP lspap;

    SetFlags(Flags::Collapsed, FALSE);
    m_startIndex = startIndex;
    m_formattingWidth = formattingWidth;

    ASSERT(m_pPreviousLineBreak == NULL);
    ASSERT(m_pLsLine == NULL);
    if (pPreviousLineBreak)
    {
        m_pPreviousLineBreak = pPreviousLineBreak;
        AddRefInterface(m_pPreviousLineBreak);
        pLsPreviousLineBreak = pPreviousLineBreak->GetLsBreakRecord();
    }

    ASSERT(!m_pTextFormatter->m_lsHostContext.hasMultiCharacterClusters);

    SetupLsPap(m_pTextFormatter, m_formattingWidth, &lspap);

    IFCTEXT(ResultFromLSErr(LsCreateLine(m_pTextFormatter->m_pLsContext,
                                    reinterpret_cast<PLSPARACLIENT>(this),
                                    &lspap,
                                    m_startIndex,
                                   &s_lineRestrictions,
                                    pLsPreviousLineBreak,
                                   &pLsLineBreak,
                                   &m_lsLineInfo,
                                   &m_pLsLine
                                   )));

    // If the parent block's TextWrapping is set to WrapWholeWords
    // we call LsCreateLine again with fForceBreakAsNext = true.
    if (m_pTextFormatter->m_lsHostContext.pParagraphProperties->GetTextWrapping() == DirectUI::TextWrapping::WrapWholeWords
        && m_lsLineInfo.fForcedBreak)
    {
        lspap.fForceBreakAsNext = fTrue;

        if (m_pLsLine != NULL)
        {
            IFCTEXT(ResultFromLSErr(LsDestroyLine(m_pTextFormatter->m_pLsContext, m_pLsLine)));
            m_pLsLine = NULL;
        }
        if (pLsLineBreak != NULL)
        {
            IFCTEXT(ResultFromLSErr(LsDestroyBreakRecord(m_pTextFormatter->m_pLsContext, pLsLineBreak)));
            pLsLineBreak = NULL;
        }

        IFCTEXT(ResultFromLSErr(LsCreateLine(m_pTextFormatter->m_pLsContext,
                                    reinterpret_cast<PLSPARACLIENT>(this),
                                    &lspap,
                                    m_startIndex,
                                   &s_lineRestrictions,
                                    pLsPreviousLineBreak,
                                   &pLsLineBreak,
                                   &m_lsLineInfo,
                                   &m_pLsLine
                                   )));
    }

    SetFlags(Flags::HasMultiCharacterClusters, m_pTextFormatter->m_lsHostContext.hasMultiCharacterClusters);

    IFCTEXT(ResultFromLSErr(LsQueryLineMaxDepth(m_pLsLine, reinterpret_cast<LONG *>(&m_depthQueryMax))));

    // Get line metrics.
    IFCTEXT(CalculateLineMetrics());

    IFCTEXT(CreateLineBreak(pLsLineBreak));

Cleanup:
    if (txhr != Result::Success)
    {
        if (m_pLsLine != NULL)
        {
            LsDestroyLine(m_pTextFormatter->m_pLsContext, m_pLsLine);
            m_pLsLine = NULL;
        }
        if (pLsLineBreak != NULL)
        {
            LsDestroyBreakRecord(m_pTextFormatter->m_pLsContext, pLsLineBreak);
        }
    }
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::SetupLsPap
//
//  Synopsis:
//      Sets relevant flags on LSPAP structure based on formatter context.
//
//---------------------------------------------------------------------------
void LsTextLine::SetupLsPap(
    _In_ LsTextFormatter *pTextFormatter,
    _In_ XFLOAT paragraphWidth,
    _Out_ LSPAP *pLsPap
    )
{
    LsSpan *pMasterSpan = NULL;
    TextParagraphProperties *pTextParagraphProperties = NULL;

    // Setup paragraph properties.
    memset(pLsPap, 0, sizeof(LSPAP));

    // Get the master span.
    pMasterSpan = pTextFormatter->m_lsHostContext.pTextStore->GetMasterSpan();
    pTextParagraphProperties = pTextFormatter->m_lsHostContext.pParagraphProperties;

    pLsPap->cpFirst                 = pMasterSpan->GetCharacterIndex();
    pLsPap->dcpTagMasterSpan        = pMasterSpan->GetLength();
    pLsPap->lsspq                   = reinterpret_cast<LSSPANQUALIFIER>(pMasterSpan);
    pLsPap->lskeop                  = lskeopEndPara1;

    // Para flow direction is ES/WS depending on the paragraph's reading order which
    // is determined by TextStore->Initialize().  We only support LTR and RTL reading order.
    if (pTextFormatter->m_lsHostContext.pTextStore->GetParagraphFlowDirection() == FlowDirection::RightToLeft)
    {
        pLsPap->lstflow = lstflowWS;
    }
    else
    {
        pLsPap->lstflow = lstflowES;
    }

    // Set flag fFmiTreatHyphenAsRegular to make Hyphen follow line breaking class table like
    // regular characters. If the flag is not set, LS will consider Hyphen to have direct break
    // opp before and after which is not always desirable, e.g. space after hyphen may be put
    // to the start of next line.
    pLsPap->fTreatHyphenAsRegular = 1;
    pLsPap->fSpacesInfluenceHeight  = 1;   // Runs containing just spaces affect line height
    pLsPap->fIgnoreHeigntOfEOL = 1;        // Spaces at the end of line due to wrapping do not affect line height

    if (pTextParagraphProperties->GetTextWrapping() != DirectUI::TextWrapping::NoWrap)
    {
        pLsPap->fApplyBreakingRules = 1;
        pLsPap->fJustify = pTextParagraphProperties->GetFlags(TextParagraphProperties::Flags::Justify);
    }

    // If line is collapsed, apply breaking rules.
    if (pTextFormatter->m_lsHostContext.collapsedLine)
    {
        pLsPap->fApplyBreakingRules = 1;
    }

    pLsPap->durExtraLeftIndentFirstLine = TextDpi::ToTextDpi(pTextParagraphProperties->GetFirstLineIndent());
    pLsPap->lsdevinfo = s_deviceInfo;
    pLsPap->durColumn = TextDpi::ToTextDpi(paragraphWidth);

    // set OpticalAlignment flag
    if (pTextParagraphProperties->GetFlags(TextParagraphProperties::Flags::TrimSideBearings))
    {
        pLsPap->fApplyOpticalAlignment = 1;
    }
}


//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::DetermineTrailingCharacterSpacing
//
//  Synopsis:
//      Returns the amount of extra whitespace at the end of line due to
//      the CharacterSpacing property.
//
//---------------------------------------------------------------------------
void LsTextLine::DetermineTrailingCharacterSpacing(
    _Out_ XFLOAT *pTrailingCharacterSpacing
)
{
    XUINT32                        lastRunEndOffset     = 0;
    XFLOAT                         characterSpacing     = 0.0f;
    LsRun                         *pLastSpaceableRun    = NULL;

    lastRunEndOffset     = m_startIndex + m_length - 1;

    pLastSpaceableRun = m_pTextStore->GetRunCache()->GetLsRun(lastRunEndOffset, NULL);

    if (pLastSpaceableRun != NULL)
    {
        // Skip backwards to the last visible text or embedded object run

        while (    pLastSpaceableRun != NULL
               &&  pLastSpaceableRun->GetTextRun()->GetType() != TextRunType::Text
               &&  pLastSpaceableRun->GetTextRun()->GetType() != TextRunType::Object)
        {
            if (pLastSpaceableRun->GetTextRun()->GetLength() > lastRunEndOffset)
            {
                pLastSpaceableRun = NULL;
            }
            else
            {
                lastRunEndOffset -= pLastSpaceableRun->GetTextRun()->GetLength();
                pLastSpaceableRun = m_pTextStore->GetRunCache()->GetLsRun(lastRunEndOffset, NULL);
            }
        }

        if (pLastSpaceableRun != NULL  &&  pLastSpaceableRun->GetSpaceLastCharacter())
        {
            TextRunProperties *pProperties = NULL;

            if (pLastSpaceableRun->GetTextRun()->GetType() == TextRunType::Text)
            {
                const TextCharactersRun *pTextRun = reinterpret_cast<const TextCharactersRun*>(pLastSpaceableRun->GetTextRun());
                pProperties = pTextRun->GetProperties();
            }
            else if (pLastSpaceableRun->GetTextRun()->GetType() == TextRunType::Object)
            {
                const ObjectRun *pObjectTextRun = reinterpret_cast<const ObjectRun*>(pLastSpaceableRun->GetTextRun());
                pProperties = pObjectTextRun->GetProperties();
            }

            if (pProperties != NULL)
            {
                characterSpacing = pProperties->GetCharacterSpacing() / CharacterSpacingScale
                                 * pProperties->GetFontSize();
            }
        }
    }

    *pTrailingCharacterSpacing = characterSpacing;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CalculateLineMetrics
//
//  Synopsis:
//      Calculates line metrics from formatted line data.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::CalculateLineMetrics()
{
    Result::Enum txhr = Result::Success;
    LSLINEAREA lineArea;
    XFLOAT trailingCharacterSpacing;

    memset(&lineArea, 0, sizeof(LSLINEAREA));

    // Get line metrics. Some metrics are stored as LS units privately since they may
    // be needed during hit testing, etc.
    IFCTEXT(ResultFromLSErr(LsQueryLineDup(m_pLsLine,
                                      &lineArea)));

    m_width = TextDpi::FromTextDpi(lineArea.upLimLineProper);
    m_widthIncludingTrailingWhitespace = TextDpi::FromTextDpi(lineArea.upLimLine);
    m_start = TextDpi::FromTextDpi(lineArea.upStartMainText);
    m_length = m_lsLineInfo.cpLimToContinue - m_startIndex;

    // Set text baseline (ascent) and height
    if (m_lsLineInfo.dvrMultiLineHeight != dvHeightIgnore &&
        !(m_lsLineInfo.dvrMultiLineHeight == 0 && m_lsLineInfo.durContent == 0))
    {
        // dvrMultiLineHeight returned by LS does not take into account baseline alignment.
        // Because of that, dvrAscent + dvrDescent may be > dvrMultiLineHeight.
        // To mitigate this problem, get the max of those values.
        m_textHeight = TextDpi::FromTextDpi(Math::Max((XINT32)m_lsLineInfo.dvrMultiLineHeight, (XINT32)(m_lsLineInfo.dvrAscent + m_lsLineInfo.dvrDescent)));
        m_textBaseline = TextDpi::FromTextDpi(m_lsLineInfo.dvrAscent);
    }
    else
    {
        // Line is empty so text height and text baseline are based on the default typeface.
        XFLOAT baseline;
        XFLOAT lineSpacing;
        const TextRunProperties *pDefaultTextRunProperties = m_pTextParagraphProperties->GetDefaultTextRunProperties();

        IFC_FROM_HRESULT_RTS(pDefaultTextRunProperties->GetFontTypeface()->GetCompositeFontFamily()->GetTextLineBoundsMetrics(
                                m_pTextParagraphProperties->GetTextLineBounds(),
                                &baseline,
                                &lineSpacing));

        m_textHeight   = lineSpacing * pDefaultTextRunProperties->GetFontSize();
        m_textBaseline = baseline * pDefaultTextRunProperties->GetFontSize();
    }

    // Line's height is always the text height.
    m_baseline = m_textBaseline;
    m_height = m_textHeight;

    m_newlineLength = 0;
    if (m_lsLineInfo.endr != endrEndParaSpan && m_lsLineInfo.endr != endrSoftCR)
    {
        ASSERT(m_lsLineInfo.endr != endrEndPara); // Expecting endrEndParaSpan instead of endrEndPara.

        // TODO: incoroporate hyphenation calculation here, dcp depend calculated by LS is imprecise
        // when hyphenator lookahead exists.
        m_dependentLength = m_lsLineInfo.dcpDepend;
    }
    else if (m_lsLineInfo.endr == endrEndParaSpan)
    {
        // cpLimToContinue is not reliable for the last paragraph of the line. It does not contain
        // any hidden runs at the end. To get accurate value dcpDepend needs to be added.
        m_length += m_lsLineInfo.dcpDepend;
    }
    else
    {
        LsRun *pLastRun = m_pTextStore->GetRunCache()->GetLsRun(m_startIndex + m_length - 1, NULL);
        ASSERT(pLastRun != NULL);
        const TextRun *pLastTextRun = pLastRun->GetTextRun();
        ASSERT(pLastTextRun != NULL && pLastTextRun->GetType() == TextRunType::EndOfLine);
        m_newlineLength = pLastTextRun->GetLength();
    }

    // Calculate trailing whitespace length
    if (lineArea.upLimLineProper == lineArea.upLimLine)
    {
        // No trailing whitespace seen my LS, count only newline length.
        m_trailingWhitespaceLength = m_newlineLength;
    }
    else
    {
        // LS detected trailing spaces, hit test to find the start character index.
        CharacterHit charHit;
        IFCTEXT(GetCharacterHitFromDistance(m_width, &charHit));
        m_trailingWhitespaceLength = m_lsLineInfo.cpLimToContinue - charHit.firstCharacterIndex - charHit.trailingLength;
    }

    // Neither returned width should include any space generated by
    // CharacterSpacing at the end of the last run.

    DetermineTrailingCharacterSpacing(&trailingCharacterSpacing);

    m_width = (XFLOAT)Math::Max(0.0f, m_width - trailingCharacterSpacing);
    m_widthIncludingTrailingWhitespace = (XFLOAT)Math::Max(0.0f, m_widthIncludingTrailingWhitespace - trailingCharacterSpacing);

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CreateLineBreak
//
//  Synopsis:
//      Creates line break from LS break info.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::CreateLineBreak(
    _In_opt_ PLSBREAKRECLINE pLsLineBreak
        // LS line break
    )
{
    Result::Enum txhr = Result::Success;

    switch (m_lsLineInfo.endr)
    {
        // EndPara means end of content, don't create a break record
        case endrEndParaSpan:
        case endrStopped:
            break;

        default:
            ASSERT(m_pTextLineBreak == NULL);
            if (pLsLineBreak)
            {
                IFC_OOM_RTS(m_pTextLineBreak = new LsTextLineBreak(m_pTextFormatter->m_pLsContext, pLsLineBreak));
            }
            else
            {
                IFC_OOM_RTS(m_pTextLineBreak = new LsTextLineBreak());
            }
            break;
    }
Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::FormatCollapsed
//
//  Synopsis:
//      Given a line that has overflowed, formats a collapsed line at the
//      collapsing width, retaining relevant metrics from the original line
//      and new line.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::FormatCollapsed(
    _In_ LsTextLine *pLine,
        // LsTextLine which has overflowed and needs to be collapsed.
    _In_ XUINT32 startIndex,
        // Start index.
    _In_ XFLOAT collapsingWidth,
        // Width of collapsed line.
    _In_opt_ TextCollapsingSymbol *pCollapsingSymbol,
        // Collapsing symbol.
    _In_opt_ LsTextLineBreak *pPreviousLineBreak
        // Line breaking information for the previous line, if any exists.
    )
{
    Result::Enum txhr = Result::Success;
    PLSBREAKRECLINE pLsPreviousLineBreak = NULL;
    PLSBREAKRECLINE pLsLineBreak = NULL;
    XFLOAT formattingWidth = 0.0f;
    LSPAP lspap;
    LSLINERESTR lineRestrictions = s_lineRestrictions;

    m_startIndex = startIndex;
    m_formattingWidth = collapsingWidth;

    ASSERT(m_pPreviousLineBreak == NULL);
    ASSERT(m_pLsLine == NULL);
    if (pPreviousLineBreak)
    {
        m_pPreviousLineBreak = pPreviousLineBreak;
        AddRefInterface(m_pPreviousLineBreak);
        pLsPreviousLineBreak = pPreviousLineBreak->GetLsBreakRecord();
    }

    // Set up collapsing symbol.
    ASSERT(m_pCollapsingSymbol == NULL);
    m_pCollapsingSymbol = pCollapsingSymbol;
    AddRefInterface(m_pCollapsingSymbol);

    // Formatting width excludes the collapsing symbol width.
    formattingWidth = MAX(0.0f, m_formattingWidth - (m_pCollapsingSymbol ? m_pCollapsingSymbol->GetWidth() : 0));

    SetupLsPap(m_pTextFormatter, formattingWidth, &lspap);

    //
    // If we want to clip the last word on the line then we need to make sure one more
    // word is included.  Do this by disabling forced breaks and turning on clipping.
    //

    if (m_pTextFormatter->m_lsHostContext.clipLastWordOnLine)
    {
        lspap.fForceBreakAsNext = 1;
        lineRestrictions.ellt = elltEndClipped;
    }

    // We use EmergencyBreak for character ellipsis, we should not put break opportunities between every two
    // characters, instead we just need to turn on force breaking flag.
    // That flag is already on by default for character ellipsis.
    ASSERT(lspap.fForceBreakAsNext == 0 || !m_pTextFormatter->m_lsHostContext.useEmergencyBreaking);

    // TODO: Currently we format the line twice for collapsing (one time in measure and another time in arrange).
    // By changing to use LS ellipsis feature, we can reduce to only one format.
    IFCTEXT(ResultFromLSErr(LsCreateLine(m_pTextFormatter->m_pLsContext,
                                    reinterpret_cast<PLSPARACLIENT>(this),
                                    &lspap,
                                    m_startIndex,
                                   &lineRestrictions,
                                    pLsPreviousLineBreak,
                                   &pLsLineBreak,
                                   &m_lsLineInfo,
                                   &m_pLsLine
                                   )));

    IFCTEXT(ResultFromLSErr(LsQueryLineMaxDepth(m_pLsLine, reinterpret_cast<LONG *>(&m_depthQueryMax))));

    // Set overflow and collapsed flags.
    SetFlags(Flags::HasMultiCharacterClusters, m_pTextFormatter->m_lsHostContext.hasMultiCharacterClusters);
    SetFlags(Flags::Collapsed, TRUE);

    // Get line metrics.
    IFCTEXT(CalculateLineMetrics());

    // Adjust metrics. Use text start, height, and width from newly formatted line because we don't care about metrics for characters that aren't visible.
    // Adjust width to include collapsing symbol, collapsed line has no trailing whitespaces.
    if (m_pCollapsingSymbol)
    {
        m_width += m_pCollapsingSymbol->GetWidth();
    }
    m_widthIncludingTrailingWhitespace = m_width;

    // Preserve character length and newline length of original line.
    m_length = pLine->m_length;
    m_newlineLength = pLine->m_newlineLength;

    // Collapsed line has no trailing whitespaces, or it wouldn't have overflowed.
    m_trailingWhitespaceLength = 0;

Cleanup:

    // Clean up the host context.
    m_pTextFormatter->m_lsHostContext.pTextSource = NULL;
    m_pTextFormatter->m_lsHostContext.pParagraphProperties = NULL;
    m_pTextFormatter->m_lsHostContext.pTextStore = NULL;
    m_pTextFormatter->m_lsHostContext.hasMultiCharacterClusters = FALSE;
    m_pTextFormatter->m_lsHostContext.collapsedLine = FALSE;
    m_pTextFormatter->m_lsHostContext.useEmergencyBreaking = FALSE;
    m_pTextFormatter->m_lsHostContext.clipLastWordOnLine = FALSE;

    // Destroy the LS break record, whatever it is. A collapsed line should have no break record.
    if (pLsLineBreak != NULL)
    {
        LsDestroyBreakRecord(m_pTextFormatter->m_pLsContext, pLsLineBreak);
    }

    if ((txhr != Result::Success) && m_pLsLine != NULL)
    {
        LsDestroyLine(m_pTextFormatter->m_pLsContext, m_pLsLine);
        m_pLsLine = NULL;
    }

    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetDistanceInsideTextCell
//
//  Synopsis:
//      Calculates distance in ls text cell from cp relative to cell start.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::GetDistanceInsideTextCell(
    _In_ XINT32 offset,
        // Character offset from text cell start.
    _In_ bool trailing,
        // True if at trailing edge of cell.
    _In_reads_(sublineCount) LSQSUBINFO* pSublineInfo,
        // Array containing data for each subline.
    _In_ XUINT32 sublineCount,
        // Number of sublines.
    _In_ LSTEXTCELL lsTextCell,
        // LS text cell being searched.
    _Out_ XINT32 *pHitTestDistance
        // Distance of character position from cell start.
    ) const
{
    Result::Enum txhr = Result::Success;
    *pHitTestDistance = 0;
    XINT32 distanceInCell = 0;
    XINT32 direction = (pSublineInfo[sublineCount - 1].lstflowSubline == pSublineInfo[0].lstflowSubline) ? 1 : -1;
    bool matched = false;
    XINT32 wholeAdvance = 0;
    XINT32 remainingAdvance = 0;

    // Assume caret stops at every codepoint in the run.
    XINT32 caretStopCount = lsTextCell.cpEndCell + 1 - lsTextCell.cpStartCell;
    bool stopAtEveryCharacter = true;

    // Determine how many caret stops are in the line.
    IFCTEXT(StopAtEachCharacterInCell(pSublineInfo, sublineCount, lsTextCell, m_pTextFormatter->m_lsHostContext.pFontAndScriptServices, &stopAtEveryCharacter));

    if (!stopAtEveryCharacter)
    {
        caretStopCount = 1;
    }

    wholeAdvance = lsTextCell.dupCell / caretStopCount;
    remainingAdvance = lsTextCell.dupCell % caretStopCount;

    for (XINT32 i = 1; i <= caretStopCount; i++)
    {
        XINT32 caretAdvance = wholeAdvance;
        if (remainingAdvance > 0)
        {
            caretAdvance++;
            remainingAdvance--;
        }

        if (offset < i)
        {
            matched = true;
            if (trailing)
            {
                // Hit-test at the trailing edge of the current caret stop, include the current caret advance
                *pHitTestDistance = (distanceInCell + caretAdvance) * direction;
            }
            else
            {
                // Hit-test at the leading edge of the current caret stop, return the accumulated distance
                *pHitTestDistance = distanceInCell * direction;
            }
            break;
        }
        distanceInCell += caretAdvance;
    }

    if (!matched)
    {
        // Hit-test beyond the last caret stop, return the total accumated distance up to the trailing edge of the last caret stop.
        *pHitTestDistance = distanceInCell * direction;
    }

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CollectTextBoundsToBaseLevel
//
//  Synopsis:
//      Calculates distance from the given cp to a base level that
//      two sets of sublines have in common.
//
//  Notes:
//      Base level is the highest subline's level that both sets of sublines
//      have in common. This method starts collecting text bounds at the
//      specified CP. The first bounds being collected is the one from
//      the CP to the end of its immediate enclosing subline. The
//      subsequent bounds are from the end of run to the end of subline
//      of the lower level until it reaches the base level.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::CollectTextBoundsToBaseLevel(
    _Inout_ XINT32 *pCp,
        // Start character position.
    _Inout_ XINT32 *pDistance,
        // Distance from start of line to start cp.
    _In_reads_(sublineCount) Ptls6::LSQSUBINFO* pSublineInfo,
        // Subline info.
    _In_ LONG sublineCount,
        // Subline count.
    _In_ XINT32 cpEndInSubline,
        // Last cp in this subline.
    _Out_ LONG *pBaseLevelDepth,
        // Base level depth, to be calculated.
    _Out_ XUINT32 *pcBoundsToBaseLevel,
        // Number of bounds from start to base level.
    _Outptr_result_buffer_(*pcBoundsToBaseLevel) TextBounds **ppBoundsToBaseLevel
        // Bounds array from start to base level.
    ) const
{
    Result::Enum txhr = Result::Success;
    LONG baseLevelDepth = sublineCount - 1;
    TextBounds *pBounds = NULL;
    *pBaseLevelDepth = sublineCount;
    *pcBoundsToBaseLevel = 0;
    *ppBoundsToBaseLevel = NULL;
    XINT32 sublineEndDistance = 0;

    // Create temporary bounds structure of max depth, since actual count is not yet known.
    IFC_OOM_RTS(pBounds = new TextBounds[m_depthQueryMax]);

    if (cpEndInSubline < pSublineInfo[sublineCount - 1].cpFirstSubline + pSublineInfo[sublineCount - 1].dcpSubline)
    {
        // The immedidate enclosing subline already contains the lscp end. It means we are already
        // at base level.
        goto Cleanup;
    }

    sublineEndDistance = (pSublineInfo[sublineCount - 1].lstflowSubline == pSublineInfo[0].lstflowSubline)
                       ? pSublineInfo[sublineCount - 1].dupSubline
                       : -pSublineInfo[sublineCount - 1].dupSubline;

    sublineEndDistance += pSublineInfo[sublineCount - 1].pointUvStartSubline.u;

    if (CalculateValidTextBounds(*pDistance,
                                 sublineEndDistance,
                                 0,
                                 m_height,
                                 pSublineInfo[sublineCount - 1].lstflowSubline,
                                 pBounds[*pcBoundsToBaseLevel]
                                 ))
    {
        (*pcBoundsToBaseLevel)++;
    }

    // Collect text bounds from end of run to the end of subline at lower levels until we reach the
    // common level. We reach common level when the subline at that level contains the lscpEnd.
    for (; baseLevelDepth > 0
           && (cpEndInSubline >= pSublineInfo[baseLevelDepth - 1].cpFirstSubline + pSublineInfo[baseLevelDepth - 1].dcpSubline)
         ; baseLevelDepth--)
    {
        LONG sublineIndex = baseLevelDepth - 1;
        XINT32 runEndDistance = 0;
        sublineEndDistance = (pSublineInfo[sublineIndex].lstflowSubline == pSublineInfo[0].lstflowSubline)
                           ? pSublineInfo[sublineIndex].dupSubline
                           : -pSublineInfo[sublineIndex].dupSubline;
        sublineEndDistance += pSublineInfo[sublineIndex].pointUvStartSubline.u;

        runEndDistance =  pSublineInfo[sublineIndex].lstflowSubline == pSublineInfo[0].lstflowSubline ?
                          pSublineInfo[sublineIndex].dupRun
                        : -pSublineInfo[sublineIndex].dupRun;
        runEndDistance += pSublineInfo[sublineIndex].pointUvStartRun.u;

        if (CalculateValidTextBounds(runEndDistance,
                                     sublineEndDistance,
                                     0,
                                     m_height,
                                     pSublineInfo[sublineIndex].lstflowSubline,
                                     pBounds[*pcBoundsToBaseLevel]
                                    ))
        {
            (*pcBoundsToBaseLevel)++;
        }
    }

    // Base level depth must be at least 1 because both cp at least share the main line.
    ASSERT(baseLevelDepth >= 1);
    *pBaseLevelDepth = baseLevelDepth;

    // Allocate the correct amount of memory for the output array and copy text bounds to it.
    IFC_OOM_RTS(*ppBoundsToBaseLevel = new TextBounds[*pcBoundsToBaseLevel]);
    for (XUINT32 i = 0; i < (*pcBoundsToBaseLevel); i++)
    {
        CopyBounds(pBounds[i], (*ppBoundsToBaseLevel)[i]);
    }

    // Move the current LSCP and distance to the end of run on the base level subline.
    *pCp = pSublineInfo[baseLevelDepth - 1].cpFirstRun + pSublineInfo[baseLevelDepth - 1].dcpRun;

    *pDistance = pSublineInfo[baseLevelDepth - 1].lstflowSubline == pSublineInfo[0].lstflowSubline ?
                          pSublineInfo[baseLevelDepth - 1].dupRun
                        : -pSublineInfo[baseLevelDepth - 1].dupRun;
    (*pDistance) += pSublineInfo[baseLevelDepth - 1].pointUvStartRun.u;

Cleanup:
    delete [] pBounds;
    pBounds = NULL;
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CollectTextBoundsFromBaseLevel
//
//  Synopsis:
//      Calculates distance from the given cp on a base level that two sets of
//      sublines have in common, to the end of the specified sublines.
//
//  Notes:
//      Base level is the highest subline's level that both sets of sublines have
//      in common. This method starts collecting text bounds at the specified
//      LSCP. The first bounds being collected  is the one from the LSCP to the
//      start of run at the base level subline. The subsequent bounds are
//      from the start of the higher level subline to the start of the run
//      within the same subline, until it reaches the immediate enclosing subline
//      of the last LSCP.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::CollectTextBoundsFromBaseLevel(
    _Inout_ XINT32 *pCp,
        // Start character position.
    _Inout_ XINT32 *pDistance,
        // Distance from start of line to start cp.
    _In_reads_(sublineCount) Ptls6::LSQSUBINFO* pSublineInfo,
        // Subline info.
    _In_ LONG sublineCount,
        // Subline count.
    _In_ LONG baseLevelDepth,
        // Base level depth.
    _Out_ XUINT32 *pcBoundsFromBaseLevel,
        // Number of bounds from base level to high sublines.
    _Outptr_result_buffer_(*pcBoundsFromBaseLevel) TextBounds **ppBoundsFromBaseLevel
        // Bounds array from base level to higher sublines.
) const
{
    Result::Enum txhr = Result::Success;

    // Current characte position is after the run end of the 1st cp. It must not be in the run of the last cp
    // because the two runs don't overlap at above base level.
    ASSERT(*pCp <= pSublineInfo[baseLevelDepth - 1].cpFirstRun);
    *pcBoundsFromBaseLevel = 0;
    TextBounds *pBounds = NULL;

    // Create temporary bounds structure of max depth, since actual count is not yet known.
    IFC_OOM_RTS(pBounds = new TextBounds[m_depthQueryMax]);

    if (CalculateValidTextBounds(*pDistance,
                                 pSublineInfo[baseLevelDepth - 1].pointUvStartRun.u,
                                 0,
                                 m_height,
                                 pSublineInfo[baseLevelDepth - 1].lstflowSubline,
                                 pBounds[*pcBoundsFromBaseLevel]
                                 ))
    {
        (*pcBoundsFromBaseLevel)++;
    }

    // Collect text bounds from start of subline to start of run at higher level sublines until it
    // reaches the immediate enclosing subline of the last LSCP.
    for (LONG i = baseLevelDepth; i < sublineCount - 1; i++)
    {
        if (CalculateValidTextBounds(pSublineInfo[i].pointUvStartSubline.u,
                                     pSublineInfo[i].pointUvStartRun.u,
                                     0,
                                     m_height,
                                     pSublineInfo[i].lstflowSubline,
                                     pBounds[*pcBoundsFromBaseLevel]
                                     ))

            {
                (*pcBoundsFromBaseLevel)++;
            }
    }

    // Allocate the correct amount of memory for the output array and copy text bounds to it.
    IFC_OOM_RTS(*ppBoundsFromBaseLevel = new TextBounds[*pcBoundsFromBaseLevel]);
    for (XUINT32 i = 0; i < (*pcBoundsFromBaseLevel); i++)
    {
        CopyBounds(pBounds[i], (*ppBoundsFromBaseLevel)[i]);
    }

    // Move the current LSCP and distance to the start of the immediate enclosing subline.
    *pCp = pSublineInfo[sublineCount - 1].cpFirstSubline;
    *pDistance = pSublineInfo[sublineCount - 1].pointUvStartSubline.u;

Cleanup:
    delete [] pBounds;
    pBounds = NULL;
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CalculateValidTextBounds
//
//  Synopsis:
//      Populates a TextBounds struct with correct values based on LS widths
//      and flow direction. Returns true if valid TextBounds struct was
//      populated from input.
//
//  Notes:
//      This is a private helper, and since vertical flow is not yet
//      supported vertical metrics values are not converted from ls units
//      or adjusted for flow dir.
//
//---------------------------------------------------------------------------
bool LsTextLine::CalculateValidTextBounds(
    _In_ XINT32 left,
        // Value to be used as bounds' leftmost point
    _In_ XINT32 right,
        // Value to be used as bounds' rightmost point
    _In_ XFLOAT top,
        // Value to be used as bounds' topmost point
    _In_ XFLOAT bottom,
        // Value to be used as bounds' bottommost point
    _In_ LSTFLOW lstFlow,
        // Bounds' flow direction.
    _In_ TextBounds& bounds
        // TextBounds struct whose values are to be filled in.
     ) const
{
    if (left == right || top == bottom)
    {
        // Invalid text bounds.
        return false;
    }

    if (left < right)
    {
        bounds.rect.X = TextDpi::FromTextDpi(left);
        bounds.rect.Width = TextDpi::FromTextDpi(right - left);
    }
    else
    {
        bounds.rect.X = TextDpi::FromTextDpi(right);
        bounds.rect.Width = TextDpi::FromTextDpi(left - right);
    }
    bounds.rect.Y = top;
    bounds.rect.Height = bottom - top;
    bounds.flowDirection = (lstFlow == lstflowWS || lstFlow == lstflowWN) ? FlowDirection::RightToLeft : FlowDirection::LeftToRight;

    return true;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CopyBounds
//
//  Synopsis:
//      Copies data from one TextBounds struct to another.
//
//---------------------------------------------------------------------------
void LsTextLine::CopyBounds(
    _In_ TextBounds &sourceBounds,
        // Source bounds.
    _In_ TextBounds &destBounds
        // Destination bounds.
    ) const
{
    destBounds.rect.X = sourceBounds.rect.X;
    destBounds.rect.Y = sourceBounds.rect.Y;
    destBounds.rect.Width = sourceBounds.rect.Width;
    destBounds.rect.Height = sourceBounds.rect.Height;
    destBounds.flowDirection = sourceBounds.flowDirection;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::CreateEmptyBounds
//
//  Synopsis:
//      Creates empty (degenerate) bounds when a bounds query has
//      no content or valid bounds.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::CreateEmptyBounds(
    _Out_ XUINT32 *pcBounds,
        // Size of bounds array returned.
    _Outptr_result_buffer_(*pcBounds) TextBounds **ppBounds
        // Pointer to bounds array.
    ) const
{
    Result::Enum txhr = Result::Success;
    *pcBounds = 1;
    IFC_OOM_RTS(*ppBounds = new TextBounds[1]);

    (*ppBounds)[0].rect.X = 0.0f;
    (*ppBounds)[0].rect.Y = 0.0f;
    (*ppBounds)[0].rect.Width = 0.0f;
    (*ppBounds)[0].rect.Height = 0.0f;
    (*ppBounds)[0].flowDirection = FlowDirection::LeftToRight;

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetClosestCaretIndex
//
//  Synopsis:
//      Gets the closest caret position at or preceding a character index's
//      leading edge and the offset to the next caret position.
//
//  Notes:
//      This method is inclusive.  If index's leading edge is a caret
//      position then it will be returned.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::GetClosestCaretIndex(
    _In_ XINT32 index,
    _Out_ XINT32 *pLeftCaretIndex,
    _Out_ XINT32 *pOffsetToNextCaretIndex,
    _Out_ bool *pFoundCaretIndex
    ) const
{
    Result::Enum txhr = Result::Success;
    PLSQSUBINFO pSublineInfo = NULL;
    LONG sublineCount;
    LSTEXTCELL lsTextCell = {0};
    bool stopAtEveryCharacter = true;
    LSERR lserr = 0;

    *pFoundCaretIndex = TRUE;

    // Even though we won't use it, LsQueryLineCpPpoint will error out if we don't pass in
    // storage for pSublineInfo.
    IFC_OOM_RTS(pSublineInfo = new LSQSUBINFO[m_depthQueryMax]);

    lserr = LsQueryLineCpPpoint(m_pLsLine, index, m_depthQueryMax, pSublineInfo, &sublineCount, &lsTextCell);
    IFCTEXT(ResultFromLSErr(lserr));

    // Check if this text cell requires stops at every character.
    IFCTEXT(StopAtEachCharacterInCell(pSublineInfo, sublineCount, lsTextCell, m_pTextFormatter->m_lsHostContext.pFontAndScriptServices, &stopAtEveryCharacter));

    if (stopAtEveryCharacter)
    {
        index = Math::Min(index, lsTextCell.cpEndCell);

        // If index references a hidden character, then LsQueryLineCpPpoint will return the closest
        // visible cp to the left which is what we want. Ensure that we return cpStartCell if that
        // is the case/
        *pLeftCaretIndex = index;
        *pOffsetToNextCaretIndex = 1;
    }
    else
    {
        // Caret stops at the edges of the text cell.
        *pLeftCaretIndex = lsTextCell.cpStartCell;
        *pOffsetToNextCaretIndex = lsTextCell.cpEndCell - lsTextCell.cpStartCell + 1;
    }

    // If index references a hidden character, then LsQueryLineCpPpoint will return the closest
    // visible cp to the left which is what we want.  However, if there is no visible cp to the left,
    // it will take the closest to the right, in which case we want to return the original index.
    if (index < lsTextCell.cpStartCell)
    {
        *pLeftCaretIndex = index;

        if (stopAtEveryCharacter)
        {
            *pOffsetToNextCaretIndex = 1;
        }
        else
        {
            *pOffsetToNextCaretIndex = lsTextCell.cpStartCell - index;
        }

        *pFoundCaretIndex = FALSE;
    }

Cleanup:
    delete[] pSublineInfo;

    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsTextLine::GetNextVisibleIndex
//
//  Synopsis:
//      Gets the closest non-hidden character at or following an index.
//
//  Notes:
//      This method is inclusive -- if index is part of a visible run then
//      then it will be returned.
//
//---------------------------------------------------------------------------
Result::Enum LsTextLine::GetNextVisibleIndex(
    _In_ XINT32 index,
    _Out_ XINT32 *pNextVisibleIndex,
    _Out_ bool *pFoundCaretIndex
    ) const
{
    ASSERT(index >= static_cast<XINT32>(m_startIndex));

    Result::Enum txhr = Result::Success;

    *pFoundCaretIndex = FALSE;

    while (static_cast<XUINT32>(index) < m_startIndex + m_length)
    {
        // Fetch LS run at this CP using null fetch context, so only significant runs will be fetched.
        LsRun *pLsRun = NULL;
        const TextRun *pTextRun;
        pLsRun = m_pTextStore->GetRunCache()->GetLsRun(index, NULL);

        // The LS run fetched must be significant and contain a corresponding text run.
        ASSERT_EXPECT_RTS(pLsRun != NULL);
        pTextRun = pLsRun->GetTextRun();
        ASSERT_EXPECT_RTS(pTextRun != NULL);

        if (pTextRun->GetType() == TextRunType::Text || pTextRun->GetType() == TextRunType::Object)
        {
            *pFoundCaretIndex = TRUE;
            break;
        }

        index = pTextRun->GetCharacterIndex() + pTextRun->GetLength();
    }

    *pNextVisibleIndex = index;

Cleanup:
    return txhr;
}
