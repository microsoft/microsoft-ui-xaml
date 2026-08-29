// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      TextAnalysisNode stores the analysis results as they are populated
//      by ITextAnalysisSink.

#pragma once

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  TextAnalysisNode
        //
        //  TextAnalysisNode stores the analysis results as they are populated
        //  ITextAnalysisSink.
        //
        //---------------------------------------------------------------------------
        template<class T>
        struct TextAnalysisNode
        {
            // The value for the property that the node holds.
            T Value;

            // The start of the text range for which the value of the text property applies.
            XUINT32 Position;

            // The end of the text range for which the value of the text property applies.
            XUINT32 Length;

            // A pointer to the next node in the list.
            TextAnalysisNode<T>* Next;
        };
    }
}
