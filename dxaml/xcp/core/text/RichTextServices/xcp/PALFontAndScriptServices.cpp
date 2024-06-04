// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TypefaceCollection.h"

#include "xstrutil.h"

#include "Fonts.h"
#include "PALFontAndScriptServices.h"
#include "Ctypes.h"
#include <StringConversions.h>
#include <Windows.Globalization.h>
#include <Windows.Globalization.Fonts.h>
#include "PALResourceManager.h"
#include "XStringBuilder.h"
#include "corep.h"
#include <RuntimeEnabledFeatures.h>

PALFontAndScriptServices::PALFontAndScriptServices(
    _In_ CCoreServices *pCore,
    IPALFontAndScriptServices *pPALFontAndScriptServices
    )
{
    m_pInstalledFontCollection = NULL;
    m_pUltimateFallbackFontFamily = NULL;
    m_pUltimateFont = NULL;
    m_strDefaultLanguageString = xstring_ptr();
    m_strDefaultFontNameString = xstring_ptr();
    XCP_WEAK(&m_pCore);
    m_pCore = pCore;
    m_pPALFontAndScriptServices = pPALFontAndScriptServices;
    AddRefInterface(pPALFontAndScriptServices);
    m_referenceCount = 1;
}

PALFontAndScriptServices::~PALFontAndScriptServices()
{
    ReleaseInterface(m_pUltimateFont);

    ReleaseInterface(m_pInstalledFontCollection);

    m_pUltimateFallbackFontFamily = NULL;

    ReleaseInterface(m_pPALFontAndScriptServices);
}

//---------------------------------------------------------------------------
//
// Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 PALFontAndScriptServices::AddRef()
{
    return ++m_referenceCount;
}

//---------------------------------------------------------------------------
//
// Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 PALFontAndScriptServices::Release()
{
    ASSERT(m_referenceCount > 0);

    XUINT32 referenceCount = --m_referenceCount;

    if (0 == m_referenceCount)
    {
        delete this;
    }

    return referenceCount;
}

HRESULT PALFontAndScriptServices::GetSystemFontCollection(
        _Outptr_ IFontCollection **ppFontCollection
        )
{
    HRESULT             hr                    = S_OK;
    IFssFontCollection* pSystemFontCollection = NULL;

    if (!m_pInstalledFontCollection)
    {
        IFC(m_pPALFontAndScriptServices->GetSystemFontCollection(&pSystemFontCollection));
        IFC(CTypefaceCollection::CreatePALWrapper(this,
            pSystemFontCollection,
            NULL, //Fallback font collection. We do not need a fallback
                  //font collection since this is the system font collection.
            &m_pInstalledFontCollection));
    }

    AddRefInterface(m_pInstalledFontCollection);
    *ppFontCollection = m_pInstalledFontCollection;

Cleanup:
    ReleaseInterface(pSystemFontCollection);
    RRETURN(hr);
}

void PALFontAndScriptServices::ResetSystemFontCollection()
{
    // Reset the cached font collection object so the PAL class is forced to retrieve the fresh
    // CTypefaceCollection from the platform-specific m_pPALFontAndScriptServices on the next
    // call to GetSystemFontCollection. *Used by test code.

    ReleaseInterface(m_pInstalledFontCollection);
    ReleaseInterface(m_pUltimateFont);

    // This ideally should be released via ref-counting, but it never was the expectation
    // that callers of GetUltimateFallbackFontFamily need to release the object. So it
    // is held with a single reference count by m_pInstalledFontCollection instead, and
    // released when that goes away.
    m_pUltimateFallbackFontFamily = nullptr;
}

HRESULT PALFontAndScriptServices::ClearDefaultLanguageString()
{
    m_strDefaultLanguageString = xstring_ptr();
    m_strDefaultFontNameString = xstring_ptr();
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method: PALFontAndScriptServices::GetDefaultLanguageString
//
//  Returns the default language string
//
//------------------------------------------------------------------------
DECLARE_CONST_XSTRING_PTR_STORAGE(c_strDefaultLanguageString, L"en-US");

HRESULT PALFontAndScriptServices::GetDefaultLanguageString(_Out_ xstring_ptr* pstrDefaultLanguageString)
{
    BOOL applicationLanguagesSupported = FALSE;
    HRESULT hr = S_OK;

    if (m_strDefaultLanguageString.IsNullOrEmpty())
    {
        // get default language from ::Windows::Globalization::ApplicationLanguages[0] if supported. Otherwise( not running in APP context ) default to en-US.
        wrl_wrappers::HString strPrimaryLanguage;
        Microsoft::WRL::ComPtr<wg::IApplicationLanguagesStatics> spApplicationLanguagesStatics;
        Microsoft::WRL::ComPtr<wfc::IVectorView<HSTRING>> spLanguages;

        IFC_NOTRACE(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_ApplicationLanguages).Get(),
            &spApplicationLanguagesStatics));

        IFC_NOTRACE(spApplicationLanguagesStatics->get_Languages(&spLanguages));

        IFC_NOTRACE(spLanguages->GetAt(0, strPrimaryLanguage.GetAddressOf()));
        UINT32 length;
        const wchar_t* buffer = strPrimaryLanguage.GetRawBuffer(&length);
        xstring_ptr::CloneBuffer(buffer, length, &m_strDefaultLanguageString);
        applicationLanguagesSupported = TRUE;
    }

Cleanup:

    if (!applicationLanguagesSupported && m_strDefaultLanguageString.IsNullOrEmpty()) // default to "en-US" if application lanugage is not supported
    {
        m_strDefaultLanguageString = XSTRING_PTR_FROM_STORAGE(c_strDefaultLanguageString);
    }

    *pstrDefaultLanguageString = m_strDefaultLanguageString;
    RRETURN(S_OK);
}

HRESULT PALFontAndScriptServices::GetDefaultFontNameString(_Out_ xstring_ptr* pstrDefaultFontNameString)
{
    if (m_strDefaultFontNameString.IsNullOrEmpty())
    {
        xstring_ptr strDefaultLanguage;
        IFC_RETURN(GetDefaultLanguageString(&strDefaultLanguage));
        // For TH2, only resolve for Japanese and Korean.
        if (wcscmp(strDefaultLanguage.GetBuffer(), L"ja") == 0
            || wcscmp(strDefaultLanguage.GetBuffer(), L"ko") == 0
            || wcscmp(strDefaultLanguage.GetBuffer(), L"ja-JP") == 0
            || wcscmp(strDefaultLanguage.GetBuffer(), L"ko-KR") == 0)
        {
            Microsoft::WRL::ComPtr<wg::Fonts::ILanguageFontGroupFactory> spLangFontGrp;
            if (SUCCEEDED(wf::GetActivationFactory(wrl_wrappers::HStringReference(RuntimeClass_Windows_Globalization_Fonts_LanguageFontGroup).Get(), &spLangFontGrp)))
            {
                Microsoft::WRL::ComPtr<wg::Fonts::ILanguageFontGroup> recommendedFonts;
                if (SUCCEEDED(spLangFontGrp->CreateLanguageFontGroup(wrl_wrappers::HStringReference(strDefaultLanguage.GetBuffer()).Get(), &recommendedFonts)))
                {
                    Microsoft::WRL::ComPtr<wg::Fonts::ILanguageFont> spLanguageFont;
                    if (SUCCEEDED(recommendedFonts->get_UITextFont(&spLanguageFont)))
                    {
                        wrl_wrappers::HString familyName;
                        if (SUCCEEDED(spLanguageFont->get_FontFamily(familyName.GetAddressOf())))
                        {
                            xstring_ptr::CloneBuffer(familyName.GetRawBuffer(nullptr), &m_strDefaultFontNameString);
                        }
                    }
                }
            }
        }
        else
        {
            m_strDefaultFontNameString = XSTRING_PTR_FROM_STORAGE(c_strUltimateFallbackFontNameCobaltStorage);

            // Windows does not use a version of RichEdit that will properly handle variable fonts, and the font fallback does not work as
            // expected when it is passed a variable font name (e.g. SegoeUI Variable).  See:
            //
            // Bug 35560704: [HP][Quanta][Win11 21H2] The number displays unknown character in windows settings search bar when input number 0-9 on AR image.
            //
            // To combat this, it has been requested that we don't use SegoeUI Variable as the default font unless the current locale uses only latin script.
            xstring_ptr defaultLanguageString;
            IFC_RETURN(GetDefaultLanguageString(&defaultLanguageString));
            WCHAR pszBuffer[6];
            unsigned int cBuffer = ARRAYSIZE(pszBuffer);
            if (::GetLocaleInfoEx(defaultLanguageString.GetBuffer(), LOCALE_SSCRIPTS, pszBuffer, cBuffer) <= 0 || CSTR_EQUAL != CompareStringOrdinal(pszBuffer, -1, L"Latn;", -1, TRUE))
            {
                m_strDefaultFontNameString = XSTRING_PTR_FROM_STORAGE(c_strUltimateFallbackFontNameTHStorage);
            }
            // For testing and debug purposes we will allow the ultimate fallback font to be specified via the registry
            static const WCHAR XAML_KEY_NAME[] = L"Software\\Microsoft\\XAML";
            static const WCHAR XAML_VALUE_NAME[] = L"AutoFontFamily";
            WCHAR szAutoFontFamily[MAX_PATH];
            DWORD cbData = sizeof(szAutoFontFamily);
            if (RegGetValue(HKEY_LOCAL_MACHINE, XAML_KEY_NAME, XAML_VALUE_NAME, RRF_RT_REG_SZ, NULL, szAutoFontFamily, &cbData) == ERROR_SUCCESS)
            {
                IFC_RETURN(xstring_ptr::CloneBuffer(szAutoFontFamily, &m_strDefaultFontNameString));
            }
            else if (!RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::ForceDWriteTypographicModel))
            {
                // The typographic model was not explicitly requested, so see if it is disabled or not applicable due to the quirk
                if (RuntimeFeatureBehavior::GetRuntimeEnabledFeatureDetector()->IsFeatureEnabled(RuntimeFeatureBehavior::RuntimeEnabledFeature::DisableDWriteTypographicModel))
                {
                    // Since the topographic model is not enabled, don't use a variable default font.
                m_strDefaultFontNameString = XSTRING_PTR_FROM_STORAGE(c_strUltimateFallbackFontNameTHStorage);
                }
            }
        }
    }
    *pstrDefaultFontNameString = m_strDefaultFontNameString;
    return S_OK;
}

//------------------------------------------------------------------------
//
//  Method:   GetUltimateFallbackFontFamily
//
//  Synopsis:
//      returns the composite font to be used as a latch ditch fallback.
//
//------------------------------------------------------------------------

HRESULT PALFontAndScriptServices::GetUltimateFallbackFontFamily(
    _Outptr_ CCompositeFontFamily **ppUltimateCompositeFontFamily
)
{
    HRESULT              hr                  = S_OK;
    IFontCollection     *pFontCollection     = NULL;

    if (m_pUltimateFallbackFontFamily == NULL)
    {
        IFC(PALFontAndScriptServices::GetSystemFontCollection(&pFontCollection));
        IFC(m_pInstalledFontCollection->LookupDefaultFontFamily(&m_pUltimateFallbackFontFamily));
        XCP_FAULT_ON_FAILURE(m_pUltimateFallbackFontFamily->GetNumberofFontLookups() != 0);
    }

    *ppUltimateCompositeFontFamily = m_pUltimateFallbackFontFamily;

Cleanup:
    ReleaseInterface(pFontCollection);
    RRETURN(hr);
}


HRESULT PALFontAndScriptServices::CreateCustomFontFace(
    _In_                            CUIElement              *pUIElement, // used to trigger a re-layout when the download is completed
    _In_                            XUINT32                  cRelativeUri,
    _In_reads_((cRelativeUri + 1)) const WCHAR             *pRelativeUri,
    _In_                            IPALUri                 *pBaseUri,
    _In_                            XINT32                   bUseFileAccess,
    _In_                            FssFontSimulations::Enum styleSimulations,
    _Outptr_                     IFssFontFace           **ppFontFace
    )
{
    HRESULT  hr            = S_OK;
    IPALResource* pResource = NULL;
    const WCHAR *pUrlFragment = NULL;
    XUINT32  faceIndex     = 0;
    XUINT32  cUrlFragment  = 0;

    // For the case of TrueType collection, we need to extract the the Fragment from the RelativeUri
    if (cRelativeUri > 2)
    {
        // a face index fragment is at least 2 characters # and a digit
        cUrlFragment = cRelativeUri - 1;
        // we look for # followed by digits at the end of the Url
        while (cUrlFragment && xisdigit(pRelativeUri[cUrlFragment]) && (pRelativeUri[cUrlFragment] != L'#'))
        {
            cUrlFragment--;
        }

        if ((cUrlFragment > 0) && (pRelativeUri[cUrlFragment] == L'#'))
        {
            pUrlFragment = &pRelativeUri[cUrlFragment+1];
            cUrlFragment = cRelativeUri - cUrlFragment - 1;
        }
        else
        {
            cUrlFragment = 0;
        }
    }

    if (cUrlFragment >= 1)
    {
        // ttc fonts, we have a font face index to extract from the uri like mincho.ttc#1
        XUINT32 cScratch;
        const WCHAR* pScratch;

        hr = UnsignedFromDecimalString(cUrlFragment, pUrlFragment, &cScratch, &pScratch, &faceIndex);

        if (hr == S_OK)
        {
            // cut out the fragment from the RelativeUri
            cRelativeUri = cRelativeUri - cUrlFragment - 1;
        }
        // on the Mac we can have a valid fragment that is not a number, like #R4096, that start with letter D or R
        // followed by a number, we just ignore fragments that don't convert into a number
        hr = S_OK;
    }

    IFC(GetResource(pBaseUri, cRelativeUri, pRelativeUri, &pResource));
    IFC(m_pPALFontAndScriptServices->CreateCustomFontFace(pResource, faceIndex, styleSimulations, ppFontFace));

Cleanup:
    ReleaseInterface(pResource);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: CTypefaceCollection::CreateFromMemory
//
//------------------------------------------------------------------------
HRESULT PALFontAndScriptServices::CreateFontCollectionFromMemory(
        _In_        CSharedName          *pName,
        _In_        IPALMemory           *pMemory,
        _Outptr_ IFontCollection     **ppFontCollection
        )
{
    return GetSystemFontCollection(ppFontCollection);
}

//------------------------------------------------------------------------
//
//  Method: GetResource
//
//  Gets a resource by combining the relative uri of the resource
//  with the base URI using the App Model rules
//
//------------------------------------------------------------------------
HRESULT PALFontAndScriptServices::GetResource(
        _In_                      IPALUri  *pBaseUri,
        _In_                      XUINT32   cRelativeUri,
        _In_reads_(cRelativeUri+1) const WCHAR *pRelativeUri,
        _Outptr_               IPALResource** ppResource
    )
{
    HRESULT  hr           = S_OK;
    IPALUri *pCombinedUri = NULL;
    xephemeral_string_ptr strRelativeUri(pRelativeUri, cRelativeUri);
    IPALResourceManager *pResourceManager = NULL;

    IFC(m_pCore->GetResourceManager(&pResourceManager));
    IFC(pResourceManager->CombineResourceUri(pBaseUri, strRelativeUri, &pCombinedUri));
    IFC(pResourceManager->TryGetLocalResource(pCombinedUri, ppResource));
    if (!*ppResource)
    {
        IFC_NOTRACE(E_FAIL);
    }

Cleanup:
    ReleaseInterface(pCombinedUri);
    ReleaseInterface(pResourceManager);
    RRETURN(hr);
}

HRESULT PALFontAndScriptServices::CreateFontCollectionFromUri(
        _In_                      IPALUri          *pBaseUri,
        _In_                      XUINT32           cRelativeUri,   // Length of pRelativeUri, does not include NULL terminator
        _In_reads_(cRelativeUri) WCHAR            *pRelativeUri,   // Note:  This string is not necessarily NULL terminated
        _Outptr_               IFontCollection **ppFontCollection
        )
{
    HRESULT              hr              = S_OK;
    xstring_ptr          strRelativeUriNullTerminated;
    XStringBuilder       relativeUriNullTerminatedBuilder;
    IPALResource        *pResource = NULL;
    IFssFontCollection  *pFontCollection = NULL;
    IFontCollection     *pInstalledFontCollection = NULL;
    CTypefaceCollection *pCustomFontTypefaceCollection = NULL;

    // GetAbsolutePath expects us to pass it a NULL-terminated string.
    // Since CreateFontCollectionFromUri hands us a string fragment from somewhere in the middle
    // of a bigger string, we must make a copy of the fragment and NULL-terminate it.
    IFC(relativeUriNullTerminatedBuilder.Initialize(pRelativeUri, cRelativeUri));
    ASSERT(relativeUriNullTerminatedBuilder.IsNullTerminated());
    IFC(relativeUriNullTerminatedBuilder.DetachString(&strRelativeUriNullTerminated));

    IFC_NOTRACE(GetResource(pBaseUri, cRelativeUri, strRelativeUriNullTerminated.GetBuffer(), &pResource));
    IFC(m_pPALFontAndScriptServices->CreateCustomFontCollection(pResource, &pFontCollection));

    if (m_pInstalledFontCollection == NULL)
    {
        GetSystemFontCollection(&pInstalledFontCollection);
    }
    IFC(CTypefaceCollection::CreatePALWrapper(this, pFontCollection, m_pInstalledFontCollection, &pCustomFontTypefaceCollection));
    *ppFontCollection = pCustomFontTypefaceCollection;
    pCustomFontTypefaceCollection = NULL;

Cleanup:
    ReleaseInterface(pFontCollection);
    ReleaseInterface(pInstalledFontCollection);
    ReleaseInterface(pCustomFontTypefaceCollection);
    ReleaseInterface(pResource);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method: GetUltimateFont
//
//  Returns the CFontFamily to be used as the ultimate fallback during
//  property inheritance.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT PALFontAndScriptServices::GetUltimateFont(_Outptr_ CFontFamily **ppUltimateFont)
{
    if (m_pUltimateFont == nullptr)
    {
        const xstring_ptr c_strUltimateFontNameAuto = XSTRING_PTR_FROM_STORAGE(c_strUltimateFontNameAutoStorage);
        CREATEPARAMETERS createParameters(GetCoreServices(), c_strUltimateFontNameAuto);
        IFC_RETURN(CreateDO(&m_pUltimateFont, &createParameters));
        IFCEXPECT_RETURN(m_pUltimateFont != nullptr);
    }

    m_pUltimateFont->AddRef();
    *ppUltimateFont = m_pUltimateFont;

    return S_OK;
}

//------------------------------------------------------------------------
//
// Return an interface to perform text analysis with.
//
//------------------------------------------------------------------------
HRESULT PALFontAndScriptServices::CreateTextAnalyzer(
    _Outptr_ PALText::ITextAnalyzer** ppTextAnalyzer
    )
{
    RRETURN(m_pPALFontAndScriptServices->CreateTextAnalyzer(ppTextAnalyzer));
}

//------------------------------------------------------------------------
//
// Return an interface to perform script analysis with.
//
//------------------------------------------------------------------------
HRESULT PALFontAndScriptServices::CreateScriptAnalyzer(
    _Outptr_ PALText::IScriptAnalyzer** ppScriptAnalyzer
    )
{
    RRETURN(m_pPALFontAndScriptServices->CreateScriptAnalyzer(ppScriptAnalyzer));
}

//------------------------------------------------------------------------
//
// Return an interface to perform glyph analysis with.
//
//------------------------------------------------------------------------
HRESULT PALFontAndScriptServices::CreateGlyphAnalyzer(
    _Outptr_ PALText::IGlyphAnalyzer** ppGlyphAnalyzer
    )
{
    RRETURN(m_pPALFontAndScriptServices->CreateGlyphAnalyzer(ppGlyphAnalyzer));
}

//------------------------------------------------------------------------
//
//  Method:   GetSystemFontFallback
//
//------------------------------------------------------------------------

HRESULT PALFontAndScriptServices::GetSystemFontFallback(
    _Outptr_ IFssFontFallback **ppFontFallback
)
{
    RRETURN(m_pPALFontAndScriptServices->GetSystemFontFallback(ppFontFallback));
}

//------------------------------------------------------------------------
//
//  Method:   CreateFontFallbackBuilder
//
//------------------------------------------------------------------------

HRESULT PALFontAndScriptServices::CreateFontFallbackBuilder(
    _Outptr_ IFssFontFallbackBuilder **ppFontFallbackBuilder
)
{
    RRETURN(m_pPALFontAndScriptServices->CreateFontFallbackBuilder(ppFontFallbackBuilder));
}
