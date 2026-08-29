// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Result.h"
#include "CharacterHit.h"
#include "TextBounds.h"
#include "TextObject.h"

namespace RichTextServices
{
    class TextLineBreak;
    class TextDrawingContext;
    class TextCollapsingSymbol;
    
    //---------------------------------------------------------------------------
    //
    //  TextLine
    //
    //  TextLine class contains formatting results for a line of text.
    //
    //---------------------------------------------------------------------------
    class TextLine : public TextObject
    {
    public:

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::Arrange
        //
        //  Synopsis:
        //      Arranges the content of the line.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum Arrange(
            _In_ const XRECTF &bounds
                // Bounds of the line within its container.
            ) = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::Draw
        //
        //  Synopsis:
        //      Creates rendering data for the line's contents.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum Draw(
            _In_ TextDrawingContext *pDrawingContext,
                // The TextDrawingContext object onto which the TextLine is drawn.
            _In_ const XPOINTF &origin,
                // A point value that represents the drawing origin.
            _In_ XFLOAT viewportWidth
                // A value that represents the viewport width available for rendering.
            ) = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::Collapse
        //
        //  Synopsis:
        //      Creates a new TextLine shortened to the specified constraining width. 
        //
        //  Remarks:
        //      Creates a new TextLine shortened to the specified constraining width. 
        //
        //-----------------------------------------------------------------------
        virtual
        Result::Enum Collapse(
            _In_ XFLOAT collapsingWidth,
                // Width of collapsed line.
            _In_ DirectUI::TextTrimming collapsingStyle,
                // Collapsing style, character or word trailing.
            _In_opt_ TextCollapsingSymbol *pCollapsingSymbol,
                // Collapsing symbol to be displayed.
            _Outptr_ TextLine **ppCollapsedLine
                // Collapsed TextLine.
            ) = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::HasCollapsed
        //
        //  Synopsis:
        //      Returns TRUE if the visual width of this line will be smaller than 
        //      its logical width, FALSE otherwise.
        //
        //-----------------------------------------------------------------------
        virtual
        bool HasCollapsed() const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::HasMultiCharacterClusters
        //
        //  Synopsis:
        //      Returns FALSE if the caller can rely on all unicode code points
        //      in the line mapping to independent glyphs runs.  I.e., glyph
        //      clusters always map to single characters, never runs of combined
        //      characters such as surrogate pairs or base characters with
        //      combining marks.
        //
        //  Notes:
        //      A conservative implementation simply returns TRUE.
        //
        //      This method is useful for callers who may be able to avoid
        //      reformatting a line in the future if it has not complex content.
        //      In particular, caret navigation can be implemented without a line
        //      format by caching this value.
        //
        //-----------------------------------------------------------------------
        virtual
        bool HasMultiCharacterClusters() const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::GetCharacterHitFromDistance
        //
        //  Synopsis:
        //      Gets the character hit corresponding to the specified distance 
        //      from the beginning of the line.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum GetCharacterHitFromDistance(
            _In_ XFLOAT distance,
                // A value that represents the distance from the beginning of the line.
            _Out_ CharacterHit *pCharacterHit
                // The CharacterHit object at the specified distance from the beginning of the line.
            ) const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::GetDistanceFromCharacterHit
        //
        //  Synopsis:
        //      Gets the distance from the beginning of the line to the specified 
        //      character hit.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum GetDistanceFromCharacterHit(
            _In_ const CharacterHit &characterHit,
                // The CharacterHit object whose distance you want to query.
            _Out_ XFLOAT *pDistance
                // A value that represents the distance from the beginning of the line.
            ) const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::GetPreviousCaretCharacterHit
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
        //      On input, characterHit.TrailingLength is treated as a flag.
        //      If 0, characterHit references the leading edge of the indicated
        //      character cluster, otherwise it references the trailing edge.
        //
        //      On output, pPreviousCharacterHit->TrailingLength always points
        //      exactly at the trailing edge of the returned cluster.
        //
        //-----------------------------------------------------------------------
        virtual
        Result::Enum GetPreviousCaretCharacterHit(
            _In_ const CharacterHit &characterHit,
            _Out_ CharacterHit *pPreviousCharacterHit
            ) const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::GetNextCaretCharacterHit
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
        //      On input, characterHit.TrailingLength is treated as a flag.
        //      If 0, characterHit references the leading edge of the indicated
        //      character cluster, otherwise it references the trailing edge.
        //
        //      On output, pNextCharacterHit->TrailingLength always points
        //      exactly at the trailing edge of the returned cluster.
        //
        //-----------------------------------------------------------------------
        virtual
        Result::Enum GetNextCaretCharacterHit(
            _In_ const CharacterHit &characterHit,
            _Out_ CharacterHit *pNextCharacterHit
            ) const = 0;

        //-----------------------------------------------------------------------
        //
        //  Member:
        //      TextLine::GetTextBounds
        //
        //  Synopsis:
        //      Gets an array of bounding rectangles that represent the range of 
        //      characters within a text line.
        //
        //-----------------------------------------------------------------------
        virtual 
        Result::Enum GetTextBounds(
            _In_ XINT32 firstCharacterIndex,
                // A value that represents the index of first character of specified range.
            _In_ XINT32 textLength,
                // A value that represents the number of characters of the specified range.
            _Out_ XUINT32 *pcBounds,
                // The size of array of text bounds.
            _Outptr_result_buffer_(*pcBounds) TextBounds **ppBounds
                // An array of TextBounds structures that represent the range of characters within a text line.
            ) const = 0;

        virtual FlowDirection::Enum GetDetectedParagrahDirection() const = 0;
        virtual FlowDirection::Enum GetParagrahDirection() const = 0;
        // Gets the width of the line, excluding trailing whitespace characters.
        // Excludes whitespace resulting from CharacterSpacing set on the last 
        // character of the line.
        XFLOAT GetWidth() const;

        // Gets the width of the line, including trailing whitespace characters.
        XFLOAT GetWidthIncludingTrailingWhitespace() const;

        // Gets the distance from the start of a paragraph to the starting point of a line.
        XFLOAT GetStart() const;

        // Gets the height of the line.
        XFLOAT GetHeight() const;

        // Gets the height of the text and any other content in the line.
        XFLOAT GetTextHeight() const;

        // Gets the distance from the top of the line to the baseline.
        XFLOAT GetBaseline() const;

        // Gets the distance from the top of the text and any other content in the line to the baseline.
        XFLOAT GetTextBaseline() const;

        // Gets the distance that black pixels extend prior to the left leading alignment edge of the line.
        XFLOAT GetOverhangLeading() const;

        // Gets the distance that black pixels extend following the right trailing alignment edge of the line.
        XFLOAT GetOverhangTrailing() const;

        // Gets the total number of character positions of the current line.  This includes trailing whitespaces and newline characters.
        XUINT32 GetLength() const;

        // Gets the number of whitespace characters beyond the last non-blank character in a line.
        XUINT32 GetTrailingWhitespaceLength() const;

        // Gets the number of newline characters at the end of a line.
        XUINT32 GetNewlineLength() const;

        // Gets the number of characters following the last character of the line that 
        // may trigger reformatting of the current line.
        XUINT32 GetDependentLength() const;

        // Gets the state of the line when broken by line breaking process.
        TextLineBreak* GetTextLineBreak() const;

        // Gets the flag that indicates TextReadingOrder is different 
        bool AlignmentFollowsReadingOrder() const;

    protected:
        XFLOAT m_width;
            // The width of the line, excluding trailing whitespace characters.

        XFLOAT m_widthIncludingTrailingWhitespace;
            // The width of the line, including trailing whitespace characters.

        XFLOAT m_start;
            // The distance from the start of a paragraph to the starting point of a line.

        XFLOAT m_height;
            // The height of the line.

        XFLOAT m_textHeight;
            // The height of the text and any other content in the line.

        XFLOAT m_baseline;
            // The distance from the top of the line to the baseline.

        XFLOAT m_textBaseline;
            // The distance from the top of the  text and any other content in the line to the baseline.

        XFLOAT m_overhangLeading;
            // The distance that black pixels extend prior to the left leading alignment edge of the line.

        XFLOAT m_overhangTrailing;
            // The distance that black pixels extend following the right trailing alignment edge of the line.

        XUINT32 m_length;
            // The total number of character positions of the current line.

        XUINT32 m_trailingWhitespaceLength;
            // The number of whitespace code points beyond the last non-blank character in a line.

        XUINT32 m_newlineLength;
            // The number of newline characters at the end of a line.

        XUINT32 m_dependentLength;
            // The number of characters following the last character of the line that 
            // may trigger reformatting of the current line.

        TextLineBreak *m_pTextLineBreak;
            // The state of the line when broken by line breaking process.

        bool m_alignmentFollowsReadingOrder{};
            // The flag to indicate whether the detected text reading order is different from the set flow direction.
    };
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetWidth
//
//  Returns:
//      The width of the line, excluding trailing whitespace characters.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetWidth() const
{
    return m_width;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetWidthIncludingTrailingWhitespace
//
//  Returns:
//      The width of the line, including trailing whitespace characters.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetWidthIncludingTrailingWhitespace() const
{
    return m_widthIncludingTrailingWhitespace;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetStart
//
//  Returns:
//      The distance from the start of a paragraph to the starting point of a line.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetStart() const
{
    return m_start;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetHeight
//
//  Returns:
//      The height of the line.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetHeight() const
{
    return m_height;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetTextHeight
//
//  Returns:
//      The height of the text and any other content in the line.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetTextHeight() const
{
    return m_textHeight;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetBaseline
//
//  Returns:
//      The distance from the top of the line to the baseline.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetBaseline() const
{
    return m_baseline;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetTextBaseline
//
//  Returns:
//      The distance from the top of the  text and any other content in the line to the baseline.
//
//---------------------------------------------------------------------------
inline XFLOAT RichTextServices::TextLine::GetTextBaseline() const
{
    return m_textBaseline;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetLength
//
//  Returns:
//      The total number of character positions of the current line.
//
//---------------------------------------------------------------------------
inline XUINT32 RichTextServices::TextLine::GetLength() const
{
    return m_length;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetTrailingWhitespaceLength
//
//  Returns:
//      The number of whitespace code points beyond the last non-blank character in a line.
//
//---------------------------------------------------------------------------
inline XUINT32 RichTextServices::TextLine::GetTrailingWhitespaceLength() const
{
    return m_trailingWhitespaceLength;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetNewlineLength
//
//  Returns:
//      The number of newline characters at the end of a line.
//
//---------------------------------------------------------------------------
inline XUINT32 RichTextServices::TextLine::GetNewlineLength() const
{
    return m_newlineLength;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::GetDependentLength
//
//  Returns:
//      The number of characters following the last character of the line that 
//      may trigger reformatting of the current line.
//
//---------------------------------------------------------------------------
inline XUINT32 RichTextServices::TextLine::GetDependentLength() const
{
    return m_dependentLength;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextLine::AlignmentFollowsReadingOrder
//
//  Returns:
//      Whether the detected text reading order is different from the set
//      flow direction.
//
//---------------------------------------------------------------------------
inline bool RichTextServices::TextLine::AlignmentFollowsReadingOrder() const
{
    return m_alignmentFollowsReadingOrder;
}
