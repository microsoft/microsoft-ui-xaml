// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "MePullParser.h"
#include "XamlParserState.h"
#include "LineInfo.h"
#include "XamlNodeStreamValidator.h"

class XamlParserContext;
class XamlNode;
class XamlNamespace;

// Reads the XAML to parse from the XamlScanner one token at a time and
// parses it into a stream of XamlNodes that can be serialized by a
// XamlWriter (to an object graph, etc.).
class XamlPullParser : private IPullParserNodeCallback
{
public:
    friend class XamlParserState;
    // TODO: removed textsettings paramter
    XamlPullParser(_In_ const std::shared_ptr<XamlParserContext>& inContext, _In_ const std::shared_ptr<XamlScanner>& inScanner);

    _Check_return_ HRESULT Parse(_Out_opt_ XamlNode& outNode);

    // TODO: Can these be private?
    XamlParserState::ParseState CurrentState();
    XamlParserState::ParseState PreviousState();

private:

    class PrefixNamespacePair
    {
    public:
        PrefixNamespacePair(_In_ const xstring_ptr& inPrefix, std::shared_ptr<XamlNamespace> inNamespace, const XamlLineInfo& inLineInfo)
            : m_Prefix(inPrefix)
            , m_Namespace(inNamespace)
            , m_LineInfo(inLineInfo)
        {

        }
        xstring_ptr m_Prefix;
        std::shared_ptr<XamlNamespace> m_Namespace;
        XamlLineInfo m_LineInfo;

    };
    _Check_return_ HRESULT AcceptNode(const XamlNode& node) final;
    void SetLineInfoOnWorkingNode();
    void SetLineInfoOnWorkingNode(const XamlLineInfo& inLineInfo);
    _Check_return_ HRESULT EnqueueWorkingNode();
    _Check_return_ HRESULT EnqueueWorkingNode(const XamlLineInfo& inLineInfo);
    _Check_return_ HRESULT InternalEnqueueNode(const XamlNode& node);

    _Check_return_ HRESULT Logic_PrefixDefinition();
    _Check_return_ HRESULT Logic_StartElement();
    _Check_return_ HRESULT Logic_StartEmptyElement();
    _Check_return_ HRESULT Logic_StartPropertyElement();
    _Check_return_ HRESULT Logic_EndObject();

    _Check_return_ HRESULT Logic_EndMember();
    _Check_return_ HRESULT Logic_EndElement();
    _Check_return_ HRESULT Logic_Attribute();
    _Check_return_ HRESULT Logic_Directive();
    _Check_return_ HRESULT Logic_TextContent();
    _Check_return_ HRESULT Logic_EndOfAttributes();
    _Check_return_ HRESULT Logic_EndOfStream();

public:
    static _Check_return_ HRESULT Static_Logic_PrefixDefinition(_In_ XamlPullParser* xpp) { return xpp->Logic_PrefixDefinition(); }
    static _Check_return_ HRESULT Static_Logic_StartElement(_In_ XamlPullParser* xpp) { return xpp->Logic_StartElement(); }
    static _Check_return_ HRESULT Static_Logic_StartEmptyElement(_In_ XamlPullParser* xpp) { return xpp->Logic_StartEmptyElement(); }
    static _Check_return_ HRESULT Static_Logic_StartPropertyElement(_In_ XamlPullParser* xpp) { return xpp->Logic_StartPropertyElement(); }
    static _Check_return_ HRESULT Static_Logic_EndObject(_In_ XamlPullParser* xpp) { return xpp->Logic_EndObject(); }

    static _Check_return_ HRESULT Static_Logic_EndMember(_In_ XamlPullParser* xpp) { return xpp->Logic_EndMember(); }
    static _Check_return_ HRESULT Static_Logic_EndElement(_In_ XamlPullParser* xpp) { return xpp->Logic_EndElement(); }
    static _Check_return_ HRESULT Static_Logic_Attribute(_In_ XamlPullParser* xpp) { return xpp->Logic_Attribute(); }
    static _Check_return_ HRESULT Static_Logic_Directive(_In_ XamlPullParser* xpp) { return xpp->Logic_Directive(); }
    static _Check_return_ HRESULT Static_Logic_TextContent(_In_ XamlPullParser* xpp) { return xpp->Logic_TextContent(); }
    static _Check_return_ HRESULT Static_Logic_EndOfAttributes(_In_ XamlPullParser* xpp) { return xpp->Logic_EndOfAttributes(); }
    static _Check_return_ HRESULT Static_Logic_EndOfStream(_In_ XamlPullParser* xpp) { return xpp->Logic_EndOfStream(); }


private:
    _Check_return_ HRESULT Logic_AttributeOrDirective(bool bIsDirective);
    _Check_return_ HRESULT Logic_StartInitProperty(const std::shared_ptr<XamlType>& inType);
    _Check_return_ HRESULT Logic_StartItemsProperty(const std::shared_ptr<XamlType>& inCollectionType);
    _Check_return_ HRESULT Logic_AttributeValueNode(const std::shared_ptr<XamlText>& inXamlText);
    _Check_return_ HRESULT Logic_TextValueNode(const std::shared_ptr<XamlText>& inXamlText);
    _Check_return_ HRESULT Logic_StartObject(_In_ const std::shared_ptr<XamlType>& inXamlType, _In_ const xstring_ptr& xmlnsPrefix, _In_ const XamlParserFrame::ElementKind& inElementKind);
    _Check_return_ HRESULT Logic_StartMember(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty, _In_ const xstring_ptr& xmlnsPrefix);
    _Check_return_ HRESULT Logic_StartContentProperty(const std::shared_ptr<XamlProperty>& inContentProperty);
    _Check_return_ HRESULT Logic_StartGetObjectFromMember(const std::shared_ptr<XamlType>& inPropertyType);
    _Check_return_ HRESULT Logic_CheckForStartGetCollectionFromMember();
    _Check_return_ HRESULT Logic_EndCollectionFromMember();
    _Check_return_ HRESULT Logic_EndItemsProperty();
    _Check_return_ HRESULT Logic_EndContentProperty();

    // Perform any final leading or trailing whitespace trimming of literal text
    // depending on the elements around it.
    _Check_return_ HRESULT Logic_ApplyFinalTextTrimming(
        _In_ const std::shared_ptr<XamlText>& spText,
        _Out_ xstring_ptr* pstrTrimmed);

    // Gets a value indicating whether whitespace is discardable at this phase
    // in the parsing.  Here we discard whitespace between property elements but
    // keep it between object elements for collections that accept it.
    // Discarding trailing whitespace in collections cannot be decided here.
    // See Logic_ApplyFinalTextTrimming.
    _Check_return_ HRESULT Logic_IsDiscardableWhitespace(
        _In_ const std::shared_ptr<XamlText>& spText,
        _Out_ bool& bIsDiscardable);

    // Gets a value indicating whether the specified XamlProperty has the means
    // to accept a literal string as its content.
    _Check_return_ HRESULT CanAcceptString(_In_ const std::shared_ptr<XamlProperty>& spXamlProperty, _Out_ bool& bCanAcceptString);

    _Check_return_ HRESULT EmitImplicitPreambleNodesIfRequired(_In_ const std::shared_ptr<XamlType>& inXamlType);
    _Check_return_ HRESULT EmitPendingNamespaceNodes();
    _Check_return_ HRESULT IsInElementContent(bool& bIsInElementContent)
    {
        // This function assumes you are checking before you
        // push your current node.
        //
        // What it is telling you is whether the node on the top of
        // the stack is an open element tag.

        // ZPTD: No _Check_return_ HRESULT on this function
        if (m_ParserContext->IsStackEmpty())
        {
            bIsInElementContent = FALSE;
        }
        else
        {
            // It's element content if the parent isn't a property and it's
            // an objectelement.
            bIsInElementContent = (m_ParserContext->get_CurrentKind() == XamlParserFrame::ekElement);
        }
        RRETURN(S_OK);
    }

    _Check_return_ HRESULT PopImplicitNodesIfRequired();
private:
    std::shared_ptr<XamlParserContext> m_ParserContext;
    std::shared_ptr<XamlScanner> m_Scanner;
    xqueue<XamlNode> m_NodeQueue;

    // TODO: Similar thing may be duplicated.
    xvector<PrefixNamespacePair> m_PendingNamespaces;
    XamlNodeStreamValidator m_Validator;
    XamlParserState m_ParserState;
    XamlNode m_WorkingNode;
};


