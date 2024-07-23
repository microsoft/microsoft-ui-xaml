// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      PALFontAndScriptServices provides access to platform independent
//      font and script data.

#pragma once

#include "FontAndScriptServices.h"
class CTypefaceCollection;
class CGlyphTypefaceCollection;
class CCompositeFontFamily;
class CFontFamily;
class CGlyphTypefaceCache;
class CString;
class CCoreServices;

// TODO: This file should not be placed under \xcp\core\inc\ as it is now.
//                 We need to pick the right location and namespace for it.

//---------------------------------------------------------------------------
//
//  PALFontAndScriptServices
//
//  Provides a platform-independent interface to access font and script data.
//
//---------------------------------------------------------------------------
class PALFontAndScriptServices final : public IFontAndScriptServices
{
public:

    // Initializes a new instance of the PALFontAndScriptServices class.
    PALFontAndScriptServices(_In_ CCoreServices *pCore, IPALFontAndScriptServices *pPALFontAndScriptServices);

    // Release resources associated with the PALFontAndScriptServices.
    virtual ~PALFontAndScriptServices();

/*********************   IFontAndScriptServices implementation. **********************/

    // Returns the collection of font families installed on the system.
    
    HRESULT GetSystemFontCollection(
    _Outptr_ IFontCollection** ppFontCollection
    ) override;

    // Creates a FontCollection given a memory stream.
    
    HRESULT CreateFontCollectionFromMemory(
        _In_        CSharedName          *pName,
        _In_        IPALMemory           *pMemory,
        _Outptr_ IFontCollection     **ppFontCollection
    ) override;

    // Creates a FontCollection from a given URI.
    
    HRESULT CreateFontCollectionFromUri(
        _In_                      IPALUri              *pBaseUri,
        _In_                      XUINT32               cRelativeUri,   // Length of pRelativeUri, does not include NULL terminator
        _In_reads_(cRelativeUri) WCHAR                *pRelativeUri,   // Note:  This string is not necessarily NULL terminated
        _Outptr_               IFontCollection     **ppFontCollection
    ) override;

    // Creates a FontFace from a given URI.
    
    HRESULT CreateCustomFontFace(
        _In_                            CUIElement              *pUIElement, // used to trigger a re-layout when the download is completed
        _In_                            XUINT32                  cRelativeUri,
        _In_reads_((cRelativeUri + 1)) const WCHAR             *pRelativeUri,
        _In_                            IPALUri                 *pBaseUri,
        _In_                            XINT32                   bUseFileAccess,
        _In_                            FssFontSimulations::Enum styleSimulations,
        _Outptr_                     IFssFontFace           **ppFontFace
    ) override;

    // Returns the default language string.
    
    HRESULT GetDefaultLanguageString(_Out_ xstring_ptr* pstrDefaultLanguageString) override;

    HRESULT GetDefaultFontNameString(_Out_ xstring_ptr* pstrDefaultFontNameString) override;

    // Returns the composite font to be used as a latch ditch fallback.
    
    HRESULT GetUltimateFallbackFontFamily(_Outptr_ CCompositeFontFamily **ppUltimateCompositeFontFamily) override;

    // Returns the CFontFamily to be used as the ultimate fallback during property inheritance.
    
    _Check_return_ HRESULT GetUltimateFont(_Outptr_ CFontFamily **ppUltimateFont) override;

    
    CCoreServices *GetCoreServices() override {return m_pCore;}

    HRESULT ClearDefaultLanguageString() override;

/**********************************************************************************************/


/****************************   IPALFontAndScriptServices Implementation   ********************/

    // Return an interface to perform text analysis with.
    HRESULT CreateTextAnalyzer(
        _Outptr_ PALText::ITextAnalyzer** ppTextAnalyzer
        ) override;

    // Return an interface to perform glyph analysis with.
    HRESULT CreateGlyphAnalyzer(
        _Outptr_ PALText::IGlyphAnalyzer** ppGlyphAnalyzer
        ) override;

    // Return an interface to perform script analysis with.
    HRESULT CreateScriptAnalyzer(
        _Outptr_ PALText::IScriptAnalyzer** ppScriptAnalyzer
        ) override;

    HRESULT GetSystemFontFallback(
        _Outptr_ PALText::IFontFallback **ppFontFallback
        ) override;

    HRESULT CreateFontFallbackBuilder(
        _Outptr_ PALText::IFontFallbackBuilder **ppFontFallbackBuilder
        ) override;

        // Increments the count of references to this object.
        XUINT32 AddRef() override;

        // Decrements the count of references to this object.
        XUINT32 Release() override;

        IPALFontAndScriptServices* GetPALFontAndScriptServices() const;

        void ResetSystemFontCollection();

/*********************************************************************************/


private:

    CTypefaceCollection            *m_pInstalledFontCollection;
    CCompositeFontFamily           *m_pUltimateFallbackFontFamily;
    CFontFamily                    *m_pUltimateFont;
    xstring_ptr                     m_strDefaultLanguageString;
    xstring_ptr                     m_strDefaultFontNameString;
    CCoreServices                  *m_pCore;
    XUINT32                         m_referenceCount;
    IPALFontAndScriptServices      *m_pPALFontAndScriptServices;

    // Gets a resource by combining the relative uri of the resource
    // with the base URI using the App Model rules
    HRESULT GetResource(
        _In_                      IPALUri  *pBaseUri,
        _In_                      XUINT32   cRelativeUri,
        _In_reads_(cRelativeUri+1) const WCHAR *pRelativeUri,
        _Outptr_               IPALResource** ppResource
    );

};

inline
IPALFontAndScriptServices* PALFontAndScriptServices::GetPALFontAndScriptServices() const
{
    return m_pPALFontAndScriptServices;
}
