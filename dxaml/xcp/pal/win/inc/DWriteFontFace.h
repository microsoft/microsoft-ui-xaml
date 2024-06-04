// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      DWrite based implementation of IFontFace.

#pragma once

class DWriteFontFamily;
interface IDWriteFontFace4;

//---------------------------------------------------------------------------
//
//  DWriteFontFace
//
//      DWrite based implementation of IFontFace.
//
//---------------------------------------------------------------------------
class DWriteFontFace : public CXcpObjectBase<PALText::IFontFace, CXcpObjectAddRefPolicy>
{
public:
    // Initializes a new instance of the DWriteFontFace class.
    DWriteFontFace(
        _In_ IDWriteFontFace *pDWriteFontFace,
        _In_opt_ DWriteFontFamily *pDWriteFontFamily
        );

    DWriteFontFace(
        _In_ IDWriteFont *pDWriteFont
        );

    // Returns true if the font face is either TrueType or TrueTypeCollection.
    bool HasTrueTypeOutlines() override;

    // Gets the OpenType Offset for the font.
    HRESULT TryGetFontOffset(
        _Outptr_result_bytebuffer_(*pcOffset) const void **ppOffset,
        _Out_ XUINT32 *pcOffset,
        _Out_ bool *pExists
        ) override;

    // Determines whether two FontFaces are equal.
    bool Equals(
        _In_ IFontFace *pFontFace
        ) override;

    // Obtains the algorithmic style simulation flags of a font face.
    PALText::FontSimulations::Enum GetSimulations() override;

    // Determines whether the font is a symbol font.
    bool IsSymbolFont() override;

    // Obtains design units and common metrics for the font face.
    void GetMetrics(
        _Out_ PALText::FontMetrics* pFontFaceMetrics
        ) override;

    // Obtains the number of glyphs in the font face.
    XUINT16 GetGlyphCount() override;

    // Obtains ideal glyph metrics in font design units.
    HRESULT GetDesignGlyphMetrics(
        _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
        _In_ XUINT32 glyphCount,
        _Out_writes_(glyphCount) PALText::GlyphMetrics *pGlyphMetrics,
        _In_ bool isSideways
        ) override;

    // Returns the nominal mapping of UTF-32 Unicode code points to glyph indices
    // as defined by the font 'cmap' table.
    virtual HRESULT GetGlyphIndices(
        _In_reads_(codePointCount) XUINT32 const *pCodePoints,
        _In_ XUINT32 codePointCount,
        _Out_writes_(codePointCount) XUINT16 *pGlyphIndices
        );

    // Finds the specified OpenType font table if it exists and returns a pointer to it.
    HRESULT TryGetFontTable(
        _In_ XUINT32 openTypeTableTag,
        _Outptr_result_bytebuffer_(*pTableSize) const void **ppTableData,
        _Out_ XUINT32 *pTableSize,
        _Outptr_ void **ppTableContext,
        _Out_ bool *pExists
        ) override;

    // Returns true if the font is monospaced
    bool IsMonospacedFont() override;

    // Returns the advances in design units for a sequences of glyphs.
    HRESULT GetDesignGlyphAdvances(
        _In_ XUINT32 glyphCount,
        _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
        _Out_writes_(glyphCount) XINT32 *pGlyphAdvances,
        _In_ bool isSideways
        ) override;

    // Returns the pixel-aligned advances for a sequences of glyphs.
    HRESULT GetGdiCompatibleGlyphAdvances(
        _In_ XFLOAT emSize,
        _In_ XFLOAT pixelsPerDip,
        _In_opt_ CMILMatrix const *pTransform,
        _In_ TextOptions const *pTextOptions,
        _In_ bool isSideways,
        _In_ XUINT32 glyphCount,
        _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
        _Out_writes_(glyphCount) XINT32 *pGlyphAdvances
    ) override;

    // Retrieves the kerning pair adjustments from the font's kern table.
    HRESULT GetKerningPairAdjustments(
        _In_ XUINT32 glyphCount,
        _In_reads_(glyphCount) XUINT16 const* pGlyphIndices,
        _Out_writes_(glyphCount) XINT32* pGlyphAdvanceAdjustments
        ) override;

    // Returns whether or not the font supports pair-kerning.
    bool HasKerningPairs() override;

    // Returns true if the font has a COLR table
    bool IsColorFont() override;

    BOOL IsCharacterLocal(UINT32 ch) override;

    // Gets DWrite's peer associated with this object.
    IDWriteFontFace4* GetFontFace() const;

    HRESULT GetFontFamilyNames(_Outptr_ IDWriteLocalizedStrings **ppDWriteLocalizedStrings) const;

private:

    // DWrite's peer associated with this object.
    IDWriteFontFace4 *m_pDWriteFontFace = nullptr;

    IDWriteFont *m_pDWriteFont = nullptr;

    // The font family that contains this DWriteFontFace.
    DWriteFontFamily *m_pDWriteFontFamily = nullptr;

    // Release resources associated with the DWriteFontFace.
    ~DWriteFontFace() override;
};

inline IDWriteFontFace4* DWriteFontFace::GetFontFace() const
{
    return m_pDWriteFontFace;
}

