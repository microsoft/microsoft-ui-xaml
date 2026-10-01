// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "TextLineBreak.h"
#include "TextFormatter.h"
#include <DependentResource.h>

namespace RichTextServices
{
    namespace Internal
    {
        class LsTextFormatter;

        struct LsBreakRecordTraits
        {
            using Handle = DependentResource<TextFormatter, Ptls6::PLSBREAKRECLINE, LsBreakRecordTraits>;
            static void Destroy(TextFormatter& owner, Ptls6::PLSBREAKRECLINE record) noexcept;

        private:
            friend class LsTextLine;
            static Handle Adopt(LsTextFormatter& origin, Ptls6::PLSBREAKRECLINE& producerSlot) noexcept;
        };

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
            explicit LsTextLineBreak(LsBreakRecordTraits::Handle&& record) noexcept;

            // Parameterless constructor, if no LS break record exists
            LsTextLineBreak();

            // Gets LineServices BreakRecord.
            Ptls6::PLSBREAKRECLINE GetLsBreakRecord() const;

        protected:

            // Destructor.
            ~LsTextLineBreak();

        private:

            LsBreakRecordTraits::Handle m_breakRecord;
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
            return m_breakRecord.Get();
        }
    }
}
