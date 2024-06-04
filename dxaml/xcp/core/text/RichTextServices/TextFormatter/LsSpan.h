// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Defines LsSpan represented by a plsspan value dispatched to LS.

#pragma once

#include "LsClient.h"

namespace RichTextServices
{
    namespace Internal
    {
        //-------------------------------------------------------------------
        //
        //  LsSpan
        //
        //  Span represented by a plsspan value dispatched to LS.
        //
        //-------------------------------------------------------------------
        class LsSpan
        {
        public:

            // Constructor
            LsSpan(
                _In_ XUINT32 characterIndex, 
                _In_ LsSpanType::Enum type,
                _In_opt_ LsSpan *pParent
                );

            // Returns the parent of this span.
            LsSpan * GetParent() const;

            // Returns the index of the first character of the span.
            XUINT32 GetCharacterIndex() const;

            // Returns the type of the span.
            LsSpanType::Enum GetType() const;

            // Returns the length of the span.
            XINT32 GetLength() const;

            // Sets the length of the span.
            void SetLength(_In_ XINT32 length);

            // Returns the bidi level of the span.
            virtual XUINT8 GetBidiLevel() const;

            // Gets the first child span open at a given cp. If no child span is open, returns null.
            void GetOpenChildSpan(
                _In_ XUINT32 cpCurrent, 
                _Outptr_ LsSpan **ppChildSpan
                ) const;

        private:

            XUINT32 m_characterIndex;
                // Index of the first character of the span (relative to the current context)

            LsSpanType::Enum m_type;
                // Type of the span.

            LsSpan *m_pParent;
                // The parent of this span.

            LsSpan *m_pFirstChild;
                // The first child of this span.

            LsSpan *m_pNextSibling;
                // The next sibling in the parent's collection

            XINT32 m_length;
                // The length of this span.

            friend class TextStore;
        };

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsSpan::GetParent
        //
        //  Synopsis:
        //      Returns the parent of this span.
        //
        //-------------------------------------------------------------------
        inline LsSpan * LsSpan::GetParent() const
        {
            return m_pParent;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsSpan::GetCharacterIndex
        //
        //  Synopsis:
        //      Returns the index of the first character of the span.
        //
        //-------------------------------------------------------------------
        inline XUINT32 LsSpan::GetCharacterIndex() const
        {
            return m_characterIndex;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsSpan::GetType
        //
        //  Synopsis:
        //      Returns the type of the span.
        //
        //-------------------------------------------------------------------
        inline LsSpanType::Enum LsSpan::GetType() const
        {
            return m_type;
        }

        //-------------------------------------------------------------------
        //
        //  Member:
        //      LsSpan::GetLength
        //
        //  Synopsis:
        //      Returns the length of this span. Returns -1 if the span's
        //      length has not yet been set.
        //
        //-------------------------------------------------------------------
        inline XINT32 LsSpan::GetLength() const
        {
            return m_length;
        }
    }
}

