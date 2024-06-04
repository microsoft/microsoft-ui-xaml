// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xstring_ptr.h"
#include "stack_vector.h"

namespace Parser
{
    class CollectionInitializationStringParser {
    private:
        enum class Symbols
        {
            // Terminals
            TS_CHARACTER,
            TS_RESERVEDSYMBOL,
            TS_COMMA,
            TS_SINGLEQUOTE,
            TS_DOUBLEQUOTE,
            TS_BACKSLASH,
            TS_EOS,

            // Non-terminals
            NTS_ITEMS,
            NTS_ITEM,
            NTS_TEXT,
            NTS_LITERALTEXT,
            NTS_Z,
            NTS_ESCAPEDCHARACTER
        };
        enum class CurrentItemState
        {
            NO_CURRENT_ITEM,
            TEXT_CURRENT_VALUE,
            LITERALTEXT_CURRENT_VALUE,
        };
        enum class EscapeSequenceState
        {
            NO_ESCAPE_SEQUENCE,
            REACHED_ESCAPE_SEQUENCE,
            PARSING_ESCAPE_SEQUENCE,
        };
        enum class WhitespaceState
        {
            NO_PRECEDING_WS,   // item does not start with whitespace
            PRECEDING_WS       // item starts with whitespace, handle whitespace before literal text
        };
        static Symbols ConvertCharacterToSymbol(WCHAR c);

    public:
        static _Check_return_ HRESULT ParseInitializationString(const xstring_ptr_view& input, Jupiter::stack_vector<xstring_ptr, 8>& output);
    };
}