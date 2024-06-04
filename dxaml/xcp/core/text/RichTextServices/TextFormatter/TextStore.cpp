// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "TextStore.h"
#include "TextRunData.h"
#include "LsParagraphSpan.h"
#include "LsRun.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::TextStore
//
//  Synopsis:
//      Initializes a new instance of the TextStore class.
//
//---------------------------------------------------------------------------
TextStore::TextStore()
    : m_pFontAndScriptServices(NULL),
      m_pTextSource(NULL),
      m_pMasterSpan(NULL),
      m_paragraphFlowDirection(RichTextServices::FlowDirection::LeftToRight),
      m_detectedParagraphFlowDirection(RichTextServices::FlowDirection::LeftToRight)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::~TextStore
//
//  Synopsis:
//      Release resources associated with the TextStore.
//
//---------------------------------------------------------------------------
TextStore::~TextStore()
{
    Clear();
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::Clear
//
//  Synopsis:
//      Clears all runs in the cache.
//
//---------------------------------------------------------------------------
void TextStore::Clear()
{    
    m_textSegment.Clear();
    m_runCache.Clear();

    // Delete the master span and all of its children.
    // Spans are forming a tree and to avoid recursive delete, we are walking the tree in following order:
    //  - If children are present, navigate through first child chain to reach the leaf node in DFS order. 
    //    During this process clear m_pFirstChild to avoid reaching visited children.
    //  - If node has a sibling, navigate there next, otherwise navigate next to the parent.
    //  - Delete just visited node, and repeat the process.
    if (m_pMasterSpan != NULL)
    {
        LsSpan *pTempSpan;
        LsSpan *pNextSpan = m_pMasterSpan;
        while (pNextSpan != NULL)
        {
            while (pNextSpan->m_pFirstChild != NULL)
            {
                pNextSpan = pNextSpan->m_pFirstChild;
                pNextSpan->m_pParent->m_pFirstChild = NULL; // Not needed anymore
            }

            pTempSpan = pNextSpan;
            if (pNextSpan->m_pNextSibling != NULL)
            {
                pNextSpan = pNextSpan->m_pNextSibling;
            }
            else
            {
                pNextSpan = pNextSpan->m_pParent;
            }
            
            delete pTempSpan;
        }
        m_pMasterSpan = NULL;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::DetermineParagraphTextReadingOrder
//
//  Synopsis:
//      Determine the text reading order of the content for the textsource.
//
//---------------------------------------------------------------------------
Result::Enum TextStore::DetermineParagraphTextReadingOrder(
    _Inout_ RichTextServices::FlowDirection::Enum *pParagraphFlowDirection
)
{
    Result::Enum txhr = Result::Success;
    PALText::ITextAnalyzer *pTextAnalyzer = NULL;
    TextSegment textSegment;
    FssReadingDirection::Enum readingDirection;
    bool isAmbiguousReadingDirection;

    IFCTEXT(textSegment.Populate(0, m_pTextSource, *pParagraphFlowDirection));

    if (textSegment.GetTotalLength() > 0)
    {
        IFC_FROM_HRESULT_RTS(m_pFontAndScriptServices->CreateTextAnalyzer(&pTextAnalyzer));
        IFC_FROM_HRESULT_RTS(pTextAnalyzer->GetContentReadingDirection(
                &textSegment,
                0,
                textSegment.GetTotalLength(),
                &readingDirection,
                &isAmbiguousReadingDirection
            ));

        if (!isAmbiguousReadingDirection)
        {
            *pParagraphFlowDirection = (readingDirection == FssReadingDirection::LeftToRight) ? RichTextServices::FlowDirection::LeftToRight : RichTextServices::FlowDirection::RightToLeft;
        }
    }

Cleanup:
    textSegment.Clear();
    ReleaseInterface(pTextAnalyzer);
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::Initialize
//
//  Synopsis:
//      Initializes the text store with host provided objects.
//
//---------------------------------------------------------------------------
Result::Enum TextStore::Initialize(
    _In_ IFontAndScriptServices *pFontAndScriptServices,
        // Provides an interface to access font and script specific data.
    _In_ TextSource* pTextSource,
        // The source of content and formatting data.
    _In_ TextParagraphProperties *pTextParagraphProperties
        // Paragraph properties for the current content.
    )
{
    Result::Enum txhr = Result::Success;

    // TODO: FontAndScriptServices object may change when multiple TextFormatters are created and used to format the same content.
    // ASSERT_EXPECT_RTS(m_pFontAndScriptServices == NULL || m_pFontAndScriptServices == pFontAndScriptServices);
    // NOTE: IFontAndScriptServices is not AddRef'd here since the caller controls the lifetime.
    m_pFontAndScriptServices = pFontAndScriptServices;

    // TODO: BRING THIS ASSERT BACK ASAP.
    // It is removed because run caches may be shared across linked containers but text sources may be different.
    // We should enable one text source per paragraph through break record, the same as run cache.
    //ASSERT_EXPECT_RTS(m_pTextSource == NULL || m_pTextSource == pTextSource);  
    m_pTextSource = pTextSource;

    if (m_pMasterSpan == NULL)
    {
        m_paragraphFlowDirection = pTextParagraphProperties->GetFlowDirection();
        m_detectedParagraphFlowDirection = m_paragraphFlowDirection;

        // If TextReadingOrder is set to DetectFromContent, override the set flow direction.
        if(pTextParagraphProperties->GetFlags(TextParagraphProperties::Flags::DetermineTextReadingOrderFromContent))
        {
            DetermineParagraphTextReadingOrder(&m_paragraphFlowDirection);
            m_detectedParagraphFlowDirection = m_paragraphFlowDirection;
        }
        else if (pTextParagraphProperties->GetFlags(TextParagraphProperties::Flags::DetermineAlignmentFromContent))
        {
            DetermineParagraphTextReadingOrder(&m_detectedParagraphFlowDirection);
        }

        IFC_OOM_RTS(m_pMasterSpan = new LsParagraphSpan(0, m_paragraphFlowDirection));
    }
Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::FetchRun
//
//  Synopsis:
//      Fetch the next LsRun from the content source.
//
//---------------------------------------------------------------------------
Result::Enum TextStore::FetchRun(
    _In_ XUINT32 characterIndex,
        // Index of first character in the text run.
    _In_ LsSpan *pLsSpan,
        // LsSpan object which uniquely identifies the run in case where 
        // multiple runs are starting at the same position.
    _Outptr_ LsRun **ppLsRun
        // On successful return point to the fetched LsRun object.
    )
{
    Result::Enum txhr = Result::Success;
    TextRunData *pRunData = NULL;

#if DBG
    LsRun *pLsRun = NULL;
    pLsRun = m_runCache.GetLsRun(characterIndex, NULL);
    ASSERT(pLsRun == NULL);
#endif
    
    if (m_textSegment.IsEmpty())
    {
        IFCTEXT(m_textSegment.Populate(
            characterIndex,
            m_pTextSource,
            m_paragraphFlowDirection));
        IFCTEXT(m_textSegment.Analyze(m_pFontAndScriptServices));
    }
    
    pRunData = m_textSegment.FetchNextRun();
    IFCTEXT(CreateLsRuns(characterIndex, pRunData, pLsSpan, ppLsRun));
    pRunData->Detach();

Cleanup:
    delete pRunData;
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextStore::CreateLsRuns
//
//  Synopsis:
//      Create LsRun(s) from RunData object.
//
//---------------------------------------------------------------------------
Result::Enum TextStore::CreateLsRuns(
    _In_ XUINT32 characterIndex,
        // Index of first character in the text run.
    _In_ TextRunData *pRunData,
        // Represents TextRun and its properties acquired during text analysis process.
    _In_ LsSpan *pLsSpan,
        // LsSpan object which uniquely identifies the run in case where 
        // multiple runs are starting at the same position.
    _Outptr_ LsRun **ppLsRun
        // On successful return point to the fetched LsRun object.
    )
{
    Result::Enum txhr = Result::Success;
    LsRun *pLsRun = NULL;
    LsRun *pTempLsRun;
    XUINT8 contextBidiLevel;
    XUINT8 runBidiLevel;

    // Analyze TextRun and create appropriate LsRuns.
    // NOTE: Multiple LsRuns may be created from a single TextRun.

    // First, check if any Reverse objects need to be opened/closed.
    // If the Bidi level of the context (LsSpan) is different than the Bidi level 
    // of the TextRun, Reverse objects need to be opened/closed.
    contextBidiLevel = pLsSpan->GetBidiLevel();
    runBidiLevel = pRunData->GetBidiLevel();
    if (contextBidiLevel != runBidiLevel)
    {
        if (contextBidiLevel < runBidiLevel)
        {
            // Bidi level is increasing - for each new level create run representing 
            // opening of Reverse Object.
            while (contextBidiLevel < runBidiLevel)
            {
                contextBidiLevel++;
                IFCTEXT(LsRun::CreateReverseObjectOpen(characterIndex, contextBidiLevel, &pLsSpan, &pTempLsRun));
                m_runCache.AppendLsRun(pTempLsRun);

                // Remember the first run, so it can be returned back to LS.
                if (pLsRun == NULL)
                {
                    pLsRun = pTempLsRun;
                }
            }
        }
        else
        {
            // Bidi level is decreasing - for each dropped level create run representing 
            // closing of Reverse Object.
            while (contextBidiLevel > runBidiLevel)
            {
                IFCTEXT(LsRun::CreateReverseObjectClose(characterIndex, contextBidiLevel, &pLsSpan, &pTempLsRun));
                m_runCache.AppendLsRun(pTempLsRun);
                contextBidiLevel--;

                // Remember the first run, so it can be returned back to LS.
                if (pLsRun == NULL)
                {
                    pLsRun = pTempLsRun;
                }
            }
        }
    }

    // At this point all LS Spans are properly opened/closed. Create LsRun based on TextRun.
    IFCTEXT(LsRun::CreateFromTextRun(characterIndex, pRunData, pLsSpan, &pTempLsRun));
    m_runCache.AppendLsRun(pTempLsRun);
    
    // Remember the first run, so it can be returned back to LS.
    if (pLsRun == NULL)
    {
        pLsRun = pTempLsRun;
    }

    *ppLsRun = pLsRun;

Cleanup:
    return txhr;
}
