// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "LsRunCache.h"
#include "LsRun.h"
#include "LsParagraphSpan.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;
using namespace Ptls6;

//---------------------------------------------------------------------------
//
//  Member:
//      LsRunCache::LsRunCache
//
//  Synopsis:
//      Constructor.
//
//---------------------------------------------------------------------------
LsRunCache::LsRunCache()
{
    m_pFirstRun = NULL;
    m_pLastRun = NULL;
    m_pRecentRun = NULL;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRunCache::~LsRunCache
//
//  Synopsis:
//      Destructor.
//
//---------------------------------------------------------------------------
LsRunCache::~LsRunCache()
{
    Clear();
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRunCache::GetTextRun
//
//  Synopsis:
//      Fetches a cached run of text that is matching specified character index.
//      NOTE: returned LsRun may start before character index.
//
//---------------------------------------------------------------------------
LsRun * LsRunCache::GetLsRun(
    _In_ XUINT32 characterIndex,
        // Index of the first character of the run
    _In_opt_ const Ptls6::LSFETCHPOSITION *pFetchPos
        // Context of the run provided by LS.
    ) const
{
    LsRun *pLsRun = m_pRecentRun;

#if DBG
    {
        if (m_pRecentRun != NULL)
        {
            // Make sure that m_pRecentRun is actually somewhere in the cache.
            bool bIsInCache = false;
            LsRun *pRun       = m_pFirstRun;
    
            while (    (!bIsInCache)
                   &&  (pRun != NULL))
            {
                bIsInCache = (pRun == m_pRecentRun);
                pRun = pRun->m_pNext;
            }
    
            if (!bIsInCache) 
            {
                // The above apparently redundant if statement makes it 
                // easy to set a breakpoint on the ASSERT.
                ASSERT(bIsInCache);
            }
        }
    }
#endif

    // Try to find a run with specified character index in the cache.
    // Start the search from the last accessed TextRun, to optimize for sequential access.
    while (pLsRun != NULL)
    {
        if (characterIndex >= pLsRun->m_characterIndex &&
            characterIndex <  pLsRun->m_characterIndex + pLsRun->m_length)
        {
            break;
        }
        else if (characterIndex == pLsRun->m_characterIndex &&
            pLsRun->GetTextRun() != NULL &&
            pLsRun->GetTextRun()->GetType() == TextRunType::EndOfParagraph)
        {
            // Special processing for EOP run, which may have length of 0.
            break;
        }
        else if (characterIndex < pLsRun->m_characterIndex)
        {
            pLsRun = pLsRun->m_pPrevious;
        }
        else
        {
            pLsRun = pLsRun->m_pNext;
        }
    }

    // Update the cache, if the requested run is not the same as the cached one.
    if (pLsRun != NULL && pLsRun != m_pRecentRun)
    {
        m_pRecentRun = pLsRun;
    }

    // There might be multiple runs which are starting at the same position. 
    // Searching process above will select the last run at such position.
    // If run context is provided, use it to disambiguate and return appropriate run.
    if (pFetchPos != NULL && 
        pLsRun != NULL && pLsRun->m_characterIndex == characterIndex &&
        pLsRun->m_pPrevious != NULL && pLsRun->m_pPrevious->m_length == 0)
    {
        if (pFetchPos->fetchruntype == Ptls6::fetchruntypeOpenSpan)
        {
            // Currently OpenSpan context represents only start of Reverse Object. 
            // There might be multiple Reverse Objects starting at the same position, 
            // so iterate though them and try to find one with matching Span Id.
            while (pLsRun->m_pPrevious != NULL && pLsRun->m_pPrevious->m_length == 0)
            {
                if (pLsRun->m_pPrevious->m_lsrun.fetchruntype == Ptls6::fetchruntypeLSObject &&
                    pLsRun->m_pPrevious->m_lsrun.fetchout.lsobject.lsspqMaster == pFetchPos->fetchin.spanopen.lsspan.lsspq)
                {
                    break;
                }
                pLsRun = pLsRun->m_pPrevious;
            }
#if DBG
            if (pLsRun->m_pPrevious != NULL)
            {
                if (pLsRun->m_pPrevious->m_length == 0)
                {
                    ASSERT(pLsRun->m_pPrevious->m_lsrun.fetchruntype == Ptls6::fetchruntypeLSObject);
                    ASSERT(pLsRun->m_pPrevious->m_lsrun.fetchout.lsobject.lsspqMaster == pFetchPos->fetchin.spanopen.lsspan.lsspq);
                }
            }
#endif
        }
        else if (pFetchPos->fetchruntype == Ptls6::fetchruntypeLSObject)
        {
            // Currently LsObject context may represent Embedded Object (reported as LsObject) 
            // or end of Reverse Object (reported as CloseSpan).
            // There might be multiple Reverse Objects ending at the same position, 
            // so iterate though them and try to find one with matching Span Id.
            while (pLsRun->m_pPrevious != NULL && pLsRun->m_pPrevious->m_length == 0)
            {
                if (pLsRun->m_pPrevious->m_lsrun.fetchruntype == Ptls6::fetchruntypeCloseSpan &&
                    pLsRun->m_pPrevious->m_lsrun.fetchout.spanclose.lsspan.lsspq == pFetchPos->fetchin.lsobject.lsspan.lsspq)
                {
                    break;
                }
                pLsRun = pLsRun->m_pPrevious;
            }
            ASSERT(pLsRun->m_pPrevious != NULL);
#if DBG
            if (pLsRun->m_pPrevious->m_length == 0)
            {
                // LS passes in context from previous run as fetchPos. Since we use LSObject type for both inline objects and reverse objects,
                // this context may mean either 1) a preceding inline object, in which case we want the first run starting at this cp, 0 length or not
                // or 2) a preceding reverse object span closing, in which case we match context. Case 1) should actually go into the else {} loop
                // below, but will be matched here because both reverse objects and inline objects will have LSObject as the fetchrunType.
                // When an inline object is followed by a reverse object close, the while loop above will break out because inline object has length 
                // > 0, but it doesn't have type CloseSpan.
                ASSERT(pLsRun->m_pPrevious->m_lsrun.fetchruntype == Ptls6::fetchruntypeCloseSpan);
                ASSERT(pLsRun->m_pPrevious->m_lsrun.fetchout.spanclose.lsspan.lsspq == pFetchPos->fetchin.lsobject.lsspan.lsspq);
            }
#endif
        }
        else
        {
            // For any other context, the first run starting at specified position is returned.
            pLsRun = pLsRun->m_pPrevious;
            while (pLsRun->m_pPrevious != NULL && pLsRun->m_pPrevious->m_length == 0)
            {
                pLsRun = pLsRun->m_pPrevious;
            }
        }
    }


#if DBG && XCP_MONITOR
    // Optional trace of cached runs during FetchRun

    if (GetPALDebuggingServices()->GetTraceFlags() & TraceFetchedRuns)
    {
        LsRun           *pRun          = NULL;
        const WCHAR     *pFetchRunType = NULL;
        const WCHAR     *pFetchPosType = NULL;
        LSSPANQUALIFIER  lsspq         = 0;

        if (pFetchPos)
        {
            switch (pFetchPos->fetchruntype)
            {
            case fetchruntypeOpenSpan:  pFetchPosType = L"OpenSpan lsspq: "; lsspq = pFetchPos->fetchin.spanopen.lsspan.lsspq; break;
            case fetchruntypeLSObject:  pFetchPosType = L"LSObject lsspq: "; lsspq = pFetchPos->fetchin.lsobject.lsspan.lsspq; break;
            case fetchruntypeText:      pFetchPosType = L"Text";             lsspq = 0;                                        break;
            case fetchruntypeHidden:    pFetchPosType = L"Hidden";           lsspq = 0;                                        break;
            case fetchruntypeCloseSpan: pFetchPosType = L"CloseSpan";        lsspq = 0;                                        break;
            default:                    pFetchPosType = L"UNRECOGNISED";     lsspq = 0;                                        break;
            }
        }
        else
        {
            pFetchPosType = L"none";
            lsspq = 0;
        }

        pRun = this->m_pFirstRun;
        while (pRun != NULL)
        {
            switch (pRun->m_lsrun.fetchruntype)
            {
            case fetchruntypeText:      pFetchRunType = L"Text";         break;
            case fetchruntypeHidden:    pFetchRunType = L"Hidden";       break;
            case fetchruntypeOpenSpan:  pFetchRunType = L"OpenSpan";     break;
            case fetchruntypeCloseSpan: pFetchRunType = L"CloseSpan";    break;
            case fetchruntypeLSObject:  pFetchRunType = L"LSObject";     break;
            default:                    pFetchRunType = L"UNRECOGNISED"; break;
            }
            pRun = pRun->m_pNext;
        }
    }
#endif


    return pLsRun;
}

//---------------------------------------------------------------------------
//
//  Synopsis:
//      Searches sequentially for a reverse object opening with the specified
//      attributes as described in parameter comments.
//
//---------------------------------------------------------------------------
LsRun *LsRunCache::FindReverseObjectOpen(
    XUINT32 characterIndex,         // The reverse object opened at this cp
    _In_ LsSpan* pParentMasterSpan  // The reverse object that has this span as its parent master span
    ) const
{
    LsRun* pCurrent = m_pFirstRun;
    while (pCurrent != NULL)
    {
        ASSERT(pCurrent->m_characterIndex <= characterIndex);

        if ((pCurrent->m_characterIndex == characterIndex) &&
            (pCurrent->m_lsrun.fetchruntype == Ptls6::fetchruntypeLSObject) &&
            (pCurrent->m_lsrun.fetchout.lsobject.idobj == LsObjectId::ReverseObject))
        {
            LsSpan* pMasterSpan = reinterpret_cast<LsSpan*>(pCurrent->m_lsrun.fetchout.lsobject.lsspqMaster);
            if (pMasterSpan->GetParent() == pParentMasterSpan)
            {
                break;
            }
        }
        pCurrent = pCurrent->m_pNext;
    }

    return pCurrent;
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRunCache::AppendLsRun
//
//  Synopsis:
//      Appends a run of text to the end of collection of cached runs.
//
//---------------------------------------------------------------------------
void LsRunCache::AppendLsRun(
    _In_ LsRun *pLsRun
        // A new run of text to be appended to the end of collection of cached runs.
    )
{
    ASSERT(pLsRun != NULL);
    if (m_pFirstRun == NULL)
    {
        ASSERT(m_pLastRun == NULL);
        ASSERT(m_pRecentRun == NULL);
        m_pFirstRun = m_pLastRun = m_pRecentRun = pLsRun;
    }
    else
    {
        ASSERT(pLsRun->m_characterIndex == m_pLastRun->m_characterIndex + m_pLastRun->m_length);
        pLsRun->m_pPrevious = m_pLastRun;
        m_pLastRun->m_pNext = pLsRun;
        m_pLastRun = m_pRecentRun = pLsRun;
    }

#if DBG
    {
        const WCHAR *pFetchRunType = NULL;
        switch (pLsRun->m_lsrun.fetchruntype)
        {
        case fetchruntypeText:      pFetchRunType = L"Text";         break;
        case fetchruntypeHidden:    pFetchRunType = L"Hidden";       break;
        case fetchruntypeOpenSpan:  pFetchRunType = L"OpenSpan";     break;
        case fetchruntypeCloseSpan: pFetchRunType = L"CloseSpan";    break;
        case fetchruntypeLSObject:  pFetchRunType = L"LSObject";     break;
        default:                    pFetchRunType = L"UNRECOGNISED"; break;
        }
    }
#endif
}

//---------------------------------------------------------------------------
//
//  Member:
//      LsRunCache::Clear
//
//  Synopsis:
//      Clears all runs in the cache.
//
//---------------------------------------------------------------------------
void LsRunCache::Clear()
{
    while (m_pFirstRun != NULL)
    {
        LsRun *pLsRun = m_pFirstRun;
        m_pFirstRun = m_pFirstRun->m_pNext;
        delete pLsRun;
    }
    m_pLastRun = NULL;
    m_pRecentRun = NULL;
}
