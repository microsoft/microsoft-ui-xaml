// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TextLine.h>

namespace RichTextServices
{
    class TextSource;
    class TextParagraphProperties;

    namespace Internal
    {
        class LsTextLineBreak;
        class LsTextFormatter;
        class TextStore;

        //---------------------------------------------------------------------------
        //
        //  LsTextLine
        //
        //  TextLine class contains formatting results for one line of text.
        //
        //---------------------------------------------------------------------------
        class LsTextLine final : public TextLine
        {
        public:

            // Creates a new instance of the TextLine class.
            static Result::Enum Create(
                _In_ XUINT32 startIndex,
                _In_ XFLOAT formattingWidth,
                _In_opt_ LsTextLineBreak *pPreviousLineBreak,
                _In_ LsTextFormatter *pTextFormatter,
                _Outptr_ LsTextLine **ppTextLine
                );

            // Arranges the content of the line
            Result::Enum Arrange(
                _In_ const XRECTF &bounds
                );

            // Creates rendering data for the line's contents.
            Result::Enum Draw(
                _In_ TextDrawingContext *pDrawingContext,
                _In_ const XPOINTF &origin,
                _In_ XFLOAT viewportWidth
                );

            //  Creates a new TextLine shortened to the specified constraining width

            virtual Result::Enum Collapse(
                _In_ XFLOAT collapsingWidth,
                _In_ DirectUI::TextTrimming collapsingStyle,
                _In_opt_ TextCollapsingSymbol *pCollapsingSymbol,
                _Outptr_ TextLine **ppCollapsedLine
                );

            // Returns TRUE if Collapse() has been called on this line, FALSE otherwise.

            virtual bool HasCollapsed() const;

            // Returns FALSE if the caller can rely on all unicode code points in the line mapping
            // to independent glyphs runs.
            bool HasMultiCharacterClusters() const;

            // Gets the character hit corresponding to the specified distance
            // from the beginning of the line
            Result::Enum GetCharacterHitFromDistance(
                _In_ XFLOAT distance,
                _Out_ CharacterHit *pCharacterHit
                ) const;

            // Gets the distance from the beginning of the line to the specified
            // character hit.
            Result::Enum GetDistanceFromCharacterHit(
                _In_ const CharacterHit &characterHit,
                _Out_ XFLOAT *pDistance
                ) const;

            // Gets the previous character hit for caret navigation.
            Result::Enum GetPreviousCaretCharacterHit(
                _In_ const CharacterHit &characterHit,
                _Out_ CharacterHit *pPreviousCharacterHit
                ) const;

            // Gets the next character hit for caret navigation.
            Result::Enum GetNextCaretCharacterHit(
                _In_ const CharacterHit &characterHit,
                _Out_ CharacterHit *pNextCharacterHit
                ) const;

            // Gets an array of bounding rectangles that represent the range of
            // characters within a text line.
            Result::Enum GetTextBounds(
                _In_ XINT32 firstCharacterIndex,
                _In_ XINT32 textLength,
                _Out_ XUINT32 *pcBounds,
                _Outptr_result_buffer_(*pcBounds) TextBounds **ppBounds
                ) const;

            FlowDirection::Enum GetDetectedParagrahDirection() const
            {
                return m_detectedParaFlowDirection;
            }

            FlowDirection::Enum GetParagrahDirection() const
            {
                return m_paraFlowDirection;
            }


            // Gets the line's start index.
            XUINT32 GetStartIndex() const;

            // Gets maximum width at which line was formatted.
            XFLOAT GetFormattingWidth() const;

            // Gets actual paragraph constraint width for display.
            XFLOAT GetViewportWidth() const;

            // Gets paragraph properties with which line was formatted.
            const TextParagraphProperties* GetParagraphProperties() const;

        protected:

            // Destructor

            virtual ~LsTextLine();

        private:

            // Constructor.
            LsTextLine(
                _In_ LsTextFormatter *pTextFormatter
                );

            // Formats the line and stores formatting results.
            Result::Enum Format(
                _In_ XUINT32 startIndex,
                _In_ XFLOAT formattingWidth,
                _In_opt_ LsTextLineBreak *pPreviousLineBreak
                );

            // Formats a collapsed line at the collapsing width, retaining relevant metrics from the original line and new line.
            Result::Enum FormatCollapsed(
                _In_ LsTextLine *pLine,
                _In_ XUINT32 startIndex,
                _In_ XFLOAT collapsingWidth,
                _In_opt_ TextCollapsingSymbol *pCollapsingSymbol,
                _In_opt_ LsTextLineBreak *pPreviousLineBreak
                );

            // Sets relevant flags on LSPAP structure.
            static void SetupLsPap(
                _In_ LsTextFormatter *pTextFormatter,
                _In_ XFLOAT paragraphWidth,
                _Out_ Ptls6::LSPAP *pLsPap
                );

            // Returns the amount of trailing space generated at the end of line
            // by the CharacterSpacing property.
            void DetermineTrailingCharacterSpacing(
                _Out_ XFLOAT *pTrailingCharacterSpacing
            );

            // Calculates line metrics from formatted line info.
            Result::Enum CalculateLineMetrics();

            // Creates line break if necessary depending on LS line ending data.
            Result::Enum CreateLineBreak(
                _In_opt_ Ptls6::PLSBREAKRECLINE pLsLineBreak
                );

            // Calculates distance in ls text cell from cp relative to cell start.
            Result::Enum GetDistanceInsideTextCell(
                _In_ XINT32 offset,
                _In_ bool trailing,
                _In_reads_(sublineCount) Ptls6::LSQSUBINFO* pSublineInfo,
                _In_ XUINT32 sublineCount,
                _In_ Ptls6::LSTEXTCELL lsTextCell,
                _Out_ XINT32 *pHitTestDistance
                ) const;

            // Calculates distance from the given cp to a base level that two sets of sublines have in common.
            Result::Enum CollectTextBoundsToBaseLevel(
                _Inout_ XINT32 *pCp,
                _Inout_ XINT32 *pDistance,
                _In_reads_(sublineCount) Ptls6::LSQSUBINFO* pSublineInfo,
                _In_ LONG sublineCount,
                _In_ XINT32 cpEndInSubline,
                _Out_ LONG *pBaseLevelDepth,
                _Out_ XUINT32 *pcBoundsToBaseLevel,
                _Outptr_result_buffer_(*pcBoundsToBaseLevel) TextBounds **ppBoundsToBaseLevel
                ) const;

            // Calculates distance from the given cp on a base level that two sets of sublines have in common,
            // to the end of the specified sublines.
            Result::Enum CollectTextBoundsFromBaseLevel(
                _Inout_ XINT32 *pCp,
                _Inout_ XINT32 *pDistance,
                _In_reads_(sublineCount) Ptls6::LSQSUBINFO* pSublineInfo,
                _In_ LONG sublineCount,
                _In_ LONG baseLevelDepth,
                _Out_ XUINT32 *pcBoundsFromBaseLevel,
                _Outptr_result_buffer_(*pcBoundsFromBaseLevel) TextBounds **ppBoundsFromBaseLevel
                ) const;

            // Calculates valid text bounds from LS width and flow units.
            bool CalculateValidTextBounds(
                _In_ XINT32 left,
                _In_ XINT32 right,
                _In_ XFLOAT top,
                _In_ XFLOAT bottom,
                _In_ Ptls6::LSTFLOW lstFlow,
                _In_ TextBounds& bounds
                ) const;

            // Copies one TextBounds struct to another.
            void CopyBounds(
                _In_ TextBounds &sourceBounds,
                _In_ TextBounds &destBounds
                ) const;

            // Creates empty (degenerate) bounds when line has no content.
            Result::Enum CreateEmptyBounds(
                _Out_ XUINT32 *pcBounds,
                _Outptr_result_buffer_(*pcBounds) TextBounds **ppBounds
                ) const;


            struct Flags
            {
                enum Enum
                {
                    Collapsed                 = 0x00000001,
                        // Indicates whether or not part of this line is collapsed.
                    HasMultiCharacterClusters = 0x00000002,
                        // TRUE iff the line has at least one group of more than one characters mapped onto any
                        // number of glyphs.
                };
            };

            // Gets value for specified flags.
            bool GetFlags(_In_ XUINT32 flags) const;

            // Sets flags to specified value.
            void SetFlags(
                _In_ XUINT32 flags,
                _In_ bool value
                );

            Result::Enum GetClosestCaretIndex(
                _In_ XINT32 caretIndex,
                _Out_ XINT32 *pLeftCaretIndex,
                _Out_ XINT32 *pOffsetToNextCaretIndex,
                _Out_ bool *pFoundCaretIndex
                ) const;

            Result::Enum GetNextVisibleIndex(
                _In_ XINT32 index,
                _Out_ XINT32 *pNextVisibleIndex,
                _Out_ bool *pFoundCaretIndex
                ) const;

        private:

            LsTextFormatter *m_pTextFormatter;
                // Display context for this line.

            Ptls6::PLSLINE m_pLsLine;
                // Pointer to Line Services line wrapped by this object.

            Ptls6::LSLINFO m_lsLineInfo;
                // Line metrics for this line maintained by Line Services.

            XUINT32 m_startIndex;
                // Character offset of first position in the line.

            XFLOAT m_formattingWidth;
                // Maximum allowable width during formatting.

            XFLOAT m_viewportWidth;
                // Viewport width at render time.

            XUINT32 m_flags;
                // Storage for flags.

            XUINT32 m_depthQueryMax;
                // Max subtree depth, used to allocate memory for subline arrays, etc.

            TextSource *m_pTextSource;
                // The text source associated with this line.

            LsTextLineBreak *m_pPreviousLineBreak;
                // The line break ending the previous line.

            TextParagraphProperties *m_pTextParagraphProperties;
                // The properties of the paragraph to which this line belongs.

            Internal::TextStore *m_pTextStore;
                // The run cache used in formatting this line.

            TextCollapsingSymbol *m_pCollapsingSymbol;
                // Symbol to be displayed when line is collapsed.

            RichTextServices::FlowDirection::Enum m_detectedParaFlowDirection;
            RichTextServices::FlowDirection::Enum m_paraFlowDirection;
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::GetStartIndex
        //
        //  Synopsis:
        //      Gets first character position in the line.
        //
        //---------------------------------------------------------------------------
        inline XUINT32 LsTextLine::GetStartIndex() const
        {
            return m_startIndex;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::GetFormattingWidth
        //
        //  Synopsis:
        //      Gets width at which line was formatted.
        //
        //---------------------------------------------------------------------------
        inline XFLOAT LsTextLine::GetFormattingWidth() const
        {
            return m_formattingWidth;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::GetViewportWidth
        //
        //  Synopsis:
        //      Gets viewport width passed to TextLine::Draw during rendering.
        //
        //---------------------------------------------------------------------------
        inline XFLOAT LsTextLine::GetViewportWidth() const
        {
            return m_viewportWidth;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::GetParagraphProperties
        //
        //  Synopsis:
        //      Gets paragraph properties with which line was formatted.
        //
        //---------------------------------------------------------------------------
        inline const TextParagraphProperties* LsTextLine::GetParagraphProperties() const
        {
            return m_pTextParagraphProperties;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::GetFlags()
        //
        //  Returns:
        //      Gets value for specified flags.
        //
        //---------------------------------------------------------------------------
        inline bool LsTextLine::GetFlags(_In_ XUINT32 flags) const
        {
            return ((m_flags & flags) == flags) ? TRUE : FALSE;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLine::SetFlags()
        //
        //  Returns:
        //      Sets flags to specified value.
        //
        //---------------------------------------------------------------------------
        inline void LsTextLine::SetFlags(
            _In_ XUINT32 flags,
            _In_ bool value
            )
        {
            if (value)
            {
                m_flags |= flags;
            }
            else
            {
                m_flags &= ~flags;
            }
        }
    }
}
