// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "LineInfo.h"
#include "XamlNodeType.h"

#include "IXamlSchemaObject.h"
#include "XamlNamespace.h"
#include "XamlType.h"
#include "XamlText.h"

// This is really evil.
#define GET_VOID_SP(TYPE) *(static_cast< const std::shared_ptr<TYPE>* >(static_cast<const void*>(&m_spShared)))

class XamlProperty;
struct XamlQualifiedObject;

namespace Parser 
{
struct XamlPredicateAndArgs;
}

class XamlNode 
{
public:
    XamlNode()
        : m_nodeType(xntNone)
        , m_bIsRetrieved(false)
        , m_bIsUnknown(false)
    {
    }

    void InitAddNamespaceNode(
        _In_ const xstring_ptr& ssPrefix,
        _In_ const std::shared_ptr<XamlNamespace>& xamlNamespace);

    void InitStartObjectNode(
        _In_ const std::shared_ptr<XamlType>& inXamlType,
        _In_ bool bIsRetrieved,
        _In_ bool bIsUnknown);

    void InitEndObjectNode();

    void InitStartMemberNode(
        _In_ const std::shared_ptr<XamlProperty>& inXamlProperty);

    void InitEndMemberNode();

    void InitEndOfAttributesNode();

    void InitEndOfStreamNode();

    void InitTextValueNode(_In_ const std::shared_ptr<XamlText>& inXamlText);

    void InitValueNode(_In_ const std::shared_ptr<XamlQualifiedObject>& inQO);
    
    void InitStartConditionalScopeNode(_In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs);
    
    void InitEndConditionalScopeNode();

    const XamlLineInfo& get_LineInfo() const;
    void set_LineInfo(const XamlLineInfo& lineInfo);
    const XamlNodeType& get_NodeType() const;
    const xstring_ptr& get_Prefix() const;

    // Returns the predicate and arguments associated with this node.
    const auto& get_XamlPredicateAndArgs() const 
    { 
        ASSERT(m_nodeType == XamlNodeType::xntStartConditionalScope);

        return m_xamlPredicateAndArgs; 
    }

    const std::shared_ptr<XamlNamespace>& get_Namespace() const
    {
        ASSERT(m_nodeType == XamlNodeType::xntNamespace);
        return GET_VOID_SP(XamlNamespace);
    }

    const std::shared_ptr<XamlType>& get_XamlType() const
    {
        ASSERT(m_nodeType == XamlNodeType::xntStartObject);
        ASSERT(!!m_spShared);
        return GET_VOID_SP(XamlType);
    }

    const std::shared_ptr<XamlProperty>& get_Property() const
    {
        ASSERT(m_nodeType == XamlNodeType::xntStartProperty);
        ASSERT(!!m_spShared);
        return GET_VOID_SP(XamlProperty);
    }

    const std::shared_ptr<XamlText>& get_Text() const
    {
        ASSERT(m_nodeType == XamlNodeType::xntText);
        return GET_VOID_SP(XamlText);
    }

    const std::shared_ptr<XamlQualifiedObject>& get_Value() const;

    bool get_IsRetrievedObject() const;

    bool IsUnknown() const;

private:
    std::shared_ptr<IXamlSchemaObject> m_spShared;
    std::shared_ptr<XamlQualifiedObject> m_spValue;
    std::shared_ptr<Parser::XamlPredicateAndArgs> m_xamlPredicateAndArgs;
    XamlNodeType m_nodeType;
    bool m_bIsRetrieved : 1;
    bool m_bIsUnknown : 1;
    XamlLineInfo m_lineInfo;
    xstring_ptr m_prefix;   
};

#undef GET_VOID_SP

