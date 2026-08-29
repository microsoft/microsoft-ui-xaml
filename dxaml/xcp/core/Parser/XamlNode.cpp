// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlNode.h"
#include "XamlPredicateHelpers.h"

void XamlNode::InitAddNamespaceNode(
    _In_ const xstring_ptr& ssPrefix,
    _In_ const std::shared_ptr<XamlNamespace>& xamlNamespace)
{
    m_nodeType = xntNamespace;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
    m_prefix = ssPrefix;
    m_spShared = xamlNamespace;
}

void XamlNode::InitStartObjectNode(
    _In_ const std::shared_ptr<XamlType>& inXamlType,
    _In_ bool bIsRetrieved,
    _In_ bool bIsUnknown)
{
    m_nodeType = xntStartObject;
    m_bIsRetrieved = bIsRetrieved;
    m_bIsUnknown = bIsUnknown;
    m_spShared = inXamlType;
}

void XamlNode::InitEndObjectNode()
{
    m_nodeType = xntEndObject;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
}

void XamlNode::InitStartMemberNode(_In_ const std::shared_ptr<XamlProperty>& inXamlProperty)
{
    m_nodeType = xntStartProperty;
    m_bIsRetrieved = false;
    m_bIsUnknown = inXamlProperty->IsUnknown();
    m_spShared = inXamlProperty;
}

void XamlNode::InitEndMemberNode()
{
    m_nodeType = xntEndProperty;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
}

void XamlNode::InitEndOfAttributesNode()
{
    m_nodeType = xntEndOfAttributes;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
}

void XamlNode::InitEndOfStreamNode()
{
    m_nodeType = xntEndOfStream;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
}

void XamlNode::InitTextValueNode(_In_ const std::shared_ptr<XamlText>& inXamlText)
{
    m_nodeType = xntText;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
    m_spShared = inXamlText;
}

void XamlNode::InitValueNode(_In_ const std::shared_ptr<XamlQualifiedObject>& inQO)
{
    m_nodeType = xntValue;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
    m_spValue = inQO;
}

void XamlNode::InitStartConditionalScopeNode(_In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    ASSERT(!xamlPredicateAndArgs->IsEmpty());
    m_nodeType = XamlNodeType::xntStartConditionalScope;
    m_xamlPredicateAndArgs = xamlPredicateAndArgs;
}

void XamlNode::InitEndConditionalScopeNode()
{
    m_nodeType = XamlNodeType::xntEndConditionalScope;
    m_bIsRetrieved = false;
    m_bIsUnknown = false;
}


const XamlLineInfo& XamlNode::get_LineInfo() const
{
    ASSERT(m_nodeType != xntNone);
    return m_lineInfo;
}

void XamlNode::set_LineInfo(const XamlLineInfo& lineInfo)
{
    m_lineInfo = lineInfo;
}

const XamlNodeType& XamlNode::get_NodeType() const
{
    return m_nodeType;
}

const xstring_ptr& XamlNode::get_Prefix() const
{
    ASSERT(m_nodeType == XamlNodeType::xntNamespace);
    return m_prefix;
}

const std::shared_ptr<XamlQualifiedObject>& XamlNode::get_Value() const
{
    ASSERT(m_nodeType == XamlNodeType::xntValue);
    return m_spValue;
}

bool XamlNode::get_IsRetrievedObject() const
{
    ASSERT(m_nodeType == XamlNodeType::xntStartObject);
    return m_bIsRetrieved;
}

bool XamlNode::IsUnknown() const
{
    // ZPTD: Which assert?
    return m_bIsUnknown;
}
