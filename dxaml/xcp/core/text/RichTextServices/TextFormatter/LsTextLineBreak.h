// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextLineBreak.h"

namespace RichTextServices
{
    namespace Internal
    {
        class LsTextFormatter;

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
                _In_ LsTextFormatter *pTextFormatter,
                _In_ Ptls6::PLSBREAKRECLINE pBreakRecord
                );

            // Parameterless constructor, if no LS break record exists
            LsTextLineBreak();

            // Gets LineServices BreakRecord.
            Ptls6::PLSBREAKRECLINE GetLsBreakRecord() const;

            // Gets the formatter that owns the LineServices context used to produce the wrapped
            // BreakRecord. A break record is bound to the exact LS context that created it, so
            // continuation formatting must run on this formatter. Non ref-counting accessor - the
            // returned formatter is kept alive by this object's own reference (see m_pTextFormatter).
            LsTextFormatter* GetTextFormatter() const;

        protected:

            // Destructor.
            ~LsTextLineBreak();

        private:

            Ptls6::PLSBREAKRECLINE m_pBreakRecord;
                // Pointer to wrapped LineServices BreakRecord.

            LsTextFormatter *m_pTextFormatter;
                // Owner of the LineServices context used to destroy the wrapped BreakRecord.
                // Held with a reference so the context outlives this cached break record,
                // mirroring LsTextLine's ownership of the formatter.
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

        //---------------------------------------------------------------------------
        //
        //  Member:
        //      LsTextLineBreak::GetTextFormatter
        //
        //  Returns:
        //      The formatter that owns the LineServices context used to create the wrapped
        //      BreakRecord, or NULL for a parameterless (empty) break record.
        //
        //---------------------------------------------------------------------------
        inline LsTextFormatter* LsTextLineBreak::GetTextFormatter() const
        {
            return m_pTextFormatter;
        }
    }
}
