// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DWrite based implementaiton of ITextAnalyzer.

#pragma once

//---------------------------------------------------------------------------
//
//  DWriteTextAnalyzer
//
//  Analyzes various text properties for complex script processing.
//
//---------------------------------------------------------------------------

#include <dwrite_2.h>

class DWriteTextAnalyzer final : public PALText::ITextAnalyzer, public PALText::IGlyphAnalyzer, public PALText::IScriptAnalyzer
{
public:
    // Creates and initializes a new instance of the DWriteTextAnalyzer class.
    static HRESULT Create(
        _In_ IDWriteFactory *pDWriteFactory,
        _Outptr_ ITextAnalyzer **ppTextAnalyzer
        );

    // Creates and initializes a new instance of the DWriteTextAnalyzer class.
    static HRESULT Create(
        _In_ IDWriteFactory *pDWriteFactory,
        _Outptr_ IGlyphAnalyzer **ppGlyphAnalyzer
        );

    // Creates and initializes a new instance of the DWriteTextAnalyzer class.
    static HRESULT Create(
        _In_ IDWriteFactory *pDWriteFactory,
        _Outptr_ IScriptAnalyzer **ppScriptAnalyzer
        );

    // Analyzes a text range for script boundaries.
    HRESULT AnalyzeScript(
        _In_ PALText::ITextAnalysisSource *pAnalysisSource,
        _In_ XUINT32 textPosition,
        _In_ XUINT32 textLength,
        _In_ PALText::ITextAnalysisSink *pAnalysisSink
        ) override;

    // Analyzes a text range for script directionality.
    HRESULT AnalyzeBidi(
        _In_ PALText::ITextAnalysisSource *pAnalysisSource,
        _In_ XUINT32 textPosition,
        _In_ XUINT32 textLength,
        _In_ PALText::ITextAnalysisSink *pAnalysisSink
        ) override;

    // Analyzes a text range for number substitution.
    HRESULT AnalyzeNumberSubstitution(
        _In_ PALText::ITextAnalysisSource *pAnalysisSource,
        _In_ XUINT32 textPosition,
        _In_ XUINT32 textLength,
        _In_ PALText::ITextAnalysisSink *pAnalysisSink
        ) override;

    // Analyzes a text range for line-break opportunities.
    HRESULT AnalyzeLineBreakpoints(
        _In_ PALText::ITextAnalysisSource *pAnalysisSource,
        _In_ XUINT32 textPosition,
        _In_ XUINT32 textLength,
        _In_ PALText::ITextAnalysisSink *pAnalysisSink
        ) override;

    // Determines the complexity of text, and whether or not full script
    // shaping needs to be called (GetGlyphs).
    HRESULT GetTextComplexity(
        _In_reads_(textLength) WCHAR const *pTextString,
        _In_  XUINT32 textLength,
        _In_  PALText::IFontFace *pFontFace,
        _Out_ bool *pIsTextSimple,
        _Out_ XUINT32 *pTextLengthRead
        ) override;

    // Parses the input text string and maps it to the set of glyphs and associated
    // glyph data according to the font and the writing system's rendering rules.
    HRESULT GetGlyphs(
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
        ) override;

    // Place glyphs output from the GetGlyphs method according to the font
    // and the writing system's rendering rules.
    HRESULT GetGlyphPlacements(
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
        ) override;

    // Applies spacing between characters, properly adjusting glyph clusters
    // and diacritics.
    HRESULT ApplyCharacterSpacing(
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
        ) override;

    // Checks whether character spacing can be applied to a given script.
    HRESULT GetCharacterSpaceability(
        _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
        _In_ XUINT32 textLength,
        _In_reads_(textLength) WCHAR const *pTextString,
        _In_reads_(textLength) XUINT16 const *pClusterMap,
        _In_ XUINT32 glyphCount,
        _In_reads_(glyphCount) PALText::ShapingGlyphProperties const *pGlyphProps,
        _Inout_updates_(glyphCount) bool *pIsSpaceableScript
        ) override;

    // Checks whether we should restrict the caret to whole clusters,
    // like Thai and Devanagari. Scripts such as Arabic by default allow
    // navigation between clusters. Others like Thai always navigate
    // across whole clusters.
    HRESULT IsRestrictCaretToClusters(
        _In_ PALText::ScriptAnalysis const *pScriptAnalysis,
        _Out_ bool *pRestrictCaretToClusters
        ) override;

    // Determines the most appropriate reading order based on the content
    HRESULT GetContentReadingDirection(
        _In_ PALText::ITextAnalysisSource *pAnalysisSource,
        _In_ XUINT32 textPosition,
        _In_ XUINT32 textLength,
        _Out_ PALText::ReadingDirection::Enum *readingDirection,
        _Out_ bool *isAmbiguousReadingDirection
    ) override;

    // Increments the count of references to this object.
    XUINT32 AddRef() override;

    // Decrements the count of references to this object.
    XUINT32 Release() override;

private:

    // DWrite peer.
    IDWriteTextAnalyzer *m_pDWriteTextAnalyzer;

    IDWriteTextAnalyzer1 *m_pDWriteTextAnalyzer1;


    XUINT32 m_referenceCount;

    // Initializes a new instance of the DWriteTextAnalyzer class.
    DWriteTextAnalyzer(_In_ IDWriteTextAnalyzer *pDWriteTextAnalyzer);

    // Release resources associated with the DWriteTextAnalyzer.
    ~DWriteTextAnalyzer();
};

class FontFallbackWrapper : public CXcpObjectBase<PALText::IFontFallback, CXcpObjectAddRefPolicy>
{
public:
    FontFallbackWrapper(IDWriteFontFallback1 *pFontFallback)
    {
        Microsoft::WRL::ComPtr<IDWriteFontFallback1> fontFallback(pFontFallback);
        fontFallback.Swap(m_fontFallback);
    }

    static HRESULT Create(IDWriteFontFallback* pFontFallback, IFontFallback** ppFontFallbackWrapper)
    {
        Microsoft::WRL::ComPtr<IDWriteFontFallback1> fontFallback;
        IFC_RETURN(pFontFallback->QueryInterface(IID_PPV_ARGS(fontFallback.ReleaseAndGetAddressOf())));
        *ppFontFallbackWrapper = new FontFallbackWrapper(fontFallback.Get());
        return S_OK;
    }

    HRESULT MapCharacters(
        PALText::ITextAnalysisSource* pAnalysisSource,
        XUINT32 textPosition,
        XUINT32 textLength,
        _In_opt_ PALText::IFontCollection* pBaseFontCollection,
        _In_opt_z_ wchar_t const* pBaseFamilyName,
        XUINT32 baseWeight,
        XUINT32 baseStyle,
        XUINT32 baseStretch,
        FLOAT opticalSize,
        _Deref_out_range_(0, textLength) UINT32* pMappedLength,
        _COM_Outptr_ PALText::IFontFace** ppMappedFont,
        _Out_ FLOAT* pScale
        ) override;

private:
    friend class FontFallbackBuilderWrapper;
    friend class CTextBlock;
    Microsoft::WRL::ComPtr<IDWriteFontFallback1> m_fontFallback;
};

