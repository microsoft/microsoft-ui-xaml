// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextRunData.h"
#include "FontAndScriptServices.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::TextRunData
//
//  Synopsis:
//      Initializes a new instance of the TextRunData class.
//
//---------------------------------------------------------------------------
TextRunData::TextRunData(
    _In_ TextRun *pTextRun,
    _In_opt_ TextRunData *pPreviousRunData,
    _In_ XUINT8 bidiLevel
    ) :
    m_pNext(NULL),
    m_pPrevious(NULL),
    m_pTextRun(pTextRun),
    m_pFontFace(NULL),
    m_fontScale(1.0f),
    m_bidiLevel(bidiLevel),
    m_lineBreakpoints(NULL),
    m_glyphBased(false),
    m_initialSpacing(0),
    m_spaceLastCharacter(TRUE),
    m_pNumberSubstitution(NULL)
{
    m_scriptAnalysis.script = 0;
    m_scriptAnalysis.shapes = FssScriptShapes::Default;

    // Fixup double linked list if new TextRunData has been inserted in the middle of existing list.
    if (pPreviousRunData != NULL)
    {
        if (pPreviousRunData->m_pNext != NULL)
        {
            m_pNext = pPreviousRunData->m_pNext;
            m_pNext->m_pPrevious = this;
        }

        m_pPrevious = pPreviousRunData;
        m_pPrevious->m_pNext = this;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::TextRunData
//
//  Synopsis:
//      Initializes a new instance of the TextRunData class.
//
//  Notes:
//      TextRunData does not AddRef/Release GlyphTypeface, it assumes it is 
//      valid as long as TextCore and GlyphTypefaceCache are alive. 
//      GlyphTypfaceCache explicitly *deletes* all typefaces when it is torn
//      down, and since LsRuns are cached by text controls we can't guarantee
//      in what order they will be destroyed relative to the 
//      GlyphTypefaceCache. Releasing a GlyphTypeface after the 
//      typeface cache has deleted it will cause AV.
//
//---------------------------------------------------------------------------
TextRunData::TextRunData(
    _In_opt_ TextRunData *pPreviousRunData,
    _In_ TextRun *pTextRun,
    _In_opt_ IFssFontFace *pFontFace,
    _In_ XFLOAT fontScale,
    _In_ const FssScriptAnalysis &scriptAnalysis,
    _In_ XUINT8 bidiLevel,
    _In_ bool glyphBased
    ) :
    m_pNext(NULL),
    m_pPrevious(NULL),
    m_pTextRun(pTextRun),
    m_pFontFace(pFontFace),
    m_fontScale(fontScale),
    m_scriptAnalysis(scriptAnalysis),
    m_bidiLevel(bidiLevel),
    m_lineBreakpoints(NULL),
    m_glyphBased(!!glyphBased),
    m_initialSpacing(0),
    m_spaceLastCharacter(TRUE),
    m_pNumberSubstitution(NULL)
{
    AddRefInterface(m_pFontFace);

    // Fixup double linked list if new TextRunData has been inserted in the middle of existing list.
    if (pPreviousRunData != NULL)
    {
        if (pPreviousRunData->m_pNext != NULL)
        {
            m_pNext = pPreviousRunData->m_pNext;
            m_pNext->m_pPrevious = this;
        }

        m_pPrevious = pPreviousRunData;
        m_pPrevious->m_pNext = this;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::~TextRunData
//
//  Synopsis:
//      Release resources associated with the TextRunData.
//
//  Notes:
//      TextRunData does not AddRef/Release GlyphTypeface, it assumes it is 
//      valid as long as TextCore and GlyphTypefaceCache are alive. 
//      GlyphTypfaceCache explicitly *deletes* all typefaces when it is torn
//      down, and since LsRuns are cached by text controls we can't guarantee
//      in what order they will be destroyed relative to the 
//      GlyphTypefaceCache. Releasing a GlyphTypeface after the 
//      typeface cache has deleted it will cause AV.
//
//---------------------------------------------------------------------------
TextRunData::~TextRunData()
{
    ReleaseInterface(m_pTextRun);
    ReleaseInterface(m_pFontFace);
    ReleaseInterface(m_pNumberSubstitution);
    delete[] m_lineBreakpoints;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::Detach
//
//  Synopsis:
//      Detaches externally managed objects from the instance.
//
//---------------------------------------------------------------------------
void TextRunData::Detach()
{
    m_pTextRun  = NULL;

    // TODO: Can we safely assume that pFontFace will stay alive and hence not need to AddRef/Release it?
    //m_pFontFace = NULL;
    ReleaseInterface(m_pFontFace);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::Split
//
//  Synopsis:
//      Splits the run into two adjacent runs.
//
//---------------------------------------------------------------------------
Result::Enum TextRunData::Split(
    _In_ XUINT32 offset, 
    _Outptr_ TextRunData **ppRunData
    )
{
    Result::Enum txhr = Result::Success;
    TextRunData *pNewRunData = NULL;
    TextCharactersRun *pTextRun = NULL;
    TextCharactersRun *pNewTextRun = NULL;
    
    // Only TextCharactersRun should require splitting during analysis. 
    // Assert for safety before casting.
    ASSERT_EXPECT_RTS(m_pTextRun != NULL);
    ASSERT_EXPECT_RTS(m_pTextRun->GetType() == TextRunType::Text);
    // We should not have line breakpoint information yet.
    ASSERT(m_lineBreakpoints == NULL);
    pTextRun = reinterpret_cast<TextCharactersRun *>(m_pTextRun);

    IFCTEXT(pTextRun ->Split(offset, &pNewTextRun));

    IFC_OOM_RTS(pNewRunData = new TextRunData(
        this, 
        pNewTextRun, 
        m_pFontFace, 
        m_fontScale,
        m_scriptAnalysis, 
        m_bidiLevel, 
        m_glyphBased
    ));
    pNewTextRun = NULL;
    *ppRunData = pNewRunData;
    pNewRunData = NULL;

Cleanup:
    delete pNewRunData;
    ReleaseInterface(pNewTextRun);
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::SetAnalysisProperties
//
//  Synopsis:
//      Sets text analysis properties.
//
//---------------------------------------------------------------------------
void TextRunData::SetAnalysisProperties(
    _In_ const FssScriptAnalysis &scriptAnalysis,
    _In_ XUINT8 bidiLevel,
    _In_opt_ IDWriteNumberSubstitution* pNumberSubstitution
    )
{
    m_scriptAnalysis = scriptAnalysis;
    m_bidiLevel = bidiLevel;
    ReplaceInterface(m_pNumberSubstitution,pNumberSubstitution);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::SetLineBreakpoints
//
//  Synopsis:
//      Sets line breakpoint information.
//
//---------------------------------------------------------------------------
HRESULT TextRunData::SetLineBreakpoints(
    _In_reads_(breakpointsCount) PALText::LineBreakpoint const* lineBreakpoints,
    XUINT32 breakpointsCount
    )
{
    HRESULT hr = S_OK;
    
    XUINT32 textLength = m_pTextRun->GetLength();

    ASSERT(textLength <= breakpointsCount);
    ASSERT(m_lineBreakpoints == NULL);

    m_lineBreakpoints = new PALText::LineBreakpoint[textLength];
    memcpy(m_lineBreakpoints, lineBreakpoints, textLength * sizeof(PALText::LineBreakpoint));

    RRETURN(hr);//RRETURN_REMOVAL
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::SetFontFace
//
//  Synopsis:
//      Sets font properties.
//
//  Notes:
//      TextRunData does not AddRef/Release GlyphTypeface, it assumes it is 
//      valid as long as TextCore and GlyphTypefaceCache are alive. 
//      GlyphTypfaceCache explicitly *deletes* all typefaces when it is torn
//      down, and since LsRuns are cached by text controls we can't guarantee
//      in what order they will be destroyed relative to the 
//      GlyphTypefaceCache. Releasing a GlyphTypeface after the 
//      typeface cache has deleted it will cause AV.
//
//---------------------------------------------------------------------------
void TextRunData::SetFontFace(
    _In_ IFssFontFace *pFontFace
    )
{
    ReleaseInterface(m_pFontFace);
    m_pFontFace = pFontFace;
    AddRefInterface(m_pFontFace);
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::SetFontScale
//
//  Synopsis:
//      Sets font scaling factor
//
//  Notes:
//
//---------------------------------------------------------------------------
void TextRunData::SetFontScale(
    _In_ XFLOAT fontScale
    )
{
    m_fontScale = fontScale;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextRunData::SetGlyphBased
//
//  Synopsis:
//      Sets whether this run requires shaping or not.
//
//---------------------------------------------------------------------------
void TextRunData::SetGlyphBased(
    _In_ bool glyphBased
    )
{
    m_glyphBased |= !!glyphBased;
}

