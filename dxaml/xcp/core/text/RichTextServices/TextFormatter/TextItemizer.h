// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextItemizer handles the process of breaking down text into ranges
//      where each range has the same properties.

#pragma once

#include "TextAnalysisNode.h"

namespace RichTextServices
{
    namespace Internal
    {
        class TextRunData;

        //---------------------------------------------------------------------------
        //
        //  TextItemizer
        //
        //  Handles the process of breaking down text into ranges where each range
        //  has the same properties.
        //
        //---------------------------------------------------------------------------
        class TextItemizer : public PALText::ITextAnalysisSink
        {
        public:
            // Initializes a new instance of the TextItemizer class.
            TextItemizer(_In_ XUINT8 defaultBidiLevel);

            // Release resources associated with the TextItemizer.
            ~TextItemizer();

            // Breaks down a given RunData collection into a more fine grained collection where
            // each RunData has common attributes (Scripts, Number Substitution & Bidi).
            Result::Enum Itemize(
                _Inout_ TextRunData *pRunData
                );

            // Report script analysis for the text range.
            virtual HRESULT SetScriptAnalysis(
                _In_ XUINT32 textPosition,
                _In_ XUINT32 textLength,
                _In_ FssScriptAnalysis const* pScriptAnalysis
                );

            // Set bidirectional level on the range, called once per each
            // level run change (either explicit or resolved implicit).
            virtual HRESULT SetBidiLevel(
                _In_ XUINT32 textPosition,
                _In_ XUINT32 textLength,
                _In_ XUINT8 explicitLevel,
                _In_ XUINT8 resolvedLevel
                );

            // Set number substitution on the range, called once per each
            // level run change (either explicit or resolved implicit).
            virtual HRESULT SetNumberSubstitution(
                _In_ XUINT32 textPosition,
                _In_ XUINT32 textLength,
                _In_ IDWriteNumberSubstitution* pNumberSubstitution
                );

            // Set line breakpoints for the given range.
            virtual HRESULT SetLineBreakpoints(
                _In_ XUINT32 textPosition,
                _In_ XUINT32 textLength,
                _In_reads_(textLength) PALText::LineBreakpoint const* lineBreakpoints
                );


            // Referenced counted object interface.
            virtual XUINT32 AddRef();
            virtual XUINT32 Release();

        private:

            // Default bidi level for the text.
            XUINT8 m_defaultBidiLevel;

            // The head of linked list of script analysis results.
            TextAnalysisNode<FssScriptAnalysis> *m_pFirstScriptAnalysisNode;

            // The current node in the linked list of script analysis results.
            TextAnalysisNode<FssScriptAnalysis> *m_pCurrentScriptAnalysisNode;

            // The head of linked list of bidi analysis results.
            TextAnalysisNode<XUINT8> *m_pFirstBidiAnalysisNode;

            // The current node in the linked list of bidi analysis results.
            TextAnalysisNode<XUINT8> *m_pCurrentBidiAnalysisNode;

            // The head of linked list of number substitution results.
            TextAnalysisNode<IDWriteNumberSubstitution*> *m_pFirstNumberSubstitutionAnalysisNode;

            // The current node in the linked list of number substitution results.
            TextAnalysisNode<IDWriteNumberSubstitution*> *m_pCurrentNumberSubstitutionAnalysisNode;

            // Retrieves the next range boundary and text analysis nodes representing
            // values for this range.
            XUINT32 GetNextRangeLength(
                _In_ XUINT32 rangeStart,
                _Outptr_ TextAnalysisNode<FssScriptAnalysis> **ppScriptAnalysisNode,
                _Outptr_ TextAnalysisNode<XUINT8> **ppBidiAnalysisNode,
                _Outptr_ TextAnalysisNode<IDWriteNumberSubstitution*>  **ppNumberSubstitutionAnalysisNode
                );
        };
    }
}
