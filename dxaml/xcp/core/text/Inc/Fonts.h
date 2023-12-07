// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//
//      Font support classes
//
//  Overview:
//
//      These classes exist to represent the mapping from a user requested
//      typeface, locale and codepoint to a specific glyphtypeface and
//      glyph index.
//
//      The font system supports implicit virtual fotns, constructed simply
//      by using a comma-separated list of names in the FontFamily parameter.
//
//      The font system also supports more sophisticated virtual fonts
//      described by composite font files. In a composite font, each entry
//      in the list may specify a range of codepoints and/or a locale to
//      which it is applicable.
//
//      In addition to user defined virtual fonts, a similar fallback
//      mechanism is inherent in support for italic style and bold weight:
//      when a bold or italic glyph is required and the directly matching
//      glyphtypeface is not present (or is present but has no support for
//      the required codepoint), we will fall back to applying
//      algorithmic italicization or emboldening to a (more) regular
//      representation of the font.
//
//      Note: support for composite fonts is limited. The
//      underlying architecture makes provision for composite fonts, but no
//      implementation is provided either for parsing the font files, or for
//      applying the locale and codepoint restricitions.
//
//
//  Caching:
//
//      The process of looking through a range of glyphtypefaces to find
//      one that supports a given codepoint is expensive. Since the same
//      combinations are used with high repetivity, the results of the lookup
//      are cached.
//
//      Thus the classes are optimized for two purposes
//
//          1) For fast lookup of a cached glyph
//          2) for correct behavior of the full fallback algorithm.
//
//
//  Fast lookup of a cached glyph:
//
//      The class CFontTypeface is instantiated by the TextBreaker to
//      correspond to a combination of font family, weight, style, stretch
//      and locale.
//
//      The CFontTypeface also contains a pointer to the CFontFamily that
//      represents the clients FontFamily property, and the values of
//      weight, style, stretch and locale to use when calling the fontfamily
//      lookup.
//
//      CFontTypeface exposes its functionality through its LookupGlyph
//      method.
//
//  The fallback process when lookup fails:
//
//      When the glyph is not cached, the code runs the full lookup process.
//      The following classes represent the lookup data:
//
//      1. CFontFamily represents the text element FontFamily property,
//         which may be a physical font family, a composite font,
//         or a list of physical and/or composite fonts.
//         A CFontFamily refers to alist of CFontLookups that are
//         searched in sequence.
//
//      2. CFontLookup represents a single entry in a list of fonts being
//         searched for a given glyph. It may be a single entry in a
//         list of fonts specified in a FontFamily property, or it may
//         be a FontFamilyMap entry in a composite font file.
//         A CFontLookup may refer either to a CPhysicalFontFamily or
//         to a CFontFamily. A reference to a CFontFamily occurs when
//         a FontFamilyMap in a composite font file refers to another
//         composite font or a comma separated list of fonts.
//
//      3. CPhysicalFontFamily represents a font family that is backed
//         by .ttf or .otf font files. CPhysicalFontFamily cannot
//         represent a composite font file.
//         CPhysicalFontFamily lists the CPhysicalFontTypefaces that
//         make up this font family.
//
//  APIs called by clients
//
//      The driving client of the font classes is TextBreaker, which
//      needs to know what GlyphTypeface to use for each character.
//
//      TextBreaker calls CFontFamily::GetFontTypeface once for each
//      change of family, weight, style, stretch or locale to obtain a
//      CFontTypeface corresponding to a Family, weight, style, stretch
//      and langauge. CFontFamily caches CFontTypefaces as they contain
//      the expensively constructed codepoint to glyph/typeface map.
//
//      TextBreaker then calls CFontTypeface::LookupGlyphTypeface for
//      every character to determine which CGlyphtypeface to use for each.
//
//  Construction of font class instances
//
//      CFontFamily
//           Constructed at parse time to represent the FontFamily string.
//           CFontFamilies are stored in a hash table so that multiple
//           references to the same FontFamily string share the same
//           CFontFamily.
//
//       CFontLookup
//           Constructed as part of the process of parsing the FontFamily
//           parameter, or during the loading of a composite font family.
//
//       CPhysicalFontFamily
//           CPhysicalFontFamily is constructed during CTypefaceCollection
//           construction.
//           CPhysicalFontFamilies are stored in a hash table for easy
//           lookup during FontFamily and FontLookup interpretation.
//
//       CCompositeFontFamily
//           CCompositeFontFamily is constructed on demand during the first
//           usage of a CFontFamily, or during the interpretation of a
//           composite font file during CTypefaceCollection
//           construction.
//           CCompositeFontFamilies are stored in a hash table for easy
//           lookup during FontFamily and FontLookup interpretation.
//
//
//  TypefaceCollections and FontSources
//
//      A typeface collection serves 2 major purposes
//
//      1. It contains a hash table of CPhysicalFontFamily whose key
//         is a resolved family name.
//
//      2. It contains a hash table cache of of CCompositeFontFamily
//         whose keys are the actual property values of the
//         CFontFamily property used by the client. This cache allows
//         the client to map a FontFamily property quickly.
//
//
//      There are three scenarios which cause a typeface collection
//      to be built:
//
//      1. The standard installed fonts form the installed typeface
//         collection. The composite font family hash in this collection
//         is used to resolve all font family properties on elements
//         that do not have an assigned font source
//
//      2. The first assignment of a font source to an element causes
//         construction of a typeface collection containing the font
//         or fonts in the font source. The composite font family hash
//         in this collection is used to resolve all font family
//         properties on elements with this font source set.
//
//      3. The first explicit reference to a new font source uri in a
//         FontFamily property (of the form "uri#family") causes the
//         construction of a TypefaceCollection containing the fonts
//         at that uri (usually a zip file). The composite font family
//         hash in this collection is unused.
//
//
//      TODO: BaseUri support. To support elements with differing
//      base uris, the composite font family hashes need to be
//      per base uri, as well as per font source.
//
//
//  Construction of CTypefaceCollection
//
//      1. Standard installed fonts. The installed fonts are enumerated
//         as a list of uris, then built into a tyeface collection by
//         static CTypefaceCollection::CreateFromUriList.
//
//      2. Fonts from a FontSource. The font source package is passed
//         first to SetFromZipPackage, then if it turns out not to be
//         a zip, to SetFromFontFilePackage. The TypefaceCollections
//         m_pFallbackCollection pointer is set to the system installed
//         typeface collection. LookupPhysicalFontFamily will follow this
//         pointer when it can't find font in this font source.
//
//      3. Fonts at a uri referenced in a FontFamily construction.
//         During the construction of the lookups for the list of fonts
//         in a FontFamily property, reference to a new uri causses the
//         creation of a new TypefaceCollection in a pending state. The
//         associated source uri will be downloaded lazily at the first
//         use of the fontlookup referencing this family name.
//
//------------------------------------------------------------------------

#include "FontAndScriptServices.h"
#include <unordered_map>

//------------------------------------------------------------------------
//
//  Class: CCompositeFontFamily
//
//  Representation of a font family that includes a list of font lookups.
//
//------------------------------------------------------------------------

class CCompositeFontFamily final : public IObject
{
friend class CTextBlock;
private:
    IFontAndScriptServices   *m_pFontAndScriptServices;
    IFssFontFallback         *m_pFontFallback;
    CSharedName              *m_pBaseFontName;
    IFssFontCollection       *m_pBaseFontCollection;
    XUINT32                   m_cFontLookups;
    SpanBase<IFssFontFamily*>*m_pFontLookups;
    CSharedName              *m_pFontFamilyName;
    std::unordered_map<CWeightStyleStretch, xref_ptr<CFontTypeface>> m_fontTypefaces;
    XINT32                    m_cReferences;
    XFLOAT                    m_eBaseline;
    XFLOAT                    m_eLineSpacing;
    XFLOAT                    m_eCapHeight;

    _Check_return_ HRESULT CreateAndAddLookup(
        _In_ IFontCollection *pFontCollection,
        _In_ const xstring_ptr_view& fontFamily,
        _Inout_opt_ IFssFontFallbackBuilder **ppFontFallbackBuilder
    );

    _Check_return_ HRESULT DetermineDefaultLineMetrics();

public:

    CCompositeFontFamily (_In_ IFontAndScriptServices *pFontAndScriptServices)
    {
        m_pFontAndScriptServices    = pFontAndScriptServices;
        m_pFontFallback             = NULL;
        m_pBaseFontName             = NULL;
        m_pBaseFontCollection       = NULL;
        m_pFontLookups              = NULL;
        m_cFontLookups              = 0;
        m_pFontFamilyName           = NULL;
        m_eBaseline                 = XFLOAT_MIN; // Unset
        m_eLineSpacing              = XFLOAT_MIN; // Unset
        m_eCapHeight                = XFLOAT_MIN;
        m_cReferences               = 1;
    }

    ~CCompositeFontFamily();

    XUINT32 AddRef() override{return m_cReferences++;}
    XUINT32 Release() override
    {
        XUINT32 cReferences = --m_cReferences;
        if (m_cReferences == 0)
        {
            delete this;
        }
        return cReferences;
    }

    // CreateNew is called by the cache lookup to create a
    // new entry when none is found in the cache.
    static _Check_return_ HRESULT CreateNew(
        _In_        IFontAndScriptServices *pFontAndScriptServices,
        _In_        IFontCollection        *pFontCollection, // Fallback/default font source
        _In_        IPALUri                *pBaseUri,            // Base uri for relative uris
        _In_        CSharedName            *pFontFamilyName,
        _Outptr_ CCompositeFontFamily  **ppCompositeFontFamily
    );

    static _Check_return_ HRESULT CreateFromDWrite(
        _In_                           IFontAndScriptServices *pFontAndScriptServices,
        _In_                           IFontCollection        *pFontCollection,
        _Outptr_                    CCompositeFontFamily  **ppCompositeFontFamily
    );

    _Check_return_ HRESULT MapCharacters(
        _In_z_ const WCHAR *pText,
        XUINT32 textLength,
        _In_z_ const WCHAR *pLocaleName,
        _In_z_ const WCHAR *pLocaleNameList,
        _In_opt_ const NumberSubstitutionData* pNumberSubstitutionData,
        CWeightStyleStretch weightStyleStretch,
        _Outptr_ IFssFontFace **ppMappedFontFace,
        _Out_ XUINT32 *pMappedLength,
        _Out_ XFLOAT *pMappedScale
    );

    _Check_return_ HRESULT LookupNominalFontFace(
        _In_        CWeightStyleStretch weightStyleStretch,
        _Outptr_ IFssFontFace      **ppFontFace
    );

    const xref_ptr<CFontTypeface>& GetFontTypeface(
        _In_            CWeightStyleStretch   weightStyleStretch
    );

    _Check_return_ HRESULT GetLineMetrics(
        _Out_opt_ XFLOAT *peBaseline,
        _Out_opt_ XFLOAT *peLineSpacing,
        _Out_opt_ XFLOAT *peCapHeight
    );

    _Check_return_ HRESULT GetTextLineBoundsMetrics(
        _In_ DirectUI::TextLineBounds textLineBounds,
        _Out_ XFLOAT *peBaseline,
        _Out_ XFLOAT *peLineSpacing
    );

    void SetBaseline   (XFLOAT eBaseline)    {m_eBaseline    = eBaseline;}
    void SetLineSpacing(XFLOAT eLineSpacing) {m_eLineSpacing = eLineSpacing;}
    void SetCapHeight  (XFLOAT eCapHeight)   {m_eCapHeight   = eCapHeight;}

    _Check_return_ HRESULT GetFamilyName(_Out_ CValue *pValue)
    {
        return m_pFontFamilyName->GetValue(pValue);
    }

    UINT32 GetNumberofFontLookups() { return m_cFontLookups; }

    IFontAndScriptServices *GetFontAndScriptServices() const {return m_pFontAndScriptServices;}
};


