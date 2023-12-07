// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FontAndScriptServices.h"
#include "TypefaceCollection.h"
#include <TrimWhitespace.h>

using namespace DirectUI;

//------------------------------------------------------------------------
//
//  Method:  DecodeHexDigit
//
//  Synopsis:
//     Handles a single hex digit in the %nn sytax used in the FontFamily.
//     We parse this ourselves because the font family component of a
//     'FontFamily' property is not parsed by the url code.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT DecodeHexDigit(_In_ WCHAR digit, _Out_ XUINT32 *pValue)
{
    HRESULT hr = S_OK;

    if ((digit >= L'0')  &&  (digit <= L'9'))
    {
        *pValue = digit - L'0';
    }
    else if ((digit >= L'a')  &&  (digit <= L'f'))
    {
        *pValue = digit - L'a' + 10;
    }
    else if ((digit >= L'A')  &&  (digit <= L'F'))
    {
        *pValue = digit - L'A' + 10;
    }
    else
    {
        hr = WC_E_HEXDIGIT;
    }

    RRETURN(hr);
}




//------------------------------------------------------------------------
//
//  Method:  ExtractSingleFamily
//
//  Synopsis:
//      Copies text up to a single ',' to a specified buffer (pBuffer).
//      Doubled ','s are copied as a single comma.
//      Returns pointers to the source uri and to family name within pBuffer.
//      Occurrences of '%nn' in the family name are decoded.
//      No decoding happens within the source uri.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT ExtractSingleFamily(
    _In_reads_(cString) const        WCHAR    *pString,
    _In_                              XUINT32   cString,
    _In_reads_(cString)              WCHAR    *pBuffer,  // At least as large as the input string
    _Out_                             XUINT32  *pcUsed,
    _Outptr_result_buffer_(*pcFamily)     WCHAR   **ppFamily, // Somewhere in pBuffer
    _Out_                             XUINT32  *pcFamily,
    _Outptr_result_buffer_maybenull_(*pcSource) WCHAR   **ppSource, // Somewhere in pBuffer
    _Out_                             XUINT32  *pcSource
)
{
    XUINT32  i                 = 0;
    XUINT32  j                 = 0;
    WCHAR   *pFirstHash        = NULL;
    WCHAR    character         = 0;
    XINT32   fFoundSingleComma = FALSE;
    WCHAR   *pFamily           = NULL;
    XUINT32  cFamily           = 0;
    WCHAR   *pSource           = NULL;
    XUINT32  cSource           = 0;
    XUINT32  cUsed             = 0;

    while (    (i < cString)
           &&  (!fFoundSingleComma))
    {
        ASSERT(j < cString);

        character = pString[i];

        switch (character)
        {
        case L',':
            if (    (i+1 < cString)
                &&  (pString[i+1] == L','))
            {
                i++; // Skip one of the pair of ','s.
                pBuffer[j++] = character;
            }
            else
            {
                fFoundSingleComma = TRUE;
            }
            break;

        case L'#':
            if (pFirstHash == NULL)
            {
                pFirstHash = pBuffer + j;
            }
            pBuffer[j++] = character;
            break;

        default:
            pBuffer[j++] = character;
            break;
        }

        i++;
    }

    cUsed = i; // Number of characters used from source, including any
               // terminiating single comma.

    // We now have 'j' characters representing the details
    // of a single family copied into pBuffer, with any
    // double-commas replaced by single commas.

    if (pFirstHash == NULL)
    {
        // It's all a family name
        pFamily = pBuffer;
        cFamily = j;
    }
    else
    {
        // There is both a source and a family name
        IFCEXPECT_ASSERT_RETURN(*pFirstHash == L'#');
        IFCEXPECT_ASSERT_RETURN(pFirstHash >= pBuffer);
        IFCEXPECT_ASSERT_RETURN(pFirstHash < pBuffer+j);
        pSource = pBuffer;
        cSource = static_cast<XUINT32>(pFirstHash - pBuffer);
        pFamily = pFirstHash+1;
        // Cast is safe since "pFirstHash < pBuffer+j" assert ensures that the argument is >= 0.
        cFamily = static_cast<XUINT32>(j - (pFamily-pBuffer));

        TrimWhitespace(cSource, pSource, &cSource, &pSource);
        TrimTrailingWhitespace(cSource, pSource, &cSource, &pSource);
    }

    // Process '%nn' encodings in the family name

    i=0; j=0;
    while (i < cFamily)
    {
        XUINT32 highNybble;
        XUINT32 lowNybble;

        character = pFamily[i];

        if (pFamily[i] == L'%')
        {
            IFCEXPECT_RETURN(i+2 < cFamily);
            IFC_RETURN(DecodeHexDigit(pFamily[i+1], &highNybble));
            IFC_RETURN(DecodeHexDigit(pFamily[i+2], &lowNybble));
            i+=2;
            character = (WCHAR)((highNybble << 4) + lowNybble);
        }

        pFamily[j] = character;
        i++;
        j++;
    }
    cFamily = j;

    TrimWhitespace(cFamily, pFamily, &cFamily, &pFamily);
    TrimTrailingWhitespace(cFamily, pFamily, &cFamily, &pFamily);

    *pcUsed   = cUsed;
    *ppFamily = pFamily;
    *pcFamily = cFamily;
    *ppSource = pSource;
    *pcSource = cSource;

    return S_OK;
}




//------------------------------------------------------------------------
//
//  Method:  CreateAndAddLookup
//
//  Synopsis:
//      Takes a parsed font source string and family name string,
//      creates the associated lookup record and adds it to the
//      specified composite font.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::CreateAndAddLookup(
    _In_ IFontCollection *pFontCollection,
    _In_ const xstring_ptr_view& fontFamily,
    _Inout_opt_ IFssFontFallbackBuilder **ppFontFallbackBuilder
)
{
    HRESULT               hr           = S_OK;
    IFssFontFamily       *pFontFamily  = NULL;
    CSharedName          *pFamilyName  = NULL;
    IFssFontCollection   *pRealFontCollection = static_cast<CTypefaceCollection*>(pFontCollection)->GetRealFontCollectionNoAddRef();

    IFCEXPECT_ASSERT(!fontFamily.IsNullOrEmpty());
    IFCEXPECT_ASSERT(pFontCollection != NULL);

    IFC(CSharedName::Create(fontFamily, &pFamilyName));

    IFC(pFontCollection->LookupPhysicalFontFamily(
        pFamilyName,
       &pFontFamily
    ));

    if (pFontFamily)
    {
        if (!m_pFontLookups)
        {
            m_pFontLookups = new SpanBase<IFssFontFamily*>();
        }

        IFC(m_pFontLookups->MakeRoom(m_cFontLookups+1));
        (*m_pFontLookups)[m_cFontLookups++] = pFontFamily;
        pFontFamily = NULL;
    }

    //
    // The first lookup added is considered the base font and the rest are just additional
    // lookups.  Store the base font so that it can be treated as such during font fallback.
    // If no font fallback builder was requested that means this is the default fallback and
    // we shouldn't store the base font either.
    //

    if (ppFontFallbackBuilder != NULL)
    {
        if (m_pBaseFontName == NULL)
        {
            m_pBaseFontName = pFamilyName;
            AddRefInterface(pFamilyName);

            m_pBaseFontCollection = pRealFontCollection;
            AddRefInterface(pRealFontCollection);
        }
        else
        {
            if (*ppFontFallbackBuilder == NULL)
            {
                IFC(m_pFontAndScriptServices->CreateFontFallbackBuilder(ppFontFallbackBuilder));
            }

            PALText::UNICODE_RANGE ranges[] = { {0,0x1FFFFF} };
            WCHAR const* targetFamilyNames[1] = { pFamilyName->GetString() };
            IFC((*ppFontFallbackBuilder)->AddMapping(
                    ranges,
                    ARRAY_SIZE(ranges),
                    targetFamilyNames,
                    ARRAY_SIZE(targetFamilyNames),
                    pRealFontCollection));
        }
    }

Cleanup:
    ReleaseInterface(pFontFamily);
    ReleaseInterface(pFamilyName);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:  CCompositeFontFamily::DetermineDefaultLineMetrics
//
//  Synopsis:
//      Determines the line metrics (baseline, line spacing, and capheight) of a
//      composite font by extracting them from the most regular face
//      in the first lookup.
//
//      Only updates baseline, spacing, and capheight where they are initialized
//      to XFLOAT_MIN. We require at least one to be XFLOAT_MIN
//      on entry.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::DetermineDefaultLineMetrics()
{
    HRESULT               hr                      = S_OK;
    XUINT32               iLookup                 = 0;
    XINT32                fLookNoFurther          = FALSE;
    IFssFontFamily       *pFontLookup             = NULL;
    IFssFontFace         *pFontFace               = NULL;
    XUINT16               lineSpacing             = 0;
    CCompositeFontFamily *pUltimateFallbackFamily = NULL;

    IFCEXPECT_ASSERT( (m_eBaseline    == XFLOAT_MIN)
                  ||  (m_eLineSpacing == XFLOAT_MIN)
                  ||  (m_eCapHeight == XFLOAT_MIN));

    m_eBaseline    = XFLOAT_MIN;
    m_eLineSpacing = XFLOAT_MIN;
    m_eCapHeight   = XFLOAT_MIN;

    fLookNoFurther    = FALSE;
    iLookup           = 0;

    while (    (!fLookNoFurther)
           &&  (iLookup < m_cFontLookups))
    {
        pFontLookup = (*m_pFontLookups)[iLookup];

        hr = pFontLookup->LookupNominalFontFace(
            static_cast<XUINT32>(DirectUI::CoreFontWeight::Normal),
            static_cast<XUINT32>(DirectUI::FontStyle::Normal),
            static_cast<XUINT32>(DirectUI::FontStretch::Normal),
            &pFontFace
            );

        if (pFontFace != NULL)
        {
            FssFontMetrics fontFaceMetrics;
            pFontFace->GetMetrics(&fontFaceMetrics);
            lineSpacing = fontFaceMetrics.LineGap + fontFaceMetrics.Ascent + fontFaceMetrics.Descent;

            if (m_eBaseline == XFLOAT_MIN)
            {
                m_eBaseline = XFLOAT(fontFaceMetrics.Ascent) / XFLOAT(fontFaceMetrics.DesignUnitsPerEm);
            }
            if (m_eLineSpacing == XFLOAT_MIN)
            {
                m_eLineSpacing = XFLOAT(lineSpacing) / XFLOAT(fontFaceMetrics.DesignUnitsPerEm);
            }
            if (m_eCapHeight == XFLOAT_MIN)
            {
                m_eCapHeight = XFLOAT(fontFaceMetrics.CapHeight) / XFLOAT(fontFaceMetrics.DesignUnitsPerEm);
            }

            fLookNoFurther = TRUE;
        }

        iLookup++;
    }

    if (!fLookNoFurther)
    {
        // There are no working lookups in this font family,
        // for example, it might be a non-existant or mis-spelled
        // family name, such as FontFamily="Courir New".
        // Get the line metrics from the ultimate fallback font.

        // we might still be waiting for a pending download:
        if (hr == E_PENDING)
        {
            IFC(hr);
        }

        IFC(m_pFontAndScriptServices->GetUltimateFallbackFontFamily(&pUltimateFallbackFamily));

        IFC(pUltimateFallbackFamily->GetLineMetrics(
            &m_eBaseline,
            &m_eLineSpacing,
            &m_eCapHeight
        ));
    }

    IFCEXPECT_ASSERT(   m_eBaseline    != XFLOAT_MIN
                    &&  m_eLineSpacing != XFLOAT_MIN
                    &&  m_eCapHeight   != XFLOAT_MIN);
Cleanup:
    ReleaseInterface(pFontFace);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Method:  CCompositeFontFamily::GetLineMetrics
//
//  Synopsis:
//      Returns the baseline offset and linespacing for this composite
//      font family.
//
//      Uses DetermineDefaultLineMetrics to caluclate them if they are not
//      already set.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::GetLineMetrics(
    _Out_opt_ XFLOAT *peBaseline,
    _Out_opt_ XFLOAT *peLineSpacing,
    _Out_opt_ XFLOAT *peCapHeight
)
{
    IFCEXPECT_ASSERT_RETURN(   (peBaseline != NULL)
                    ||  (peLineSpacing != NULL)
                    ||  (peCapHeight != NULL));

    if (    (m_eBaseline    == XFLOAT_MIN)
        ||  (m_eLineSpacing == XFLOAT_MIN)
        ||  (m_eCapHeight == XFLOAT_MIN))
    {
        IFC_RETURN(DetermineDefaultLineMetrics());
    }

    if (peBaseline    != NULL) {*peBaseline    = (m_eBaseline    == XFLOAT_MIN ? 0.0f : m_eBaseline);}
    if (peLineSpacing != NULL) {*peLineSpacing = (m_eLineSpacing == XFLOAT_MIN ? 0.0f : m_eLineSpacing);}
    if (peCapHeight   != NULL) {*peCapHeight   = (m_eCapHeight   == XFLOAT_MIN ? 0.0f : m_eCapHeight);}

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:  CCompositeFontFamily::GetTextLineBoundsMetrics
//
//  Synopsis:
//      Returns the ascent and descent for this font family constrained
//  by TextLineBounds
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::GetTextLineBoundsMetrics(
    _In_ TextLineBounds textLineBounds,
    _Out_ XFLOAT *peBaseline,
    _Out_ XFLOAT *peLinespacing
)
{
    XFLOAT baseline;
    XFLOAT linespacing;
    XFLOAT capheight;

    IFC_RETURN(GetLineMetrics(&baseline, &linespacing, &capheight));

    switch (textLineBounds)
    {
    case DirectUI::TextLineBounds::Full:
        *peBaseline = baseline;
        *peLinespacing = linespacing;
        break;

    case DirectUI::TextLineBounds::TrimToCapHeight:
        *peBaseline = capheight;
        *peLinespacing = linespacing - baseline + capheight;
        break;

    case DirectUI::TextLineBounds::TrimToBaseline:
        *peBaseline = baseline;
        *peLinespacing = baseline;
        break;

    case DirectUI::TextLineBounds::Tight:
        *peBaseline = capheight;
        *peLinespacing = capheight;
        break;
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:  CCompositeFontFamily::CreateNew
//
//  Synopsis:
//      Builds a new CCompositeFontFamily object by parsing the FontFamily
//      property syntax and building a corresponding list of FontLookups.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::CreateNew(
    _In_        IFontAndScriptServices   *pFontAndScriptServices,
    _In_        IFontCollection          *pFontCollection, // Font source on this element
    _In_        IPALUri                  *pBaseUri,            // Base uri for relative uris
    _In_        CSharedName              *pFontFamilyName,     // We AddRef this
    _Outptr_ CCompositeFontFamily    **ppCompositeFontFamily
)
{
    HRESULT               hr                   = S_OK;
    CCompositeFontFamily *pCompositeFontFamily = NULL;
    const WCHAR          *pParse               = NULL;
    const WCHAR          *pLimit               = NULL;
    XUINT32               iNameLength          = 0;
    XUINT32               cUsed                = 0;
    WCHAR                *pSingleFamilyBuffer  = NULL;
    XUINT32               cSingleFamilyBuffer  = 0;
    WCHAR                *pFamily              = NULL;
    XUINT32               cFamily              = NULL;
    WCHAR                *pSource              = NULL;
    XUINT32               cSource              = NULL;
    IFontCollection      *pFontSourceTypefaces = NULL;
    IPALUri              *pDefaultUri          = NULL;
    IFssFontFallback     *pSystemFontFallback  = NULL;
    IFssFontFallbackBuilder *pFontFallbackBuilder = NULL;


    pCompositeFontFamily = new CCompositeFontFamily(pFontAndScriptServices);


    // Copy the family name

    ASSERT(pFontFamilyName);
    pCompositeFontFamily->m_pFontFamilyName = pFontFamilyName;
    pFontFamilyName->AddRef();

    // We'll use a temporary string to contain the individual family names.
    // We allocate the length of the whole property string, guaranteed
    // to be enough.

    cSingleFamilyBuffer = pFontFamilyName->GetLength();
    pSingleFamilyBuffer = new WCHAR[cSingleFamilyBuffer];


    // Parse the name into a list of comma-separated families, each
    // containing an optional font source, and a family name.

    pParse = pFontFamilyName->GetString();
    pLimit = pFontFamilyName->GetString() + pFontFamilyName->GetLength();

    while (pParse < pLimit)
    {
        IFCEXPECT(pLimit-pParse < XUINT32_MAX);
        iNameLength = static_cast<XUINT32>(pLimit - pParse);
        TrimWhitespace(iNameLength, pParse, &iNameLength, &pParse);


        // Extract a single family name

        IFC(ExtractSingleFamily(
            pParse,
            iNameLength,
            pSingleFamilyBuffer,
           &cUsed,
           &pFamily,
           &cFamily,
           &pSource,
           &cSource
        ));

        ASSERT(cUsed <= iNameLength);

        if (cFamily > 0)
        {
            if (cSource == 0)
            {
                // No explicit font source provided
                // pFamily/cFamily already contain the de-escaped family name
                pFontSourceTypefaces = pFontCollection; // Use default font source
                AddRefInterface(pFontCollection);
            }
            else
            {
                if (pBaseUri == NULL)
                {
                    if (pDefaultUri == NULL)
                    {
                        IXcpHostSite *pHostSite = pFontAndScriptServices->GetCoreServices()->GetHostSite();
                        if (pHostSite)
                        {
                            pDefaultUri = pHostSite->GetBaseUri();
                            if (pDefaultUri)
                            {
                                pDefaultUri->AddRef();
                            }
                        }
                    }
                    if (pDefaultUri == NULL)
                    {
                        IFC(gps->UriCreate(8, L"file:///", &pDefaultUri));
                    }
                    pBaseUri = pDefaultUri;
                }


                // Note that if CreateFromUri does not find an already
                // downloaded or downloading font source in the cache, then
                // the entry it creates records the Uri but the download
                // is not started.
                // It is left up to later requests such as LookupGlyphTypeface
                // to start the download as necessary.

                IFCEXPECT_ASSERT(pSource != NULL);
                IFCEXPECT_ASSERT(cSource > 0);

                hr = pFontAndScriptServices->CreateFontCollectionFromUri(
                    pBaseUri,
                    cSource,
                    pSource,
                   &pFontSourceTypefaces
                );

                if (hr != S_OK)
                {
                    // in case of error, we want to display the text using the fallback font
                    hr = S_OK;
                    pFontSourceTypefaces = pFontCollection; // Use default font source
                    AddRefInterface(pFontCollection);
                }

                //ReleaseInterface(pSourceUri);
            }

            IFC(pCompositeFontFamily->CreateAndAddLookup(
                pFontSourceTypefaces,
                xephemeral_string_ptr(pFamily, cFamily),
                &pFontFallbackBuilder
            ));

            ReleaseInterface(pFontSourceTypefaces);
        }


        // Advance to the next family

        pParse      += cUsed;
        iNameLength -= cUsed;
    }


    //
    // Create the final composed font fallback lookup with the system font fallback.
    //

    IFC(pFontAndScriptServices->GetSystemFontFallback(&pSystemFontFallback));

    //
    // If there is a builder then we had more lookups than just the base font,
    // so we have to add the system font fallback mappings in.
    // Otherwise, just use the system font fallback and the base font will
    // come into play during mapping.
    //

    if (pFontFallbackBuilder)
    {
        IFC(pFontFallbackBuilder->AddMappings(pSystemFontFallback));

        IFC(pFontFallbackBuilder->CreateFontFallback(&pCompositeFontFamily->m_pFontFallback));
    }
    else
    {
        pCompositeFontFamily->m_pFontFallback = pSystemFontFallback;
        pSystemFontFallback = NULL;
    }

    // Success!

    *ppCompositeFontFamily = pCompositeFontFamily;
    pCompositeFontFamily   = NULL;

Cleanup:
    delete [] pSingleFamilyBuffer;
    delete pCompositeFontFamily;
    ReleaseInterface(pDefaultUri);
    ReleaseInterface(pFontFallbackBuilder);
    ReleaseInterface(pSystemFontFallback);
    RRETURN(hr);
}




//------------------------------------------------------------------------
//
//  Method:  CCompositeFontFamily::CreateFromDWrite
//
//  Synopsis:
//      Builds a new CCompositeFontFamily object by parsing the
//      xml description of a font file
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::CreateFromDWrite(
    _In_      IFontAndScriptServices   *pFontAndScriptServices,
    _In_      IFontCollection          *pFontCollection,
    _Outptr_  CCompositeFontFamily    **ppCompositeFontFamily
)
{
    const xstring_ptr c_strUltimateFallbackFontNameTH = XSTRING_PTR_FROM_STORAGE(c_strUltimateFallbackFontNameTHStorage);

    std::unique_ptr<CCompositeFontFamily> pCompositeFontFamily{ new CCompositeFontFamily(pFontAndScriptServices) };

    IFC_RETURN(CSharedName::Create(c_strUltimateFallbackFontNameTH, &pCompositeFontFamily->m_pFontFamilyName));

    // TODO: CCompositeFontFamily::DetermineDefaultLineMetrics() requires that the UltimateFallbackFont have
    // these values initialized.  We should probably force-calculate these from the font.  But for now, just
    // use the pre-TH values to avoid infinite recursion.
    pCompositeFontFamily->SetBaseline(0.9f);
    pCompositeFontFamily->SetLineSpacing(1.2f);
    pCompositeFontFamily->SetCapHeight(0.7f);

    // Our font fallback is just the system's.
    IFC_RETURN(pFontAndScriptServices->GetSystemFontFallback(&pCompositeFontFamily->m_pFontFallback));

    // LookupNominalFontFace expects us to have one font in our lookup, which is whatever is the default
    // in the system fallback table.  We know it's Segoe UI, so use that.
    IFC_RETURN(pCompositeFontFamily->CreateAndAddLookup(pFontCollection, c_strUltimateFallbackFontNameTH, nullptr));

    // Success!
    *ppCompositeFontFamily = pCompositeFontFamily.release();
    return S_OK;
}




//------------------------------------------------------------------------
//
//  Method: CCompositeFontFamily destructor
//
//  Releases the FontLookups belonging to this font family, as well as the
//  SpanBases of FontLookups and FontTypefaces
//
//------------------------------------------------------------------------

CCompositeFontFamily::~CCompositeFontFamily()
{
    ReleaseInterface(m_pFontFamilyName);

    // Delete lookups

    for (XUINT32 i=0; i<m_cFontLookups; i++)
    {
        ReleaseInterface((*m_pFontLookups)[i]);
    }

    // Delete SpanBase lists of lookups and typefaces
    delete m_pFontLookups;
    ReleaseInterface(m_pFontFallback);
    ReleaseInterface(m_pBaseFontName);
    ReleaseInterface(m_pBaseFontCollection);
}


//---------------------------------------------------------------------------
//
//  Function:
//      MapChar
//
//  Synopsis:
//      Maps the ASCII digits to the translated native digits unicode codepoints.
//
//---------------------------------------------------------------------------
inline void MapChar(_In_ const NumberSubstitutionData* pMappingData, WCHAR nominalChar, _Outptr_result_z_ WCHAR const** ppMappedString)
{
    switch (nominalChar)
    {
        case L'0':
        case L'1':
        case L'2':
        case L'3':
        case L'4':
        case L'5':
        case L'6':
        case L'7':
        case L'8':
        case L'9':   *ppMappedString = pMappingData->numerals[nominalChar - L'0']; break;
        case L'%':   *ppMappedString = pMappingData->percentSymbol; break;
        case L'.':   *ppMappedString = pMappingData->decimalSeparator; break;
        case L',':   *ppMappedString = pMappingData->groupSeparator; break;
        default:     ASSERT(FALSE);
    }
}


//------------------------------------------------------------------------
//
//  Class: FontFallbackAnalysisSource
//
//  Helper for IDWriteFontFallbacks' MapCharacters to wrap a text string
//  and locale to the analysis source that DWrite wants.
//
//------------------------------------------------------------------------

struct FontFallbackAnalysisSource : PALText::ITextAnalysisSource
{
    FontFallbackAnalysisSource(
        _In_z_ const WCHAR *pText,
        XUINT32 textLength,
        _In_z_ const WCHAR *pLocaleName,
        _In_z_ const WCHAR *pLocaleNameList)
        : m_pText(pText)
        , m_textLength(textLength)
        , m_pLocaleName(pLocaleName)
        , m_pLocaleNameList(pLocaleNameList)
    {
    }

    HRESULT GetTextAtPosition(
        _In_ XUINT32 textPosition,
        _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
        _Out_ XUINT32 *pTextLength
        ) override
    {
        if (textPosition <= m_textLength)
        {
            *ppTextString = m_pText + textPosition;
            *pTextLength = m_textLength - textPosition;
            return S_OK;
        }
        return E_UNEXPECTED;
    }

    HRESULT GetTextBeforePosition(
        _In_ XUINT32 textPosition,
        _Outptr_result_buffer_(*pTextLength) WCHAR const **ppTextString,
        _Out_ XUINT32 *pTextLength
        ) override
    {
        if (textPosition <= m_textLength)
        {
            *ppTextString = m_pText;
            *pTextLength = textPosition;
            return S_OK;
        }
        return E_UNEXPECTED;
    }

    // Get paragraph reading direction.
    PALText::ReadingDirection::Enum GetParagraphReadingDirection() override
    {
        // NOTE: Not used by font fallback.
        return PALText::ReadingDirection::LeftToRight;
    }

    // Get locale name on the range affected by it.
    HRESULT GetLocaleName(
        _In_ XUINT32 textPosition,
        _Out_ XUINT32 *pTextLength,
        _Outptr_result_z_ WCHAR const **ppLocaleName
        ) override
    {
        *pTextLength = m_textLength - textPosition;
        *ppLocaleName = m_pLocaleName;
        return S_OK;
    }

    HRESULT GetLocaleNameList(
        _In_ UINT32 textPosition,
        _Out_ UINT32* pTextLength,
        _Outptr_result_z_ WCHAR const** pplocaleNameList
    ) override
    {
        *pTextLength = m_textLength - textPosition;
        *pplocaleNameList = m_pLocaleNameList;
        return S_OK;
    }


    // Get number substitution on the range affected by it.
    HRESULT GetNumberSubstitution(
        _In_ XUINT32 textPosition,
        _Out_ XUINT32* pTextLength,
        _Outptr_ IDWriteNumberSubstitution** ppNumberSubstitution
        ) override
    {
        *pTextLength = m_textLength - textPosition;
        *ppNumberSubstitution = NULL;
        return S_OK;
    }

private:
    // We only expect to be stack allocated
    XUINT32 AddRef() override { return 1; }
    XUINT32 Release() override { return 1; }

    const WCHAR *m_pText;
    XUINT32 m_textLength;
    const WCHAR *m_pLocaleName;
    const WCHAR *m_pLocaleNameList;
};


//------------------------------------------------------------------------
//
//  Method: CCompositeFontFamily::MapCharacters
//
//  Map characters from the specified range in the ITextAnalysisSource to
//  a single font.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::MapCharacters(
    _In_z_ const WCHAR *pText,
    XUINT32 textLength,
    _In_z_ const WCHAR *pLocaleName,
    _In_z_ const WCHAR *pLocaleNameList,
    _In_opt_ const NumberSubstitutionData* pNumberSubstitutionData,
    _In_ CWeightStyleStretch weightStyleStretch,
    _Outptr_ IFssFontFace **ppMappedFontFace,
    _Out_ XUINT32 *pMappedLength,
    _Out_ XFLOAT *pMappedScale
)
{
    WCHAR const* pBaseFontName = m_pBaseFontName ? m_pBaseFontName->GetString() : NULL;
    bool haveBaseFont = (pBaseFontName && pBaseFontName[0] && m_pBaseFontCollection);
    XUINT32 textPosition = 0;

    *ppMappedFontFace = NULL;
    *pMappedLength = 0;
    *pMappedScale = 1;

    //
    // For number substituted runs we expect them to only have numbers in them, but because
    // the substitution strings can have more characters than the original, it's awkward to
    // map it all at once.  So just map one character at a time.
    //

    if (pNumberSubstitutionData)
    {
        MapChar(pNumberSubstitutionData, pText[0], &pText);
        textLength = static_cast<XUINT32>(wcslen(pText));
    }

    FontFallbackAnalysisSource analysisSource(pText, textLength, pLocaleName, pLocaleNameList);

    //
    // Leading spaces should inherit the following font.  This will only happen for
    // text at the beginning of a chunk with no preceding text -- spaces after text
    // will inherit the following font (handled after MapCharacters).
    //

    XUINT32 leadingSpaceLength = 0;
    while ((leadingSpaceLength < textLength) &&
           (pText[leadingSpaceLength] == UNICODE_SPACE))
    {
        leadingSpaceLength++;
    }

    //
    // If the entire run consists of space, it's important that we still pass the text to
    // MapCharacters to return a useable font. Otherwise we could incorrectly choose a
    // font that doesn't actually support the space character, or a font that supports
    // the character but lacks the glyph data because it has not been downloaded yet,
    // which could result in an error later while measuring or drawing.
    //

    if (leadingSpaceLength == textLength)
    {
        leadingSpaceLength = 0;
    }
    else
    {
        textPosition += leadingSpaceLength;
        textLength -= leadingSpaceLength;
    }

    if (textLength > 0)
    {
        IFC_RETURN(m_pFontFallback->MapCharacters(
            &analysisSource,
            textPosition,
            textLength,
            haveBaseFont ? m_pBaseFontCollection : NULL,
            haveBaseFont ? pBaseFontName : NULL,
            MIN(MAX(1, weightStyleStretch.m_weight), 999), // Scope to a valid weight or DWrite will complain
            MIN(MAX(0, weightStyleStretch.m_style), 2),    // Scope to a valid style
            MIN(MAX(1, weightStyleStretch.m_stretch), 9),  // Scope to a valid stretch
            pMappedLength,
            ppMappedFontFace,
            pMappedScale));
    }

    *pMappedLength += leadingSpaceLength;

    //
    // DWrite doesn't treat space as neutral but we want to, so make sure trailing spaces
    // inherit the previous font.
    //

    {
        auto* pMappedFont = *ppMappedFontFace;
        XUINT32 trailingTextEnd = *pMappedLength;

        while ((trailingTextEnd < textLength) &&
            (pText[trailingTextEnd] == UNICODE_SPACE) &&
            (pMappedFont != nullptr && pMappedFont->IsCharacterLocal(UNICODE_SPACE)))
        {
            trailingTextEnd++;
        }

        *pMappedLength = trailingTextEnd;
    }

    //
    // For number substituted strings we map just one character at a time, so change the
    // mapped length to 1 on the way out.
    //

    if (pNumberSubstitutionData)
    {
        ASSERT(wcslen(pText) == *pMappedLength);
        *pMappedLength = 1;
    }

    //
    // If we were unable to find the character in the users requested font then we need to
    // try the ultimate fallback font, and if that fails we make do with the missing glyph
    // from the users requested font, or the ultimate fallback font if no font was requested
    // by the user.
    //

    if (*ppMappedFontFace == NULL)
    {
        IFC_RETURN(LookupNominalFontFace(weightStyleStretch, ppMappedFontFace));

        if (*ppMappedFontFace == NULL)
        {
            //
            // Last resort is the ultimate font from the system.
            //

            CCompositeFontFamily *pUltimateFontFamily;
            IFC_RETURN(m_pFontAndScriptServices->GetUltimateFallbackFontFamily(&pUltimateFontFamily));
            IFC_RETURN(pUltimateFontFamily->LookupNominalFontFace(weightStyleStretch, ppMappedFontFace));
            // fail fast if mapped fontface is still null after ultimate font fall back.
            XCP_FAULT_ON_FAILURE(*ppMappedFontFace != nullptr);
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: CCompositeFontFamily::LookupNominalGlyphTypeface
//
//  For a given font family, weight, style and stretch, determines which
//  glyph typeface to use for the missing glyph.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CCompositeFontFamily::LookupNominalFontFace(
    _In_        CWeightStyleStretch weightStyleStretch,
    _Outptr_ IFssFontFace         **ppFontFace
)
{
    HRESULT hr = S_OK;

    // Simply use the nominal glyphtypeface
    // implied by the first font. It will provide a glyph typeface
    // that generates the (correct shape of) missing glyph.

    if(m_cFontLookups > 0)
    {
        hr = (*m_pFontLookups)[0]->LookupNominalFontFace(
                weightStyleStretch.m_weight,
                weightStyleStretch.m_style,
                weightStyleStretch.m_stretch,
                ppFontFace);

        // Explicitly consider this particular error for downloadable fonts, treating it as if
        // there are no fonts in the family. The caller is already prepared for this case and
        // is prepared for a null pointer, resorting to a fallback path and alternate font
        // like XAML's ultimate fallback family.
        if (hr == DWRITE_E_REMOTEFONT)
        {
            hr = S_OK;
            *ppFontFace = NULL;
        }
        IFC(hr);
    }
    else
    {
        // There are no fonts in this composite font family
        *ppFontFace = NULL;
    }

Cleanup:
    return hr;
}

const xref_ptr<CFontTypeface>& CCompositeFontFamily::GetFontTypeface(
    _In_            CWeightStyleStretch   weightStyleStretch
)
{
    auto& value = m_fontTypefaces[weightStyleStretch];
    if (!value)
    {
        value = make_xref<CFontTypeface>(this, weightStyleStretch);
    }
    return value;
}

