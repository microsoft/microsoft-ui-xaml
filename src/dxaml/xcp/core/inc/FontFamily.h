// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "FontAndScriptServices.h"
#include "MultiParentShareableDependencyObject.h"
#include "xcpmath.h" // XFLOAT_MIN

class  CFontFamily;
struct UnicodeRange;
class  CFontLookup;
class  CPhysicalFontFamily;
class  CCompositeFontFamily;

//------------------------------------------------------------------------
//
//  Class:  CWeightStyleStretch
//
//  Encapsulates the values which distinguish the typefaces that make up
//  a FontFamily in a package that is quick to pass around.
//
//------------------------------------------------------------------------

struct CWeightStyleStretch
{
public:
    XUINT16 m_weight  : 10;  // 0 - 999, normal = 400
    XUINT16 m_style   :  2;  // 0 = normal, 1 = oblique, 2 = italic
    XUINT16 m_stretch :  4;  // 0 - 9, normal = 5

    XUINT16 AsXUINT16() const {return (m_weight<<6)|(m_style<<4)|(m_stretch);}
    bool operator==(const CWeightStyleStretch& other) const
    {
        return AsXUINT16() == other.AsXUINT16();
    }

    void Set(XUINT32 weight, XUINT32 style, XUINT32 stretch)
    {
        m_weight  = weight;
        m_style   = style;
        m_stretch = stretch;
    }
    CWeightStyleStretch(XUINT32 weight, XUINT32 style, XUINT32 stretch)
    {
        Set(weight, style, stretch);
    }

    CWeightStyleStretch(XUINT16 compactedWeightStyleStretch)
    {
        // weight, style and stretch are being compacted into a XUINT16 as
        // XUINT16 weight  : 10;  // 0 - 999, normal = 400
        // XUINT16 style   :  2;  // 0 = normal, 1 = oblique, 2 = italic
        // XUINT16 stretch :  4;  // 0 - 9, normal = 5
        Set( (compactedWeightStyleStretch & 0xFFC0) >> 6, (compactedWeightStyleStretch & 0x30) >> 4, compactedWeightStyleStretch & 0xf);
    }

    CWeightStyleStretch()
    {
        Set(static_cast<XUINT32>(DirectUI::CoreFontWeight::Normal),
            static_cast<XUINT32>(DirectUI::FontStyle::Normal),
            static_cast<XUINT32>(DirectUI::FontStretch::Normal));
    }

    _Check_return_ XFLOAT GetNormalizedWeight()  { return (XINT32(m_weight) - static_cast<XUINT32>(DirectUI::CoreFontWeight::Normal)) *  0.05f; } // -20 .. 29.95
    _Check_return_ XFLOAT GetNormalizedStyle()   { return (XINT32(m_style) - static_cast<XUINT32>(DirectUI::FontStyle::Normal)) *  7.00f; } //   0 .. 21
    _Check_return_ XFLOAT GetNormalizedStretch() { return (XINT32(m_stretch) - static_cast<XUINT32>(DirectUI::FontStretch::Normal)) * 11.00f; } // -55 .. 44

    _Check_return_ XFLOAT Distance(CWeightStyleStretch other) // Distance between two faces
    {
        XFLOAT deltaWeight  =  GetNormalizedWeight()  - other.GetNormalizedWeight();
        XFLOAT deltaStyle   =  GetNormalizedStyle()   - other.GetNormalizedStyle();
        XFLOAT deltaStretch =  GetNormalizedStretch() - other.GetNormalizedStretch();

        return (deltaWeight  * deltaWeight)
             + (deltaStyle   * deltaStyle)
             + (deltaStretch * deltaStretch);
    }

    _Check_return_ XFLOAT DotProduct(CWeightStyleStretch other) // Distance between two faces
    {
        return (GetNormalizedWeight()  * other.GetNormalizedWeight())
             + (GetNormalizedStyle()   * other.GetNormalizedStyle())
             + (GetNormalizedStretch() * other.GetNormalizedStretch());
    }


    bool IsBetterMatch(
        CWeightStyleStretch target,
        CWeightStyleStretch bestSoFar
    )
    {
        // First compare with target and best-so-far by distance.

        XFLOAT distanceFromTarget       = Distance(target);
        XFLOAT distanceFromBestToTarget = bestSoFar.Distance(target);

        if (distanceFromTarget < distanceFromBestToTarget)
        {
            return true;
        }
        else if (distanceFromTarget == distanceFromBestToTarget)
        {
            // Same length distance, try comparing by dot product

            XFLOAT dotProductWithTarget     = DotProduct(target);
            XFLOAT dotProductBestWithTarget = bestSoFar.DotProduct(target);

            if (dotProductWithTarget > dotProductBestWithTarget)
            {
                return true; // Prefer typefaces with a greater dot product
            }
            else if (dotProductWithTarget == dotProductBestWithTarget)
            {
                // This wss is no closer to target than is bestSoFar, and has
                // the same dot product with target as bestSoFar has.

                // Tiebreaker algorithm:

                if (   m_stretch > bestSoFar.m_stretch
                    || (   m_stretch == bestSoFar.m_stretch
                        && (   m_style > bestSoFar.m_style
                            || (    m_style == bestSoFar.m_style
                                 && m_weight > bestSoFar.m_weight))))
                {
                    return true;
                }
            }
        }
        return false; // In all other cases, this is a worse match
    }
};

namespace std {
    template<>
    struct hash<CWeightStyleStretch>
    {
        typedef CWeightStyleStretch argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type& arg) const
        {
            return std::hash<XUINT16>()(arg.AsXUINT16());
        }
    };
}

template <class C> class SpanBase;

//------------------------------------------------------------------------
//
//  Class: CSharedName
//
//      A string used as a key in a hash table of font families.
//
//      The string is treated as case insensitive for ASCII characters.
//      i.e., codepoints U+0041-U+005A are treated as the same as
//      U+0061-U+007A.
//
//      Properties:
//        o  Stores a null-terminated xstring_ptr.
//        o  Can be created from an xstring_ptr or an WCHAR* and length, in which cases it creates a new CSharedName using a copy of the string.
//        o  Supports AddRef / Release
//        o  Can get a const WCHAR* from it.
//        o  [Not yet implemented] Can index it with [], read only, bounds checked in debug builds.
//
//------------------------------------------------------------------------

struct CSharedName final : public IObject
{
private:
    xstring_ptr                          m_strName;
    XUINT32                              m_cRef = 1;

public:
    CSharedName()
    {
    }

    ~CSharedName(){}

    static _Check_return_ HRESULT Create(
        _In_                       XUINT32       cString,
        _In_reads_(cString) const WCHAR        *pString,
        _Outptr_                CSharedName **ppName
    );


    static _Check_return_ HRESULT Create(
        _In_ const xstring_ptr_view& strName,
        _Outptr_  CSharedName **ppName
        );

    XUINT32 Release() override;
    XUINT32 AddRef() override;

    _Check_return_ const WCHAR *GetString() const {return m_strName.GetBuffer();}
    _Check_return_     XUINT32  GetLength() const {return m_strName.GetCount();}

    _Check_return_ HRESULT GetValue(_Out_ CValue *pValue)
    {
        pValue->SetString(m_strName);
        return S_OK;
    }

    XUINT32 GetHash() const
    {
        XUINT32 hash = 0;

        // Handle majority of string 2 code units (32 bits) at a time

        const WCHAR* pString = GetString();
        const XUINT32 cString = GetLength();

        for (XUINT32 i=0; i<cString/2; i++)
        {
            XUINT32 codeUnitPair = *((XUINT32*)&pString[i*2]);
            if (codeUnitPair - 0x0041000 < 0x001b0000)
            {
                // High order character is upper case ASCII, convert to lower case
                codeUnitPair |= 0x00200000;
            }
            if ((codeUnitPair & 0xffff) - 0x0041 < 0x001b)
            {
                // Low order character is upper case, convert to lower case
                codeUnitPair |= 0x0020;
            }

            hash = (hash * 16777619)  ^  codeUnitPair;
        }

        if (cString & 1)
        {
            // Handle last code unit of odd length string
            XUINT32 codeUnit = pString[cString-1];
            if (XINT32(codeUnit) - 0x0041 < 0x001b)
            {
                // Low order character is upper case ASCII, convert to lower case
                codeUnit |= 0x0020;
            }
            hash = (hash * 16777619)  ^  codeUnit;
        }

        return hash;
    }

    bool EqualKeys(const CSharedName& other) const
    {
        const XUINT32 cString = GetLength();

        if (cString != other.GetLength())
        {
            return false;
        }

        const WCHAR* pString = GetString();
        const WCHAR* pOtherString = other.GetString();

        for (XUINT32 i=0; i<cString; i++)
        {
            WCHAR thisCodeUnit  = pString[i];
            WCHAR otherCodeUnit = pOtherString[i];
            if (XINT16(thisCodeUnit & 0xffdf) - 0x0041 < 0x001b)
            {
                // Clear bit 5 of both code units. This will
                // make the comparison case insensitive.
                thisCodeUnit  &= 0xffdf;
                otherCodeUnit &= 0xffdf;
            }
            if (thisCodeUnit != otherCodeUnit)
            {
                return false;
            }
        }
        return true;
    }
};

inline bool operator==(const xref_ptr<CSharedName>& lhs, const xref_ptr<CSharedName>& rhs)
{
    ASSERT(lhs && rhs);
    return (lhs.get() == rhs.get()) || (lhs->EqualKeys(*rhs));
}

namespace std {
    template<>
    struct hash<xref_ptr<CSharedName>>
    {
        typedef xref_ptr<CSharedName> argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type& arg) const
        {
            return arg->GetHash();
        }
    };
}


//------------------------------------------------------------------------
//
//  Class:  CFontContext
//
//  Contains the font source and base uri which are used to resolve
//  the FontFamily property string value into physical and composite
//  typefaces.
//
//------------------------------------------------------------------------

class CFontContext
{
private:
    IFontCollection *m_pFontCollection;
    IPALUri         *m_pBaseUri;

public:
    CFontContext(
        _In_ IFontCollection *pFontCollection,
        _In_ IPALUri         *pBaseUri
    )
    {
        AddRefInterface(pFontCollection);
        m_pFontCollection = pFontCollection;
        m_pBaseUri        = pBaseUri;
    }

    //Copy constructor
    CFontContext(const CFontContext &fontContext)
    {
        ReleaseInterface(m_pFontCollection);
        ReleaseInterface(m_pBaseUri);
        m_pFontCollection = fontContext.m_pFontCollection;
        m_pBaseUri        = fontContext.m_pBaseUri;
        AddRefInterface(m_pFontCollection);
        AddRefInterface(m_pBaseUri);
    }

    CFontContext& operator = (const CFontContext &fontContext)
    {
        ReleaseInterface(m_pFontCollection);
        ReleaseInterface(m_pBaseUri);
        m_pFontCollection = fontContext.m_pFontCollection;
        m_pBaseUri        = fontContext.m_pBaseUri;
        AddRefInterface(m_pFontCollection);
        AddRefInterface(m_pBaseUri);

        return *this;
    }

    CFontContext()
    {
        m_pFontCollection = NULL;
        m_pBaseUri        = NULL;
    }

    ~CFontContext()
    {
        ReleaseInterface(m_pFontCollection);
        ReleaseInterface(m_pBaseUri);
    }

    IFontCollection *GetFontCollection() {return m_pFontCollection;}
    IPALUri         *GetBaseUri()        {return m_pBaseUri;}

    void SetFontSource(IFontCollection *pFontCollection)
    {
        AddRefInterface(pFontCollection);
        ReleaseInterface(m_pFontCollection);
        m_pFontCollection = pFontCollection;
    }

    void SetBaseUri(IPALUri *pBaseUri)
    {
        m_pBaseUri = pBaseUri;
    }
};

//------------------------------------------------------------------------
//
//  Class: CFontTypeface
//
//  Representation a combination of a client specified of a FontFamily
//  text element property, weight, style, stretch and language.
//
//  Provides the cache by codepoint of which glyph typeface to use.
//
//------------------------------------------------------------------------
class CFontTypeface final : public IObject
{
private:

    // Data to pass to CFontFamily when the codepoint is not
    // already cached.
    CCompositeFontFamily *m_pCompositeFontFamily;
    CWeightStyleStretch   m_weightStyleStretch;
    XUINT32               m_cReferences;

    // FontTypeFace attributes
    XFLOAT                m_capHeight;

    CFontTypeface() = delete;  // Clients must use constructor below
    ~CFontTypeface();   // Clients must use Release

public:
    CFontTypeface(
        _In_ CCompositeFontFamily *pCompositeFontFamily,
        CWeightStyleStretch   weightStyleStretch
    )
    : m_pCompositeFontFamily(pCompositeFontFamily)
    , m_weightStyleStretch(weightStyleStretch)
    , m_cReferences(1)
    , m_capHeight(XFLOAT_MIN)
    {
    }

    _Check_return_ HRESULT MapCharacters(
        _In_z_ const WCHAR *pText,
        XUINT32 textLength,
        _In_z_ const WCHAR *pLocaleName,
        _In_z_ const WCHAR *pLocaleNameList,
        _In_opt_ const NumberSubstitutionData* pNumberSubstitutionData,
        _Outptr_ IFssFontFace **ppMappedFontFace,
        _Out_ XUINT32 *pMappedLength,
        _Out_ XFLOAT *pMappedScale
    );

    // TODO: PS#93554: Remove it after DWrite integration is done.
    // NOTE: used by LsTextLine.cpp
    CCompositeFontFamily *GetCompositeFontFamily() {return m_pCompositeFontFamily;}

    // Attribute Setters
    void SetCapHeight(XFLOAT capHeight) {m_capHeight = capHeight;}

    std::size_t hash() const
    {
        return std::hash<CWeightStyleStretch>()(m_weightStyleStretch);
    }

    XUINT32 AddRef() override {return ++m_cReferences;}

    XUINT32 Release() override
    {
        XUINT32 cReferences = --m_cReferences;
        if (!cReferences)
        {
            delete this;
        }
        return cReferences;
    }
};


namespace std {
    template<>
    struct hash<CFontTypeface>
    {
        typedef CFontTypeface argument_type;
        typedef std::size_t result_type;

        result_type operator()(const argument_type& arg) const
        {
            return arg.hash();
        }
    };
}



//------------------------------------------------------------------------
//
//  Class: CFontFamily
//
//  Representation of the FontFamily= property.
//
//  Records the family name requested, and lazily caches the
//  corresponding CCompositeFontFamily.
//
//  Construction only sets the m_pFontFamilyName property.
//
//  m_pTypefaceCollection and m_pCompositeFamily correspond to the
//  last TypefaceCollection in which GetFontTypeface was called.
//
//  Thus repeated calls to GetFontTypeface using the sam CTypefaceCollection
//  avoid looking up the CCompositeFontFamily or worse re-constructing it.
//
//------------------------------------------------------------------------

class CFontFamily final : public CMultiParentShareableDependencyObject
{
    friend class CTextBlock;

    CCompositeFontFamily *m_pCompositeFontFamily = nullptr;
    CSharedName          *m_pFontFamilyName = nullptr;
    CFontContext          m_fontContext;

    _Check_return_ HRESULT EnsureCompositeFontFamily(
        _In_            CFontContext         *pFontContext
    );

public:
    CFontFamily(_In_ CCoreServices *pCore)
        : CMultiParentShareableDependencyObject(pCore)
    {}

    ~CFontFamily() override;

    // Create is called by the parser and simply records the
    // font family name
    static _Check_return_ HRESULT Create(
        _Outptr_ CDependencyObject **ppObject,
        _In_        CREATEPARAMETERS   *pCreate
    );

    KnownTypeIndex GetTypeIndex() const override
    {
        return DependencyObjectTraits<CFontFamily>::Index;
    }

    _Check_return_ HRESULT GetFontTypeface(
        _In_            CFontContext         *pFontContext,
        _In_            CWeightStyleStretch   weightStyleStretch,
        _Outptr_result_maybenull_ CFontTypeface       **ppFontTypeface
    );

    _Check_return_ HRESULT GetTextLineBoundsMetrics(
        _In_            CFontContext            *pFontContext,
        _In_            DirectUI::TextLineBounds textLineBounds,
        _Out_           XFLOAT                  *peBaseline,
        _Out_           XFLOAT                  *peLineSpacing
    );

    _Check_return_ HRESULT get_Source(_Out_ xstring_ptr* value);
    _Check_return_ HRESULT put_Source(_In_ xephemeral_string_ptr& value);
};
