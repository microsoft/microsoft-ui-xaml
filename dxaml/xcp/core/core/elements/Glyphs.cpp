// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "FontAndScriptServices.h"
#include "rtsinterop.h"
#include "textrunproperties.h"
#include "TextDrawingContext.h"
#include "D2DTextDrawingContext.h"

using namespace DirectUI;

class CGlyphTypefaceCollection;

//------------------------------------------------------------------------
//
//  Method:   dtor
//
//  Synopsis:
//      Destructor for glyphs object
//
//------------------------------------------------------------------------

CGlyphs::~CGlyphs()
{
    ReleaseInterface(m_pFill);
    delete [] m_pGlyphIndices;
    delete [] m_pUOffsets;
    delete [] m_pVOffsets;
    delete    m_pBounds;

    TRACE(
        TraceFontLoad,
        L"Glyphs[%I64x] ReleaseInterface m_pDefaultBaseUri %I64x refcount %d",
        reinterpret_cast<XUINT64>(this),
        reinterpret_cast<XUINT64>(m_pDefaultBaseUri),
        m_pDefaultBaseUri == NULL ? 0 : (m_pDefaultBaseUri->AddRef(),m_pDefaultBaseUri->Release())
    );

    ReleaseInterface(m_pDefaultBaseUri);

    // We need to Release the font face since we now AddRef it in the FontFace creation methods.
    // This is to be in sync with DWrite.
    ReleaseInterface(m_pFontFace);

    ReleaseInterface(m_pTextDrawingContext);
}

//------------------------------------------------------------------------
//
//  Method:   SetValue
//
//  Synopsis:
//      Sets a value in the dependency object.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT
CGlyphs::SetValue(_In_ const SetValueParams& args)
{
    StyleSimulations prevStyleSimulations = m_nStyleSimulations;

    // Glyphs don't have children
    if (args.m_pDP == nullptr)
    {
        IFC_RETURN(E_FAIL);
    }

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Glyphs_FontUri)
    {
        // don't allow this value to be null for now
        if (args.m_value.GetType() == valueNull)
        {
            IFC_RETURN(E_INVALIDARG);
        }

        // if the following checks are not true, then it will be a failure in SetValue
        // because of the Value/Property type mismatch.  To avoid being inconsistent with any
        // future cleverness in SetValue to report that failure, we can let SetValue handle it.

        if (args.m_value.AsObject() && args.m_value.AsObject()->GetTypeIndex() == KnownTypeIndex::String)
        {
            const CString* pString;
            IFC_RETURN(DoPointerCast(pString, args.m_value));
            IFC_RETURN(GetContext()->CheckUri(pString->m_strString, udaNone));
        }

        // When ever the URI changes we'll force the glyph typeface to reload when
        // EnsureFontFace is called.
        ReleaseInterface(m_pFontFace);
    }

    // Call the base SetValue function.
    IFC_RETURN(CFrameworkElement::SetValue(args));

    if (args.m_pDP->GetIndex() == KnownPropertyIndex::Glyphs_OriginY)
    {
        m_bIsOriginYSet = TRUE;
    }
    else if (args.m_pDP->GetIndex() == KnownPropertyIndex::Glyphs_StyleSimulations)
    {
        // Whenever the StyleSimulations change we'll force the glyph typeface to reload when
        // EnsureFontFace is called.
        if (prevStyleSimulations != m_nStyleSimulations)
        {
            ReleaseInterface(m_pFontFace);
        }
    }


    // Changing most attributes will require regeneration of the glyphs and positions.

    switch (args.m_pDP->GetIndex())
    {
    case KnownPropertyIndex::Glyphs_UnicodeString:
    case KnownPropertyIndex::Glyphs_Indices:
    case KnownPropertyIndex::Glyphs_FontUri:
    case KnownPropertyIndex::Glyphs_StyleSimulations:
//  case KnownPropertyIndex::Glyphs_IsSideWays:  //  re-enable this if/when the property comes back (see ISSUE#1471)
    case KnownPropertyIndex::Glyphs_FontRenderingEmSize:
    case KnownPropertyIndex::Glyphs_OriginX:
    case KnownPropertyIndex::Glyphs_OriginY:
        m_fGlyphsDirty = TRUE;
        InvalidateMeasure();
        break;
    }

    return S_OK;
}




//------------------------------------------------------------------------
//
//  Method:  GetDefaultBaseUri
//
//  Determine the base uri for cases when 'GetBaseUri()' returns NULL.
//  This is either the host site base uri, or if there's no host
//  site we're running in agview, and the base uri is 'file:///'.
//
//------------------------------------------------------------------------

IPALUri *CGlyphs::GetDefaultBaseUri()
{
    if (m_pDefaultBaseUri == NULL)
    {

        IXcpHostSite *pHostSite = GetContext()->GetHostSite();

        if (pHostSite)
        {
            TRACE(TraceFontLoad, L"GetDefaultBaseUri getting host site base uri");
            m_pDefaultBaseUri = pHostSite->GetBaseUri();
            AddRefInterface(m_pDefaultBaseUri);
        }

        if (m_pDefaultBaseUri == NULL)
        {
            TRACE(TraceFontLoad, L"GetDefaultBaseUri using \"file:////\" base uri");
            void(gps->UriCreate(8, L"file:///", &m_pDefaultBaseUri));
        }
    }


    if (m_pDefaultBaseUri != NULL)
    {
        m_pDefaultBaseUri->AddRef();
    }

    return m_pDefaultBaseUri;
}




//------------------------------------------------------------------------
//
//  Method:  EnsureFontFace
//
//  Convenience function to access the font uri and make the font available in
//  the m_pFontFace member.
//
//  Returns E_PENDING if the font is still downloading.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CGlyphs::EnsureFontFace()
{
    HRESULT    hr        = S_OK;
    CTextCore *pTextCore = NULL;
    IPALUri   *pBaseUri  = NULL;

    if (!m_pFontFace)
    {
        // compatibility with WPF, if FontUri has not been set yet, we skip the validation
        // to allow CGlyphs to be constructed through managed code
        if (m_strFontUri.IsNull())
        {
            IFC_NOTRACE(E_PENDING);
        }

        // Determine the base uri of this Glyphs element

        pBaseUri = GetBaseUri();
        if (pBaseUri == NULL)
        {
            if (!IsActive())
            {
                // This Glyphs element doesn't have a base URI, and we are not in the live tree.
                // We don't have enough context to download the correct font.
                // Return E_PENDING, just as we do if the font URI has not been set.
                IFC_NOTRACE(E_PENDING);
            }

            // This Glyphs element doesn't have a base URI, but we are in the live tree.
            // Fall back to using the app's base URI to download the font.
            pBaseUri = GetDefaultBaseUri();
        }

        #if DBG
        {
            WCHAR *pstrbaseuri = NULL;
            XUINT32 cbaseuri = XINTERNET_MAX_PATH_LENGTH+1;
            pstrbaseuri = new WCHAR[cbaseuri];
            if (pstrbaseuri)
            {
                pstrbaseuri[0]=0;
                if (pBaseUri)
                {
                    void(pBaseUri->GetCanonical(&cbaseuri,pstrbaseuri));
                }
                TRACE(
                    TraceFontLoad,
                    L"CGlyphs[%I64x]::EnsureFontFace, baseuri=\'%s\', relativeuri=\'%s\'",
                    reinterpret_cast<XUINT64>(this),
                    pstrbaseuri,
                    m_strFontUri.GetBuffer()
                );
                delete [] pstrbaseuri;
            }
        }
        #endif

        IFC(GetContext()->GetTextCore(&pTextCore));

        IFssFontFace *pFontFace = NULL;
        IFontAndScriptServices *pFontAndScriptServices = NULL;
        IFC(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));

        // The CreateCustomFontFace returns an AddRef'ed FontFace.
        hr = pFontAndScriptServices->CreateCustomFontFace(
            this,
            m_strFontUri.GetCount(),
            m_strFontUri.GetBuffer(),
            pBaseUri,
            FALSE, // local font
            static_cast<FssFontSimulations::Enum>(m_nStyleSimulations),
           &pFontFace
        );

        m_pFontFace = pFontFace;
        pFontFace = NULL;

        //TODO: MERGE: Ideally specific errors should be handled here rather than catching
        //      error code. The problem with trying to catch specific error codes is that the user can
        //      pass any Uri, even invalid ones as the font source. This in turn means that there is
        //      a large set of errors that can be returned. Preferably SetValue should fail when
        //      an invalid source is set an exception thrown before the invalid value is stored.
        //      Bug #22140 tracks throwing the exception in managed code.

        // If the download failed, mark the glyph as "still downloading" so that rendering is suspended.
        if(FAILED(hr))
        {
            hr = E_PENDING;
        }
    }

Cleanup:
    ReleaseInterface(pBaseUri);
    return hr;
}




//------------------------------------------------------------------------
//
//  Method: ParseInteger
//
//  Wraps the SignedFromDecimalString method for the convenience of the
//  ParseIndices method.
//
//  Implements checking for min and max values.
//
//------------------------------------------------------------------------

inline _Check_return_ HRESULT ParseInteger(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ XUINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    XINT32 min,
    XINT32 max,
    _Out_ XINT32 *pResult
    )
{
    HRESULT hr =
        SignedFromDecimalString(cString, pString, pcSuffix, ppSuffix, pResult);

    if (!SUCCEEDED(hr))
    {
        TRACE(TraceGlyphs, L"Expected integer but found '%s'", pString);
        IFC(hr);
    }

    if (*pResult < min)
    {
        TRACE(TraceGlyphs, L"Expected integer greater than or equal to %d but found %d", min, *pResult);
        IFC(E_UNEXPECTED);
    }

    if (*pResult > max)
    {
        TRACE(TraceGlyphs, L"Expected integer less than or equal to %d but found %d", max, *pResult);
        IFC(E_UNEXPECTED);
    }

    hr = S_OK;

Cleanup:
    return hr;
}



//------------------------------------------------------------------------
//
//  Method: ParseFloat
//
//  Wraps the FloatFromString method for the convenience of the
//  ParseIndices method.
//
//  Implements checking for min and max values.
//
//------------------------------------------------------------------------

inline _Check_return_ HRESULT ParseFloat(
    _In_ XUINT32 cString,
    _In_reads_(cString) const WCHAR* pString,
    _Out_ XUINT32* pcSuffix,
    _Outptr_result_buffer_(*pcSuffix) const WCHAR** ppSuffix,
    _Out_ XFLOAT* pResult
    )
{
    HRESULT hr =
        FloatFromString(cString, pString, pcSuffix, ppSuffix, pResult);

    if (!SUCCEEDED(hr))
    {
        TRACE(TraceGlyphs, L"Expected float but found '%s'", *pString);
        IFC(hr);
    }

    hr = S_OK;

Cleanup:
    return hr;
}



//------------------------------------------------------------------------
//
//  Method:   ParseIndices
//
//  Synopsis:
//
//  Parses the glyph indices mini-language.
//
//  The following are generated:
//
//    o  pGlyphIndices.   The glyph indices themselves. Where a glyph index is
//                        omitted in the markup, -1 is used. In this case the
//                        glyph index should be determined by looking up the
//                        corresponding character code in the font cmap.
//
//    o  pGlyphUOffsets.  Baseline aligned offset to apply to a glyph. 0 if not
//                        specified.
//
//    o  pGlyphVOffsets.  Baseline perpendicular offset to apply to glyph. 0 if
//                        not specified.
//
//    o  pAdvanceWidths.  Amount to advance parallel to the baseline before rendering
//                        the next glyph. Defaults to 0x80000000. If not specified
//                        The advance width for this glyph should be determined from
//                        the font HMTX table.
//
//    o  pClusterMap.     An array containing one entry per 16 bit codepoint in the
//                        character string being rendered, the entry value being
//                        the offset into the glyph string of the first glyph that
//                        represents the cluster of characters/glyphs that contains
//                        this character.
//
//    o  pCodepointCount. The number of codepoints described by the Indices property.
//                        (This may be less than teh number of codepoints in the
//                        UnicodeString property, when codepoints beyond CodepointCount
//                        should be rendered with glyph index determined from the cmap
//                        and advance width from the hmtx.
//
//    o  pGlyphCount.     The number of glyphs described by the indices property.
//
//
//------------------------------------------------------------------------

_Check_return_ HRESULT ParseIndices(
    _In_                               const xstring_ptr& strIndices,
    _Inout_                            CGlyphPathBuilder *pPathBuilder
)
{
    ENTERSECTION(ParseIndices);

    const WCHAR *pParse = strIndices.GetBuffer();
    XUINT32  cParse = strIndices.GetCount();
    HRESULT  hr     = E_UNEXPECTED;

    XUINT32 codepointCountToStartOfCluster = 0;
    XUINT32 glyphCountToStartOfCluster     = 0;

    // Cluster parsing:
    // In the absence of (n:m) cluster specifications, each glyph is considered
    // to correspond 1::1 with a (16 bit) codepoint, in it's own cluster.
    // There presence of (n:m) markup implies that the next m glyph index specifications
    // make a cluster that represents n codepoints. (n and m may be omitted, when they
    // default to 1, so that for example the glyph index for a ligature glyph
    // such as 'fi' may be preceeded by '(2)' implying 1 glyph representing 2 codepoints.
    // At the start of the loop, and whenever we finish a cluster, we set codepointsInCluster
    // and glyphsInCluster to 1.
    // Whenever finishing a glyph we increment glyphCountIntoCluster.
    // When GlyphCountIntoCluster reaches glyphsInCluster the cluster is complete.

    XINT32 codepointsInCluster   = 1;
    XINT32 glyphsInCluster       = 1;
    XINT32 glyphCountIntoCluster = 0;

    XINT32 bFirstRun = TRUE;

    // One loop iteration per glyph, but run at least once even if cParse == 0
    // to handle the case of Indices="", which is legal.
    while (cParse > 0 || bFirstRun)
    {
        bFirstRun = FALSE;

        TrimWhitespace(cParse, pParse, &cParse, &pParse);

        // The various bits of information available for a glyph index
        XINT32 glyphIndex     = 0xffff;        // Invalid glyph index used to mean not present
        XFLOAT advanceWidth   = -XFLOAT_MAX;   // Unsupported value meaning not present (most negative float value)
        XFLOAT uOffset        = 0.0f;
        XFLOAT vOffset        = 0.0f;


        // Process any '(n:m)' cluster specification
        if (cParse>0 && *pParse==L'(')
        {
            if (codepointsInCluster != 1  ||  glyphsInCluster != 1)
            {
                // We're already in the middle of a non 1::1 cluster. This is not expected.
                TRACE(TraceGlyphs, L"New cluster spec starts in middle of cluster at position %d in '%s'", pParse - strIndices.GetBuffer(), strIndices.GetBuffer());
                IFC(E_UNEXPECTED);
            }

            pParse++;  cParse--;  // Skip the "("
            TrimWhitespace(cParse, pParse, &cParse, &pParse);

            // Parse a cluster specification
            IFC(ParseInteger(cParse, pParse, &cParse, &pParse, 1, 65535, &codepointsInCluster));
            TrimWhitespace(cParse, pParse, &cParse, &pParse);

            // An optional glyph count must be preceeded by ":"

            if (cParse > 0  &&  *pParse == L':')
            {
                pParse++;  cParse--;  // Skip the ":"
                TrimWhitespace(cParse, pParse, &cParse, &pParse);
                IFC(ParseInteger(cParse, pParse, &cParse, &pParse, 1, 65535, &glyphsInCluster));
                TrimWhitespace(cParse, pParse, &cParse, &pParse);
            }

            // There must be a closing ')' at the end of a cluster spec.
            if (cParse<=0  ||  *pParse != L')')
            {
                TRACE(TraceGlyphs, L"Missing ')' in cluster spec at position %d in '%s'", pParse - strIndices.GetBuffer(), strIndices.GetBuffer());
                IFC(E_UNEXPECTED);
            }

            pParse++;  cParse--;  // Skip the ")"
            TrimWhitespace(cParse, pParse, &cParse, &pParse);
        }


        if (cParse > 0  &&  *pParse != L';')
        {
            if (cParse > 0  &&  *pParse != L',')
            {
                // Parse optional glyph index
                IFC(ParseInteger(cParse, pParse, &cParse, &pParse, 0, 65535, &glyphIndex));
                TrimWhitespace(cParse, pParse, &cParse, &pParse);
            }

            if (cParse > 0  &&  *pParse != L','  &&  *pParse != L';')
            {
                TRACE(TraceGlyphs, L"Glyphs parse error in '%s'", strIndices.GetBuffer());
                TRACE(TraceGlyphs, L"                       %*s^expected ',' or ';'", pParse - strIndices.GetBuffer(), L"");

                IFC(E_UNEXPECTED);
            }

            if (cParse > 0  &&  *pParse == L',')
            {
                // Parse optional advance width and subsequent values
                pParse++;  cParse--;  // Skip the ","
                TrimWhitespace(cParse, pParse, &cParse, &pParse);

                if (cParse > 0  &&  *pParse != L','  &&  *pParse != L';')
                {
                    // Parse optional advance width
                    IFC(ParseFloat(cParse, pParse, &cParse, &pParse, &advanceWidth));
                    TrimWhitespace(cParse, pParse, &cParse, &pParse);
                }

                if (cParse > 0  &&  *pParse == L',')
                {
                    // Parse optional combining uOffset and subsequent values
                    pParse++;  cParse--;  // Skip the ","
                    TrimWhitespace(cParse, pParse, &cParse, &pParse);

                    if (cParse > 0  &&  *pParse != L','  &&  *pParse != L';')
                    {
                        // Parse optional uOffset width
                        IFC(ParseFloat(cParse, pParse, &cParse, &pParse, &uOffset));
                        TrimWhitespace(cParse, pParse, &cParse, &pParse);
                    }

                    if (cParse > 0  &&  *pParse == L',')
                    {
                        // Parse optional vOffset
                        pParse++;  cParse--;  // Skip the ","
                        TrimWhitespace(cParse, pParse, &cParse, &pParse);

                        if (cParse > 0  &&  *pParse != L','  &&  *pParse != L';')
                        {
                            // Parse optional vOffset
                            IFC(ParseFloat(cParse, pParse, &cParse, &pParse, &vOffset));
                            TrimWhitespace(cParse, pParse, &cParse, &pParse);
                        }
                    }
                }
            }
        }

        // Return glyph information where requested

        XUINT32 GlyphBufferOffset = glyphCountToStartOfCluster + glyphCountIntoCluster;
        IFC(pPathBuilder->EnsureGlyphCapacity(GlyphBufferOffset));
        _Analysis_assume_(pPathBuilder->m_GlyphCapacity > GlyphBufferOffset);
        pPathBuilder->m_pIndices[GlyphBufferOffset]  = XUINT16(glyphIndex);
        pPathBuilder->m_pAdvances[GlyphBufferOffset] = advanceWidth;
        pPathBuilder->m_pUOffsets[GlyphBufferOffset] = uOffset;
        pPathBuilder->m_pVOffsets[GlyphBufferOffset] = vOffset;

        // Complete handling this cluster

        glyphCountIntoCluster++;

        if (glyphCountIntoCluster >= glyphsInCluster)
        {
            // Set cluster map entries
            IFC(pPathBuilder->EnsureCodepointCapacity(codepointCountToStartOfCluster+codepointsInCluster-1));
            _Analysis_assume_(pPathBuilder->m_CodepointCapacity >= codepointCountToStartOfCluster+codepointsInCluster);
            for (int i=0; i<codepointsInCluster; i++)
            {
                pPathBuilder->m_pClusterMap[codepointCountToStartOfCluster+i] = XUINT16(glyphCountToStartOfCluster);
            }

            // Advance to next cluster
            codepointCountToStartOfCluster += codepointsInCluster;
            glyphCountToStartOfCluster     += glyphsInCluster;
            glyphCountIntoCluster = 0;

            // Set default cluster size for next cluster
            codepointsInCluster = 1;
            glyphsInCluster     = 1;
        }


        // Everything about this glyph has been handled. At this point we
        // must either be at the end of the string, or there must be a ';'

        if (cParse > 0)
        {
            if (*pParse != L';')
            {
                TRACE(TraceGlyphs, L"Expected ';' at position %d in '%s'", pParse - strIndices.GetBuffer(), strIndices.GetBuffer());
                IFC(E_UNEXPECTED);
            }

            pParse++;  cParse--;  // Skip the ";"
            TrimWhitespace(cParse, pParse, &cParse, &pParse);
        }
    }

    // Check we're not part way through a cluster

    if (glyphCountIntoCluster > 0)
    {
        IFC(E_UNEXPECTED); // Indices property was incomplete
    }

    // We made it to the end of the string without error

    // The Glyphs spec requires that at least one character or glyph has been specified

    IFCEXPECT(glyphCountToStartOfCluster > 0  ||  codepointCountToStartOfCluster > 0);

    pPathBuilder->m_GlyphCount     = glyphCountToStartOfCluster;
    pPathBuilder->m_CodepointCount = codepointCountToStartOfCluster;

    hr = S_OK;

Cleanup:
    LEAVESECTION(ParseIndices);
    return hr;
}




//------------------------------------------------------------------------
//
//  Method:   DetermineGlyphsAndPositions
//
//  Synopsis:
//
//  Interpret the glyph indices and UnicodeString parameters
//
//  Combine the indices and UnicodeString parameters to generate the list
//  of glyph indices and positions that will be required for rendering.
//
//  Note: If basic parsing succeeds but the font is not yet ready,
//  returns E_PENDING.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CGlyphs::DetermineGlyphsAndPositions()
{
    HRESULT         hr                      = E_FAIL;
    XFLOAT          unitsPerEm              = 0;
    XUINT32         fullCodepointCount      = 0;
    XUINT32         fullGlyphCount          = 0;
    XUINT32         iCodepoint              = 0;
    XUINT32         bNonZeroVOffsetsPresent = FALSE;


    ENTERSECTION(GlyphsFinalize);

    CGlyphPathBuilder *pPathBuilder = NULL;
    IFC(GetContext()->GetGlyphPathBuilder(&pPathBuilder));

    // Some properties are required

    IFCEXPECT(m_eFontRenderingEmSize >= 0.0f);


    // In Jolt v1, IsSideways MUST be false

    IFCEXPECT(!m_bIsSideways);

    // compatibility with WPF, if FontUri has not been set yet or FontFace not set from a SetSource, we skip the validation
    // to allow CGlyphs to be constructed through managed code
    if ((m_pFontFace == NULL) && m_strFontUri.IsNull())
    {
        hr = S_OK;
        goto Cleanup;
    }

    // Determine size requirements for index and position arrays

    if (!m_strIndices.IsNull())
    {
        IFC(ParseIndices(
            m_strIndices,
            pPathBuilder
        ));
        //TRACE(TraceGlyphs, L"Parsed glyph indices property: %d codepoints, %d glyphs", codepointCount, glyphCount);

        fullCodepointCount = pPathBuilder->m_CodepointCount;
        fullGlyphCount     = pPathBuilder->m_GlyphCount;

        // If there is a UnicodeString property, it may describe characters beyond those
        // described by the indices property.

        if (!m_strUnicodeString.IsNull())
        {
            if (m_strUnicodeString.GetCount() > pPathBuilder->m_CodepointCount)
            {
                // Characters describe by only the UnicodeString are by definition
                // one character to one glyph
                fullGlyphCount += m_strUnicodeString.GetCount() - pPathBuilder->m_CodepointCount;
                fullCodepointCount = m_strUnicodeString.GetCount();
            }
        }
    }
    else
    {
        pPathBuilder->m_GlyphCount     = 0;
        pPathBuilder->m_CodepointCount = 0;

        if (!m_strUnicodeString.IsNull())
        {
            fullCodepointCount = m_strUnicodeString.GetCount();
            fullGlyphCount     = m_strUnicodeString.GetCount();
        }
        else
        {
            fullCodepointCount = 0;
            fullGlyphCount     = 0;
        }
    }

//    WPF compat breaking change reverted to preserve compatibility with Silverlight 1.0:
//    if ((fullCodepointCount == 0) && (fullGlyphCount == 0))
//    {
//        TRACE(TraceAlways, L"Glyphs Indices and UnicodeString properties cannot both be empty");
//        IFC(E_UNEXPECTED);
//    }


    // Fill out the buffers where the UnicodeString provides more information

    if (!m_strUnicodeString.IsNull())
    {
        XUINT32 iGlyph = pPathBuilder->m_GlyphCount;
        IFC(pPathBuilder->EnsureCodepointCapacity(fullCodepointCount));
        IFC(pPathBuilder->EnsureGlyphCapacity(fullGlyphCount));
        _Analysis_assume_(pPathBuilder->m_CodepointCapacity > fullCodepointCount);
        _Analysis_assume_(pPathBuilder->m_GlyphCapacity > fullGlyphCount);
        for (iCodepoint=pPathBuilder->m_CodepointCount; iCodepoint<fullCodepointCount && iGlyph<fullGlyphCount ; iCodepoint++, iGlyph++)
        {
            pPathBuilder->m_pIndices[iGlyph]        = 0xffff;      // Will look up glyph indices below
            pPathBuilder->m_pUOffsets[iGlyph]       = 0.0f;
            pPathBuilder->m_pVOffsets[iGlyph]       = 0.0f;
            pPathBuilder->m_pAdvances[iGlyph]       = -XFLOAT_MAX;  // Will look up advance width below
            pPathBuilder->m_pClusterMap[iCodepoint] = XUINT16(iGlyph);
        }
    }

    ASSERT(pPathBuilder->m_CodepointCapacity > fullCodepointCount);
    ASSERT(pPathBuilder->m_GlyphCapacity > fullGlyphCount);

    // Fill in glyph indices where not specified.

    iCodepoint = 0;
    while (iCodepoint < fullCodepointCount)
    {
        // Determine cluster size in codepoints

        XUINT32 clusterCodepointCount = 1;
        while (     iCodepoint + clusterCodepointCount < fullCodepointCount
               &&   pPathBuilder->m_pClusterMap[iCodepoint+clusterCodepointCount] == pPathBuilder->m_pClusterMap[iCodepoint])
        {
            clusterCodepointCount++;
        }

        // Determine cluster size in glyphs

        XUINT32 clusterGlyphCount = 0;
        if (iCodepoint + clusterCodepointCount < fullCodepointCount)
        {
            clusterGlyphCount = pPathBuilder->m_pClusterMap[iCodepoint+clusterCodepointCount] - pPathBuilder->m_pClusterMap[iCodepoint];
        }
        else
        {
            clusterGlyphCount = fullGlyphCount - pPathBuilder->m_pClusterMap[iCodepoint];
        }

        // Handle cluster containing no glyph index specification

        if (clusterCodepointCount == 1  &&  clusterGlyphCount == 1)
        {
            // 1:1 codepoint to glyph mappings may omit glyph index as long as there
            // is a codepoint provided in the UnicodeString.
            if (pPathBuilder->m_pIndices[pPathBuilder->m_pClusterMap[iCodepoint]] == 0xffff)
            {
                if (    !m_strUnicodeString.IsNull()
                    &&  m_strUnicodeString.GetCount() > iCodepoint)
                {
                    if (m_pFontFace != NULL)
                    {
                        XUINT16 glyphIndex = 0;
                        XUINT32 character = m_strUnicodeString.GetChar(iCodepoint);
                        IFC(m_pFontFace->GetGlyphIndices(&character, 1, &glyphIndex));

                        pPathBuilder->m_pIndices[pPathBuilder->m_pClusterMap[iCodepoint]] = glyphIndex;
                    }
                }
                else
                {
                    TRACE(TraceAlways, L"No glyph index in indices property and no corresponding character provided in UnicodeString: character number %d, glyph number %d.", iCodepoint, pPathBuilder->m_pClusterMap[iCodepoint]);
                    IFC(E_UNEXPECTED);
                }
            }
        }
        else
        {
            // Non 1:1 clusters must have all glyph indices specified
            for (XUINT32 iGlyph = pPathBuilder->m_pClusterMap[iCodepoint]; iGlyph < pPathBuilder->m_pClusterMap[iCodepoint] + clusterGlyphCount; iGlyph++)
            {
                if  (pPathBuilder->m_pIndices[iGlyph] == 0xffff)
                {
                    TRACE(TraceAlways, L"missing glyph index in non 1:1 cluster: glyph number %d.", iGlyph);
                    IFC(E_UNEXPECTED);
                }
            }
        }

        iCodepoint += clusterCodepointCount;
    }

    // If the font is not available we can't go further, at least we have
    // validated the syntax of the indices property.

    if (m_pFontFace == NULL)
    {
        IFC_NOTRACE(E_PENDING);
    }

    // Zero or vanishingly small rendering sizes are valid but don't display
    // this line was moved here so that we still validate the syntax

    if (m_eFontRenderingEmSize < REAL_EPSILON)
    {
        hr = S_OK;
        goto Cleanup;
    }

    // with the font source case, we could now have a null m_strFontUri but we need a non null m_pFontFace
    IFCEXPECT(m_pFontFace != NULL);

    // Scale the advance widths where specified, and use the default glyph advance width where not specified

    for (XUINT32 iGlyph=0; iGlyph<fullGlyphCount; iGlyph++)
    {
        if (pPathBuilder->m_pAdvances[iGlyph] != -XFLOAT_MAX)
        {
            // Advance width in markup is a percentage of the em size
            pPathBuilder->m_pAdvances[iGlyph] *= m_eFontRenderingEmSize / 100.0f;
        }
        else
        {
            // No width given in the markup, use the default width from the font hmtx.

            FssFontMetrics fssFontMetrics;
            FssGlyphMetrics fssGlyphMetrics;
            m_pFontFace->GetMetrics(&fssFontMetrics);
            unitsPerEm = XFLOAT(fssFontMetrics.DesignUnitsPerEm);
            IFC(m_pFontFace->GetDesignGlyphMetrics(&(pPathBuilder->m_pIndices[iGlyph]), 1, &fssGlyphMetrics, FALSE /*IsSideways*/));
            pPathBuilder->m_pAdvances[iGlyph] = fssGlyphMetrics.AdvanceWidth * m_eFontRenderingEmSize / unitsPerEm;
        }
    }


    // Success, transfer the constructed glyph run to member storage

    m_cGlyphs = fullGlyphCount;

    delete [] m_pGlyphIndices;  m_pGlyphIndices = NULL;
    delete [] m_pUOffsets;      m_pUOffsets     = NULL;
    delete [] m_pVOffsets;      m_pVOffsets     = NULL;

    if (fullGlyphCount > 0)
    {
        m_pGlyphIndices = new XUINT16[fullGlyphCount];
        memcpy(m_pGlyphIndices, pPathBuilder->m_pIndices, fullGlyphCount * sizeof(XUINT16));


        // Check whether there are any non-zero V offsets. Very frequently all v offsets will be
        // zero, and in this case we don't allocate a VOffset array.

        bNonZeroVOffsetsPresent = FALSE;
        for (XUINT32 i=0; i<fullGlyphCount; i++)
        {
            if (pPathBuilder->m_pVOffsets[i] != 0.0f)
            {
                bNonZeroVOffsetsPresent = TRUE;
                break;
            }
        }

        if (bNonZeroVOffsetsPresent)
        {
            m_pVOffsets = new XFLOAT[fullGlyphCount];
            for (XUINT32 i=0; i<fullGlyphCount; i++)
            {
                // Convert dv markup units (percentage of font em size) to world units.
                m_pVOffsets[i] = pPathBuilder->m_pVOffsets[i] * m_eFontRenderingEmSize / 100.0f;
            }
        }


        // Generate U offsets from advance width and combining u offset.

        m_pUOffsets = new XFLOAT[fullGlyphCount];
        m_eAccumulatedAdvance = 0.0f;
        for (XUINT32 i=0; i<fullGlyphCount; i++)
        {
            // Convert du markup units (percentage of font em size) to world units
            m_pUOffsets[i] = m_eAccumulatedAdvance + (pPathBuilder->m_pUOffsets[i] * m_eFontRenderingEmSize / 100.0f);
            m_eAccumulatedAdvance += pPathBuilder->m_pAdvances[i];
        }
    }

    hr = S_OK;
    m_fGlyphsDirty = FALSE;

    // Bounds will need recomputing

    delete m_pBounds;
    m_pBounds = NULL;

Cleanup:

    // We must leave with all glyph description arrays or none
    if (hr != S_OK)
    {
        delete [] m_pGlyphIndices;  m_pGlyphIndices = NULL;
        delete [] m_pUOffsets;      m_pUOffsets     = NULL;
        delete [] m_pVOffsets;      m_pVOffsets     = NULL;
    }

    LEAVESECTION(GlyphsFinalize);

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   GetOrigin
//
//  Synopsis:
//
//      Determines the origin of the glyphrun.
//
//      Note: Excludes the value of the OriginX and OriginY properties.
//            Effective values or OriginX and OriginY properties, including
//            special defaulting behavior.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CGlyphs::GetOrigin(_Out_ XFLOAT* peOriginX, _Out_ XFLOAT* peOriginY)
{
    HRESULT    hr      = S_OK;

    *peOriginX = 0.0f;
    *peOriginY = 0.0f;

    ASSERT(m_pFontFace != NULL);

    // If the y origin was unset in the markup there is a special case: the
    // text should be positioned so that the ascent of the characters is at 0,0.
    // Normally the baseline origin of the characters is at 0,0.

    if (!m_bIsOriginYSet)
    {
        PALText::FontMetrics fontMetrics;
        m_pFontFace->GetMetrics(&fontMetrics);
        *peOriginY = XFLOAT((double(fontMetrics.Ascent) * m_eFontRenderingEmSize) / double(fontMetrics.DesignUnitsPerEm));
    }
    else
    {
        *peOriginY = m_eOriginY;
    }

    *peOriginX = m_eOriginX;

    return hr;
}


//------------------------------------------------------------------------
//
//  Method:   CalculateBounds
//
//  Synopsis:
//
//      Determines the bounding box of the glyphrun.
//
//      Note: Excludes the value of the OriginX and OriginY properties.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CGlyphs::CalculateBounds()
{
    HRESULT    hr      = S_OK;
    XRECTF_WH *pBounds = NULL;
    FssFontMetrics fssFontMetrics;

    if (m_fGlyphsDirty)
    {
        IFC(EnsureFontFace());
        ASSERT(m_pFontFace != NULL);

        IFC(DetermineGlyphsAndPositions());
    }

    if (!m_pBounds)
    {
        pBounds = new XRECTF_WH;

        pBounds->X      = 0;
        pBounds->Y      = 0;
        pBounds->Width  = 0;
        pBounds->Height = 0;

        if (m_cGlyphs > 0)
        {
            XFLOAT         left;
            XFLOAT         right;
            XFLOAT         advance;

            // Effective values or OriginX and OriginY properties, including
            // special defaulting behavior.

            XFLOAT         eOriginX;
            XFLOAT         eOriginY;

            // The vertical bounds are derived from the top and bottom of the font

            IFC(EnsureFontFace());
            m_pFontFace->GetMetrics(&fssFontMetrics);

            IFC(GetOrigin(&eOriginX, &eOriginY));

            pBounds->Y = - XFLOAT(  (m_eFontRenderingEmSize * fssFontMetrics.Ascent)
                            / double(fssFontMetrics.DesignUnitsPerEm));
            if (m_pVOffsets)
            {
                pBounds->Y -= m_pVOffsets[0];
            }

            pBounds->Height =   XFLOAT(  (m_eFontRenderingEmSize * (fssFontMetrics.Ascent + fssFontMetrics.Descent))
                              / double(fssFontMetrics.DesignUnitsPerEm));

            // Determine leftmost and rightmost extents

            // Start with the first glyph

            left = m_pUOffsets[0];
            XINT32 advanceWidth;
            IFC(m_pFontFace->GetDesignGlyphAdvances(1, &m_pGlyphIndices[0], &advanceWidth, FALSE /*isSideWays*/));
            advance = XFLOAT(  (m_eFontRenderingEmSize * advanceWidth)
                                  / double(fssFontMetrics.DesignUnitsPerEm));

            right = left + advance;

            // Take the remaining glyphs into account

            for (XUINT32 i=1; i<m_cGlyphs; i++)
            {
                if (m_pUOffsets[i] < left)
                {
                    left = m_pUOffsets[i];
                }

                IFC(m_pFontFace->GetDesignGlyphAdvances(1, &m_pGlyphIndices[i], &advanceWidth, FALSE /*isSideWays*/));
                advance = XFLOAT(  (m_eFontRenderingEmSize * advanceWidth)
                                  / double(fssFontMetrics.DesignUnitsPerEm));

                if (m_pUOffsets[i] + advance > right)
                {
                    right = m_pUOffsets[i] + advance;
                }
            }

            pBounds->X     = left;
            pBounds->Width = right - left;

            // Add in the origin

            pBounds->X += eOriginX;
            pBounds->Y += eOriginY;
        }

        m_pBounds = pBounds;
        pBounds = NULL;
    }

Cleanup:
    delete pBounds;
    return hr;
}





//------------------------------------------------------------------------
//
//  Method:   EnterImpl
//
//  Synopsis:
//
//      Causes the object and its "children" to enter scope. If bLive,
//      then the object can now respond to OM requests and perform actions
//      like downloads and animation.
//
//      Derived classes are expected to first call <base>::EnterImpl, and
//      then call Enter on any "children".
//
//      After entering scope, calls DetermineGlyphsAndPositions in order to
//      detect syntax errors in the Indices and UnicodeString attributes
//      as early as possible.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CGlyphs::EnterImpl(
    _In_ CDependencyObject *pNamescopeOwner,
    EnterParams params
)
{
    HRESULT hr = S_OK;

    // First bring this Glyphs element into scope.

    IFC(CFrameworkElement::EnterImpl(pNamescopeOwner, params));


    // Check for syntax errors

    m_fGlyphsDirty = TRUE;

    hr = DetermineGlyphsAndPositions();

    if (hr == E_PENDING)
    {
        hr = S_OK; // We'll call DetermineGlyphsAndPositions again later
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Cleanup the drawing context's hardware resources on leave.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::LeaveImpl(_In_ CDependencyObject *pNamescopeOwner, LeaveParams params)
{
    IFC_RETURN(CFrameworkElement::LeaveImpl(pNamescopeOwner, params));
    if (params.fIsLive)
    {
        if (m_pTextDrawingContext != nullptr)
        {
            m_pTextDrawingContext->CleanupRealizations();
        }
    }
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CGlyphs::MeasureOverride
//
//  Synopsis: Returns the desired size for layout purposes.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::MeasureOverride(XSIZEF availableSize, XSIZEF& desiredSize)
{
    desiredSize.width = 0;
    desiredSize.height = 0;

    // it could be:
    // E_PENDING means that we are still waiting for the font download
    // E_FAIL could be caused by a failed font download, in that case we want to swallow the failure and return an empty rectangle for the desiredSize
    // or other failures like E_INVALIDARG
    // an error during downloading would have cause an user notification
    if (SUCCEEDED(CalculateBounds()))
    {
        desiredSize.width = m_eAccumulatedAdvance; // WPF use the accumulated advance for their AlignmentBox
        desiredSize.height = m_pBounds->Height;
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   CGlyphs::GetActualWidth
//
//  Synopsis: Returns the actual width of the glyph element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGlyphs::GetActualWidth(_Out_ XFLOAT* peWidth)
{
    HRESULT hr = S_OK;
    if (!peWidth)
    {
        IFC(E_INVALIDARG);
    }

    IFC(CalculateBounds());

    *peWidth = m_eAccumulatedAdvance; // WPF use the accumulated advance for width;

Cleanup:
    if (FAILED(hr) && peWidth)
    {
        *peWidth = 0;
    }

    if (hr == E_PENDING)
    {
        // E_PENDING means that we are still waiting for the font download
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   CGlyphs::GetActualHeight
//
//  Synopsis: Returns the actual height of the glyph element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGlyphs::GetActualHeight(_Out_ XFLOAT* peHeight)
{
    HRESULT hr = S_OK;
    if (!peHeight)
    {
        IFC(E_INVALIDARG);
    }

    IFC(CalculateBounds());

    *peHeight = m_pBounds->Height;

Cleanup:
    if (FAILED(hr) && peHeight)
    {
        *peHeight = 0;
    }

    if (hr == E_PENDING)
    {
        // E_PENDING means that we are still waiting for the font download
        hr = S_OK;
    }

    return hr;
}

//------------------------------------------------------------------------
//
//  Method:   PreRender
//
//  Synopsis:
//
//  rendering code that is common between Edges and D2D/DWrite rendering
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::PreRender(
    _Out_ bool *pfNeedRendering
    )
{
    HRESULT           hr               = S_OK;

    *pfNeedRendering = TRUE;

    // DetermineGlyphsAndPositions will need the glyph typeface in order to
    // generate glyph indices and glyph advance widths.

    IFC(EnsureFontFace());
    ASSERT(m_pFontFace != NULL); // By this point we must have identified the FontFace,
                                 // else there is a problem with the typeface specified.

    // If the UnicodeString or Indices attributes have changed since they
    // were first parsed, they will need reinterpreting.

    if (m_fGlyphsDirty)
    {
        hr = DetermineGlyphsAndPositions();

        // If the glyph information is invalid then we shouldn't render anything. Glyphs are already
        // downloaded by this point so the glyph data should no longer change. Returning success and
        // not generating any edges is correct as we will never be able to generate edges from the
        // invalid data so cleaning dirtiness is ok.
        if (FAILED(hr))
        {
            hr = S_OK;
            *pfNeedRendering = FALSE;
            goto Cleanup;
        }
    }


    // If there is no text, or no fill, skip this glyphs element

    if (    m_cGlyphs < 1
        ||  !m_pGlyphIndices
        ||  !m_pFill)
    {
        hr = S_OK;
        *pfNeedRendering = FALSE;
        goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Draws GlyphRun into the TextDrawingContext.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CGlyphs::DrawGlyphRun()
{
    HRESULT hr = S_OK;
    PALText::GlyphRun *pGlyphRun = NULL;
    XUINT16 *pGlyphIndices = NULL;
    XFLOAT *pGlyphAdvances = NULL;
    PALText::GlyphOffset *pGlyphOffsets = NULL;
    bool needRendering;
    XPOINTF position;
    xref::weakref_ptr<CDependencyObject> pBrushSource;

    // Should be called only when we are drawing through TextDrawingContext.
    ASSERT(m_pTextDrawingContext != NULL);

    // Prepare font face, glyphs and glyph position and determine if anything need to be rendered
    IFC(PreRender(&needRendering));
    if (!needRendering)
    {
        goto Cleanup;
    }

    //
    // Create GlyphRun representing the content of the Glyphs element.
    //

    pGlyphIndices  = new XUINT16[m_cGlyphs];
    pGlyphAdvances = new(ZERO_MEM_FAIL_FAST) XFLOAT [m_cGlyphs];
    pGlyphOffsets  = new PALText::GlyphOffset[m_cGlyphs];
    memcpy(pGlyphIndices, m_pGlyphIndices, m_cGlyphs * sizeof(XUINT16));
    for (XUINT32 glyphIndex = 0; glyphIndex < m_cGlyphs; glyphIndex++)
    {
        pGlyphOffsets[glyphIndex].AdvanceOffset  = m_pUOffsets[glyphIndex];
        pGlyphOffsets[glyphIndex].AscenderOffset = m_pVOffsets ? m_pVOffsets[glyphIndex]: 0.0f;
    }

    pGlyphRun = new PALText::GlyphRun();
    pGlyphRun->FontFace      = m_pFontFace;
    pGlyphRun->FontEmSize    = m_eFontRenderingEmSize;
    pGlyphRun->GlyphCount    = m_cGlyphs;
    pGlyphRun->IsSideways    = m_bIsSideways;
    pGlyphRun->BidiLevel     = 0;
    pGlyphRun->GlyphIndices  = pGlyphIndices;
    pGlyphRun->GlyphAdvances = pGlyphAdvances;
    pGlyphRun->GlyphOffsets  = pGlyphOffsets;
    AddRefInterface(pGlyphRun->FontFace);
    pGlyphIndices  = NULL;
    pGlyphAdvances = NULL;
    pGlyphOffsets  = NULL;

    //
    // Draw GlyphRun into the TextDrawingContext
    //

    pBrushSource = xref::get_weakref(this);
    IFC(GetOrigin(&position.x, &position.y));
    m_pTextDrawingContext->Clear();
    m_pTextDrawingContext->SetIsColorFontEnabled(m_isColorFontEnabled);
    m_pTextDrawingContext->SetColorFontPaletteIndex(static_cast<UINT32>(m_colorPaletteIndex));
    IFC(m_pTextDrawingContext->DrawGlyphRun(position, m_eAccumulatedAdvance, pGlyphRun, pBrushSource, nullptr));
    pGlyphRun = NULL; // // It is getting released by the TextDrawingContext destructor.

Cleanup:
    delete [] pGlyphIndices;
    delete [] pGlyphAdvances;
    delete [] pGlyphOffsets;
    if (pGlyphRun != NULL)
    {
        delete [] pGlyphRun->GlyphIndices;
        delete [] pGlyphRun->GlyphAdvances;
        delete [] pGlyphRun->GlyphOffsets;
        ReleaseInterface(pGlyphRun->FontFace);
        delete pGlyphRun;
    }

    RRETURN(hr);
}

//------------------------------------------------------------------------
//
// Creates TextDrawingContext if necessary.
//
//------------------------------------------------------------------------
HRESULT CGlyphs::EnsureTextDrawingContext()
{
    if (m_pTextDrawingContext == NULL)
    {
        CTextCore *pTextCore;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        m_pTextDrawingContext = new D2DTextDrawingContext(pTextCore);
    }

    return S_OK;
}

//----------------------------------------------------------------------------
//
//  Synopsis:
//      The virtual method which does the tree walk to clean up all
//      the device related resources like brushes, textures,
//      primitive composition data etc. in this subgraph.
//
//-----------------------------------------------------------------------------
void CGlyphs::CleanupDeviceRelatedResourcesRecursive(_In_ bool cleanupDComp)
{
    CFrameworkElement::CleanupDeviceRelatedResourcesRecursive(cleanupDComp);
    if (m_pTextDrawingContext != nullptr)
    {
        m_pTextDrawingContext->CleanupRealizations();
    }
}

//------------------------------------------------------------------------
//
// Renders Glyphs element in PC rendering walk.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::HWRender(
    _In_ IContentRenderer* pContentRenderer
    )
{
    HRESULT hr = S_OK;

    IFC(EnsureTextDrawingContext());
    IFC(DrawGlyphRun());

    IFC(m_pTextDrawingContext->HWRender(pContentRenderer));

Cleanup:
    // If the glyphs have not downloaded their font file yet, the glyphs won't
    // draw but everything else still can.
    if (hr == E_PENDING)
    {
        hr = S_OK;
    }
    return hr;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Returns independently animated brushes for rendering with PC.
//      There is no stroke brush.
//
//------------------------------------------------------------------------
void
CGlyphs::GetIndependentlyAnimatedBrushes(
    _Outptr_ CSolidColorBrush **ppFillBrush,
    _Outptr_ CSolidColorBrush **ppStrokeBrush
    )
{
    if (m_pFill != NULL && m_pFill->OfTypeByIndex<KnownTypeIndex::SolidColorBrush>())
    {
        SetInterface(*ppFillBrush, static_cast<CSolidColorBrush *>(m_pFill));
    }

    *ppStrokeBrush = NULL;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Generate bounds for the content of the element.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::GenerateContentBounds(
    _Out_ XRECTF_RB* pBounds
    )
{
    HRESULT hr = S_OK;

    hr = CalculateBounds();

    if (hr == S_OK)
    {
        *pBounds = ToXRectFRB(*m_pBounds);
    }
    else if (hr == E_PENDING)
    {
        pBounds->left = 0.0f;
        pBounds->top = 0.0f;
        pBounds->right = CFrameworkElement::GetActualWidth();
        pBounds->bottom = CFrameworkElement::GetActualHeight();
        hr = S_OK;
    }
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a point intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::HitTestLocalInternal(
    _In_ const XPOINTF& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds = { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = DoesRectContainPoint(innerBounds, target);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Synopsis:
//      Test if a quad intersects with the element in local space.
//
//  NOTE:
//      Overridden in derived classes to provide more detailed hit testing.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT
CGlyphs::HitTestLocalInternal(
    _In_ const HitTestPolygon& target,
    _Out_ bool* pHit
    )
{
    XRECTF_RB innerBounds = { };

    IFC_RETURN(GetInnerBounds(&innerBounds));

    *pHit = target.IntersectsRect(innerBounds);

    return S_OK;
}
