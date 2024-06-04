// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

using namespace RichTextServices;

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatterCache::TextFormatterCache
//
//  Synopsis:
//      Initializes a new instance of the TextFormatterCache class.
//
//---------------------------------------------------------------------------
TextFormatterCache::TextFormatterCache(
    _In_ IFontAndScriptServices *pFontAndScriptServices
        // Provides an interface to access font and script specific data.
    )
{
    m_pFreeTextFormatters = NULL;
    m_pUsedTextFormatters = NULL;
    m_pFontAndScriptServices = pFontAndScriptServices;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatterCache::~TextFormatterCache
//
//  Synopsis:
//      Release resources associated with the TextFormatterCache.
//
//---------------------------------------------------------------------------
TextFormatterCache::~TextFormatterCache()
{
    ASSERT(m_pUsedTextFormatters == NULL);
    while (m_pFreeTextFormatters != NULL)
    {
        TextFormatterData *pTextFormatterData = m_pFreeTextFormatters;
        m_pFreeTextFormatters = m_pFreeTextFormatters->pNext;
        ReleaseInterface(pTextFormatterData->pTextFormatter);
        delete pTextFormatterData;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatterCache::AcquireTextFormatter
//
//  Synopsis:
//      Acquires TextFormatter for exclusive use.
//
//---------------------------------------------------------------------------
Result::Enum  
TextFormatterCache::AcquireTextFormatter(
    _Outptr_ TextFormatter **ppTextFormatter
    )
{
    Result::Enum txhr = Result::Success;
    TextFormatterData *pTextFormatterData;

    // If free TextFormatter is not available, create one and add it to the used formatters list.
    // Otherwise use existing one.
    if (m_pFreeTextFormatters == NULL)
    {
        IFCTEXT(TextFormatter::Create(m_pFontAndScriptServices, ppTextFormatter));

        //ETW event
        TraceTextFormatterCreatedInfo();

        IFC_OOM_RTS(pTextFormatterData = new TextFormatterData());
        pTextFormatterData->pTextFormatter = *ppTextFormatter;
        pTextFormatterData->pNext = m_pUsedTextFormatters;
        m_pUsedTextFormatters = pTextFormatterData;
    }
    else
    {
        pTextFormatterData = m_pFreeTextFormatters;
        m_pFreeTextFormatters = m_pFreeTextFormatters->pNext;
        pTextFormatterData->pNext = m_pUsedTextFormatters;
        m_pUsedTextFormatters = pTextFormatterData;
        *ppTextFormatter = pTextFormatterData->pTextFormatter;
    }

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatterCache::ReleaseTextFormatter
//
//  Synopsis:
//      Releases TextFormatter and makes it available for reuse.
//
//---------------------------------------------------------------------------
void 
TextFormatterCache::ReleaseTextFormatter(
    _In_opt_ TextFormatter *pTextFormatter
    )
{
    if (pTextFormatter != NULL)
    {
        // Find TextFormatter in the used list and remove it from there.
        TextFormatterData *pPrevTextFormatterData = NULL;
        TextFormatterData *pTextFormatterData = m_pUsedTextFormatters;
        while (pTextFormatterData != NULL)
        {
            if (pTextFormatterData->pTextFormatter == pTextFormatter)
            {
                break;
            }
            pPrevTextFormatterData = pTextFormatterData;
            pTextFormatterData = pTextFormatterData->pNext;
        }

        ASSERT(pTextFormatterData != NULL);

        if (pPrevTextFormatterData != NULL)
        {
            pPrevTextFormatterData->pNext = pTextFormatterData->pNext;
        }
        else
        {
            m_pUsedTextFormatters = pTextFormatterData->pNext;
        }
        pTextFormatterData->pNext = m_pFreeTextFormatters;
        m_pFreeTextFormatters = pTextFormatterData;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextFormatterCache::ReleaseUnusedTextFormatters
//
//  Synopsis:
//      Releases unused TextFormatters (TextFormatter objects in the freed 
//      formatters list) to reduce memory usage
//
//---------------------------------------------------------------------------
void
TextFormatterCache::ReleaseUnusedTextFormatters()
{
    while (m_pFreeTextFormatters != NULL)
    {
        TextFormatterData *pTextFormatterData = m_pFreeTextFormatters;
        m_pFreeTextFormatters = m_pFreeTextFormatters->pNext;
        ReleaseInterface(pTextFormatterData->pTextFormatter);
        delete pTextFormatterData;

        //ETW event
        TraceUnusedTextFormatterDeletedInfo();
    }
}

