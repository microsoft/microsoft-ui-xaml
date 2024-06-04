// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <unordered_map>
#include <FontFamily.h>

//------------------------------------------------------------------------
//
//  Class:  CTypefaceCollection
//
//  Contains the physical and composite font families constructed from
//  a given font source.
//
//  Note:
//  No entries are added to the composite font families hash until clients
//  call LookupCompositeFontFamily.
//
//------------------------------------------------------------------------

class CTypefaceCollection final : public IFontCollection
{
private:
    std::unordered_map<xref_ptr<CSharedName>, xref_ptr<CCompositeFontFamily>> m_compositeFontFamilies;
    IFssFontCollection       *m_pFssFontCollection;
    CTypefaceCollection      *m_pFallbackCollection;    // For FontSources links to the installed font collection
    XUINT32                   m_cReferences;
    xref_ptr<CSharedName>     m_spUltimateFallbackFontName;
    IFontAndScriptServices   *m_pFontAndScriptServices;    // The PAL FontAndScriptServices (if any)
                                                           // that contains this TypefaceCollection

    ~CTypefaceCollection();  // Clients must use Release

    CTypefaceCollection()
    {
        m_pFssFontCollection        = NULL;
        m_pFallbackCollection       = NULL;
        m_cReferences               = 1;
        m_pFontAndScriptServices    = NULL;
    }


public:
    static _Check_return_ HRESULT CreatePALWrapper(
        _In_        IFontAndScriptServices   *pFontAndScriptServices,
        _In_        IFssFontCollection       *pFontCollection,
        _In_        CTypefaceCollection      *pFallBackCollection,
        _Outptr_ CTypefaceCollection     **ppTypefaces
    );

    _Check_return_ HRESULT LookupCompositeFontFamily(
        _In_            CSharedName           *pName,
        _In_            IPALUri               *pBaseUri,
        _Outptr_result_maybenull_ CCompositeFontFamily **ppFontFamily
    ) override;

    _Check_return_ HRESULT LookupPhysicalFontFamily(
        _In_            CSharedName      *pName,
        _Outptr_result_maybenull_ IFssFontFamily  **ppFontFamily
    ) override;

    _Check_return_ HRESULT LookupDefaultFontFamily(
        _Outptr_result_maybenull_ CCompositeFontFamily **ppDefaultFontFamily
    );

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

    IFssFontCollection* GetRealFontCollectionNoAddRef() { return m_pFssFontCollection; }
};
