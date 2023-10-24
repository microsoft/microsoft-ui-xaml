// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Implementation of ITextAnalysisSink which is an interface that 
//      represents a collector of text analysis results.

#include "precomp.h"
#include "TextItemizer.h"
#include "TextRunData.h"
#include "TxMath.h"

using namespace RichTextServices;
using namespace RichTextServices::Internal;

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::TextItemizer
//
//  Synopsis:
//      Initializes a new instance of the TextItemizer class.
//
//---------------------------------------------------------------------------
TextItemizer::TextItemizer(
    _In_ XUINT8 defaultBidiLevel
        // Default bidi level for the text.
    ) :
    m_defaultBidiLevel(defaultBidiLevel), 
    m_pFirstScriptAnalysisNode(NULL),
    m_pCurrentScriptAnalysisNode(NULL),
    m_pFirstBidiAnalysisNode(NULL),
    m_pCurrentBidiAnalysisNode(NULL),
    m_pFirstNumberSubstitutionAnalysisNode(NULL),
    m_pCurrentNumberSubstitutionAnalysisNode(NULL)
{
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::~TextItemizer
//
//  Synopsis:
//      Release resources associated with the TextItemizer.
//
//---------------------------------------------------------------------------
TextItemizer::~TextItemizer()
{
    TextAnalysisNode<FssScriptAnalysis> *pScriptAnalysisNode;
    while (m_pFirstScriptAnalysisNode != NULL)
    {
        pScriptAnalysisNode = m_pFirstScriptAnalysisNode;
        m_pFirstScriptAnalysisNode  = m_pFirstScriptAnalysisNode->Next;
        delete pScriptAnalysisNode;
    }

    TextAnalysisNode<XUINT8> *pBidiAnalysisNode;
    while (m_pFirstBidiAnalysisNode != NULL)
    {
        pBidiAnalysisNode = m_pFirstBidiAnalysisNode;
        m_pFirstBidiAnalysisNode = m_pFirstBidiAnalysisNode->Next;
        delete pBidiAnalysisNode;
    }

    TextAnalysisNode<IDWriteNumberSubstitution*> *pNumberSubstitutionAnalysisNode;
    while (m_pFirstNumberSubstitutionAnalysisNode!= NULL)
    {
        pNumberSubstitutionAnalysisNode = m_pFirstNumberSubstitutionAnalysisNode;
        m_pFirstNumberSubstitutionAnalysisNode = m_pFirstNumberSubstitutionAnalysisNode->Next;
        delete pNumberSubstitutionAnalysisNode;
    }
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::Itemize
//
//  Synopsis:
//      Breaks down a given RunData collection into a more fine grained collection where 
//      each RunData has common attributes (Scripts, Number Substitution & Bidi).
//
//---------------------------------------------------------------------------
Result::Enum TextItemizer::Itemize(
    _Inout_ TextRunData *pRunData
        // The collection of run data to itemize into finer blocks each with common text analysis properties.
    )
{
    Result::Enum txhr = Result::Success;
    XUINT32 rangeStart = 0;
    XUINT32 rangeLength;
    TextAnalysisNode<FssScriptAnalysis> *pScriptAnalysisNode = NULL;
    TextAnalysisNode<XUINT8> *pBidiAnalysisNode = NULL;
    TextAnalysisNode<IDWriteNumberSubstitution*> *pNumberSubstitutionAnalysisNode = NULL;
    
    const FssScriptAnalysis defaultScriptAnalysis = { 0 };
    const XUINT8 defaultBidirectionalLevel = m_defaultBidiLevel;

    // Reset the current nodes, since they are being reused for itemization process.
    // NOTE: Analysis process should have been done by now, so it is safe to do that.
    m_pCurrentScriptAnalysisNode = m_pFirstScriptAnalysisNode;
    m_pCurrentBidiAnalysisNode = m_pFirstBidiAnalysisNode;
    m_pCurrentNumberSubstitutionAnalysisNode = m_pFirstNumberSubstitutionAnalysisNode;

    while (pRunData != NULL)
    {
        rangeLength = GetNextRangeLength(rangeStart, &pScriptAnalysisNode, &pBidiAnalysisNode, &pNumberSubstitutionAnalysisNode);

        while (pRunData != NULL && rangeLength > 0)
        {
            if (pRunData != NULL)
            {
                if (rangeLength < pRunData->GetLength())
                {
                    TextRunData *pTempRunData;
                    IFCTEXT(pRunData->Split(rangeLength, &pTempRunData));
                    rangeStart += rangeLength;
                    rangeLength = 0;
                }
                else
                {
                    rangeStart += pRunData->GetLength();
                    rangeLength -= pRunData->GetLength();
                }

                // End of line and end of paragraph runs need to have paragraphs bidi level 
                // in order to guarantee proper subline processing by LineServices.
                if (pRunData->GetTextRun()->GetType() == TextRunType::EndOfLine ||
                    pRunData->GetTextRun()->GetType() == TextRunType::EndOfParagraph)
                {
                    pRunData->SetAnalysisProperties(
                        pScriptAnalysisNode != NULL ? pScriptAnalysisNode->Value : defaultScriptAnalysis,
                        defaultBidirectionalLevel,
                        pNumberSubstitutionAnalysisNode != NULL? pNumberSubstitutionAnalysisNode->Value : NULL);
                }
                else
                {
                    pRunData->SetAnalysisProperties(
                        pScriptAnalysisNode != NULL ? pScriptAnalysisNode->Value : defaultScriptAnalysis,
                        pBidiAnalysisNode != NULL ? pBidiAnalysisNode->Value : defaultBidirectionalLevel,
                        pNumberSubstitutionAnalysisNode != NULL? pNumberSubstitutionAnalysisNode->Value : NULL);
                }
                pRunData = pRunData->GetNext();
            }
        }
    }

Cleanup:
    return txhr;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::SetScriptAnalysis
//
//  Synopsis:
//      Report script analysis for the text range.
//
//---------------------------------------------------------------------------
HRESULT TextItemizer::SetScriptAnalysis(
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ FssScriptAnalysis const* pScriptAnalysis
    )
{        
    HRESULT hr = S_OK;
    TextAnalysisNode<FssScriptAnalysis>* pScriptAnalysisNode;    

    pScriptAnalysisNode = new TextAnalysisNode<FssScriptAnalysis>;
    pScriptAnalysisNode->Value    = *pScriptAnalysis;
    pScriptAnalysisNode->Position = textPosition;
    pScriptAnalysisNode->Length   = textLength;
    pScriptAnalysisNode->Next     = NULL;    

    if (m_pFirstScriptAnalysisNode == NULL)
    {
        m_pFirstScriptAnalysisNode = m_pCurrentScriptAnalysisNode = pScriptAnalysisNode;
    }
    else
    {
        // Assert that the list is sorted. The itemization API assumes this.
        ASSERT(textPosition >= m_pCurrentScriptAnalysisNode->Position + m_pCurrentScriptAnalysisNode->Length);
        m_pCurrentScriptAnalysisNode->Next = pScriptAnalysisNode;
        m_pCurrentScriptAnalysisNode = pScriptAnalysisNode;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::SetBidiLevel
//
//  Synopsis:
//      Set bidirectional level on the range, called once per each
//      level run change (either explicit or resolved implicit)..
//
//---------------------------------------------------------------------------
HRESULT TextItemizer::SetBidiLevel(
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ XUINT8 explicitLevel,
    _In_ XUINT8 resolvedLevel
    )
{        
    HRESULT hr = S_OK;
    TextAnalysisNode<XUINT8>* pBidiAnalysisNode;    

    pBidiAnalysisNode = new TextAnalysisNode<XUINT8>;
    pBidiAnalysisNode->Value    = resolvedLevel;
    pBidiAnalysisNode->Position = textPosition;
    pBidiAnalysisNode->Length   = textLength;
    pBidiAnalysisNode->Next     = NULL;    

    if (m_pFirstBidiAnalysisNode == NULL)
    {
        m_pFirstBidiAnalysisNode = m_pCurrentBidiAnalysisNode = pBidiAnalysisNode;
    }
    else
    {
        // Assert that the list is sorted. The itemization API assumes this.
        ASSERT(textPosition >= m_pCurrentBidiAnalysisNode->Position + m_pCurrentBidiAnalysisNode->Length);
        m_pCurrentBidiAnalysisNode->Next = pBidiAnalysisNode;
        m_pCurrentBidiAnalysisNode       = pBidiAnalysisNode;
    }

    RRETURN(hr);//RRETURN_REMOVAL
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::SetNumberSubstitution
//
//  Synopsis:
//      Set number substitution on the range, called once per each
//      level run change (either explicit or resolved implicit)..
//
//---------------------------------------------------------------------------
HRESULT TextItemizer::SetNumberSubstitution(
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_ IDWriteNumberSubstitution* pNumberSubstitution
    )
{        
    HRESULT hr = S_OK;
    TextAnalysisNode<IDWriteNumberSubstitution*>* pNumberSubstitutionAnalysisNode;

    pNumberSubstitutionAnalysisNode = new TextAnalysisNode<IDWriteNumberSubstitution*>;
    pNumberSubstitutionAnalysisNode->Value = pNumberSubstitution;
    pNumberSubstitutionAnalysisNode->Position = textPosition;
    pNumberSubstitutionAnalysisNode->Length = textLength;
    pNumberSubstitutionAnalysisNode->Next = NULL;

    if (m_pFirstNumberSubstitutionAnalysisNode == NULL)
    {
        // DWrite will not callback to set for ranges with no digits, 
        // However we don't want any gaps in our analysis node chain due to the logic in GetNextRangeLength() 
        // We manually insert a dummy analysis node with value equal to NULL. 
        if (textPosition > 0)
        {
            TextAnalysisNode<IDWriteNumberSubstitution*>* pNumberSubstitutionAnalysisNodeDummy;
            pNumberSubstitutionAnalysisNodeDummy = new TextAnalysisNode<IDWriteNumberSubstitution*>;
            pNumberSubstitutionAnalysisNodeDummy->Value = NULL;
            pNumberSubstitutionAnalysisNodeDummy->Position = 0;
            pNumberSubstitutionAnalysisNodeDummy->Length = textPosition;

            m_pFirstNumberSubstitutionAnalysisNode = pNumberSubstitutionAnalysisNodeDummy;
            m_pFirstNumberSubstitutionAnalysisNode->Next = pNumberSubstitutionAnalysisNode;
            m_pCurrentNumberSubstitutionAnalysisNode = pNumberSubstitutionAnalysisNode;
        }
        else
        {
            m_pFirstNumberSubstitutionAnalysisNode = m_pCurrentNumberSubstitutionAnalysisNode = pNumberSubstitutionAnalysisNode;
        }
    }
    else
    {
        ASSERT(textPosition >= m_pCurrentNumberSubstitutionAnalysisNode->Position + m_pCurrentNumberSubstitutionAnalysisNode->Length);

        // Insert a dummy node if there is a gap.
        if (textPosition > m_pCurrentNumberSubstitutionAnalysisNode->Position + m_pCurrentNumberSubstitutionAnalysisNode->Length)
        {
            TextAnalysisNode<IDWriteNumberSubstitution*>* pNumberSubstitutionAnalysisNodeDummy;
            pNumberSubstitutionAnalysisNodeDummy = new TextAnalysisNode<IDWriteNumberSubstitution*>;
            pNumberSubstitutionAnalysisNodeDummy->Value = NULL;
            pNumberSubstitutionAnalysisNodeDummy->Position = m_pCurrentNumberSubstitutionAnalysisNode->Position + m_pCurrentNumberSubstitutionAnalysisNode->Length;
            pNumberSubstitutionAnalysisNodeDummy->Length = textPosition - (m_pCurrentNumberSubstitutionAnalysisNode->Position + m_pCurrentNumberSubstitutionAnalysisNode->Length);

            m_pCurrentNumberSubstitutionAnalysisNode->Next = pNumberSubstitutionAnalysisNodeDummy;
            pNumberSubstitutionAnalysisNodeDummy->Next = pNumberSubstitutionAnalysisNode;
            m_pCurrentNumberSubstitutionAnalysisNode = pNumberSubstitutionAnalysisNode;
        }
        else
        {
            m_pCurrentNumberSubstitutionAnalysisNode->Next = pNumberSubstitutionAnalysisNode;
            m_pCurrentNumberSubstitutionAnalysisNode = pNumberSubstitutionAnalysisNode;
        }
    }

    RRETURN(hr);//RRETURN_REMOVAL
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::SetLineBreakpoints
//
//  Synopsis:
//      Set line breakpoint opportunities on the range.
//
//---------------------------------------------------------------------------
HRESULT TextItemizer::SetLineBreakpoints(
    _In_ XUINT32 textPosition,
    _In_ XUINT32 textLength,
    _In_reads_(textLength) PALText::LineBreakpoint const* lineBreakpoints
    )
{
    // TextSegment's LineBreakpointAnalysisHelper receives the line breakpoint callbacks.
    ASSERT(FALSE);
    return E_NOTIMPL;
}


//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::GetNextRangeLength
//
//  Synopsis:
//      Retrieves the next range boundary and text analysis nodes representing 
//      values for this range.
//
//---------------------------------------------------------------------------
XUINT32 TextItemizer::GetNextRangeLength(
    _In_ XUINT32 rangeStart,
    _Outptr_ TextAnalysisNode<FssScriptAnalysis> **ppScriptAnalysisNode,
    _Outptr_ TextAnalysisNode<XUINT8> **ppBidiAnalysisNode,
    _Outptr_ TextAnalysisNode<IDWriteNumberSubstitution*> **ppNumberSubstitutionNode
    )
{
    XUINT32 rangeStop = XUINT32_MAX;
    TextAnalysisNode<FssScriptAnalysis> *pScriptAnalysisNode = NULL;
    TextAnalysisNode<XUINT8> *pBidiAnalysisNode = NULL;
    TextAnalysisNode<IDWriteNumberSubstitution*> *pNumberSubstitutionNode= NULL;

    if (m_pCurrentScriptAnalysisNode != NULL && m_pCurrentScriptAnalysisNode->Position <= rangeStart)
    {
        ASSERT(m_pCurrentScriptAnalysisNode->Position + m_pCurrentScriptAnalysisNode->Length > rangeStart);
        pScriptAnalysisNode = m_pCurrentScriptAnalysisNode;
    }
    if (m_pCurrentBidiAnalysisNode != NULL && m_pCurrentBidiAnalysisNode->Position <= rangeStart)
    {
        ASSERT(m_pCurrentBidiAnalysisNode->Position + m_pCurrentBidiAnalysisNode->Length > rangeStart);
        pBidiAnalysisNode = m_pCurrentBidiAnalysisNode;
    }
    if (m_pCurrentNumberSubstitutionAnalysisNode!= NULL && m_pCurrentNumberSubstitutionAnalysisNode->Position <= rangeStart)
    {
        ASSERT(m_pCurrentNumberSubstitutionAnalysisNode->Position + m_pCurrentNumberSubstitutionAnalysisNode->Length > rangeStart);
        pNumberSubstitutionNode = m_pCurrentNumberSubstitutionAnalysisNode;
    }

    if (pScriptAnalysisNode != NULL)
    {
        rangeStop = Math::Min(rangeStop, pScriptAnalysisNode->Position + pScriptAnalysisNode->Length);
    }
    if (pBidiAnalysisNode != NULL)
    {
        rangeStop = Math::Min(rangeStop, pBidiAnalysisNode->Position + pBidiAnalysisNode->Length);
    }

    if (pNumberSubstitutionNode != NULL)
    {
        rangeStop = Math::Min(rangeStop, pNumberSubstitutionNode->Position + pNumberSubstitutionNode->Length);
    }

    if (pScriptAnalysisNode != NULL && (rangeStop == pScriptAnalysisNode->Position + pScriptAnalysisNode->Length))
    {
        m_pCurrentScriptAnalysisNode = m_pCurrentScriptAnalysisNode->Next;
    }
    if (pBidiAnalysisNode != NULL && (rangeStop == pBidiAnalysisNode->Position + pBidiAnalysisNode->Length))
    {
        m_pCurrentBidiAnalysisNode = m_pCurrentBidiAnalysisNode->Next;
    }
    if (pNumberSubstitutionNode!= NULL && (rangeStop == pNumberSubstitutionNode->Position + pNumberSubstitutionNode->Length))
    {
        m_pCurrentNumberSubstitutionAnalysisNode= m_pCurrentNumberSubstitutionAnalysisNode->Next;
    }

    *ppScriptAnalysisNode = pScriptAnalysisNode;
    *ppBidiAnalysisNode = pBidiAnalysisNode;
    *ppNumberSubstitutionNode = pNumberSubstitutionNode;

    return rangeStop - rangeStart;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::AddRef
//
//  Synopsis:
//      Increments the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextItemizer::AddRef()
{
    // No-op since lifetime is managed by TextSegment, which guarantees availability during text analysis process.
    return 1;
}

//---------------------------------------------------------------------------
//
//  Member:
//      TextItemizer::Release
//
//  Synopsis:
//      Decrements the count of references to this object.
//
//---------------------------------------------------------------------------
XUINT32 TextItemizer::Release()
{
    // No-op since lifetime is managed by TextSegment, which guarantees availability during text analysis process.
    return 1;
}
