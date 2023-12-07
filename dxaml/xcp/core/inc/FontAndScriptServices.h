// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      FontAndScriptServices provides a platform-independent interface to
//      provide platform-specific font and script services such as text
//      analysis and accessing the underlying system's font data.

#pragma once

#include <paltext.h>

struct CSharedName;
struct IPALMemory;
struct IPALUri;
struct ITextAnalyzer;
struct IGlyphAnalyzer;
struct IScriptAnalyzer;
struct IFontFallback;
struct IFontFallbackBuilder;
struct CWeightStyleStretch;
class CCompositeFontFamily;
class CCoreServices;
class CUIElement;
class CFontFamily;

DECLARE_CONST_XSTRING_PTR_STORAGE(c_strUltimateFallbackFontNameStorage, L"Global User Interface");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strUltimateFallbackFontNameTHStorage, L"Segoe UI");
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strUltimateFontNameAutoStorage, L"XamlAutoFontFamily");



//---------------------------------------------------------------------------
//
//  IFontCollection
//
//  Encapsulates a collection of fonts.
//
//---------------------------------------------------------------------------
class IFontCollection : public IObject
{
public:

    //------------------------------------------------------------------------
    //
    //  Member:
    //      LookupPhysicalFontFamily
    //
    //  Synopsis:
    //
    //      Looks for a physical font in the font collection.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT LookupPhysicalFontFamily(
        _In_            CSharedName          *pName,
        _Outptr_result_maybenull_ IFssFontFamily **ppPhysicalFontFamily
        ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      LookupCompositeFontFamily
    //
    //  Synopsis:
    //
    //      Looks for a composite font in the font collection.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT LookupCompositeFontFamily(
        _In_            CSharedName           *pName,
        _In_            IPALUri               *pBaseUri,
        _Outptr_result_maybenull_ CCompositeFontFamily **ppFontFamily
        ) = 0;
};

//---------------------------------------------------------------------------
//
//  IFontAndScriptServices
//
//  provides a platform-independent interface to
//  provide platform-specific font and script services such as text
//  analysis and accessing the underlying system's font data.
//
//---------------------------------------------------------------------------
class IFontAndScriptServices : public IObject
{
public:

    //------------------------------------------------------------------------
    //
    //  Member:
    //      GetSystemFontCollection
    //
    //  Synopsis:
    //      Returns the collection of font families installed on the system.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT GetSystemFontCollection(
        _Out_ IFontCollection** ppFontCollection
        ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      CreateFontCollectionFromMemory
    //
    //  Synopsis:
    //      Creates a FontCollection given a memory stream.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT CreateFontCollectionFromMemory(
        _In_        CSharedName          *pName,
        _In_        IPALMemory           *pMemory,
        _Outptr_ IFontCollection     **ppFontCollection
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      CreateFontCollectionFromUri
    //
    //  Synopsis:
    //      Creates a FontCollection from a given URI.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT CreateFontCollectionFromUri(
        _In_                      IPALUri              *pBaseUri,
        _In_                      XUINT32               cRelativeUri,   // Length of pRelativeUri, does not include NULL terminator
        _In_reads_(cRelativeUri) WCHAR                *pRelativeUri,   // Note:  This string is not necessarily NULL terminated
        _Outptr_               IFontCollection     **ppFontCollection
    ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      CreateCustomFontFace
    //
    //  Synopsis:
    //      Creates a FontFace from a given URI.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT CreateCustomFontFace(
        _In_                            CUIElement              *pUIElement,
            // Used to trigger a re-layout when the download is completed
        _In_                            XUINT32                  cRelativeUri,
        _In_reads_((cRelativeUri + 1)) const WCHAR             *pRelativeUri,
        _In_                            IPALUri                 *pBaseUri,
        _In_                            XINT32                   bUseFileAccess,
        _In_                            FssFontSimulations::Enum styleSimulations,
        _Outptr_                     IFssFontFace           **ppFontFace
        ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      GetDefaultLanguageString
    //  Synopsis:
    //      Returns the default language string
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT GetDefaultLanguageString(
        _Out_ xstring_ptr* pstrDefaultLanguageString
        ) = 0;

    virtual
    HRESULT GetDefaultFontNameString(
        _Out_ xstring_ptr* pstrDefaultFontNameString
        ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      GetUltimateFallbackFontFamily
    //
    //  Synopsis:
    //      Returns the composite font to be used as a latch ditch fallback.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT GetUltimateFallbackFontFamily(
        _Outptr_ CCompositeFontFamily **ppUltimateCompositeFontFamily
        ) = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      GetCoreServices
    //
    //  Synopsis:
    //      Returns a pointer to CCoreServices.
    //
    //------------------------------------------------------------------------
    virtual
    CCoreServices *GetCoreServices() = 0;

    //------------------------------------------------------------------------
    //
    //  Member:
    //      GetUltimateFont
    //
    //  Synopsis:
    //      Returns the CFontFamily to be used as the ultimate fallback during
    //      property inheritance.
    //
    //------------------------------------------------------------------------
    virtual
    HRESULT GetUltimateFont(
        _Outptr_ CFontFamily **ppUltimateFont
        ) = 0;

    // Return an interface to perform text analysis with.
    virtual HRESULT CreateTextAnalyzer(
        _Outptr_ PALText::ITextAnalyzer** ppTextAnalyzer
        ) = 0;

    // Return an interface to perform glyph analysis with.
    virtual HRESULT CreateGlyphAnalyzer(
        _Outptr_ PALText::IGlyphAnalyzer** ppGlyphAnalyzer
        ) = 0;

    // Return an interface to perform script analysis with.
    virtual HRESULT CreateScriptAnalyzer(
            _Outptr_ PALText::IScriptAnalyzer **ppScriptAnalyzer
            ) = 0;

    virtual HRESULT GetSystemFontFallback(
        _Outptr_ PALText::IFontFallback **ppFontFallback
        ) = 0;

    virtual HRESULT CreateFontFallbackBuilder(
        _Outptr_ PALText::IFontFallbackBuilder **ppFontFallbackBuilder
        ) = 0;

    virtual HRESULT ClearDefaultLanguageString() = 0;
};

