// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "BlockNode.h"
#include "DynamicArray.h"
#include "LineMetrics.h"
#include "ParagraphTextSource.h"

namespace RichTextServices
{
    class TextLine;
}

class ParagraphNodeBreak;
class PageNode;
class ContainerNode;

//---------------------------------------------------------------------------
//
//  ParagraphNode
//
//  Represents layout results for one paragraph of content.
//
//---------------------------------------------------------------------------
class ParagraphNode : public BlockNode,
                      private IEmbeddedElementHost
{
public:

    ParagraphNode(
        _In_ BlockLayoutEngine *pBlockLayoutEngine,
        _In_opt_ CParagraph *pParagraph, // May be NULL for TextBlock which has no Paragraphs, or when RichTextBlock has no content.
        _In_opt_ ContainerNode *pParentNode
        );
    ~ParagraphNode() override;

    // BlockNode overrides.
    XFLOAT GetBaselineAlignmentOffset() const override;

    IEmbeddedElementHost *GetEmbeddedElementHost();

    // IEmbeddedElementHost overrides.
    _Check_return_ HRESULT AddElement(_In_ CInlineUIContainer *pContainer) override;
    bool CanAddElement() const override;
    _Check_return_ HRESULT RemoveElement(_In_ CInlineUIContainer *pContainer) override;
    _Check_return_ HRESULT UpdateElementPosition(
        _In_ CInlineUIContainer *pContainer,
        _In_ const XPOINTF &position) override;
    _Check_return_ HRESULT GetElementPosition(
        _In_ CInlineUIContainer *pContainer,
        _Out_ XPOINTF *pPosition) override;
    const XSIZEF & GetAvailableMeasureSize() const override;

    // Query method overrides.
    _Check_return_ HRESULT IsAtInsertionPosition(
        _In_ XUINT32 position,
        _Out_ bool *pIsAtInsertionPosition) override;
    _Check_return_ HRESULT PixelPositionToTextPosition(
        _In_ const XPOINTF& pixel,
        _Out_ XUINT32 *pPosition,
        _Out_opt_ TextGravity *pGravity) override;
    _Check_return_ HRESULT GetTextBounds(
        _In_ XUINT32 start,
        _In_ XUINT32 length,
        _Inout_ xvector<RichTextServices::TextBounds> *pBounds) override;

    bool GetHasTrimmedLine() const { return m_hasTrimmedLine; }

protected:
    // BlockNode layout customization overrides.
    _Check_return_ HRESULT MeasureCore(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 paragraphMaxLines,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_ bool suppressTopMargin,
        _In_opt_ BlockNodeBreak *pPreviousBreak
        ) override;
    _Check_return_ HRESULT ArrangeCore(
        _In_ XSIZEF finalSize
        ) override;
    _Check_return_ HRESULT DrawCore(
        _In_ bool forceDraw
        ) override;
    _Check_return_ HRESULT CanBypassMeasure(
        _In_ XSIZEF availableSize,
        _In_ XUINT32 paragraphMaxLines,
        _In_ bool allowEmptyContent,
        _In_ bool measureBottomless,
        _In_opt_ BlockNodeBreak *pPreviousBreak,
        _Out_ bool *pCanBypass
        ) override;

private:
    DynamicArray<LineMetrics> m_lines;
    ParagraphTextSource m_textSource;
    RichTextServices::TextRunCache *m_pTextRunCache;
    PageNode *m_pPageNode;

    // The width we would use if no constraint was passed (different from m_desiredSize.Width
    // when TextTrimmingMode != None).
    XFLOAT m_untrimmedDesiredWidth;

    bool m_hasTrimmedLine = false;

    void DeleteLineCache();

    _Check_return_ HRESULT EnsureTextCaches(
        _In_opt_ ParagraphNodeBreak *pPreviousBreak
        );

    HRESULT FormatLineAtIndex(
        _In_ XUINT32 lineIndex,
        _In_ DirectUI::TextTrimming textTrimming,
        _In_ XFLOAT layoutWidth,
        _Inout_ RichTextServices::TextFormatter **pTextFormatter,
        _Inout_ RichTextServices::TextParagraphProperties **pParagraphProperties,
        _Inout_ __pre_deref_except_maybenull RichTextServices::TextLine **ppTextLine
        );

    XFLOAT CalculateLineOffset(
        _In_ DirectUI::TextAlignment textAlignment,
        _In_ RichTextServices::TextLine *pTextLine,
        _In_ XFLOAT paragraphWidth
        );

    void ApplyLineStackingStrategy(
        _In_ DirectUI::LineStackingStrategy lineStackingStrategy,
        _In_ XFLOAT fontBaseline,
        _In_ XFLOAT fontLineAdvance,
        _In_ XFLOAT lineHeight,
        _In_ RichTextServices::TextLine *pTextLine,
        _In_ bool firstLine,
        _In_ bool lastLine,
        _Out_ XFLOAT *pLineAdvance,
        _Out_ XFLOAT *pLineOffset
        );

    _Check_return_ HRESULT TrimLineIfNecessary(
        _In_ const LineMetrics *pNextLineMetrics,
        _In_ RichTextServices::TextLine *pCurrentTextLine,
        _In_ XFLOAT collapsingWidth,
        _In_ DirectUI::TextTrimming collapsingStyle,
        _Outptr_result_maybenull_ RichTextServices::TextLine **ppCollapsedTextLine
        );
    bool ShouldShowParagraphEllipsisOnCurrentLine(_In_ bool isLastLine);

    // Embedded element helpers.
    void RemoveEmbeddedElements();
    void MarkEmbeddedElementsInvisible();

    // Gets the paragraph-relative offset from an offset local to this node.
    XUINT32 GetPositionInParagraph(_In_ XUINT32 localOffset) const;

    // Gets a local offset in the node from a paragraph-relative offset.
    XUINT32 GetLocalPositionFromParagraphPosition(_In_ XUINT32 paragraphPosition) const;

    XUINT32 GetLineIndexFromPosition(
        _In_ XUINT32 positionInParagraph,
        _Out_opt_ XPOINTF *pLineOffset
        ) const;
    XUINT32 GetLineIndexFromPoint(
        _In_ XPOINTF point,
        _Out_ XPOINTF *pLineOffset
        ) const;

    // Queries a TextLine to determine if the specified position is an insertion position.
    static _Check_return_ HRESULT IsAtInsertionPositionInTextLine(
        _In_ XUINT32 position,
        _In_ XUINT32 lineStartIndex,
        _In_ RichTextServices::TextLine *pTextLine,
        _Out_ bool *pIsAtInsertionPosition
        );

};

inline IEmbeddedElementHost *ParagraphNode::GetEmbeddedElementHost()
{
    return this;
}
