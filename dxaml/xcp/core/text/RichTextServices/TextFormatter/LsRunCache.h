// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      LsRunCache provides run caching services in order to improve performance.

#pragma once

namespace RichTextServices
{
    namespace Internal
    {
        class LsRun;
        class LsSpan;

        //---------------------------------------------------------------------------
        //
        //  LsRunCache
        //
        //  Provides run caching services in order to improve performance.
        //
        //---------------------------------------------------------------------------
        class LsRunCache
        {
        public:

            // Initializes a new instance of the LsRunCache class.
            LsRunCache();

            // Release resources associated with the TextRunCache.
            ~LsRunCache();

            // Clears all runs in the cache.
            void Clear();

            // Fetches a cached run of text that is matching specified character index.
            // NOTE: returned TextRun may start before character index.
            LsRun * GetLsRun(
                _In_ XUINT32 characterIndex, 
                _In_opt_ const Ptls6::LSFETCHPOSITION *pFetchPos
                ) const;

            // Fetches a cached run for a particular reverse object opening.
            LsRun *FindReverseObjectOpen(XUINT32 characterIndex, _In_ LsSpan* pParentMasterSpan) const;

            // Appends a run of text to the end of collection of cached runs.
            void AppendLsRun(_In_ LsRun *pLsRun);

        private:

            LsRun *m_pFirstRun;
                // The head of double-linked collection of TextRuns.

            LsRun *m_pLastRun;
                // The tail of double-linked collection of TextRuns.

            mutable LsRun *m_pRecentRun;
                // Recently accessed TextRun. Cached to optimize sequential access.
        };
    }
}

