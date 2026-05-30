// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextLineBreak.h"

namespace RichTextServices
{
    namespace Internal
    {
        //---------------------------------------------------------------------------
        //
        //  LsTextLineBreak
        //
        //  Contains state at the point where text line is broken by the line breaking
        //  process.
        //
        //---------------------------------------------------------------------------
        class LsTextLineBreak : public TextLineBreak
        {
        public:

            // Constructor.
            LsTextLineBreak(
                _In_ Ptls6::PLSC pLineServicesContext,
                _In_ Ptls6::PLSBREAKRECLINE pBreakRecord
                );

            // Parameterless constructor, if no LS break record exists
            LsTextLineBreak();

            // Gets LineServices BreakRecord.
            Ptls6::PLSBREAKRECLINE GetLsBreakRecord() const;

        protected:

            // Destructor.
            ~LsTextLineBreak();

        private:

            Ptls6::PLSBREAKRECLINE m_pBreakRecord;
                // Pointer to wrapped LineServices BreakRecord.

            Ptls6::PLSC m_pLineServicesContext;
                // Line services contenxt.
        };

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      BreakRecord::GetLsBreakRecord
        //
        //  Returns:
        //      Pointer to the LineServices BreakRecord wrapped by this object.
        //
        //---------------------------------------------------------------------------
        inline Ptls6::PLSBREAKRECLINE LsTextLineBreak::GetLsBreakRecord() const
        {
            return m_pBreakRecord;
        }
    }
}
