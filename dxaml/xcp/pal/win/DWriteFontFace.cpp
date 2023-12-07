// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "DWriteFontFace.h"
#include "DWriteFontFamily.h"

//---------------------------------------------------------------------------
//
//  Initializes a new instance of the DWriteFontFace class.
//
//---------------------------------------------------------------------------
DWriteFontFace::DWriteFontFace(
    _In_ IDWriteFontFace *pDWriteFontFace,
    _In_opt_ DWriteFontFamily *pDWriteFontFamily
    )
    : m_pDWriteFontFamily(pDWriteFontFamily)
{
    VERIFYHR(pDWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace4), reinterpret_cast<void **>(&m_pDWriteFontFace)));
    AddRefInterface(m_pDWriteFontFamily);
    if (m_pDWriteFontFamily)
    {
        m_canOptimizeShaping = !!m_pDWriteFontFamily->CanOptimizeShaping();
    }
}


DWriteFontFace::DWriteFontFace(
    _In_ IDWriteFont *pDWriteFont
    )
    : m_pDWriteFont(pDWriteFont)
{
    IDWriteFontFace* pFontFace = nullptr;
    VERIFYHR(pDWriteFont->CreateFontFace(&pFontFace));
    VERIFYHR(pFontFace->QueryInterface(__uuidof(IDWriteFontFace4), reinterpret_cast<void **>(&m_pDWriteFontFace)));
    ReleaseInterface(pFontFace);
    AddRefInterface(pDWriteFont);
}

//---------------------------------------------------------------------------
//
//  Release resources associated with the DWriteFontFace.
//
//---------------------------------------------------------------------------
DWriteFontFace::~DWriteFontFace()
{
    ReleaseInterface(m_pDWriteFont);
    ReleaseInterface(m_pDWriteFontFace);
    ReleaseInterface(m_pDWriteFontFamily);
}

//---------------------------------------------------------------------------
//
//  Returns true if the font face is either TrueType or TrueTypeCollection.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::HasTrueTypeOutlines()
{
    DWRITE_FONT_FACE_TYPE fontType =  m_pDWriteFontFace->GetType();
    return (fontType == DWRITE_FONT_FACE_TYPE_TRUETYPE || fontType == DWRITE_FONT_FACE_TYPE_TRUETYPE_COLLECTION);
}

//---------------------------------------------------------------------------
//
//  Gets the OpenType Offset for the font.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::TryGetFontOffset(
    _Outptr_result_bytebuffer_(*pcOffset) const void **ppOffset,
    _Out_ XUINT32 *pcOffset,
    _Out_ bool *pExists
    )
{
    return E_UNEXPECTED; // Should not be called when using DWrite.
}

//---------------------------------------------------------------------------
//
//  Determines whether two FontFaces are equal.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::Equals(
    _In_ IFontFace *pFontFace
    )
{
    // For IDWriteFontFace Equals comparison, just compare pointers because the same font will always return the same instance so long as one instance is alive and
    // shared same IDWriteFactory.
    if (m_pDWriteFontFace == reinterpret_cast<DWriteFontFace*>(pFontFace)->m_pDWriteFontFace)
    {
        return true;
    }
    else
    {
        return false;
    }
}

//---------------------------------------------------------------------------
//
//  Obtains the algorithmic style simulation flags of a font face.
//
//---------------------------------------------------------------------------
PALText::FontSimulations::Enum DWriteFontFace::GetSimulations()
{
    DWRITE_FONT_SIMULATIONS dwriteFontSimulations = m_pDWriteFontFace->GetSimulations();
    return static_cast<PALText::FontSimulations::Enum>(dwriteFontSimulations);
}

//---------------------------------------------------------------------------
//
//  Determines whether the font is a symbol font.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::IsSymbolFont()
{
    return !!m_pDWriteFontFace->IsSymbolFont();
}

//---------------------------------------------------------------------------
//
//  Obtains design units and common metrics for the font face.
//
//---------------------------------------------------------------------------
void DWriteFontFace::GetMetrics(
    _Out_ PALText::FontMetrics *pFontFaceMetrics)
{
    if (pFontFaceMetrics != NULL)
    {
        memset(pFontFaceMetrics, 0, sizeof(PALText::FontMetrics));
        DWRITE_FONT_METRICS dwriteFontMetrics;
        m_pDWriteFontFace->GetMetrics(&dwriteFontMetrics);

        memcpy(
           pFontFaceMetrics,
           &dwriteFontMetrics,
           sizeof(dwriteFontMetrics)
        );
    }
}

//---------------------------------------------------------------------------
//
//  Obtains the number of glyphs in the font face.
//
//---------------------------------------------------------------------------
XUINT16 DWriteFontFace::GetGlyphCount()
{
    return m_pDWriteFontFace->GetGlyphCount();
}

//---------------------------------------------------------------------------
//
//  Obtains ideal glyph metrics in font design units.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::GetDesignGlyphMetrics(
        _In_reads_(glyphCount) XUINT16 const * pGlyphIndices,
        _In_ XUINT32 glyphCount,
        _Out_writes_(glyphCount) PALText::GlyphMetrics * pGlyphMetrics,
        _In_ bool isSideways
        )
{
    return m_pDWriteFontFace->GetDesignGlyphMetrics(pGlyphIndices,
        glyphCount,
        reinterpret_cast<DWRITE_GLYPH_METRICS*>(pGlyphMetrics),
        isSideways);
}

//---------------------------------------------------------------------------
//
//  Returns the nominal mapping of UTF-32 Unicode code points to glyph indices
//  as defined by the font 'cmap' table.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::GetGlyphIndices(
    _In_reads_(codePointCount) XUINT32 const *pCodePoints,
    _In_ XUINT32 codePointCount,
    _Out_writes_(codePointCount) XUINT16 *pGlyphIndices
    )
{
    return m_pDWriteFontFace->GetGlyphIndices(pCodePoints, codePointCount, pGlyphIndices);
}

//---------------------------------------------------------------------------
//
//  Finds the specified OpenType font table if it exists and returns a pointer to it.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::TryGetFontTable(
    _In_ XUINT32 openTypeTableTag,
    _Outptr_result_bytebuffer_(*pTableSize) const void **ppTableData,
    _Out_ XUINT32 *pTableSize,
    _Outptr_ void **ppTableContext,
    _Out_ bool *pExists
    )
{
    return E_UNEXPECTED; // Should not be called when using DWrite.
}

//---------------------------------------------------------------------------
//
//  Returns true if the font is monospaced.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::IsMonospacedFont()
{
    return !!m_pDWriteFontFace->IsMonospacedFont();
}

//---------------------------------------------------------------------------
//
//  Returns the advances in design units for a sequences of glyphs.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::GetDesignGlyphAdvances(
    _In_ XUINT32 glyphCount,
    _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
    _Out_writes_(glyphCount) XINT32 *pGlyphAdvances,
    _In_ bool isSideways
    )
{
    HRESULT hr = S_OK;
    DWRITE_GLYPH_METRICS *pDWriteGlyphMetrics = NULL;

    IFC(m_pDWriteFontFace->GetDesignGlyphAdvances(glyphCount, pGlyphIndices, pGlyphAdvances, isSideways));

Cleanup:
    delete[] pDWriteGlyphMetrics;
    return hr;
}

//---------------------------------------------------------------------------
//
// Returns the pixel-aligned advances for a sequences of glyphs.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::GetGdiCompatibleGlyphAdvances(
    _In_ XFLOAT emSize,
    _In_ XFLOAT pixelsPerDip,
    _In_opt_ CMILMatrix const *pTransform,
    _In_ TextOptions const *pTextOptions,
    _In_ bool isSideways,
    _In_ XUINT32 glyphCount,
    _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
    _Out_writes_(glyphCount) XINT32 *pGlyphAdvances
)
{
    RRETURN(E_UNEXPECTED);
}

//---------------------------------------------------------------------------
//
//  Retrieves the kerning pair adjustments from the font's kern table.
//
//---------------------------------------------------------------------------
HRESULT DWriteFontFace::GetKerningPairAdjustments(
    _In_ XUINT32 glyphCount,
    _In_reads_(glyphCount) XUINT16 const *pGlyphIndices,
    _Out_writes_(glyphCount) XINT32 *pGlyphAdvanceAdjustments
    )
{
    HRESULT hr = S_OK;

    IFC(m_pDWriteFontFace->GetKerningPairAdjustments(glyphCount, pGlyphIndices, pGlyphAdvanceAdjustments));

Cleanup:
    return hr;    
}

//---------------------------------------------------------------------------
//
//  Returns whether or not the font supports pair-kerning.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::HasKerningPairs()
{
    return !!m_pDWriteFontFace->HasKerningPairs();
}

//---------------------------------------------------------------------------
//
//  Returns whether we can bypass full shaping.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::CanOptimizeShaping()
{
    return m_canOptimizeShaping;
}


//---------------------------------------------------------------------------
//
//  Returns whether or not the font has a COLR table.
//
//---------------------------------------------------------------------------
bool DWriteFontFace::IsColorFont()
{
    return !!m_pDWriteFontFace->IsColorFont();
}


//---------------------------------------------------------------------------
//
//  Returns whether or not the font supports a character and whether the
//  character is locally downloaded.
//
//---------------------------------------------------------------------------
BOOL DWriteFontFace::IsCharacterLocal(UINT32 ch)
{
    return m_pDWriteFontFace->IsCharacterLocal(ch);
}


//---------------------------------------------------------------------------
//
//  Get the collection of family names from the font.
//
//---------------------------------------------------------------------------

HRESULT DWriteFontFace::GetFontFamilyNames(_Outptr_ IDWriteLocalizedStrings **ppDWriteLocalizedStrings) const
{
    HRESULT hr = S_OK;
    IDWriteFontFamily *pFamily = NULL;
    if (m_pDWriteFontFamily)
    {
        pFamily = m_pDWriteFontFamily->GetFontFamily();
        AddRefInterface(pFamily);
    }
    else
    {
        IFC(m_pDWriteFont->GetFontFamily(&pFamily));
    }

    IFC(pFamily->GetFamilyNames(ppDWriteLocalizedStrings));

Cleanup:
    ReleaseInterface(pFamily);
    return hr;
}

