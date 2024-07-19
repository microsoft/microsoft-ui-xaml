// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

const XamlParserState::ParserStateTransition ConstParserStateTransitionTable[XamlParserState::psNumberOfParseStates][XamlScannerNode::sntNumberOfScannerNodeTypes] =
{
    {
    // From psError ->
    /* -> sntNone */             { nullptr, nullptr, XamlParserState::psError },
    /* -> sntElement */          { nullptr, nullptr, XamlParserState::psError },
    /* -> sntEmptyElement */     { nullptr, nullptr, XamlParserState::psError },
    /* -> sntAttribute */        { nullptr, nullptr, XamlParserState::psError },
    /* -> sntDirective */        { nullptr, nullptr, XamlParserState::psError },
    /* -> sntPrefixDefinition */ { nullptr, nullptr, XamlParserState::psError },
    /* -> sntPropertyElement */  { nullptr, nullptr, XamlParserState::psError },
    /* -> sntText */             { nullptr, nullptr, XamlParserState::psError },
    /* -> sntEndTag */           { nullptr, nullptr, XamlParserState::psError }
    },

    {
    // From psStart ->
    /* -> sntNone */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         nullptr, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    nullptr, XamlParserState::psStartTag },
    /* -> sntAttribute */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntDirective */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     nullptr, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, nullptr, XamlParserState::psStartTag },
    /* -> sntText */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntEndTag */           { nullptr,                                            nullptr, XamlParserState::psError }
    },

    {
    // From psPrefix ->
    /* -> sntNone */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         nullptr, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    nullptr, XamlParserState::psStartTag },
    /* -> sntAttribute */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntDirective */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     nullptr, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, nullptr, XamlParserState::psStartTag },
    /* -> sntText */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntEndTag */           { nullptr,                                            nullptr, XamlParserState::psError }
    },

    {
    // From psStartTag ->
    /* -> sntNone */             { nullptr,                                            nullptr,                                       XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntAttribute */        { &XamlPullParser::Static_Logic_Attribute,            nullptr,                                       XamlParserState::psAttribute },
    /* -> sntDirective */        { &XamlPullParser::Static_Logic_Directive,            nullptr,                                       XamlParserState::psDirective },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntText */             { &XamlPullParser::Static_Logic_TextContent,          &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psText },
    /* -> sntEndTag */           { &XamlPullParser::Static_Logic_EndElement,           &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psEndTag }
    },

    {
    // From psDirective ->
    /* -> sntNone */             { nullptr,                                            nullptr,                                       XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntAttribute */        { &XamlPullParser::Static_Logic_Attribute,            nullptr,                                       XamlParserState::psAttribute },
    /* -> sntDirective */        { &XamlPullParser::Static_Logic_Directive,            nullptr,                                       XamlParserState::psDirective },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntText */             { &XamlPullParser::Static_Logic_TextContent,          &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psText },
    /* -> sntEndTag */           { &XamlPullParser::Static_Logic_EndElement,           &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psEndTag }
    },

    {
    // From psAttribute ->
    /* -> sntNone */             { nullptr,                                            nullptr,                                       XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntAttribute */        { &XamlPullParser::Static_Logic_Attribute,            nullptr,                                       XamlParserState::psAttribute },
    /* -> sntDirective */        { nullptr,                                            nullptr,                                       XamlParserState::psError },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psStartTag },
    /* -> sntText */             { &XamlPullParser::Static_Logic_TextContent,          &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psText },
    /* -> sntEndTag */           { &XamlPullParser::Static_Logic_EndElement,           &XamlPullParser::Static_Logic_EndOfAttributes, XamlParserState::psEndTag }
    },

    {
    // From psText ->
    /* -> sntNone */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         nullptr, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    nullptr, XamlParserState::psStartTag },
    /* -> sntAttribute */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntDirective */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     nullptr, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, nullptr, XamlParserState::psStartTag },
    /* -> sntText */             { &XamlPullParser::Static_Logic_TextContent,          nullptr, XamlParserState::psText },
    /* -> sntEndTag */           { &XamlPullParser::Static_Logic_EndElement,           nullptr, XamlParserState::psEndTag }
    },

    {
    // From psEndTag ->
    /* -> sntNone */             { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntElement */          { &XamlPullParser::Static_Logic_StartElement,         nullptr, XamlParserState::psStartTag },
    /* -> sntEmptyElement */     { &XamlPullParser::Static_Logic_StartEmptyElement,    nullptr, XamlParserState::psStartTag },
    /* -> sntAttribute */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntDirective */        { nullptr,                                            nullptr, XamlParserState::psError },
    /* -> sntPrefixDefinition */ { &XamlPullParser::Static_Logic_PrefixDefinition,     nullptr, XamlParserState::psPrefix },
    /* -> sntPropertyElement */  { &XamlPullParser::Static_Logic_StartPropertyElement, nullptr, XamlParserState::psStartTag },
    /* -> sntText */             { &XamlPullParser::Static_Logic_TextContent,          nullptr, XamlParserState::psText },
    /* -> sntEndTag */           { &XamlPullParser::Static_Logic_EndElement,           nullptr, XamlParserState::psEndTag }
    }
};

XamlParserState::XamlParserState(void)
    : m_CurrentState(XamlParserState::psStart)
    , m_PreviousState(XamlParserState::psStart)
{
}

HRESULT XamlParserState::Transition(
    XamlScannerNode::ScannerNodeType    inputNodeType,
    fParseActionDelegate*               outpfnAction,
    fParseActionDelegate*               outpfnpostTransitionAction)
{
    XamlParserState::ParseState currentState = m_CurrentState;
    IFCEXPECT_RETURN(ConstParserStateTransitionTable[currentState][inputNodeType].m_ActionFunction != nullptr);
    IFCEXPECT_RETURN(ConstParserStateTransitionTable[currentState][inputNodeType].m_NextState != XamlParserState::psError);
    *outpfnAction = ConstParserStateTransitionTable[currentState][inputNodeType].m_ActionFunction;
    *outpfnpostTransitionAction = ConstParserStateTransitionTable[currentState][inputNodeType].m_PostTransitionActionFunction;
    m_CurrentState = ConstParserStateTransitionTable[currentState][inputNodeType].m_NextState;
    m_PreviousState = currentState;

    return S_OK;
}