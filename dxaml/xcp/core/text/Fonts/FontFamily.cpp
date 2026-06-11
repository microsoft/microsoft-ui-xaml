// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace DirectUI;

//------------------------------------------------------------------------
//
//  Method:  CFontFamily::Create
//
//  Synopsis:
//      Records the composite font family name.
//
//------------------------------------------------------------------------


_Check_return_ HRESULT CFontFamily::Create(
    _Outptr_ CDependencyObject **ppObject,
    _In_        CREATEPARAMETERS   *pCreate
)
{
    HRESULT      hr          = S_OK;
    CFontFamily *pFontFamily = NULL;

    pFontFamily = new CFontFamily(pCreate->m_pCore);

    IFCEXPECT((pCreate->m_value.GetType() == valueString) || (pCreate->m_value.GetType() == valueAny));

    if (pCreate->m_value.GetType() == valueString)
    {
        // if "Auto" : then resolve base on the language.
        // else just use whatever in the string.
        xstring_ptr strValue = pCreate->m_value.AsString();
        if (wcscmp(strValue.GetBuffer(), c_strUltimateFontNameAutoStorage.Buffer) == 0)
        {
            CTextCore *pTextCore = nullptr;
            IFontAndScriptServices *pFontAndScriptServices  = nullptr;
            IFC(pCreate->m_pCore->GetTextCore(&pTextCore));
            IFC(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
            IFC(pFontAndScriptServices->GetDefaultFontNameString(&strValue));
        }
        IFCEXPECT(!strValue.IsNullOrEmpty()); // disallow empty strings as font family names
        IFC(CSharedName::Create(strValue, &pFontFamily->m_pFontFamilyName));
    }

    *ppObject = pFontFamily;
    pFontFamily = NULL;

Cleanup:
    delete pFontFamily;
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Method:  CFontFamily::GetTextLineBoundsMetrics
//
//  Synopsis:
//      Return line metrics from the contained composite font family
//  constrained by TextLineBounds property.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFontFamily::GetTextLineBoundsMetrics(
    _In_            CFontContext         *pFontContext,
    _In_            TextLineBounds        textLineBounds,
    _Out_           XFLOAT               *pBaseline,
    _Out_           XFLOAT               *pLineSpacing
)
{
    IFC_RETURN(EnsureCompositeFontFamily(pFontContext));

    IFC_RETURN(m_pCompositeFontFamily->GetTextLineBoundsMetrics(textLineBounds, pBaseline, pLineSpacing));

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:  CFontFamily::EnsureCompositeFontFamily
//
//  Synopsis:
//      creates a new CCompositeFontFamily
//      only if this string has not been used before.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFontFamily::EnsureCompositeFontFamily(
    _In_            CFontContext         *pFontContext
)
{
    IFCEXPECT_ASSERT_RETURN(this              != NULL);
    IFCEXPECT_ASSERT_RETURN(m_pFontFamilyName != NULL);
    IFCEXPECT_ASSERT_RETURN(pFontContext      != NULL);

    // Lookup named font family in the specified font source

    if (    (m_fontContext.GetBaseUri()    != pFontContext->GetBaseUri())
        ||  (m_fontContext.GetFontCollection() != pFontContext->GetFontCollection()))
    {
        // Look up / create the composite font family

        ReleaseInterface(m_pCompositeFontFamily);

        IFC_RETURN(pFontContext->GetFontCollection()->LookupCompositeFontFamily(
            m_pFontFamilyName,
            pFontContext->GetBaseUri(),
           &m_pCompositeFontFamily
        ));

        m_pCompositeFontFamily->AddRef();

        // Copy the new font context into our font context,
        // overwriting any font context that was there already.
        m_fontContext = *pFontContext;
    }

    ASSERT(m_pCompositeFontFamily != NULL);
    IFCEXPECT_RETURN(m_pCompositeFontFamily != NULL);

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:  CFontFamily::GetFontTypeface
//
//  Synopsis:
//      Looks up a font family in the cache, creates a new CCompositeFontFamily
//      only if this string has not been used before.
//
//------------------------------------------------------------------------

_Check_return_ HRESULT CFontFamily::GetFontTypeface(
    _In_            CFontContext         *pFontContext,
    _In_            CFontFaceCriteria         fontFaceCriteria,
    _Outptr_result_maybenull_ CFontTypeface       **ppFontTypeface
)
{
    ENTERSECTION(FontFamilyGetFontTypeface);
    auto lockGuard = wil::scope_exit([]
    {
        LEAVESECTION(FontFamilyGetFontTypeface);
    });

    IFC_RETURN(EnsureCompositeFontFamily(pFontContext));

    // The composite font family will handle the font typeface lookup
    // TODO: Oddly, the callers of this method don't assume an addref
    *ppFontTypeface = m_pCompositeFontFamily->GetFontTypeface(fontFaceCriteria).get();
    return S_OK;
}




//------------------------------------------------------------------------
//
//  Method: CFontFamily destructor
//
//------------------------------------------------------------------------

CFontFamily::~CFontFamily()
{
    ReleaseInterface(m_pFontFamilyName);
    ReleaseInterface(m_pCompositeFontFamily);
}


_Check_return_ HRESULT CFontFamily::get_Source(_Out_ xstring_ptr* value)
{
    IFCPTR_RETURN(m_pFontFamilyName);

    CValue source;
    IFC_RETURN(m_pFontFamilyName->GetValue(&source));
    *value = source.AsString();

    return S_OK;
}

_Check_return_ HRESULT CFontFamily::put_Source(_In_ xephemeral_string_ptr& value)
{
    ReleaseInterface(m_pFontFamilyName);
    if (wcscmp(value.GetBuffer(), c_strUltimateFontNameAutoStorage.Buffer) == 0)
    {
        xstring_ptr strValue;
        CTextCore *pTextCore = nullptr;
        IFontAndScriptServices *pFontAndScriptServices = nullptr;
        IFC_RETURN(GetContext()->GetTextCore(&pTextCore));
        IFC_RETURN(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
        IFC_RETURN(pFontAndScriptServices->GetDefaultFontNameString(&strValue));
        IFC_RETURN(CSharedName::Create(strValue, &m_pFontFamilyName));
    }
    else
    {
        IFC_RETURN(CSharedName::Create(value, &m_pFontFamilyName));
    }

    return S_OK;
}