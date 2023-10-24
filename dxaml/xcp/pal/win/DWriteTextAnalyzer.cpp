// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontAndScriptServices.h"
#include "DWriteTextAnalyzer.h"
#include "DWriteFontFace.h"
#include "DWriteFontCollection.h"
#include <TextAnalysis.h>

//---------------------------------------------------------------------------
//
//  TextAnalysisSinkProxy
//
//---------------------------------------------------------------------------
class TextAnalysisSinkProxy : public IDWriteTextAnalysisSink
{
public:
    TextAnalysisSinkProxy(_In_ PALText::ITextAnalysisSink *pTextAnalysisSink);

    ~TextAnalysisSinkProxy();

    HRESULT __stdcall SetScriptAnalysis(
        _In_ UINT32 textPosition,
        _In_ UINT32 textLength,
        _In_ DWRITE_SCRIPT_ANALYSIS const* pScriptAnalysis
        ) override;

    HRESULT __stdcall SetLineBreakpoints(
        _In_ UINT32 textPosition,
        _In_ UINT32 textLength,
        _In_reads_(textLength) DWRITE_LINE_BREAKPOINT const* pLineBreakpoints
        ) override;

    HRESULT __stdcall SetBidiLevel(
        _In_ UINT32 textPosition,
        _In_ UINT32 textLength,
        _In_ UINT8 explicitLevel,
        _In_ UINT8 resolvedLevel
        ) override;

    HRESULT __stdcall SetNumberSubstitution(
        _In_ UINT32 textPosition,
        _In_ UINT32 textLength,
        _Notnull_ IDWriteNumberSubstitution* pNumberSubstitution
        ) override;

    ULONG __stdcall AddRef() override;

    ULONG __stdcall Release() override;

    HRESULT __stdcall QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void** ppvObject
        ) override;

private:
    PALText::ITextAnalysisSink *m_pTextAnalysisSink;

    XUINT32 m_referenceCount;

};

TextAnalysisSinkProxy::TextAnalysisSinkProxy(
    _In_ PALText::ITextAnalysisSink *pTextAnalysisSink
    ) :
    m_pTextAnalysisSink(pTextAnalysisSink),
    m_referenceCount(1)
{
}

TextAnalysisSinkProxy::~TextAnalysisSinkProxy()
{
    ReleaseInterface(m_pTextAnalysisSink);
}

HRESULT TextAnalysisSinkProxy::SetScriptAnalysis(
    _In_ UINT32 textPosition,
    _In_ UINT32 textLength,
    _In_ DWRITE_SCRIPT_ANALYSIS const* pScriptAnalysis
    )
{
    return m_pTextAnalysisSink->SetScriptAnalysis(
        textPosition,
        textLength,
        reinterpret_cast<const PALText::ScriptAnalysis*>(pScriptAnalysis));
}

HRESULT TextAnalysisSinkProxy::SetLineBreakpoints(
    _In_ UINT32 textPosition,
    _In_ UINT32 textLength,
    _In_reads_(textLength) DWRITE_LINE_BREAKPOINT const* pLineBreakpoints
    )
{
    return m_pTextAnalysisSink->SetLineBreakpoints(
        textPosition,
        textLength,
        reinterpret_cast<PALText::LineBreakpoint const*>(pLineBreakpoints));
}

HRESULT TextAnalysisSinkProxy::SetBidiLevel(
    _In_ UINT32 textPosition,
    _In_ UINT32 textLength,
    _In_ UINT8 explicitLevel,
    _In_ UINT8 resolvedLevel
    )
{
    return m_pTextAnalysisSink->SetBidiLevel(textPosition, textLength, explicitLevel, resolvedLevel);
}

HRESULT TextAnalysisSinkProxy::SetNumberSubstitution(
    _In_ UINT32 textPosition,
    _In_ UINT32 textLength,
    _In_ IDWriteNumberSubstitution* pNumberSubstitution
    )
{
    return m_pTextAnalysisSink->SetNumberSubstitution(textPosition, textLength, pNumberSubstitution);
}

ULONG TextAnalysisSinkProxy::AddRef()
{
    return ++m_referenceCount;
}

ULONG TextAnalysisSinkProxy::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

HRESULT TextAnalysisSinkProxy::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void** ppObject
    )
{
    return E_UNEXPECTED;
}

//---------------------------------------------------------------------------
//
//  TextAnalysisSourceProxy
//
//---------------------------------------------------------------------------
class TextAnalysisSourceProxy : public ABI::Microsoft::Internal::FrameworkUdk::ITextFormatCallbacks
{
public:
    TextAnalysisSourceProxy(_In_ PALText::ITextAnalysisSource *pTextAnalysisSource);

    ~TextAnalysisSourceProxy();

    HRESULT __stdcall GetTextAtPosition(
        _In_ UINT32 textPosition,
        _Outptr_result_buffer_(*pTextLength) WCHAR const** ppTextString,
        _Out_ UINT32* pTextLength
        ) override;

    HRESULT __stdcall GetTextBeforePosition(
        _In_ UINT32 textPosition,
        _Outptr_result_buffer_(*pTextLength) WCHAR const** ppTextString,
        _Out_ UINT32* pTextLength
        ) override;

    DWRITE_READING_DIRECTION __stdcall GetParagraphReadingDirection() override;

    HRESULT __stdcall GetLocaleName(
        _In_ UINT32 textPosition,
        _Out_ UINT32* pTextLength,
        _Outptr_result_z_ WCHAR const** ppLocaleName
        ) override;

    HRESULT __stdcall GetNumberSubstitution(
        _In_ UINT32 textPosition,
        _Out_ UINT32* pTextLength,
        _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
        ) override;

    BOOL STDMETHODCALLTYPE SupportsGetLocaleNameList() override
    {
        return TRUE;
    }

    HRESULT __stdcall GetLocaleNameList(
        _In_ UINT32 textPosition,
        _Out_ UINT32* pTextLength,
        _Outptr_result_z_ WCHAR const** pplocaleNameList
        ) final;

    ULONG __stdcall AddRef() override;

    ULONG __stdcall Release() override;

    HRESULT __stdcall QueryInterface(
        _In_ REFIID riid,
        _Outptr_ void** ppvObject
        ) override;

private:
    PALText::ITextAnalysisSource *m_pTextAnalysisSource;

    XUINT32 m_referenceCount;

};

TextAnalysisSourceProxy::TextAnalysisSourceProxy(
    _In_ PALText::ITextAnalysisSource *pTextAnalysisSource
    ) :
    m_pTextAnalysisSource(pTextAnalysisSource),
    m_referenceCount(1)
{
}

TextAnalysisSourceProxy::~TextAnalysisSourceProxy()
{
    ReleaseInterface(m_pTextAnalysisSource);
}

HRESULT TextAnalysisSourceProxy::GetTextAtPosition(
    _In_ UINT32 textPosition,
    _Outptr_result_buffer_(*pTextLength) WCHAR const** ppTextString,
    _Out_ UINT32* pTextLength
    )
{
    return m_pTextAnalysisSource->GetTextAtPosition(textPosition, ppTextString, pTextLength);
}

HRESULT TextAnalysisSourceProxy::GetTextBeforePosition(
    _In_ UINT32 textPosition,
    _Outptr_result_buffer_(*pTextLength) WCHAR const** ppTextString,
    _Out_ UINT32* pTextLength
    )
{
    return m_pTextAnalysisSource->GetTextBeforePosition(textPosition, ppTextString, pTextLength);
}

DWRITE_READING_DIRECTION TextAnalysisSourceProxy::GetParagraphReadingDirection()
{
    return static_cast<DWRITE_READING_DIRECTION>(m_pTextAnalysisSource->GetParagraphReadingDirection());
}

HRESULT TextAnalysisSourceProxy::GetLocaleName(
    _In_ UINT32 textPosition,
    _Out_ UINT32* pTextLength,
    _Outptr_result_z_ WCHAR const** ppLocaleName
    )
{
    return m_pTextAnalysisSource->GetLocaleName(textPosition, pTextLength, ppLocaleName);
}

HRESULT TextAnalysisSourceProxy::GetNumberSubstitution(
    _In_ UINT32 textPosition,
    _Out_ UINT32* pTextLength,
    _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
    )
{
    return m_pTextAnalysisSource->GetNumberSubstitution(textPosition,pTextLength, ppNumberSubstitution);
}

HRESULT TextAnalysisSourceProxy::GetLocaleNameList(
    _In_ UINT32 textPosition,
    _Out_ UINT32* pTextLength,
    _Outptr_result_z_ WCHAR const** pplocaleNameList
    )
{
    return m_pTextAnalysisSource->GetLocaleNameList(textPosition, pTextLength, pplocaleNameList);
}

ULONG TextAnalysisSourceProxy::AddRef()
{
    return ++m_referenceCount;
}

ULONG TextAnalysisSourceProxy::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

HRESULT TextAnalysisSourceProxy::QueryInterface(
    _In_ REFIID riid,
    _Outptr_ void** ppObject
    )
{
    if (riid == __uuidof(IDWriteTextAnalysisSource) || riid == __uuidof(IUnknown))
    {
        *ppObject = static_cast<IDWriteTextAnalysisSource*>(this);
        AddRef();
        return S_OK;
    }

    *ppObject = NULL;
    return E_NOINTERFACE;
}

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteTextAnalyzer class.
//
//---------------------------------------------------------------------------
DWriteTextAnalyzer::DWriteTextAnalyzer(
    _In_ IDWriteTextAnalyzer *pDWriteTextAnalyzer
    ) :
    m_pDWriteTextAnalyzer(pDWriteTextAnalyzer),
    m_pDWriteTextAnalyzer1(NULL),
    m_referenceCount(1)
{
    IGNOREHR(m_pDWriteTextAnalyzer->QueryInterface(IID_PPV_ARGS(&m_pDWriteTextAnalyzer1)));
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DWriteTextAnalyzer.
//
//---------------------------------------------------------------------------
DWriteTextAnalyzer::~DWriteTextAnalyzer()
{
    ReleaseInterface(m_pDWriteTextAnalyzer);
    ReleaseInterface(m_pDWriteTextAnalyzer1);
}

//---------------------------------------------------------------------------
//
//  Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 DWriteTextAnalyzer::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
// Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 DWriteTextAnalyzer::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the DWriteTextAnalyzer class.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::Create(
    _In_ IDWriteFactory* pDWriteFactory,
    _Outptr_ ITextAnalyzer** ppTextAnalyzer
    )
{
    HRESULT hr = S_OK;
    IDWriteTextAnalyzer *pDWriteTextAnalyzer = NULL;

    IFCPTR(pDWriteFactory);
    IFCPTR(ppTextAnalyzer);
    IFC(pDWriteFactory->CreateTextAnalyzer(&pDWriteTextAnalyzer));
    *ppTextAnalyzer = new DWriteTextAnalyzer(pDWriteTextAnalyzer);
    pDWriteTextAnalyzer = NULL;

Cleanup:
    ReleaseInterface(pDWriteTextAnalyzer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the DWriteTextAnalyzer class.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::Create(
    _In_ IDWriteFactory* pDWriteFactory,
    _Outptr_ IGlyphAnalyzer** ppGlyphAnalyzer
    )
{
    HRESULT hr = S_OK;
    IDWriteTextAnalyzer *pDWriteTextAnalyzer = NULL;

    IFCPTR(pDWriteFactory);
    IFCPTR(ppGlyphAnalyzer);
    IFC(pDWriteFactory->CreateTextAnalyzer(&pDWriteTextAnalyzer));
    *ppGlyphAnalyzer = new DWriteTextAnalyzer(pDWriteTextAnalyzer);
    pDWriteTextAnalyzer = NULL;

Cleanup:
    ReleaseInterface(pDWriteTextAnalyzer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Creates and initializes a new instance of the DWriteTextAnalyzer class.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::Create(
    _In_ IDWriteFactory* pDWriteFactory,
    _Outptr_ IScriptAnalyzer** ppScriptAnalyzer
    )
{
    HRESULT hr = S_OK;
    IDWriteTextAnalyzer *pDWriteTextAnalyzer = NULL;

    IFCPTR(pDWriteFactory);
    IFCPTR(ppScriptAnalyzer);
    IFC(pDWriteFactory->CreateTextAnalyzer(&pDWriteTextAnalyzer));
    *ppScriptAnalyzer = new DWriteTextAnalyzer(pDWriteTextAnalyzer);
    pDWriteTextAnalyzer = NULL;

Cleanup:
    ReleaseInterface(pDWriteTextAnalyzer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Analyzes a text range for script boundaries.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::AnalyzeScript(
    _In_ PALText::ITextAnalysisSource *pAnalysisSource,
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ PALText::ITextAnalysisSink *pAnalysisSink
    )
{
    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    TextAnalysisSinkProxy textAnalysisSinkProxy(pAnalysisSink);
    return m_pDWriteTextAnalyzer->AnalyzeScript(&textAnalysisSourceProxy, textPosition, textLength, &textAnalysisSinkProxy);
}

//---------------------------------------------------------------------------
//
//  Analyzes a text range for script directionality.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::AnalyzeBidi(
    _In_ PALText::ITextAnalysisSource *pAnalysisSource,
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ PALText::ITextAnalysisSink *pAnalysisSink
    )
{
    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    TextAnalysisSinkProxy textAnalysisSinkProxy(pAnalysisSink);
    return m_pDWriteTextAnalyzer->AnalyzeBidi(&textAnalysisSourceProxy, textPosition, textLength, &textAnalysisSinkProxy);
}

//---------------------------------------------------------------------------
//
//  Analyzes a text range for number substitution.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::AnalyzeNumberSubstitution(
    _In_ PALText::ITextAnalysisSource *pAnalysisSource,
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ PALText::ITextAnalysisSink *pAnalysisSink
    )
{
    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    TextAnalysisSinkProxy textAnalysisSinkProxy(pAnalysisSink);
    return m_pDWriteTextAnalyzer->AnalyzeNumberSubstitution(&textAnalysisSourceProxy, textPosition, textLength, &textAnalysisSinkProxy);
}

//---------------------------------------------------------------------------
//
//  Analyzes a text range for line breakpoints.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::AnalyzeLineBreakpoints(
    _In_ PALText::ITextAnalysisSource *pAnalysisSource,
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ PALText::ITextAnalysisSink *pAnalysisSink
    )
{
    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    TextAnalysisSinkProxy textAnalysisSinkProxy(pAnalysisSink);
    return m_pDWriteTextAnalyzer->AnalyzeLineBreakpoints(&textAnalysisSourceProxy, textPosition, textLength, &textAnalysisSinkProxy);
}

//---------------------------------------------------------------------------
//
//  Determines the complexity of text, and whether or not full script
//  shaping needs to be called (GetGlyphs).
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::GetTextComplexity(
   _In_reads_(textLength) WCHAR const *pTextString,
    _In_  XUINT32 textLength,
    _In_  PALText::IFontFace *pFontFace,
    _Out_ bool *pIsTextSimple,
    _Out_ XUINT32 *pTextLengthRead
    )
{
    HRESULT hr = S_OK;
    DWriteFontFace *pDWriteFontFace = reinterpret_cast<DWriteFontFace*>(pFontFace);
    BOOL isTextSimple;

    // If m_pDWriteTextAnalyzer1 is NULL then we are running over an old
    // unsupported version of DWrite
    IFCEXPECT_ASSERT(m_pDWriteTextAnalyzer1);

    IFC(m_pDWriteTextAnalyzer1->GetTextComplexity(
        pTextString,
        textLength,
        pDWriteFontFace->GetFontFace(),
        &isTextSimple,
        pTextLengthRead,
        NULL
        ));

    *pIsTextSimple = !!isTextSimple;

Cleanup:
    return hr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      DWriteFontAndScriptServices::GetGlyphs
//
//  Synopsis:
//      Parses the input text string and maps it to the set of glyphs and associated
//      glyph data according to the font and the writing system's rendering rules.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::GetGlyphs(
    _In_reads_(textLength) WCHAR const *pTextString,
    _In_ XUINT32 textLength,
    _In_ PALText::IFontFace *pFontFace,
    _In_ bool isSideways,
    _In_ bool isRightToLeft,
    _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
    _In_opt_z_ WCHAR const* pLocaleName,
    _In_opt_ IDWriteNumberSubstitution* pNumberSubstitution,
    _In_reads_opt_(featureRanges) PALText::TypographicFeatures const **ppFeatures,
    _In_reads_opt_(featureRanges) XUINT32 const *pFeatureRangeLengths,
    _In_ XUINT32 featureRanges,
    _Out_writes_(textLength) XUINT16 *pClusterMap,
    _Out_writes_(textLength) PALText::ShapingTextProperties *pTextProps,
    _Outptr_result_buffer_(*pGlyphCount) XUINT16 **ppGlyphIndices,
    _Outptr_result_buffer_(*pGlyphCount) PALText::ShapingGlyphProperties **ppGlyphProps,
    _Out_ XUINT32 *pGlyphCount
    )
{
    HRESULT hr = S_OK;
    XUINT32 maxGlyphCount;
    XUINT32 actualGlyphCount;
    XUINT16* pGlyphIndices = NULL;
    PALText::ShapingGlyphProperties* pGlyphProps = NULL;
    DWriteFontFace *pDWriteFontFace = reinterpret_cast<DWriteFontFace*>(pFontFace);
    maxGlyphCount = textLength;

    for (XINT32 shapingTriesleft = 2; shapingTriesleft >= 0; shapingTriesleft--)
    {
        IFC(UInt32Mult(3, maxGlyphCount, &maxGlyphCount));
        maxGlyphCount /= 2;
        IFC(UInt32Add(maxGlyphCount, 16, &maxGlyphCount));

        SAFE_DELETE_ARRAY(pGlyphIndices);
        pGlyphIndices = new XUINT16[maxGlyphCount];

        SAFE_DELETE_ARRAY(pGlyphProps);
        pGlyphProps   = new PALText::ShapingGlyphProperties[maxGlyphCount];

        hr = m_pDWriteTextAnalyzer->GetGlyphs(
            pTextString,
            textLength,
            pDWriteFontFace->GetFontFace(),
            isSideways,
            isRightToLeft,
            reinterpret_cast<const DWRITE_SCRIPT_ANALYSIS*>(pScriptAnalysis),
            pLocaleName,
            pNumberSubstitution,
            reinterpret_cast<const DWRITE_TYPOGRAPHIC_FEATURES **>(ppFeatures),
            pFeatureRangeLengths,
            featureRanges,
            maxGlyphCount,
            pClusterMap,
            reinterpret_cast<DWRITE_SHAPING_TEXT_PROPERTIES*>(pTextProps),
            pGlyphIndices,
            reinterpret_cast<DWRITE_SHAPING_GLYPH_PROPERTIES*>(pGlyphProps),
            &actualGlyphCount);

        if (SUCCEEDED(hr))
            break;

        if (hr != E_NOT_SUFFICIENT_BUFFER || shapingTriesleft == 0)
        {
            IFC(hr);
        }
    }

    *pGlyphCount    = actualGlyphCount;
    *ppGlyphIndices = pGlyphIndices;
    *ppGlyphProps   = pGlyphProps;
    pGlyphIndices   = NULL;
    pGlyphProps     = NULL;

Cleanup:
    delete [] pGlyphIndices;
    delete [] pGlyphProps;

    return hr;
}

//---------------------------------------------------------------------------
//
//  Place glyphs output from the GetGlyphs method according to the font
//  and the writing system's rendering rules.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::GetGlyphPlacements(
    _In_reads_(textLength) WCHAR const *pTextString,
    _In_reads_(textLength) XUINT16 const *pClusterMap,
    _In_reads_(textLength) PALText::ShapingTextProperties *pTextProps,
    _In_ XUINT32 textLength,
    _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
    _In_reads_(glyphCount) PALText::ShapingGlyphProperties const *pGlyphProps,
    _In_ XUINT32 glyphCount,
    _In_ PALText::IFontFace *pFontFace,
    _In_ XFLOAT fontEmSize,
    _In_ bool isSideways,
    _In_ bool isRightToLeft,
    _In_ const TextOptions *pTextOptions,
    _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
    _In_opt_z_ WCHAR const *pLocaleName,
    _In_reads_opt_(featureRanges) PALText::TypographicFeatures const **ppFeatures,
    _In_reads_opt_(featureRanges) XUINT32 const *pFeatureRangeLengths,
    _In_ XUINT32 featureRanges,
    _Out_writes_(glyphCount) XFLOAT *pGlyphAdvances,
    _Out_writes_(glyphCount) PALText::GlyphOffset *pGlyphOffsets
    )
{
    DWriteFontFace *pDWriteFontFace = reinterpret_cast<DWriteFontFace*>(pFontFace);

    return m_pDWriteTextAnalyzer->GetGlyphPlacements(
        pTextString,
        pClusterMap,
        reinterpret_cast<DWRITE_SHAPING_TEXT_PROPERTIES*>(pTextProps),
        textLength,
        pGlyphIndices,
        reinterpret_cast<const DWRITE_SHAPING_GLYPH_PROPERTIES*>(pGlyphProps),
        glyphCount,
        pDWriteFontFace->GetFontFace(),
        fontEmSize,
        isSideways,
        isRightToLeft,
        reinterpret_cast<const DWRITE_SCRIPT_ANALYSIS*>(pScriptAnalysis),
        pLocaleName,
        reinterpret_cast<const DWRITE_TYPOGRAPHIC_FEATURES**>(ppFeatures),
        pFeatureRangeLengths,
        featureRanges,
        pGlyphAdvances,
        reinterpret_cast<DWRITE_GLYPH_OFFSET*>(pGlyphOffsets));
}

//---------------------------------------------------------------------------
//
//  Applies spacing between characters, properly adjusting glyph clusters
//  and diacritics.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::ApplyCharacterSpacing(
    _In_ XFLOAT characterSpacing,
    _In_ bool spaceLastCharacter,
    _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
    _In_ XUINT32 textLength,
    _In_reads_(textLength) WCHAR const *pTextString,
    _In_reads_(textLength) XUINT16 const *pClusterMap,
    _In_ XUINT32 glyphCount,
    _In_reads_(glyphCount) PALText::ShapingGlyphProperties const *pGlyphProps,
    _Inout_updates_(glyphCount) XFLOAT *pGlyphAdvances,
    _Inout_updates_(glyphCount) PALText::GlyphOffset *pGlyphOffsets
    )
{
    HRESULT hr = S_OK;
    XUINT32 glyphIndex = 0;
    XFLOAT offsetAdjustment = 0.0f;
    bool *pSpaceabilityMap = NULL;

    IFCEXPECT(textLength != 0);

    pSpaceabilityMap = new bool[glyphCount]();
    IFC(GetCharacterSpaceability(pScriptAnalysis, textLength, pTextString, pClusterMap, glyphCount, pGlyphProps, pSpaceabilityMap));

    // Iterate character spacing adjustment right before the last character
    while (glyphIndex < pClusterMap[textLength-1])
    {
        ASSERT(glyphIndex < glyphCount);
        if (pSpaceabilityMap[glyphIndex])
        {
            if (pGlyphAdvances[glyphIndex] >= -characterSpacing)
            {
                offsetAdjustment = characterSpacing;
                pGlyphAdvances[glyphIndex] += characterSpacing;
            }
            else
            {
                offsetAdjustment = 0.0f;
                pGlyphAdvances[glyphIndex] = 0.0f;
            }

            // Adjust any following zero-width glyphs by a compensating amount,
            // that way marks like diacritics stay aligned to their base character,
            // whether in the same cluster or even if they're in a different
            // cluster due to partial font fallback. One example would be a base
            // character in Algerian and an unsupported diacritic in Segoe UI,
            // which doesn't align the diacritic well to the base anyway, but you
            // still don't want a gap introduced between the base and diacritic.
            for (++glyphIndex; glyphIndex < pClusterMap[textLength-1] && !pSpaceabilityMap[glyphIndex]; ++glyphIndex)
            {
                ASSERT(glyphIndex < glyphCount);
                if (pGlyphAdvances[glyphIndex] != 0.0f)
                    break;
                pGlyphOffsets[glyphIndex].AdvanceOffset -= offsetAdjustment;
            }
        }
        else
        {
            ++glyphIndex;
        }
    }

    // Override the spaceability of the last character
    if (spaceLastCharacter)
    {
        ASSERT(glyphIndex < glyphCount);
        if (pGlyphAdvances[glyphIndex] >= -characterSpacing)
        {
            offsetAdjustment = characterSpacing;
            pGlyphAdvances[glyphIndex] += characterSpacing;
        }
        else
        {
            offsetAdjustment = 0.0f;
            pGlyphAdvances[glyphIndex] = 0.0f;
        }

        // Adjust any following zero-width glyphs by a compensating amount,
        // that way marks like diacritics stay aligned to their base character,
        // whether in the same cluster or even if they're in a different
        // cluster due to partial font fallback. One example would be a base
        // character in Algerian and an unsupported diacritic in Segoe UI,
        // which doesn't align the diacritic well to the base anyway, but you
        // still don't want a gap introduced between the base and diacritic.
        for (++glyphIndex; glyphIndex < glyphCount; ++glyphIndex)
        {
            if (pGlyphAdvances[glyphIndex] != 0.0f)
                break;
            pGlyphOffsets[glyphIndex].AdvanceOffset -= offsetAdjustment;
        }
    }

Cleanup:
    delete [] pSpaceabilityMap;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Checks whether character spacing can be applied to a given script.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::GetCharacterSpaceability(
    _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
    _In_ XUINT32 textLength,
    _In_reads_(textLength) WCHAR const *pTextString,
    _In_reads_(textLength) XUINT16 const *pClusterMap,
    _In_ XUINT32 glyphCount,
    _In_reads_(glyphCount) PALText::ShapingGlyphProperties const *pGlyphProps,
    _Inout_updates_(glyphCount) bool *pSpaceabilityMap
    )
{
    HRESULT hr = S_OK;
    DWRITE_JUSTIFICATION_OPPORTUNITY *pJustificationOpportunities = NULL;
    IDWriteTextAnalyzer1 *pTextAnalyzer = NULL;

    IFCPTR(pSpaceabilityMap);
    pJustificationOpportunities = new DWRITE_JUSTIFICATION_OPPORTUNITY[glyphCount];

    IFC(m_pDWriteTextAnalyzer->QueryInterface(IID_PPV_ARGS(&pTextAnalyzer)));
    IFC(pTextAnalyzer->GetJustificationOpportunities(
        NULL,
        0,
        *(reinterpret_cast<const DWRITE_SCRIPT_ANALYSIS*>(pScriptAnalysis)),
        textLength,
        glyphCount,
        pTextString,
        pClusterMap,
        reinterpret_cast<const DWRITE_SHAPING_GLYPH_PROPERTIES*>(pGlyphProps),
        pJustificationOpportunities));

    for (XUINT32 i=0; i < glyphCount; i++)
    {
        pSpaceabilityMap[i] = (pJustificationOpportunities[i].expansionPriority > 0)        // no spacing expansion with priority zero
                                && (pJustificationOpportunities[i].applyToTrailingEdge)     // mostly true for spaceable glyphs
                                && (pJustificationOpportunities[i].expansionMinimum == 0);  // typically zero except for kashida
    }

    // Ogham space can be either blank or non-blank, depending on font. In fact, it's non-blank
    // space in the default font for Ogham (which is Segoe UI Symbol). So, we override the
    // spaceability map as non-spaceable if the corresponding character is Ogham space mark.
    for (XUINT32 i=0; i < textLength; i++)
    {
        if ((pTextString[i] == 0x1680  /* Ogham space mark */) && (pClusterMap[i] < glyphCount))
        {
            pSpaceabilityMap[pClusterMap[i]] = FALSE;
        }
    }

Cleanup:
    ReleaseInterface(pTextAnalyzer);
    delete [] pJustificationOpportunities;
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Checks whether we should restrict the caret to whole clusters,
//  like Thai and Devanagari. Scripts such as Arabic by default allow
//  navigation between clusters. Others like Thai always navigate
//  across whole clusters.
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::IsRestrictCaretToClusters(
    _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
    _Out_ bool *pRestrictCaretToClusters
    )
{
    HRESULT hr = S_OK;
    DWRITE_SCRIPT_PROPERTIES scriptProperties;
    IDWriteTextAnalyzer1 *pTextAnalyzer = NULL;

    IFCPTR(pRestrictCaretToClusters);
    IFC(m_pDWriteTextAnalyzer->QueryInterface(IID_PPV_ARGS(&pTextAnalyzer)));
    IFC(pTextAnalyzer->GetScriptProperties(
        *(reinterpret_cast<const DWRITE_SCRIPT_ANALYSIS*>(pScriptAnalysis)),
        &scriptProperties));
    *pRestrictCaretToClusters = scriptProperties.restrictCaretToClusters;

Cleanup:
    ReleaseInterface(pTextAnalyzer);
    RRETURN(hr);
}

//---------------------------------------------------------------------------
//
//  Determines the reading order of the content
//
//---------------------------------------------------------------------------
HRESULT DWriteTextAnalyzer::GetContentReadingDirection(
    _In_ PALText::ITextAnalysisSource *pAnalysisSource,
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _Out_ PALText::ReadingDirection::Enum *pReadingDirection,
    _Out_ bool *pIsAmbiguousReadingDirection
    )
{
    HRESULT hr = S_OK;
    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    DWRITE_READING_DIRECTION dwriteReadingDirection = DWRITE_READING_DIRECTION_LEFT_TO_RIGHT;
    BOOL isAmbiguousReadingDirection = false;

    IFC(::TextAnalysis_GetContentReadingDirection(
        m_pDWriteTextAnalyzer,
       &textAnalysisSourceProxy,
        textPosition,
        textLength,
       &dwriteReadingDirection,
       &isAmbiguousReadingDirection));

    // Jupiter only handles LTR and RTL.  Default to LTR and return RTL reading order iff GetContentReadingDirection returns RTL.
    *pReadingDirection = (dwriteReadingDirection == DWRITE_READING_DIRECTION_RIGHT_TO_LEFT) ? PALText::ReadingDirection::Enum::RightToLeft : PALText::ReadingDirection::Enum::LeftToRight;
    *pIsAmbiguousReadingDirection = !!isAmbiguousReadingDirection;
Cleanup:
    RRETURN(hr);
}

class FontFallbackBuilderWrapper : public CXcpObjectBase<PALText::IFontFallbackBuilder, CXcpObjectAddRefPolicy>
{
public:
    FontFallbackBuilderWrapper(IDWriteFontFallbackBuilder* pFontFallbackBuilder)
    {
        m_pFontFallbackBuilder = pFontFallbackBuilder;
        AddRefInterface(m_pFontFallbackBuilder);
    }

    ~FontFallbackBuilderWrapper() override
    {
        ReleaseInterface(m_pFontFallbackBuilder);
    }

    HRESULT AddMapping(
        _In_reads_(rangesCount) PALText::UNICODE_RANGE const* ranges,
        UINT32 rangesCount,
        _In_reads_(targetFamilyNamesCount) WCHAR const** targetFamilyNames,
        UINT32 targetFamilyNamesCount,
        _In_opt_ PALText::IFontCollection* fontCollection = NULL,
        _In_opt_z_ WCHAR const* localeName = NULL,
        _In_opt_z_ WCHAR const* baseFamilyName = NULL,
        FLOAT scale = 1.0f
        ) override
    {
        return m_pFontFallbackBuilder->AddMapping(
            reinterpret_cast<DWRITE_UNICODE_RANGE const*>(ranges),
            rangesCount,
            targetFamilyNames,
            targetFamilyNamesCount,
            fontCollection ? reinterpret_cast<DWriteFontCollection*>(fontCollection)->m_pDWriteFontCollection : NULL,
            localeName,
            baseFamilyName,
            scale
            );
    }

    HRESULT AddMappings(
        PALText::IFontFallback* pFontFallback
        ) override
    {
        return m_pFontFallbackBuilder->AddMappings(
            reinterpret_cast<FontFallbackWrapper*>(pFontFallback)->m_pFontFallback);
    }

    HRESULT CreateFontFallback(
        _COM_Outptr_ PALText::IFontFallback** ppFontFallback
        ) override
    {
        HRESULT hr = S_OK;
        IDWriteFontFallback *pFontFallback = NULL;

        IFC(m_pFontFallbackBuilder->CreateFontFallback(&pFontFallback));

        *ppFontFallback = new FontFallbackWrapper(pFontFallback);

    Cleanup:
        ReleaseInterface(pFontFallback);
        RRETURN(hr);
    }

private:
    IDWriteFontFallbackBuilder *m_pFontFallbackBuilder;
};


HRESULT FontFallbackWrapper::MapCharacters(
    PALText::ITextAnalysisSource* pAnalysisSource,
    XUINT32 textPosition,
    XUINT32 textLength,
    _In_opt_ PALText::IFontCollection* pBaseFontCollection,
    _In_opt_z_ wchar_t const* pBaseFamilyName,
    XUINT32 baseWeight,
    XUINT32 baseStyle,
    XUINT32 baseStretch,
    _Deref_out_range_(0, textLength) UINT32* pMappedLength,
    _COM_Outptr_ PALText::IFontFace** ppMappedFont,
    _Out_ FLOAT* pScale
    )
{
    HRESULT hr = S_OK;

    TextAnalysisSourceProxy textAnalysisSourceProxy(pAnalysisSource);
    IDWriteFont *pDWriteMappedFont {};

    IDWriteTextAnalysisSource* proxy;
    IFC(TextAnalysis_CreateDWritePrivateTextAnalysisSourceProxy(&textAnalysisSourceProxy, &proxy));

    *ppMappedFont = NULL;

    IFC(m_pFontFallback->MapCharacters(
        proxy, //&textAnalysisSourceProxy,
        textPosition,
        textLength,
        pBaseFontCollection ? reinterpret_cast<DWriteFontCollection*>(pBaseFontCollection)->m_pDWriteFontCollection : NULL,
        pBaseFamilyName,
        static_cast<DWRITE_FONT_WEIGHT>(baseWeight),
        static_cast<DWRITE_FONT_STYLE>(baseStyle),
        static_cast<DWRITE_FONT_STRETCH>(baseStretch),
        pMappedLength,
        &pDWriteMappedFont,
        pScale));

    if (pDWriteMappedFont != NULL)
    {
        *ppMappedFont = new DWriteFontFace(pDWriteMappedFont);
    }

Cleanup:
    ReleaseInterface(pDWriteMappedFont);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:   GetSystemFontFallback
//
//------------------------------------------------------------------------

HRESULT DWriteFontAndScriptServices::GetSystemFontFallback(
    _Outptr_ PALText::IFontFallback **ppFontFallback
)
{
    HRESULT hr = S_OK;
    IDWriteFontFallback *pFontFallback = NULL;

    IFC(m_dwriteFactory->GetSystemFontFallback(&pFontFallback));

    *ppFontFallback = new FontFallbackWrapper(pFontFallback);

Cleanup:
    ReleaseInterface(pFontFallback);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:   CreateFontFallbackBuilder
//
//------------------------------------------------------------------------

HRESULT DWriteFontAndScriptServices::CreateFontFallbackBuilder(
    _Outptr_ PALText::IFontFallbackBuilder **ppFontFallbackBuilder
)
{
    HRESULT hr = S_OK;
    IDWriteFontFallbackBuilder *pFontFallbackBuilder = NULL;

    IFC(m_dwriteFactory->CreateFontFallbackBuilder(&pFontFallbackBuilder));

    *ppFontFallbackBuilder = new FontFallbackBuilderWrapper(pFontFallbackBuilder);

Cleanup:
    ReleaseInterface(pFontFallbackBuilder);
    RRETURN(hr);
}

