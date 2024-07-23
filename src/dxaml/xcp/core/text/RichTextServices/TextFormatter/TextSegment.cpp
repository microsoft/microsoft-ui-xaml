// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextSegment.h"
#include "TextRunData.h"
#include "TextItemizer.h"
#include "PALFontAndScriptServices.h"
#include "DWriteFontAndScriptServices.h"


using namespace RichTextServices;
using namespace RichTextServices::Internal;

enum UnicodeCodePoint
{
    UnicodeBoundaryNeutralControl = 0x000080,
    UnicodeLRM = 0x00200E,   // Left-to-right mark.
    UnicodeRLM = 0x00200F,   // Right-to-left mark.
    UnicodeLRE = 0x00202A,   // Left-to-right embedding.
    UnicodeRLE = 0x00202B,   // Right-to-left embedding.
    UnicodePDF = 0x00202C,   // Pop directional formatting.
};

const WCHAR TextSegment::m_LRM = UnicodeLRM;
const WCHAR TextSegment::m_RLM = UnicodeRLM;
const WCHAR TextSegment::m_LRE = UnicodeLRE;
const WCHAR TextSegment::m_RLE = UnicodeRLE;
const WCHAR TextSegment::m_PDF = UnicodePDF;
const WCHAR TextSegment::m_hidden = UnicodeBoundaryNeutralControl;

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::TextSegment
//
//  Synopsis:
//      Initializes a new instance of the TextSegment class.
//
//---------------------------------------------------------------------------
TextSegment::TextSegment() :
    m_pFirstRunData(NULL),
    m_pRecentRunData(NULL),
    m_characterIndexOfRecentRunData(0),
    m_totalLength(0),
    m_totalLengthForAnalyzingLineBreakpoints(0),
    m_analyzingLineBreakpoints(FALSE),
    m_paragraphFlowDirection(FlowDirection::LeftToRight),
    m_pFontAndScriptServices(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::~TextSegment
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
TextSegment::~TextSegment()
{
    Clear();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::Clear
//
//  Synopsis:
//      Clears the content of the segment.
//
//---------------------------------------------------------------------------
void TextSegment::Clear()
{
    TextRunData *pTempRunData;

    while (m_pFirstRunData != NULL)
    {
        pTempRunData = m_pFirstRunData;
        m_pFirstRunData = m_pFirstRunData->GetNext();
        delete pTempRunData;
    }

    m_pFirstRunData = NULL;
    m_pRecentRunData = NULL;
    m_characterIndexOfRecentRunData = 0;
    m_totalLength = 0;
    m_totalLengthForAnalyzingLineBreakpoints = 0;
    ReleaseInterface(m_pFontAndScriptServices);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::Populate
//
//  Synopsis:
//      Populates empty text segment with content provided by given TextSource.
//
//---------------------------------------------------------------------------
Result::Enum TextSegment::Populate(
    _In_ XUINT32 characterIndex,
        // Index of first character in the text segment.
    _In_ TextSource *pTextSource,
        // Source of character and formatting data.
    _In_ RichTextServices::FlowDirection::Enum paragraphFlowDirection
        // Dominant text flow direction.
    )
{
    Result::Enum txhr = Result::Success;
    TextRun *pTextRun = NULL;
    TextRunData *pRunData = NULL;
    TextRunData *pRunDataPrev = NULL;
    XUINT8 paragraphBidiLevel = (paragraphFlowDirection == RichTextServices::FlowDirection::RightToLeft) ? 1 : 0;


    IFC_EXPECT_RTS(m_pFirstRunData == NULL);
    m_totalLength = 0;
    m_totalLengthForAnalyzingLineBreakpoints = 0;
    m_pRecentRunData = NULL;
    m_characterIndexOfRecentRunData = 0;
    m_paragraphFlowDirection = paragraphFlowDirection;


    // Fetch content from TextSource until end of line or paragraph is reached.
    do
    {
        IFCTEXT(pTextSource->GetTextRun(characterIndex, &pTextRun));
        IFC_EXPECT_RTS(pTextRun != NULL);

        characterIndex += pTextRun->GetLength();
        m_totalLength += pTextRun->GetLength();

        // When running LineBreakpoint analysis we need to hide hidden runs from DWrite because
        // the line breaking algorithm has no way to ignore them -- these hidden runs will
        // cause confusion and create or prevent break opportunities we should otherwise have.
        if (!SkipForLineBreakAnalysis(pTextRun))
        {
            m_totalLengthForAnalyzingLineBreakpoints += pTextRun->GetLength();
        }

        IFC_OOM_RTS(pRunData = new TextRunData(pTextRun, pRunDataPrev, paragraphBidiLevel));
        pTextRun = NULL;

        if (pRunDataPrev == NULL)
        {
            m_pFirstRunData = pRunData;
        }

        pRunDataPrev = pRunData;
    }
    while ( pRunData->GetTextRun()->GetType() != TextRunType::EndOfParagraph);

Cleanup:
    ReleaseInterface(pTextRun);
    return txhr;
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::NeedNumberSubstitution
//
//  Synopsis:
//      Optimize for the most common case: text source does not contain any run with locale that requires number substitution.
//
//---------------------------------------------------------------------------
HRESULT TextSegment::NeedNumberSubstitution(_Out_ bool* needNumberSubstitution)
{
    HRESULT hr = S_OK;
    bool result = false;
    DWriteFontAndScriptServices *pDWriteFontServices = NULL;
    IDWriteNumberSubstitution* pNumberSubstitution = NULL; 
    TextRunData *pRunData = m_pFirstRunData;


    pDWriteFontServices =
    static_cast<DWriteFontAndScriptServices*>(
    static_cast<PALFontAndScriptServices*>(m_pFontAndScriptServices)->GetPALFontAndScriptServices());

    while (pRunData != NULL)
    {
        if (pRunData->GetTextRun()->GetType() == TextRunType::Text)
        {
            TextCharactersRun *pCharactersRun = reinterpret_cast<TextCharactersRun *>(pRunData->GetTextRun());

            IFC(pDWriteFontServices->GetNumberSubstitution(
                pCharactersRun->GetProperties()->GetCultureInfo(),
                NULL,
                &pNumberSubstitution));

            if (pNumberSubstitution != NULL)
            {
                result = TRUE;
                break;
            }
        }
        pRunData = pRunData->GetNext();
    }

    *needNumberSubstitution = result;
    
Cleanup:

    ReleaseInterface(pNumberSubstitution);
    RRETURN(hr);

}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::Analyze
//
//  Synopsis:
//      Performs analysis on pre-populated content and creates collection of runs
//
//---------------------------------------------------------------------------
Result::Enum TextSegment::Analyze(
    _In_ IFontAndScriptServices *pFontAndScriptServices
        // Provides an interface to access font and script specific data.
    )
{
    Result::Enum txhr = Result::Success;
    PALText::ITextAnalyzer *pTextAnalyzer = NULL;
    bool needNumberSubstitution = false;

    ReplaceInterface(m_pFontAndScriptServices, pFontAndScriptServices);

    // If the segment contains no text then we can skip the text analysis process.
    if (m_totalLength > 0)
    {
        // Perform text analysis.
        TextItemizer textItemizer((XUINT8)GetParagraphReadingDirection());

        IFC_FROM_HRESULT_RTS(pFontAndScriptServices->CreateTextAnalyzer(&pTextAnalyzer));
        IFC_FROM_HRESULT_RTS(pTextAnalyzer->AnalyzeScript(this, 0, m_totalLength, &textItemizer));
        IFC_FROM_HRESULT_RTS(pTextAnalyzer->AnalyzeBidi(this, 0, m_totalLength, &textItemizer));

        IFC_FROM_HRESULT_RTS(NeedNumberSubstitution(&needNumberSubstitution));
        if (needNumberSubstitution)
        {
            IFC_FROM_HRESULT_RTS(pTextAnalyzer->AnalyzeNumberSubstitution(this, 0, m_totalLength, &textItemizer));
        }

        IFCTEXT(textItemizer.Itemize(m_pFirstRunData));

        // Perform font fallback.
        IFCTEXT(AnalyzeFonts());

        // Perform text complexity analysis to determine which ranges need shaping.        
        IFCTEXT(AnalyzeComplexity(pTextAnalyzer));        

        // Balance CharacterSpacing at end of reversed direction runs
        BalanceBidiCharacterSpacing();

        // WARNING: If we implement number substitution we need to exclude the digits from the
        // character range check IsAscii() (and probably change its name).

        // Do line breaking analysis last so that runs have already been split as much as they will be.
        if (m_totalLengthForAnalyzingLineBreakpoints > 0)
        {
            LineBreakpointAnalysisHelper lineBreakpointAnalysisHelper(this);
            IFC_FROM_HRESULT_RTS(pTextAnalyzer->AnalyzeLineBreakpoints(this, 0, m_totalLength, &lineBreakpointAnalysisHelper));
        }
    }

Cleanup:
    ReleaseInterface(pTextAnalyzer);

    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::FetchNextRun
//
//  Synopsis:
//      Fetches the next run from the collection of analyzed text content.
//
//---------------------------------------------------------------------------
TextRunData * TextSegment::FetchNextRun()
{
    TextRunData *pRunData = NULL;

    if (m_pFirstRunData != NULL)
    {
        pRunData = m_pFirstRunData;
        m_pFirstRunData = m_pFirstRunData->GetNext();
    }

    return pRunData;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetTextAtPosition
//
//  Synopsis:
//      Get a block of text starting at the specified text position.
//      Returning NULL indicates the end of text - the position is after
//      the last character. This function is called iteratively for
//      each consecutive block, tying together several fragmented blocks
//      in the backing store into a virtual contiguous string.
//
//  Returns:
//      Pointer to the first character at the given text position.
//      NULL indicates no chunk available at the specified position, either
//      because textPosition >= the entire text content length or because the
//      queried position is not mapped into the app's backing store.
//
//  Notes:
//      Although apps can implement sparse textual content that only maps part of
//      the backing store, the app must map any text that is in the range passed
//      to any analysis functions.
//
//---------------------------------------------------------------------------
HRESULT TextSegment::GetTextAtPosition(
    _In_ XUINT32 textPosition,
        // First position of the piece to obtain. All
        // positions are in UTF16 code-units, not whole characters, which
        // matters when supplementary characters are used.
    _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
        // Address that receives a pointer to the text block
        // at the specified position.
    _Out_ XUINT32 *pTextLength
        // Number of UTF16 units of the retrieved chunk.
        // The returned length is not the length of the block, but the length
        // remaining in the block, from the given position until its end.
        // So querying for a position that is 75 positions into a 100
        // position block would return 25.
    )
{
    IFCEXPECT_ASSERT_RETURN(m_pFirstRunData != NULL);
    IFCEXPECT_ASSERT_RETURN(m_totalLength > 0);

    if (textPosition >= m_totalLength)
    {
        *ppTextString = NULL;
        *pTextLength = 0;
    }
    else
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(GetAnalysisTextAtPosition(textPosition, false, ppTextString, pTextLength)));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetTextBeforePosition
//
//  Synopsis:
//      Get a block of text immediately preceding the specified position.
//
//  Returns:
//      Pointer to the first character at (textPosition - textLength).
//      NULL indicates no chunk available at the specified position, either
//      because textPosition == 0, the textPosition > the entire text content
//      length, or the queried position is not mapped into the app's backing
//      store.
//
//  Notes:
//      Although apps can implement sparse textual content that only maps part of
//      the backing store, the app must map any text that is in the range passed
//      to any analysis functions.
//
//---------------------------------------------------------------------------
HRESULT TextSegment::GetTextBeforePosition(
    _In_ XUINT32 textPosition,
        // Position immediately after the last position of the chunk to obtain.
    _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
        // Address that receives a pointer to the text block at the specified position.
    _Out_ XUINT32 *pTextLength
        // Number of UTF16 units of the retrieved block.
        // The length returned is from the given position to the front of
        // the block.
    )
{
    IFCEXPECT_ASSERT_RETURN(m_pFirstRunData != NULL);
    IFCEXPECT_ASSERT_RETURN(m_totalLength > 0);

    if (textPosition > m_totalLength || textPosition == 0)
    {
        *ppTextString = NULL;
        *pTextLength = 0;
    }
    else
    {
        IFC_RETURN(RichTextServicesHelper::MapTxErr(GetAnalysisTextAtPosition(textPosition, true, ppTextString, pTextLength)));
    }

    return S_OK;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetParagraphReadingDirection
//
//  Synopsis:
//      Get paragraph reading direction.
//
//---------------------------------------------------------------------------
FssReadingDirection::Enum TextSegment::GetParagraphReadingDirection()
{
    return (m_paragraphFlowDirection == RichTextServices::FlowDirection::LeftToRight) ? FssReadingDirection::LeftToRight : FssReadingDirection::RightToLeft;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetLocaleName
//
//  Synopsis:
//      Get locale name on the range affected by it.
//
//  Notes:
//      The localeName pointer must remain valid until the next call or until
//      the analysis returns.
//
//---------------------------------------------------------------------------
HRESULT TextSegment::GetLocaleName(
    _In_ XUINT32 textPosition,
        // Position to get the locale name of.
    _Out_ XUINT32 *pTextLength,
        // Receives the length from the given position up to the
        // next differing locale.
    _Outptr_result_z_ WCHAR const **ppLocaleName
        // Address that receives a pointer to the locale
        // at the specified position.
    )
{
    ASSERT(m_pFirstRunData != NULL);
    ASSERT(m_totalLength > 0);

    if (textPosition >= m_totalLength)
    {
        *ppLocaleName = NULL;
        *pTextLength = 0;
    }
    else
    {
        TextRun *pTextRun = GetTextRunAtPosition(textPosition);
        // TODO: Get locale name from TextRunProperties
        //*ppLocaleName = pTextRun->GetProperties()->GetLocaleName();
        *ppLocaleName = NULL;
        *pTextLength = pTextRun->GetLength() - (textPosition - m_characterIndexOfRecentRunData);
    }

    return S_OK;
}

HRESULT TextSegment::GetLocaleNameList(
    _In_ UINT32 textPosition,
    _Out_ UINT32* pTextLength,
    _Outptr_result_z_ WCHAR const** pplocaleNameList
    )
{

    *pplocaleNameList = NULL;
    *pTextLength = 0;

    ASSERT(FALSE); // we should never hit this code, GetLocalNameList will only be used for font fallback analysis
    return S_OK;
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetNumberSubstitution
//
//  Synopsis:
//      Get number substitution on the range affected by it.
//
//  Notes:
//      We don't release the IDWriteNumberSubstitution* object here. The reference will be released in DWrite code.
//
//---------------------------------------------------------------------------
HRESULT TextSegment::GetNumberSubstitution(
    _In_ XUINT32 textPosition,
    _Out_ XUINT32* pTextLength,
    _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
    )
{
    ASSERT(m_pFirstRunData != NULL);
    ASSERT(m_totalLength > 0);

    if (textPosition >= m_totalLength)
    {
        *pTextLength = 0;
        *ppNumberSubstitution = NULL;
    }
    else
    {
        TextRun *pTextRun = GetTextRunAtPosition(textPosition);
        if (pTextRun->GetType() == TextRunType::Text)
        {
            IDWriteNumberSubstitution* pNumberSubstitution = NULL;
            DWriteFontAndScriptServices *pDWriteFontServices = NULL;

            TextCharactersRun *pCharactersRun = reinterpret_cast<TextCharactersRun *>(pTextRun);

            pDWriteFontServices =
                static_cast<DWriteFontAndScriptServices*>(
                static_cast<PALFontAndScriptServices*>(m_pFontAndScriptServices)->GetPALFontAndScriptServices());
         
            IFC_RETURN(pDWriteFontServices->GetNumberSubstitution(
                pCharactersRun->GetProperties()->GetCultureInfo(),
                NULL,
                &pNumberSubstitution));

            *ppNumberSubstitution = pNumberSubstitution;
        }
        else
        {
            *ppNumberSubstitution = NULL;
        }
        *pTextLength = pTextRun->GetLength() - (textPosition - m_characterIndexOfRecentRunData);
    }

    return S_OK;
}
//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetTextRunAtPosition
//
//  Synopsis:
//      Returns a pointer to the run at a given text position.
//      This methods treats non-textual runs as if they were not there.
//
//---------------------------------------------------------------------------
TextRun * TextSegment::GetTextRunAtPosition(
    _In_ XUINT32 textPosition
        // Position to get the run node that contains it.
    ) const
{
    // These are the assumptions that must hold before calling this method:
    //  - There must be some textual content.
    //  - The given textPosition must be within the range of textual content.
    ASSERT(m_pFirstRunData != NULL);
    ASSERT(m_totalLength > 0);
    ASSERT(textPosition < m_totalLength);

    if (m_pRecentRunData == NULL)
    {
        m_pRecentRunData = m_pFirstRunData;
        m_characterIndexOfRecentRunData = 0;

        if (m_analyzingLineBreakpoints)
        {
            // During line breakpoint analysis we need to hide hidden runs from DWrite.
            while (SkipForLineBreakAnalysis(m_pRecentRunData->GetTextRun()))
            {
                m_pRecentRunData = m_pRecentRunData->GetNext();
            }
        }
    }

    // Since the assumptions at the beginning of the method are satisfied then there is no
    // way m_pRecentRunData can come out as NULL from the above loop.
    ASSERT(m_pRecentRunData != NULL);

    // Move forward through the collection of TextRunData.
    while (textPosition >= m_characterIndexOfRecentRunData + m_pRecentRunData->GetTextRun()->GetLength())
    {
        m_characterIndexOfRecentRunData += m_pRecentRunData->GetTextRun()->GetLength();

        ASSERT(m_pRecentRunData->GetNext());
        m_pRecentRunData = m_pRecentRunData->GetNext();

        if (m_analyzingLineBreakpoints)
        {
            // During line breakpoint analysis we need to hide hidden runs from DWrite.
            while (SkipForLineBreakAnalysis(m_pRecentRunData->GetTextRun()))
            {
                m_pRecentRunData = m_pRecentRunData->GetNext();
            }
        }
    }

    // Move backward through the collection of TextRunData.
    while (textPosition < m_characterIndexOfRecentRunData)
    {
        ASSERT(m_pRecentRunData->GetPrevious());
        m_pRecentRunData = m_pRecentRunData->GetPrevious();

        if (m_analyzingLineBreakpoints)
        {
            // During line breakpoint analysis we need to hide hidden runs from DWrite.
            while (SkipForLineBreakAnalysis(m_pRecentRunData->GetTextRun()))
            {
                m_pRecentRunData = m_pRecentRunData->GetPrevious();
            }
        }

        m_characterIndexOfRecentRunData -= m_pRecentRunData->GetTextRun()->GetLength();
    }

    // Since the assumptions at the beginning of the method are satisfied then there is no
    // way m_pRecentRunData can come out as NULL from the above loop.
    ASSERT(m_pRecentRunData);

    return m_pRecentRunData->GetTextRun();
}


//---------------------------------------------------------------------------
//
//  Method:
//     IsNumber
//
//  Synopsis:
//      Returns whether a character is a number.
//
//
//---------------------------------------------------------------------------
inline bool IsNumber(_In_ WCHAR character)
{
    return (character >= '0' && character <= '9');
}


//---------------------------------------------------------------------------
//
//  Method:
//     IsAscii
//
//  Synopsis:
//      Returns whether a character is an ASCII character 
//
//
//---------------------------------------------------------------------------
inline bool IsAscii(_In_ WCHAR character)
{
    return !(character & 0xFF80);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::AnalyzeComplexity
//
//  Synopsis:
//      Analyzes the text to determine the text ranges that require shaping 
//      and those that do not.
//
//---------------------------------------------------------------------------
Result::Enum TextSegment::AnalyzeComplexity(
    _In_ PALText::ITextAnalyzer *pTextAnalyzer
    )
{
    Result::Enum txhr = Result::Success;
    TextRunData *pRunData = m_pFirstRunData;

    while (pRunData != NULL)
    {
        if (pRunData->GetTextRun()->GetType() == TextRunType::Text)
        {
            TextCharactersRun *pTextRun = reinterpret_cast<TextCharactersRun *>(pRunData->GetTextRun());

            // We shape RTL Bidi to account for mirroring.
            // If the run has non default typographic featured applied to it then it is not safe to go 
            // through simple shaping and hence we default to full shaping.
            if (((pTextRun->GetProperties()->GetInheritedProperties() != NULL) 
                  && (!pTextRun->GetProperties()->GetInheritedProperties()->m_typography.IsTypographyDefault()))
                || (pRunData->GetBidiLevel() & 1))
            {
                pRunData->SetGlyphBased(TRUE);
            }
            else
            {
                const WCHAR *pCharacters = pTextRun->GetCharacters();
                XUINT32 runLength = pTextRun->GetLength();
                IFssFontFace *pFontFace = pRunData->GetFontFace();
                bool isTextSimple;
                XUINT32 textLengthRead;  
                TextRunData *pNextRunData;

                // Complexity Analysis must happen after font fallback
                ASSERT(pFontFace != NULL);

                IFC_FROM_HRESULT_RTS(pTextAnalyzer->GetTextComplexity(
                    pCharacters,
                    runLength,
                    pFontFace,
                    &isTextSimple,
                    &textLengthRead
                    ));
            
                if (isTextSimple)
                {
                    if(textLengthRead < runLength)
                    {
                        IFCTEXT(pRunData->Split(textLengthRead, &pNextRunData));
                        pNextRunData->SetGlyphBased(TRUE);
                        pRunData = pNextRunData;
                    }
                    if (pRunData->m_pNumberSubstitution)
                    {
                        pRunData->SetGlyphBased(TRUE);
                    }
                }
                else
                {
                    pRunData->SetGlyphBased(TRUE);
                }
            }
        }

        pRunData = pRunData->GetNext();
    }

Cleanup:
    return txhr;
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::AnalyzeFonts
//
//  Synopsis:
//      Analyzes the text to determine which fonts to use for the different
//      text ranges.
//
//---------------------------------------------------------------------------
Result::Enum TextSegment::AnalyzeFonts()
{
    Result::Enum txhr = Result::Success;
    TextRunData *pRunData = m_pFirstRunData;
    DWriteFontAndScriptServices *pDWriteFontServices = NULL;

    pDWriteFontServices =
    static_cast<DWriteFontAndScriptServices*>(
    static_cast<PALFontAndScriptServices*>(m_pFontAndScriptServices)->GetPALFontAndScriptServices());

    while (pRunData != NULL)
    {
        if (pRunData->GetTextRun()->GetType() == TextRunType::Text)
        {
            bool mappedAll = false;
            // Current position to begin mapping at relative to this run.  MapCharacters may not consume
            // all characters, so we'll call back a second time around with an advanced text position in
            // the same run.  We'll decide after the MapCharacters call if splitting needs to happen, and
            // this will be adjusted back after the split.
            XUINT32 alreadyMappedLength = 0;

            do
            {
                TextCharactersRun *pTextRun = reinterpret_cast<TextCharactersRun *>(pRunData->GetTextRun());
                CFontTypeface *pFontTypeface = pTextRun->GetProperties()->GetFontTypeface();
                const WCHAR *pCharacters = pTextRun->GetCharacters() + alreadyMappedLength;
                XUINT32 runLength = pTextRun->GetLength() - alreadyMappedLength;
                XUINT32 startPosition = alreadyMappedLength;

                IFssFontFace *pMappedFontFace;
                XUINT32 mappedLength;
                XFLOAT mappedScale;

                // Number Substitution
                // If a text run requires number substitution (the IDWriteNumberSubstitution* object is set on the run property)
                // We need to map the characters to its translated unicode code point based on the locale. 
                // This will ensure correct font fallback.
                NumberSubstitutionData* pMappingData = NULL;
                if (pRunData->m_pNumberSubstitution)
                {
                    IFC_FROM_HRESULT_RTS(pDWriteFontServices->GetNumberSubstitution(
                        pTextRun->GetProperties()->GetCultureInfo(),
                        &pMappingData,
                        NULL));
                }

                IFC_FROM_HRESULT_RTS(pFontTypeface->MapCharacters(
                    pCharacters,
                    runLength,
                    pTextRun->GetProperties()->GetCultureInfo().GetBuffer(),
                    pTextRun->GetProperties()->GetCultureListInfo().GetBuffer(),
                    pMappingData,
                    &pMappedFontFace,
                    &mappedLength,
                    &mappedScale
                    ));

                alreadyMappedLength += mappedLength;

                // If we already mapped part of this run, did the mapping return the same font?
                // If so, we can just keep the run all together.  If not, we need to split
                // out the previous chunk.

                IFssFontFace *pPreviousFontFace = pRunData->GetFontFace();
                XFLOAT previousScale = pRunData->GetFontScale();
                if (pPreviousFontFace && (!pPreviousFontFace->Equals(pMappedFontFace) || previousScale != mappedScale))
                {
                    TextRunData *pNextRunData;
                    IFC_FROM_HRESULT_RTS(pRunData->Split(startPosition, &pNextRunData));
                    pRunData = pNextRunData;
                    alreadyMappedLength -= startPosition;
                }

                pRunData->SetFontFace(pMappedFontFace);
                pRunData->SetFontScale(mappedScale);
                ReleaseInterface(pMappedFontFace);

                if (mappedLength == runLength)
                {
                    mappedAll = true;
                }
            } while (!mappedAll);
        }

        pRunData = pRunData->GetNext();
    }

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::BalanceBidiCharacterSpacing
//
//  Synopsis:
//
//      Sets TextRunData::initialSpace and TextRunData::spaceLastCharacter
//      to achieve balanced character spacing at direction boundaries in
//      bidirectional text.
//
//
//  Background:
//
//      Character spacing is implemented by modifying character advance widths.
//
//      The problem with this in a bidi environment is that it causes reverse
//      runs to be mispositioned:
//
//      Consider a left to right paragraph, and a junction from a left-to-right
//      run to a subsequent right-to-left run.
//
//      Assume that the whole paragraph has a positive character spacing
//      applied. We expect the extra spacing to appear balanced throughout the
//      line.
//
//      Both the last character of the left-to-right run, and the last character
//      of the right-to-left run will be extended.
//
//      Notice that the the trailing edges of the last character in each of these
//      runs is adjacent - marked by the '|' in the following example:
//
//          abcd|DCBA
//
//      Thus the extra space would appear twice at this point.
//
//      Now consider the junction between a right-to-left run and a subsequent
//      left-to-right run.
//
//      Notice that it is the leading edge of the first character of each of
//      these runs that is adjacent. Thus no extra space will appear between
//      these runs.
//
//      For example, if we start with both above cases like this:
//
//      abcd DCBA abcd
//
//      and set character spacing to about the width of a character, we would
//      like to see:
//
//      a b c d   D C B A   a b c d
//
//      but what we get is:
//
//      a b c d    D C B A  a b c d
//
//      The middle run is offset.
//
//      To resolve this issue, at any junction from a run against the paragraph
//      direction to a run with the paragraph direction, we transfer any
//      CharacterSpacing from the last cluster to the run (which is actually at
//      the leading end of the run in line direction) to the next direction
//      change boundary in line direction.
//
//      Since we're transferring the space from a direction change boundary at
//      which two run ends meet, the next direction change boundary will be the
//      meeting of two run starts.
//
//      We set the spacing as leadingSpace on the next run, which could be a
//      text run or an embedded element.
//
//      Note that since Silverlight does not support the FlowDirection property
//      on InlineUIElement, all inline objects are by design in the same
//      direction as the paragraph, so it is always correct to apply any
//      CharacterSpacing on an InlineUIElement by extending its width.
//
//
//---------------------------------------------------------------------------
void TextSegment::BalanceBidiCharacterSpacing()
{
    TextRunData *pRunData = m_pFirstRunData;
    TextRunData *pLastTextRun = NULL;
    XINT32 paragraphBidiLevel = (GetParagraphReadingDirection() == FssReadingDirection::RightToLeft) ? 1 : 0;

    while (pRunData != NULL)
    {
        switch (pRunData->GetTextRun()->GetType())
        {
            case TextRunType::EndOfLine:
            case TextRunType::EndOfParagraph:
                // Do not adjust the spacing of the last character/object of a line.
                if (pLastTextRun != NULL)
                {
                    pLastTextRun->SetSpaceLastCharacter(false);
                    pLastTextRun = NULL;
                }
                break;

            case TextRunType::Text:
            case TextRunType::Object:
                // Transfer trailing character space from the end of a reverse
                // run to initial space on the next run.
                if (pLastTextRun != NULL &&
                    (pRunData->GetBidiLevel() & 1) == paragraphBidiLevel &&
                    (pRunData->GetBidiLevel() & 1) != (pLastTextRun->GetBidiLevel() & 1))
                {
                    TextCharactersRun *pLastCharactersRun = reinterpret_cast<TextCharactersRun*>(pLastTextRun->GetTextRun());
                    if (pLastCharactersRun->GetProperties()->GetCharacterSpacing() != 0.0f)
                    {
                        // This run is in the para direction, last run was opposite,
                        // and last run has trailing character spacing to apply somewhere.

                        // Transfer character spacing from the last character of the
                        // previous run to initial space on this run.

                        pRunData->SetInitialSpacing(pLastCharactersRun->GetProperties()->GetCharacterSpacing());
                        pLastTextRun->SetSpaceLastCharacter(false);
                    }
                }

                if (pRunData->GetTextRun()->GetType() == TextRunType::Text)
                {
                    pLastTextRun = pRunData;
                }
                else
                {
                    pLastTextRun = NULL;
                }
                break;

            default:
                break;
        }

        pRunData = pRunData->GetNext();
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::SkipForLineBreakAnalysis
//
//  Synopsis:
//      Should this run be hidden from DWrite during line break analysis?
//
//---------------------------------------------------------------------------

bool TextSegment::SkipForLineBreakAnalysis(_In_ TextRun* pTextRun)
{
    TextRunType::Enum textRunType = pTextRun->GetType();
    return (textRunType == TextRunType::Hidden)
        || (textRunType == TextRunType::DirectionalControl);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::GetAnalysisTextAtPosition
//
//  Synopsis:
//      Gets text for analysis at the specified position.
//
//  Notes:
//      Analysis text returned depends on the type of TextRun at the position.
//      Some TextRun objects may return text directly, e.g. TextCharactersRun
//      of type TextRunType::Text. Other types may represent formatting or
//      directional control changes and will need to be represented by
//      Unicode control or replacement characters. TextSegment maintains
//      a 1:1 mapping between TextRun data and analysis text, so the length of
//      the segment is the sum of all the runs in it.
//
//---------------------------------------------------------------------------
Result::Enum TextSegment::GetAnalysisTextAtPosition(
    _In_ XUINT32 textPosition,
        // Position marking start or end of the requested text.
    _In_ bool getPreviousText,
        // Flag indicating direction in which text is to be fetched. If true,
        // textPosition marks the end of the text, if false, the start.
    _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
        // Address that receives the analysis text.
    _Out_ XUINT32 *pTextLength
        // Number of UTF16 units in the analysis text.
    )
{
    Result::Enum txhr = Result::Success;
    XUINT32 adjustedPosition = getPreviousText ?  (textPosition - 1) : textPosition;
    TextRun *pTextRun = GetTextRunAtPosition(adjustedPosition);

    ASSERT(!m_analyzingLineBreakpoints || !SkipForLineBreakAnalysis(pTextRun)); // We should have skipped hidden runs

    switch (pTextRun->GetType())
    {
        case TextRunType::Text:
        {
            TextCharactersRun *pCharactersRun = reinterpret_cast<TextCharactersRun *>(pTextRun);
            if (getPreviousText)
            {
                // Get text up to the textPosition..
                *ppTextString = pCharactersRun->GetCharacters();
                *pTextLength = textPosition - m_characterIndexOfRecentRunData;
            }
            else
            {
                // Get text starting at textPosition.
                *ppTextString = &(pCharactersRun->GetCharacters()[textPosition - m_characterIndexOfRecentRunData]);
                *pTextLength = pCharactersRun->GetLength() - (textPosition - m_characterIndexOfRecentRunData);
            }
            break;
        }

        case TextRunType::Hidden:
        case TextRunType::EndOfLine:
        case TextRunType::EndOfParagraph:
            // Return boundary neutral Control character.
            *ppTextString = &m_hidden;
            *pTextLength = 1;
            break;
            
        case TextRunType::Object:
            // Use boundary neutral control character in analysis so the object inherits
            // script/bidi behavior, and for line breaking use U+FFFC (object replacement character)
            // to get default "allow breaks" behavior.
            // FUTURE: ObjectRun occupies 2 character positions in the backing store but we only return
            //       one character at a time during text enumeration.  So DWrite will call back for
            //       index N and then index N+1, both times getting the same codepoint.  Ultimately we
            //       should probably move the text buffer from TextCharactersRun up to TextRun so that
            //       the enumeration code here is just collapsed to the trivial Text case above.
            //      
            *ppTextString = m_analyzingLineBreakpoints ? L"\xFFFC" : &m_hidden;
            *pTextLength = 1;
            break;

        case TextRunType::DirectionalControl:
        {
            DirectionalControlRun *pControlRun = reinterpret_cast<DirectionalControlRun *>(pTextRun);
            DirectionalControl::Enum control = pControlRun->GetDirectionalControl();
            ASSERT(control == DirectionalControl::LeftToRightEmbedding ||
                   control == DirectionalControl::RightToLeftEmbedding ||
                   control == DirectionalControl::LeftToRightMark ||
                   control == DirectionalControl::RightToLeftMark ||
                   control == DirectionalControl::PopDirectionalFormatting);

            if (control == DirectionalControl::LeftToRightEmbedding)
            {
                *ppTextString = &m_LRE;
            }
            else if (control == DirectionalControl::RightToLeftEmbedding)
            {
                *ppTextString = &m_RLE;
            }
            else if (control == DirectionalControl::LeftToRightMark)
            {
                *ppTextString = &m_LRM;
            }
            else if (control == DirectionalControl::RightToLeftMark)
            {
                *ppTextString = &m_RLM;
            }
            else if (control == DirectionalControl::PopDirectionalFormatting)
            {
                *ppTextString = &m_PDF;
            }
            *pTextLength = 1;
            break;
        }

        default:
            ASSERT(false);
            break;
    }

    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextSegment::AddRef()
{
    // No-op since lifetime is managed by TextRunCache, which guarantees availability during text analysis process.
    return 1;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextSegment::Release
//
//  Synopsis:
//      Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextSegment::Release()
{
    // No-op since lifetime is managed by TextRunCache, which guarantees availability during text analysis process.
    return 1;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineBreakpointAnalysisHelper::LineBreakpointAnalysisHelper
//
//  Synopsis:
//      Constructor, temporarily change state on TextSegment to adjust the
//      reports from the ITextAnalysisSource for this particular analysis.
//
//---------------------------------------------------------------------------

TextSegment::LineBreakpointAnalysisHelper::LineBreakpointAnalysisHelper(_In_ TextSegment* pOwner)
    : m_pOwner(pOwner)
{
    pOwner->m_analyzingLineBreakpoints = TRUE;

    // Reset run enumeration
    pOwner->m_pRecentRunData = NULL;
    
    m_oldTotalLength = pOwner->m_totalLength;
    pOwner->m_totalLength = pOwner->m_totalLengthForAnalyzingLineBreakpoints;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineBreakpointAnalysisHelper::~LineBreakpointAnalysisHelper
//
//  Synopsis:
//      Destructor, return TextSegment state back to what it was.
//
//---------------------------------------------------------------------------

TextSegment::LineBreakpointAnalysisHelper::~LineBreakpointAnalysisHelper()
{
    m_pOwner->m_totalLength = m_oldTotalLength;

    m_pOwner->m_analyzingLineBreakpoints = FALSE;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineBreakpointAnalysisHelper::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextSegment::LineBreakpointAnalysisHelper::AddRef()
{
    // No-op, this is only stack allocated.
    return 1;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineBreakpointAnalysisHelper::Release
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextSegment::LineBreakpointAnalysisHelper::Release()
{
    // No-op, this is only stack allocated.
    return 1;
}


//---------------------------------------------------------------------------
//
//  Member:
//      LineBreakpointAnalysisHelper::SetLineBreakpoints
//
//  Synopsis:
//      When line breakpoints are reported, copy them into the TextRunDatas.
//
//---------------------------------------------------------------------------

HRESULT TextSegment::LineBreakpointAnalysisHelper::SetLineBreakpoints(
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_reads_(textLength) PALText::LineBreakpoint const* lineBreakpoints
    )
{
    HRESULT hr = S_OK;
    
    // DWrite should call us back with the data all at once.
    ASSERT(textPosition == 0);
    // We shouldn't have done analysis if the length was 0.
    ASSERT(textLength > 0);

    PALText::LineBreakpoint breakpointsStackBuffer[512];
    PALText::LineBreakpoint* modifiedBreakpoints = (textLength > ARRAY_SIZE(breakpointsStackBuffer)) 
                                                    ? new PALText::LineBreakpoint[textLength] 
                                                    : breakpointsStackBuffer;
    memcpy(modifiedBreakpoints, lineBreakpoints, textLength * sizeof(PALText::LineBreakpoint));

    //
    // Using DWrite line break information in Line Services works well *except* for the scenario
    // where there are spaces across run boundaries.  Normally Line Services will only ask for
    // the "break before" condition on characters after spaces, but when the spaces cross run
    // boundaries they may ask for the "break after" condition on the character before all the
    // spaces and expect it to have the answer.  Unfortunately, that isn't quite the information
    // that DWrite gives us.  For example, with "A B" there is one break allowed -- the " "
    // has breakAfter == CanBreak and the "B" has breakBefore == CanBreak.  But LS will never
    // ask us about the spaces, and to handle the case where they sometimes ask us about the
    // "break after" condition we need to propagate back the CanBreak from the character
    // after the spaces to the one just before the spaces.
    //
    for (XUINT32 i = textLength - 1; i > 0;)
    {
        XUINT8 breakConditionBefore = modifiedBreakpoints[i].breakConditionBefore;
        if (   (breakConditionBefore == PALText::LineBreakingCondition::CanBreak)
            || (breakConditionBefore == PALText::LineBreakingCondition::MustBreak))
        {
            WCHAR const* pTextChunk = NULL;
            XUINT32 chunkLength = 0;
            while (--i > 0)
            {
                if (chunkLength == 0)
                {
                    m_pOwner->GetTextBeforePosition(i+1, &pTextChunk, &chunkLength);
                    ASSERT(chunkLength > 0);
                }

                if (!IsLSSpaceCharacter(pTextChunk[--chunkLength]))
                {
                    modifiedBreakpoints[i].breakConditionAfter = breakConditionBefore;
                    break;
                }
            }
        }
        else
        {
            i--;
        }
    }

    //
    // Copy line breakpoint data into the TextRunData(s).
    //
    TextRunData *pCurrentRunData = m_pOwner->m_pFirstRunData;
    for (XUINT32 current = 0; pCurrentRunData != NULL;)
    {
        // For runs that weren't hidden from DWrite, now stash the line breaking info on the TextRunData.
        if (!SkipForLineBreakAnalysis(pCurrentRunData->GetTextRun()))
        {
            IFC(pCurrentRunData->SetLineBreakpoints(modifiedBreakpoints + current, textLength - current));

            current += pCurrentRunData->GetTextRun()->GetLength();

            ASSERT(current <= textLength);
        }

        pCurrentRunData = pCurrentRunData->GetNext();
    }

Cleanup:
    if (modifiedBreakpoints != breakpointsStackBuffer)
    {
        delete[] modifiedBreakpoints;
    }

    return hr;
}

