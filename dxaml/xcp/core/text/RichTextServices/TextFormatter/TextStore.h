// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextStore provides content and properties access services.

#pragma once

#include "TextRunCache.h"
#include "TextSegment.h"
#include "LsRunCache.h"

namespace RichTextServices
{
    class TextSource;

    namespace Internal
    {
        class TextRunData;
        class LsSpan;

        //---------------------------------------------------------------------------
        //
        //  TextStore
        //
        //  Provides content and properties access services.
        //
        //---------------------------------------------------------------------------
        class TextStore : public TextRunCache
        {
        public:
            // Initializes a new instance of the TextStore class.
            TextStore();

            // Clears all runs in the cache.
            virtual void Clear();

            // Initializes the text store with host provided objects.
            Result::Enum Initialize(
                _In_ IFontAndScriptServices *pFontAndScriptServices,
                _In_ TextSource *pTextSource,
                _In_ TextParagraphProperties *pTextParagraphProperties
                );

            // Fetches the next LsRun from the content source.
            Result::Enum FetchRun(
                _In_ XUINT32 characterIndex,
                _In_ LsSpan *pLsSpan,
                _Outptr_ LsRun **ppLsRun
                );

            // Returns the LsRun caching service for the content.
            const LsRunCache * GetRunCache() const;

            // Returns the master span representing the content.
            LsSpan * GetMasterSpan() const;

            // Retuns the flow direction for this paragraph
            FlowDirection::Enum GetParagraphFlowDirection() const;

            // Retuns the detected flow direction for this paragraph
            FlowDirection::Enum GetDetectedParagraphFlowDirection() const;

        protected:
            // Release resources associated with the TextStore.
            virtual ~TextStore();

        private:
            // Provides an interface to access font and script specific data.
            IFontAndScriptServices *m_pFontAndScriptServices;

            // Provides access to content and formatting data.
            TextSource *m_pTextSource;

            // The master span representing the content of the current text segment.
            LsSpan *m_pMasterSpan;

            // Provides run caching services in order to improve performance.
            LsRunCache m_runCache;

            // Text segment represents currently analyzed content.
            TextSegment m_textSegment;

            // The flow direction of the paragraph.
            FlowDirection::Enum m_paragraphFlowDirection;

            FlowDirection::Enum m_detectedParagraphFlowDirection;

            // Create LsRun(s) from TextRunData object.
            Result::Enum CreateLsRuns(
                _In_ XUINT32 characterIndex,
                _In_ TextRunData *pRunData,
                _In_ LsSpan *pLsSpan,
                _Outptr_ LsRun **ppLsRun
                );

            // Determines the reading order of the textsource based on its content.
            Result::Enum DetermineParagraphTextReadingOrder(
                _Inout_ FlowDirection::Enum *pParagraphFlowDirection
                );
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextStore::GetRunCache
        //
        //  Returns:
        //      Returns the run caching service for the content.
        //
        //---------------------------------------------------------------------------
        inline const LsRunCache * TextStore::GetRunCache() const
        {
            return &m_runCache;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextStore::GetMasterSpan
        //
        //  Returns:
        //      Returns the master span representing the content.
        //
        //---------------------------------------------------------------------------
        inline LsSpan * TextStore::GetMasterSpan() const
        {
            return m_pMasterSpan;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextStore::GetParapgraphFlowDirection
        //
        //  Returns:
        //      Returns the paragraph flow direction
        //
        //---------------------------------------------------------------------------
        inline FlowDirection::Enum TextStore::GetParagraphFlowDirection() const
        {
            return m_paragraphFlowDirection;
        }

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      TextStore::GetDetectedParapgraphFlowDirection
        //
        //  Returns:
        //      Returns the detected paragraph flow direction
        //
        //---------------------------------------------------------------------------
        inline FlowDirection::Enum TextStore::GetDetectedParagraphFlowDirection() const
        {
            return m_detectedParagraphFlowDirection;
        }
    }
}
