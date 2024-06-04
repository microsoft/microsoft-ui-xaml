// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PalFontAndScriptServices.h"
#include "TypefaceCollection.h"


//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection destructor
//
//------------------------------------------------------------------------

CTypefaceCollection::~CTypefaceCollection()
{
    ASSERT(0 == m_cReferences);

    ReleaseInterface(m_pFssFontCollection);
}

//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection::CreatePALWrapper
//
//  Creates a CTypefaceCollection that wraps the PAL IFssFontCollection
//  augmenting it with CompositeFont functionality.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTypefaceCollection::CreatePALWrapper(
        _In_        IFontAndScriptServices   *pFontAndScriptServices,
        _In_        IFssFontCollection       *pFontCollection,
        _In_        CTypefaceCollection      *pFallBackCollection,
        _Outptr_ CTypefaceCollection     **ppTypefaces
    )
{
    CTypefaceCollection  *_this = NULL;
    CCompositeFontFamily *pFallbackFontFamily = NULL;

    _this = new CTypefaceCollection();
    _this->m_pFssFontCollection = pFontCollection;
    AddRefInterface(pFontCollection);

    _this->m_pFontAndScriptServices = pFontAndScriptServices;

    // We are not addrefing this because unfortunately the memory model
    // for the fallback font collection has been done this way in the past assuming
    // that the collection is the system font collection which should always be live
    // for the duration of the process. Fixing this now will require more changes
    // affecting the SL code.
    _this->m_pFallbackCollection = pFallBackCollection;

    // Note: Since this method is used only to initialize the installed font
    // collection, we don't set the new collections fallback collection pointer.
    // (The installed font collection is the utlimate fallback collection, there
    //  is no collection to fall back to after this.)

    // Always add the default fallback font. Calling LookupDefaultFontFamily
    // has the side effect of creating the default font lookup.
    IFC_RETURN(_this->LookupDefaultFontFamily(&pFallbackFontFamily));
    *ppTypefaces = _this;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection::LookupFontFamily
//
//      For a given FontFamily property string looks up the corresponding
//      CCompositeFontFamily from m_pFontFamiles, creating a new one if necessary.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTypefaceCollection::LookupCompositeFontFamily(
    _In_            CSharedName           *pName,
    _In_            IPALUri               *pBaseUri,
    _Outptr_result_maybenull_ CCompositeFontFamily **ppFontFamily
)
{
    IFCEXPECT_ASSERT_RETURN(m_pFssFontCollection != nullptr);

    xref_ptr<CSharedName> spName(pName);
    auto iter = m_compositeFontFamilies.find(spName);
    if (iter == m_compositeFontFamilies.end())
    {
        // Create a new one
        xref_ptr<CCompositeFontFamily> spFontFamily;
        IFC_RETURN(CCompositeFontFamily::CreateNew(
            m_pFontAndScriptServices,
            this,
            pBaseUri,
            spName,
            spFontFamily.ReleaseAndGetAddressOf()));

        // Grab the newly inserted iterator, and assert that it worked
        auto result = m_compositeFontFamilies.emplace(std::move(spName), std::move(spFontFamily));
        iter = result.first;
        ASSERT(result.second);
    }

    // Callers of this method don't assume an addref is called
    *ppFontFamily = iter->second.get();
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection::LookupPhysicalFontFamily
//
//      Uses m_pFssFontCollection to lookup a particular
//      physical font.
//
//      If not found, look again in the installed typeface collection.
//
//      If the typeface collection is pending download, we create a physical
//      font family that merely records the requested family name and indicates
//      that the typeface collection is not yet downloaded.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CTypefaceCollection::LookupPhysicalFontFamily(
    _In_            CSharedName   *pName,
    _Outptr_result_maybenull_ IFssFontFamily  **ppFontFamily
)
{
    IFCEXPECT_ASSERT_RETURN(m_pFssFontCollection != nullptr);

    IFC_RETURN(m_pFssFontCollection->LookupPhysicalFontFamily(
            pName->GetString(),
            ppFontFamily));    
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection::LookupDefaultFontFamily
//
//      Looks up the default font family, making it if necessary.
//
//      Jolt uses a default font family called 'Portable User interface'.
//      In later versions we expect 'Portable User Interface' to be defined
//      as a composite font file. Here we lookup 'Portable User Interface',
//      and, unless 'Portable User Interface' has already been provided as
//      a composite font file, we call CreateDefaultFontFamily.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTypefaceCollection::LookupDefaultFontFamily(
    _Outptr_result_maybenull_ CCompositeFontFamily **ppDefaultFontFamily
)
{
    IFCEXPECT_ASSERT_RETURN(m_pFssFontCollection != nullptr);

    if (!m_spUltimateFallbackFontName)
    {
        const xstring_ptr c_strUltimateFallbackFontName = XSTRING_PTR_FROM_STORAGE(c_strUltimateFallbackFontNameStorage);
        IFC_RETURN(CSharedName::Create(c_strUltimateFallbackFontName, m_spUltimateFallbackFontName.ReleaseAndGetAddressOf()));
    }

    auto iter = m_compositeFontFamilies.find(m_spUltimateFallbackFontName);
    if (iter == m_compositeFontFamilies.end())
    {
        IFCPTR_RETURN(m_pFontAndScriptServices);
        IFCPTR_RETURN(m_pFontAndScriptServices->GetCoreServices());
        xref_ptr<CCompositeFontFamily> spFontFamily;
        IFC_RETURN(CCompositeFontFamily::CreateFromDWrite(
            m_pFontAndScriptServices,
            this,
            spFontFamily.ReleaseAndGetAddressOf()));

        // Grab the newly inserted iterator, and assert that it worked
        auto result = m_compositeFontFamilies.emplace(m_spUltimateFallbackFontName, std::move(spFontFamily));
        iter = result.first;
        ASSERT(result.second);
    }

    // Callers of this method don't assume an addref is called
    *ppDefaultFontFamily = iter->second.get();
    return S_OK;
}
