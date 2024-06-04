// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <stack>
#include "CollectionInitializationStringParser.h"
#include "XStringBuilder.h"

using namespace Parser;

CollectionInitializationStringParser::Symbols CollectionInitializationStringParser::ConvertCharacterToSymbol(WCHAR c)
{
    switch (c) {
        case '[':
        case ']':
            return Symbols::TS_RESERVEDSYMBOL;
        case ',':
            return Symbols::TS_COMMA;
        case '\'':
            return Symbols::TS_SINGLEQUOTE;
        case '"':
            return Symbols::TS_DOUBLEQUOTE;
        case '\\':
            return Symbols::TS_BACKSLASH;
        case '\0':
            return Symbols::TS_EOS;
        default:
            return Symbols::TS_CHARACTER;
    }
}

_Check_return_ HRESULT CollectionInitializationStringParser::ParseInitializationString(const xstring_ptr_view& input, Jupiter::stack_vector<xstring_ptr, 8>& output)
{
    // An LL(1) parser is used to parse the initialization string, a string with elements separated by commas. The parser uses
    // an internal stack to process terminal and non-terminal symbols, and the parser table is derived from the grammar specified
    // below and implemented using if statements.

    // Grammar in BNF notation
    // INPUT ::= NTS_ITEMS
    // NTS_ITEMS ::= NTS_ITEM NTS_Z
    // NTS_Z ::= TS_COMMA NTS_ITEM NTS_Z | ""
    // NTS_ITEM ::= NTS_TEXT | TS_SINGLEQUOTE NTS_LITERALTEXT TS_SINGLEQUOTE | TS_DOUBLEQUOTE NTS_LITERALTEXT TS_DOUBLEQUOTE
    // NTS_TEXT ::= "" | TS_CHARACTER NTS_TEXT | TS_BACKSLASH NTS_ESCAPEDCHARACTER NTS_TEXT
    // NTS_LITERALTEXT ::= "" | TS_CHARACTER NTS_LITERALTEXT | TS_BACKSLASH NTS_ESCAPEDCHARACTER NTS_LITERALTEXT | TS_RESERVEDSYMBOL NTS_LITERALTEXT | TS_COMMA NTS_LITERALTEXT
    // TS_CHARACTER ::= [^ [],’”\]     // regex notation for all characters except [],’”\
    // TS_RESERVEDSYMBOL ::= "[" | "]"
    // TS_SINGLEQUOTE ::= "’" 
    // TS_DOUBLEQUOTE ::= "”"
    // TS_COMMA ::= ","
    // TS_BACKSLASH ::= "\"
    // NTS_ESCAPEDCHARACTER :: = TS_CHARACTER | TS_RESERVEDSYMBOL | TS_SINGLEQUOTE | TS_DOUBLEQUOTE | TS_COMMA | TS_BACKSLASH

    if (input.IsNullOrEmpty())
    {
        IFC_RETURN(E_INVALIDARG);
    }

    // initialize parser stack
    std::stack<Symbols> parserStack;
    parserStack.push(Symbols::TS_EOS);
    parserStack.push(Symbols::NTS_ITEMS);

    int inputIndex = 0;
    uint32_t inputCount;
    auto inputBuffer = input.GetBufferAndCount(&inputCount);
    auto inputCharacter = inputBuffer[inputIndex];

    // variables to mark state of what is being parsed
    auto currentItem = CurrentItemState::NO_CURRENT_ITEM;
    auto escapeSequence = EscapeSequenceState::NO_ESCAPE_SEQUENCE;
    auto whitespace = WhitespaceState::NO_PRECEDING_WS;

    auto currentString = xstring_ptr::EmptyString();
    XStringBuilder currentItemBuilder;
    IFC_RETURN(currentItemBuilder.Initialize());

    while (!parserStack.empty())
    {
        CollectionInitializationStringParser::Symbols currentInputSymbol = ConvertCharacterToSymbol(inputCharacter);
        if (currentInputSymbol == parserStack.top()) // current input symbol matches topmost symbol in stack
        {
            if (escapeSequence == EscapeSequenceState::REACHED_ESCAPE_SEQUENCE && parserStack.top() == Symbols::TS_BACKSLASH)
            {
                escapeSequence = EscapeSequenceState::PARSING_ESCAPE_SEQUENCE;
            }
            else if (currentItem == CurrentItemState::TEXT_CURRENT_VALUE) // text
            {
                if (escapeSequence == EscapeSequenceState::PARSING_ESCAPE_SEQUENCE)
                {
                    IFC_RETURN(currentItemBuilder.AppendChar(inputCharacter));

                    escapeSequence = EscapeSequenceState::NO_ESCAPE_SEQUENCE;
                }
                else if (parserStack.top() == Symbols::TS_CHARACTER)
                {
                    IFC_RETURN(currentItemBuilder.AppendChar(inputCharacter));
                }
                else if (parserStack.top() == Symbols::TS_COMMA || parserStack.top() == Symbols::TS_EOS)
                {
                    IFC_RETURN(currentItemBuilder.DetachString(&currentString));
                    IFC_RETURN(currentItemBuilder.Reset());

                    xstring_ptr trimmedObjectString;
                    IFC_RETURN(xstring_ptr::CloneBufferTrimWhitespace(currentString.GetBuffer(), currentString.GetCount(), &trimmedObjectString));

                    output.m_vector.push_back(trimmedObjectString);

                    currentItem = CurrentItemState::NO_CURRENT_ITEM;
                }
            }
            else if (currentItem == CurrentItemState::LITERALTEXT_CURRENT_VALUE) // literaltext
            {
                if (escapeSequence == EscapeSequenceState::PARSING_ESCAPE_SEQUENCE)
                {
                    IFC_RETURN(currentItemBuilder.AppendChar(inputCharacter));

                    escapeSequence = EscapeSequenceState::NO_ESCAPE_SEQUENCE;
                }
                else if (parserStack.top() == Symbols::TS_CHARACTER || parserStack.top() == Symbols::TS_RESERVEDSYMBOL || parserStack.top() == Symbols::TS_COMMA)
                {
                    IFC_RETURN(currentItemBuilder.AppendChar(inputCharacter));
                }
                else if (parserStack.top() == Symbols::TS_SINGLEQUOTE || parserStack.top() == Symbols::TS_DOUBLEQUOTE)
                {
                    IFC_RETURN(currentItemBuilder.DetachString(&currentString));
                    IFC_RETURN(currentItemBuilder.Reset());

                    output.m_vector.push_back(currentString);

                    currentItem = CurrentItemState::NO_CURRENT_ITEM;
                }
            }

            if (parserStack.top() == Symbols::TS_CHARACTER && escapeSequence == EscapeSequenceState::REACHED_ESCAPE_SEQUENCE) // mark end of escape sequence
            {
                escapeSequence = EscapeSequenceState::NO_ESCAPE_SEQUENCE;
            }
            if (inputIndex == inputCount - 1) // reached end of input string, set input character to EOS
            {
                inputCharacter = '\0';
            }
            else // increment input character
            {
                inputIndex++;
                inputCharacter = inputBuffer[inputIndex];
            }
            parserStack.pop();
        }
        else // Use current input symbol and topmost symbol in stack to determine which rule to apply
        {
            if (parserStack.top() == Symbols::NTS_ITEMS && (currentInputSymbol == Symbols::TS_CHARACTER || currentInputSymbol == Symbols::TS_SINGLEQUOTE || currentInputSymbol == Symbols::TS_DOUBLEQUOTE || currentInputSymbol == Symbols::TS_BACKSLASH))
            {
                // NTS_ITEMS ::= NTS_ITEM NTS_Z
                parserStack.pop();
                parserStack.push(Symbols::NTS_Z);
                parserStack.push(Symbols::NTS_ITEM);
            }
            else if (parserStack.top() == Symbols::NTS_ITEM)
            {
                if (currentInputSymbol == Symbols::TS_CHARACTER || currentInputSymbol == Symbols::TS_BACKSLASH)
                {
                    // NTS_ITEM ::= NTS_TEXT
                    parserStack.pop();
                    parserStack.push(Symbols::NTS_TEXT);

                    currentItem = CurrentItemState::TEXT_CURRENT_VALUE;

                    if (iswspace(inputCharacter))
                    {
                        whitespace = WhitespaceState::PRECEDING_WS;
                    }
                }
                else if (currentInputSymbol == Symbols::TS_SINGLEQUOTE || currentInputSymbol == Symbols::TS_DOUBLEQUOTE)
                {
                    // NTS_ITEM ::= TS_SINGLEQUOTE NTS_LITERALTEXT TS_SINGLEQUOTE | TS_DOUBLEQUOTE NTS_LITERALTEXT TS_DOUBLEQUOTE
                    parserStack.pop();
                    parserStack.push(currentInputSymbol);
                    parserStack.push(Symbols::NTS_LITERALTEXT);
                    parserStack.push(currentInputSymbol);
                }
                else // parsing table defaulted
                {
                    output.m_vector.clear();
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else if (parserStack.top() == Symbols::NTS_TEXT)
            {
                if (currentInputSymbol == Symbols::TS_CHARACTER)
                {
                    // NTS_TEXT ::= TS_CHARACTER NTS_TEXT
                    parserStack.push(Symbols::TS_CHARACTER);
                }
                else if (currentInputSymbol == Symbols::TS_COMMA || currentInputSymbol == Symbols::TS_EOS)
                {
                    // NTS_TEXT ::= ""
                    parserStack.pop();
                }
                else if (currentInputSymbol == Symbols::TS_BACKSLASH)
                {
                    // NTS_TEXT ::=  TS_BACKSLASH NTS_ESCAPEDCHARACTER NTS_TEXT
                    parserStack.push(Symbols::NTS_ESCAPEDCHARACTER);
                    parserStack.push(Symbols::TS_BACKSLASH);

                    escapeSequence = EscapeSequenceState::REACHED_ESCAPE_SEQUENCE;
                }
                else if (whitespace == WhitespaceState::PRECEDING_WS && (currentInputSymbol == Symbols::TS_SINGLEQUOTE || currentInputSymbol == Symbols::TS_DOUBLEQUOTE)) // handles whitespace before literal string
                {
                    // NTS_TEXT ::= TS_SINGLEQUOTE NTS_LITERALTEXT TS_SINGLEQUOTE | TS_DOUBLEQUOTE NTS_LITERALTEXT TS_DOUBLEQUOTE
                    IFC_RETURN(currentItemBuilder.Reset());

                    // pop preceding TS_CHARACTER and NTS_TEXT
                    while (parserStack.top() != Symbols::NTS_Z)
                    {
                        parserStack.pop();
                    }

                    parserStack.push(currentInputSymbol);
                    parserStack.push(Symbols::NTS_LITERALTEXT);
                    parserStack.push(currentInputSymbol);

                    currentItem = CurrentItemState::NO_CURRENT_ITEM;
                    whitespace = WhitespaceState::NO_PRECEDING_WS;
                }
                else // parsing table defaulted
                {
                    output.m_vector.clear();
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else if (parserStack.top() == Symbols::NTS_LITERALTEXT)
            {
                if (currentInputSymbol == Symbols::TS_CHARACTER)
                {
                    // NTS_LITERALTEXT ::= TS_CHARACTER NTS_LITERALTEXT 
                    parserStack.push(Symbols::TS_CHARACTER);

                    currentItem = CurrentItemState::LITERALTEXT_CURRENT_VALUE;
                }
                else if (currentInputSymbol == Symbols::TS_RESERVEDSYMBOL)
                {
                    // NTS_LITERALTEXT ::= TS_RESERVEDSYMBOL NTS_LITERALTEXT
                    parserStack.push(Symbols::TS_RESERVEDSYMBOL);

                    currentItem = CurrentItemState::LITERALTEXT_CURRENT_VALUE;
                }
                else if (currentInputSymbol == Symbols::TS_COMMA)
                {
                    // NTS_LITERALTEXT ::= TS_COMMA NTS_LITERALTEXT
                    parserStack.push(Symbols::TS_COMMA);

                    currentItem = CurrentItemState::LITERALTEXT_CURRENT_VALUE;
                }
                else if (currentInputSymbol == Symbols::TS_SINGLEQUOTE || currentInputSymbol == Symbols::TS_DOUBLEQUOTE)
                {
                    // NTS_LITERALTEXT ::= ""
                    parserStack.pop();

                    currentItem = CurrentItemState::LITERALTEXT_CURRENT_VALUE;
                }
                else if (currentInputSymbol == Symbols::TS_BACKSLASH)
                {
                    // NTS_LITERALTEXT ::= TS_BACKSLASH NTS_ESCAPEDCHARACTER NTS_LITERALTEXT
                    parserStack.push(Symbols::NTS_ESCAPEDCHARACTER);
                    parserStack.push(Symbols::TS_BACKSLASH);

                    escapeSequence = EscapeSequenceState::REACHED_ESCAPE_SEQUENCE;
                    currentItem = CurrentItemState::LITERALTEXT_CURRENT_VALUE;
                }
                else // parsing table defaulted
                {
                    output.m_vector.clear();
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else if (parserStack.top() == Symbols::NTS_Z)
            {
                if (currentInputSymbol == Symbols::TS_COMMA)
                {
                    // NTS_Z ::= TS_COMMA NTS_ITEM NTS_Z
                    parserStack.push(Symbols::NTS_ITEM);
                    parserStack.push(Symbols::TS_COMMA);
                }
                else if (currentInputSymbol == Symbols::TS_EOS)
                {
                    // NTS_Z ::= ""
                    parserStack.pop();
                }
                else if (currentInputSymbol == Symbols::TS_CHARACTER && iswspace(inputCharacter))
                {
                    // push TS_CHARACTER, so extra whitespaces are popped off at start of white loop
                    parserStack.push(Symbols::TS_CHARACTER);
                }
                else // parsing table defaulted
                {
                    output.m_vector.clear();
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else if (parserStack.top() == Symbols::NTS_ESCAPEDCHARACTER)
            {
                if (currentInputSymbol == Symbols::TS_CHARACTER)
                {
                    // NTS_ESCAPEDCHARACTER :: = TS_CHARACTER
                    parserStack.pop();
                    parserStack.push(Symbols::TS_CHARACTER);
                }
                else if (currentInputSymbol == Symbols::TS_RESERVEDSYMBOL)
                {
                    // NTS_ESCAPEDCHARACTER :: = TS_RESERVEDSYMBOL 
                    parserStack.pop();
                    parserStack.push(Symbols::TS_RESERVEDSYMBOL);
                }
                else if (currentInputSymbol == Symbols::TS_SINGLEQUOTE || currentInputSymbol == Symbols::TS_DOUBLEQUOTE)
                {
                    // NTS_ESCAPEDCHARACTER :: = TS_SINGLEQUOTE | TS_DOUBLEQUOTE
                    parserStack.pop();
                    parserStack.push(currentInputSymbol);
                }
                else if (currentInputSymbol == Symbols::TS_BACKSLASH)
                {
                    // NTS_ESCAPEDCHARACTER ::= TS_BACKSLASH
                    parserStack.pop();
                    parserStack.push(Symbols::TS_BACKSLASH);
                }
                else if (currentInputSymbol == Symbols::TS_COMMA)
                {
                    // NTS_ESCAPEDCHARACTER ::= TS_COMMA
                    parserStack.pop();
                    parserStack.push(Symbols::TS_COMMA);
                }
                else // parsing table defaulted
                {
                    output.m_vector.clear();
                    IFC_RETURN(E_INVALIDARG);
                }
            }
            else // parsing table defaulted
            {
                output.m_vector.clear();
                IFC_RETURN(E_INVALIDARG);
            }

        }
    }
    return S_OK;
}

