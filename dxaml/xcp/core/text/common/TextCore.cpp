// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "PALFontAndScriptServices.h"
#include "WinTextCore.h"

//------------------------------------------------------------------------
//
//  Initializes a new instance of the CTextCore.
//
//------------------------------------------------------------------------
CTextCore::CTextCore(_In_ CCoreServices *pCore)
    : m_pCore(pCore)
    , m_pWinTextCore(NULL)
    , m_pFontAndScriptServices(NULL)
    , m_pTextFormatterCache(NULL)
    , m_pDefaultTextFormatting(NULL)
    , m_pLastSelectedTextElement(NULL)
{
    XCP_WEAK(&m_pCore);
}

//------------------------------------------------------------------------
//
//  Releases text related caches.
//
//------------------------------------------------------------------------
CTextCore::~CTextCore()
{
    // Before any other cleanup is done make sure m_pLastSelectedTextElement is released first,
    // since it might need to access other members of CTextCore in its cleanup.
    ReleaseInterface(m_pLastSelectedTextElement);

    m_pCore = NULL;
    ReleaseInterface(m_pWinTextCore);
    ReleaseInterface(m_pFontAndScriptServices);
    ReleaseInterface(m_pDefaultTextFormatting);
    delete m_pTextFormatterCache;
    m_pTextFormatterCache = NULL;
}

//------------------------------------------------------------------------
//
//  Returns a cache of TextFormatters.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextCore::GetTextFormatterCache(
    _Outptr_ RichTextServices::TextFormatterCache **ppTextFormatterCache
    )
{
    if (m_pTextFormatterCache == NULL)
    {
        IFontAndScriptServices *pFontAndScriptServices;
        IFC_RETURN(GetFontAndScriptServices(&pFontAndScriptServices));
        m_pTextFormatterCache = new RichTextServices::TextFormatterCache(pFontAndScriptServices);
    }

    *ppTextFormatterCache = m_pTextFormatterCache;

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Returns font and script services.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextCore::GetFontAndScriptServices(
    _Outptr_ IFontAndScriptServices **ppFontAndScriptServices
    )
{
    HRESULT hr = S_OK;
    IPALFontAndScriptServices *pPALFontAndScriptServices = NULL;

    if (m_pFontAndScriptServices == NULL)
    {
        IFC(gps->CreateFontAndScriptServices(&pPALFontAndScriptServices));
        m_pFontAndScriptServices = new PALFontAndScriptServices(m_pCore, pPALFontAndScriptServices);
    }

    *ppFontAndScriptServices = m_pFontAndScriptServices;

Cleanup:
    ReleaseInterface(pPALFontAndScriptServices);
    RRETURN(hr);
}

//------------------------------------------------------------------------
//
//  Initialize the text core services.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextCore::Initialize()
{
    ASSERT(m_pWinTextCore == NULL);

    // Initialize the Unicode database lookup table pointer
    UcdInitialize();

    IFC_RETURN(WinTextCore::Create(m_pCore, &m_pWinTextCore));

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Returns the default values of all the text core properties.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT CTextCore::GetDefaultTextFormatting(
    _Outptr_ TextFormatting **ppDefaultTextFormatting
    )
{
    if (m_pDefaultTextFormatting == NULL)
    {
        IFC_RETURN(TextFormatting::CreateDefault(
            m_pCore,
           &m_pDefaultTextFormatting
        ));
    }

    *ppDefaultTextFormatting = m_pDefaultTextFormatting;
    AddRefInterface(m_pDefaultTextFormatting);

    return S_OK;
}

//------------------------------------------------------------------------
//
//  Clears the cached default values of all the text core properties.
//
//------------------------------------------------------------------------
void CTextCore::ClearDefaultTextFormatting()
{
    ReleaseInterface(m_pDefaultTextFormatting);
}

//------------------------------------------------------------------------
//
//  Releases the unuse TextFormatters
//
//------------------------------------------------------------------------
void CTextCore::ReleaseUnusedTextFormatters()
{
    if (m_pTextFormatterCache != NULL)
    {
        m_pTextFormatterCache->ReleaseUnusedTextFormatters();
    }
}

//------------------------------------------------------------------------
//
//  Sets the last text element that had selected text.
//
//------------------------------------------------------------------------
void CTextCore::SetLastSelectedTextElement(_In_ CUIElement *pLastSelectedTextElement)
{
    ReplaceInterface(m_pLastSelectedTextElement, pLastSelectedTextElement);
}

//------------------------------------------------------------------------
//
//  Determines whether the input text element can select text.
//  This is based on whether there is already selected text currently
//  present on the screen belonging to a different text element.
//
//------------------------------------------------------------------------
bool CTextCore::CanSelectText(_In_ CUIElement* pTextElement) const
{
    return (pTextElement == m_pLastSelectedTextElement || m_pLastSelectedTextElement == nullptr);
}

//------------------------------------------------------------------------
//
//  Clears the last stored text element.
//
//------------------------------------------------------------------------
void CTextCore::ClearLastSelectedTextElement()
{
    ReleaseInterface(m_pLastSelectedTextElement);
}

//------------------------------------------------------------------------
//
//  Update the shared IDWriteNumberSubstitution object stored in DWriteFontAndScriptServices
//
//------------------------------------------------------------------------
HRESULT CTextCore::ConfigureNumberSubstitution()
{
    IFC_RETURN(m_pWinTextCore->ConfigureNumberSubstitution());

    return S_OK;
}
//------------------------------------------------------------------------
//
//  Determines whether a given control is a text control
//
//------------------------------------------------------------------------
bool CTextCore::IsTextControl(_In_ CDependencyObject* pDO)
{
    return (pDO->OfTypeByIndex<KnownTypeIndex::TextBlock>()
        || pDO->OfTypeByIndex<KnownTypeIndex::RichTextBlock>()
        || pDO->OfTypeByIndex<KnownTypeIndex::RichTextBlockOverflow>()
        || pDO->OfTypeByIndex<KnownTypeIndex::RichEditBox>()
        || pDO->OfTypeByIndex<KnownTypeIndex::TextBox>()
        || pDO->OfTypeByIndex<KnownTypeIndex::PasswordBox>());
}

bool CTextCore::IsTextSelectionEnabled(_In_ CDependencyObject* textControl)
{
    if (auto textBlock = do_pointer_cast<CTextBlock>(textControl))
    {
        return textBlock->IsSelectionEnabled();
    }
    else if (auto richTextBlock = do_pointer_cast<CRichTextBlock>(textControl))
    {
        return richTextBlock->IsSelectionEnabled();
    }
    else if (auto richTextBlockOverflow = do_pointer_cast<CRichTextBlockOverflow>(textControl))
    {
        return richTextBlockOverflow->GetMaster()->IsSelectionEnabled();
    }
    else if (textControl->OfTypeByIndex<KnownTypeIndex::RichEditBox>() ||
             textControl->OfTypeByIndex<KnownTypeIndex::TextBox>() ||
             textControl->OfTypeByIndex<KnownTypeIndex::PasswordBox>())
    {
        return true;
    }
    else
    {
        return false;
    }
}
