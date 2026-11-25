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
    _Outptr_ TextFormatter **ppTextFormatter,
    _In_opt_ TextFormatter *pPreferredTextFormatter
    )
{
    Result::Enum txhr = Result::Success;
    TextFormatterData *pTextFormatterData;

    // If a specific formatter is requested (e.g. the one that produced a previous line break, whose
    // LineServices break record is bound to that formatter's context), it must be checked out so
    // continuation formatting runs on the exact context that created the break record - never on an
    // arbitrary formatter. A break record made by context A can only be continued as
    // "context A + break record A"; "context B + break record A" is invalid and crashes LsCreateLine.
    if (pPreferredTextFormatter != NULL)
    {
        // Case 1: the preferred formatter is still registered on the free list (it was returned to
        // the cache after its previous formatting operation and has not been evicted). Move its
        // entry from free to used, preserving the pool's used/free bookkeeping so it is released
        // back normally later.
        TextFormatterData *pPrevTextFormatterData = NULL;
        pTextFormatterData = m_pFreeTextFormatters;
        while (pTextFormatterData != NULL)
        {
            if (pTextFormatterData->pTextFormatter == pPreferredTextFormatter)
            {
                // Unlink from the free list and push onto the used list.
                if (pPrevTextFormatterData != NULL)
                {
                    pPrevTextFormatterData->pNext = pTextFormatterData->pNext;
                }
                else
                {
                    m_pFreeTextFormatters = pTextFormatterData->pNext;
                }
                pTextFormatterData->pNext = m_pUsedTextFormatters;
                m_pUsedTextFormatters = pTextFormatterData;
                *ppTextFormatter = pTextFormatterData->pTextFormatter;
                goto Cleanup;
            }
            pPrevTextFormatterData = pTextFormatterData;
            pTextFormatterData = pTextFormatterData->pNext;
        }

        // Case 2: the preferred formatter is not on the free list. Under memory pressure,
        // ReleaseUnusedTextFormatters() evicts free entries and releases the cache's reference, so a
        // formatter that is still alive only because a cached break record AddRefs it is no longer
        // registered here. Re-register that still-live formatter (rather than creating a different
        // one) so the continuation runs on its original context. It is guaranteed alive by the break
        // record's reference; the cache takes its own reference, mirroring the AddRef held for
        // Create()'d entries and balanced by ReleaseInterface in ReleaseUnusedTextFormatters/dtor.
        // Continuation formatting is sequential, so the preferred formatter is never simultaneously
        // checked out on the used list - assert that invariant rather than duplicate-register it.
#if DBG
        for (TextFormatterData *pUsed = m_pUsedTextFormatters; pUsed != NULL; pUsed = pUsed->pNext)
        {
            ASSERT(pUsed->pTextFormatter != pPreferredTextFormatter);
        }
#endif
        IFC_OOM_RTS(pTextFormatterData = new TextFormatterData());
        pTextFormatterData->pTextFormatter = pPreferredTextFormatter;
        AddRefInterface(pPreferredTextFormatter);
        pTextFormatterData->pNext = m_pUsedTextFormatters;
        m_pUsedTextFormatters = pTextFormatterData;
        *ppTextFormatter = pPreferredTextFormatter;
        goto Cleanup;
    }

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

