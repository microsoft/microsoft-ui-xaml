// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlScannerNode.h"

class XamlPullParser;

class XamlParserState
{
public:
    XamlParserState(void);

    // TODO: This might be better on the parser class.
    typedef HRESULT (*fParseActionDelegate)(_In_ XamlPullParser* xpp);

    enum ParseState
    {
        psError,
        psStart,
        psPrefix,
        psStartTag,
        psDirective,
        psAttribute,
        psText,
        psEndTag,
        psNumberOfParseStates
      
    };

    struct ParserStateTransition
    {
        fParseActionDelegate            m_ActionFunction;
        fParseActionDelegate            m_PostTransitionActionFunction;
        XamlParserState::ParseState     m_NextState;        
    };

    HRESULT Transition(
        XamlScannerNode::ScannerNodeType    inputNodeType,
        fParseActionDelegate*               outpfnAction,
        fParseActionDelegate*               outpfnpostTransitionAction);

    ParseState CurrentState() { return m_CurrentState; }

    ParseState PreviousState() { return m_PreviousState; }

private:
    ParseState m_CurrentState;
    ParseState m_PreviousState; // Needed?
};
