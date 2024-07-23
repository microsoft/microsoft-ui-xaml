// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

//------------------------------------------------------------------------
//
//  Private constructor
//
//------------------------------------------------------------------------

TextFormatting::TextFormatting()
{
    m_pCoreInheritedPropGenerationCounter = NULL;

    m_cGenerationCounter = 0;
    m_eFontSize = 0;
    m_nCharacterSpacing = 0;
    m_pFontFamily = NULL;
    m_pForeground = NULL;
    m_nFontWeight = DirectUI::CoreFontWeight::Normal;
    m_nFontStyle = DirectUI::FontStyle::Normal;
    m_nFontStretch = DirectUI::FontStretch::Normal;
    m_nTextDecorations = DirectUI::TextDecorations::None;
    m_strLanguageString = xstring_ptr();
    m_strResolvedLanguageString = xstring_ptr();
    m_strResolvedLanguageListString = xstring_ptr();
    m_nFlowDirection = DirectUI::FlowDirection::LeftToRight;
    m_isTextScaleFactorEnabled = true;
    m_freezeForeground = false;
}


//------------------------------------------------------------------------
//
//  Create a new TextFormatting object
//
//  Allocates a new object initialised to default values by copying from
//  TextCore's GetDefaultTextFormatting.
//
//------------------------------------------------------------------------

// static
_Check_return_ HRESULT TextFormatting::CreateCopy(
    _In_          CCoreServices         *pCore,
    _In_          const TextFormatting  *pTemplate,
    _Inout_ TextFormatting       **ppTextFormatting
)
{
    HRESULT         hr          = S_OK;
    TextFormatting *pFormatting = NULL;

    pFormatting = new TextFormatting();
   *pFormatting = *pTemplate;
    pFormatting->m_cReferences = 1;
    pFormatting->m_cGenerationCounter = 0; // Trigger inheritance on first access
    AddRefInterface(pFormatting->m_pFontFamily);
    AddRefInterface(pFormatting->m_pForeground);

    ReleaseInterface(*ppTextFormatting);
   *ppTextFormatting = pFormatting;
    RRETURN(hr);//RRETURN_REMOVAL
}



//------------------------------------------------------------------------
//
//  Create a new TextFormatting object
//
//  Allocates a new object initialised to default values by copying from
//  TextCore's GetDefaultTextFormatting.
//
//------------------------------------------------------------------------

// static
_Check_return_ HRESULT TextFormatting::Create(
    _In_        CCoreServices   *pCore,
    _Outptr_ TextFormatting **ppTextFormatting
)
{
    HRESULT         hr          = S_OK;
    CTextCore      *pTextCore   = NULL;
    TextFormatting *pDefault    = NULL;

    IFC(pCore->GetTextCore(&pTextCore));
    IFC(pTextCore->GetDefaultTextFormatting(&pDefault));
    IFC(CreateCopy(pCore, pDefault, ppTextFormatting));

    //TRACE(TraceAlways,
    //    L"Create  TextFormatting @0x%p  ->%d",
    //    *ppTextFormatting,
    //    (*ppTextFormatting)->m_cReferences
    //);

Cleanup:
    ReleaseInterface(pDefault);
    RRETURN(hr);
}



//------------------------------------------------------------------------
//
//  Create a new TextFormatting object
//
//  Allocates a new object initialised to default values. This method is
//  used just once to create the default values used by Create.
//
//------------------------------------------------------------------------

//static
_Check_return_ HRESULT TextFormatting::CreateDefault(
    _In_        CCoreServices   *pCore,
    _Outptr_ TextFormatting **ppDefaultTextFormatting
)
{
    HRESULT                 hr                     = S_OK;
    TextFormatting         *pDefault               = NULL;
    CTextCore              *pTextCore              = NULL;
    CFontFamily            *pUltimateFont          = NULL;
    IFontAndScriptServices *pFontAndScriptServices = NULL;
    CBrush                 *pDefaultTextBrush      = NULL;
    const XFLOAT            defaultFontSize        = 14.0f;
    xstring_ptr             strDefaultLanguage;

    // Prepare DOs and values for the default text core property object

    IFC(pCore->GetTextCore(&pTextCore));
    IFC(pTextCore->GetFontAndScriptServices(&pFontAndScriptServices));
    IFC(pFontAndScriptServices->GetUltimateFont(&pUltimateFont));

    IFC(pFontAndScriptServices->GetDefaultLanguageString(&strDefaultLanguage));

    IFC(pCore->GetDefaultTextBrush(&pDefaultTextBrush));

    // Create the text core properties object and fill in the default properties
    pDefault = new TextFormatting();
    pDefault->m_pCoreInheritedPropGenerationCounter
                                   = &pCore->m_cInheritedPropGenerationCounter;
    pDefault->m_cGenerationCounter = 0; // Trigger inheritance on first access
    pDefault->m_eFontSize          = defaultFontSize;
    pDefault->m_nCharacterSpacing  = 0;
    pDefault->m_pFontFamily        = pUltimateFont;     pUltimateFont     = NULL;
    pDefault->m_pForeground        = pDefaultTextBrush; pDefaultTextBrush = NULL;
    pDefault->m_nFontWeight = DirectUI::CoreFontWeight::Normal;
    pDefault->m_nFontStyle = DirectUI::FontStyle::Normal;
    pDefault->m_nFontStretch = DirectUI::FontStretch::Normal;
    pDefault->m_nTextDecorations   = DirectUI::TextDecorations::None;
    pDefault->m_strLanguageString = strDefaultLanguage;
    pDefault->m_strResolvedLanguageString = strDefaultLanguage;
    strDefaultLanguage = xstring_ptr::NullString();
    pDefault->m_nFlowDirection = DirectUI::FlowDirection::LeftToRight;
    pDefault->m_isTextScaleFactorEnabled = true;
    IFC(pDefault->ResolveLanguageString(pCore));
    IFC(pDefault->ResolveLanguageListString(pCore));

    *ppDefaultTextFormatting = pDefault;
    pDefault = NULL;

    TRACE(TraceAlways,
        L"Create  TextFormatting @0x%p  ->%d (default)",
        *ppDefaultTextFormatting,
        (*ppDefaultTextFormatting)->m_cReferences
    );

Cleanup:
    ReleaseInterface(pDefault);
    ReleaseInterface(pUltimateFont);
    ReleaseInterface(pDefaultTextBrush);
    RRETURN(hr);
}


//------------------------------------------------------------------------
//
//  Returns the font size scaled according to a given factor.
//
//------------------------------------------------------------------------

XFLOAT TextFormatting::GetScaledFontSize(XFLOAT fontScale) const
{
    // If the text scale factor is disabled, or if the font size is 0 or less,
    // then we'll just return the stored font size.
    // The latter case will result in an expected runtime exception
    // that we want to keep in place.
    if (!m_isTextScaleFactorEnabled || m_eFontSize <= 0)
    {
        return m_eFontSize;
    }

    // If the stored font size is less than 1, then the formula for scaling
    // will cause it to asymptotically shoot to infinity as it gets closer to 0,
    // so we'll cap the input font size to a minimum of 1.
    XFLOAT inputFontSize = std::max(m_eFontSize, 1.0f);

    // The formula that we get from design has it such that we scale
    // the output font size s_o relative to the input font size s_i,
    // making use of the font scale factor f, according to the formula
    //
    // s_o = s_i + max(-2.5 ln s_i + 15, 0) (f - 1)
    //
    return (XFLOAT)(inputFontSize + std::max(-exp(1) * log(inputFontSize) + 18, 0.0) * (fontScale - 1));
}


//------------------------------------------------------------------------
//
//  RefCount maintaining updaters
//
//------------------------------------------------------------------------

_Check_return_ HRESULT TextFormatting::SetFontFamily(
    _In_ CDependencyObject *pdo,
    _In_opt_ CFontFamily *pFontFamily)
{
    if (m_pFontFamily != pFontFamily)
    {
        // Remove any existing parent relationship with the old FontFamily.
        if (m_pFontFamily)
        {
            IFC_RETURN(m_pFontFamily->RemoveParent(pdo));
        }
        ReleaseInterface(m_pFontFamily);
        m_pFontFamily = pFontFamily;
        // Here we don't need to call AddParent since we are inheriting the value from parent.
        AddRefInterface(pFontFamily);
    }

    return S_OK;
}

_Check_return_ HRESULT TextFormatting::SetForeground(
    _In_ CDependencyObject *pdo,
    _In_opt_ CBrush *pForeground)
{
    if (m_pForeground != pForeground)
    {
        // Remove any existing parent relationship with the old Foreground.
        if (m_pForeground)
        {
            IFC_RETURN(m_pForeground->RemoveParent(pdo));
        }
        ReleaseInterface(m_pForeground);
        m_pForeground = pForeground;
        // Here we don't need to call AddParent since we are inheriting the value from parent.
        AddRefInterface(pForeground);
    }

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Synopsis:
//
//  Don't copy foreground from parent
//
//------------------------------------------------------------------------

void TextFormatting::SetFreezeForeground(_In_ bool freezeForeground)
{
    m_freezeForeground = freezeForeground;
}


void TextFormatting::SetLanguageString(_In_ xstring_ptr strLanguageString)
{
    m_strLanguageString = strLanguageString;
}

void TextFormatting::SetResolvedLanguageString(_In_ xstring_ptr strResolvedLanguageString)
{
    m_strResolvedLanguageString = strResolvedLanguageString;
}

void TextFormatting::SetResolvedLanguageListString(_In_ xstring_ptr strResolvedLanguageListString)
{
    m_strResolvedLanguageListString = strResolvedLanguageListString;
}

_Check_return_ HRESULT TextFormatting::ResolveLanguageString(_In_ CCoreServices *pCore)
{
    WCHAR nlsResolved[LOCALE_NAME_MAX_LENGTH];

    const WCHAR *pLanguageString = m_strLanguageString.GetBuffer();
    ASSERT(pLanguageString);

    // keep consistent with Blue behavior where we pass NULL to ResolveLocaleName() for empty language string
    if (m_strLanguageString.GetCount() == 0)
    {
        pLanguageString = nullptr;
    }

    // Try to resolve it and store it.
    if (ResolveLocaleName(pLanguageString, nlsResolved, ARRAYSIZE(nlsResolved)))
    {
        IFC_RETURN(xstring_ptr::CloneBuffer(nlsResolved, xstrlen(nlsResolved), &m_strResolvedLanguageString));
    }

    // If can't resolve, then we store the original language string.
    else
    {
        m_strResolvedLanguageString = m_strLanguageString;
    }

    return S_OK;
}

_Check_return_ HRESULT TextFormatting::ResolveLanguageListString(_In_ CCoreServices *pCore)
{
    HRESULT hr = S_OK;

    // to avoid calling GetFontFallbackLanguageList twice, we assume language list will never exceed 1024 characters
    const size_t sc_LangaugeList = 1024;
    WCHAR languageList[sc_LangaugeList];
    size_t  cchList = 0;

    const WCHAR *pLanguageString = m_strResolvedLanguageString.GetBuffer();
    ASSERT(pLanguageString);

    if (SUCCEEDED(Mui_GetFontFallbackLanguageList(pLanguageString, sc_LangaugeList, languageList, &cchList)))
    {
        IFC(xstring_ptr::CloneBuffer(languageList, static_cast<XUINT32>(cchList), &m_strResolvedLanguageListString));
    }
    // If can't get fallback language list, then we store the original resolved language string as the language string list
    else
    {
        m_strResolvedLanguageListString = m_strResolvedLanguageString;
    }

Cleanup:
    return(hr);
}

xstring_ptr TextFormatting::GetResolvedLanguageStringNoRef() const
{
    return m_strResolvedLanguageString;
}

xstring_ptr TextFormatting::GetResolvedLanguageListStringNoRef() const
{
    return m_strResolvedLanguageListString;
}

//------------------------------------------------------------------------
//
//  Destructor
//
//  Called only by Release, not public.
//
//------------------------------------------------------------------------

TextFormatting::~TextFormatting()
{
    ReleaseInterface(m_pFontFamily);
    ReleaseInterface(m_pForeground);
}




