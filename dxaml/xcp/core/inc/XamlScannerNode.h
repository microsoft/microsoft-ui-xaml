// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define GET_VOID_SP(TYPE) *(static_cast< const std::shared_ptr<TYPE>* >(static_cast<const void*>(&m_spTypeOrProperty)))

#include "LineInfo.h"
#include <XamlProperty.h>
#include <XamlType.h>

class XamlNamespace;
class XamlProperty;
class XamlText;
class IXamlSchemaObject;
class XamlType;

// An individual node read by the scanner.  These nodes are not exposed
// outside the scanner, but the scanner itself has methods that expose all
// of the currently read node's members.
class XamlScannerNode 
{
public:
    enum ScannerNodeType
    {
        sntNone,
        sntElement,
        sntEmptyElement,
        sntAttribute,
        sntDirective,
        sntPrefixDefinition,
        sntPropertyElement,
        sntText,
        sntEndTag,
        sntNumberOfScannerNodeTypes
    };
    
    XamlScannerNode()
        : m_NodeType(XamlScannerNode::sntNone)
        , m_bIsEmptyTag(FALSE)
    {
    }
    
    XamlScannerNode(const XamlLineInfo& lineInfo)
        : m_NodeType(XamlScannerNode::sntNone)
        , m_LineInfo(lineInfo)
        , m_bIsEmptyTag(FALSE)
    {
    }

    ~XamlScannerNode()
    {
    }

    void InitEndTag(const XamlLineInfo& inLineInfo, bool bIsFromEmpty = false)
    {
        m_NodeType = XamlScannerNode::sntEndTag;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = bIsFromEmpty;
    }

    void InitPropertyElement(
                const XamlLineInfo& inLineInfo, 
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const std::shared_ptr<XamlProperty>& inProperty,
                bool bIsFromEmpty)
    {
        m_NodeType = XamlScannerNode::sntPropertyElement;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = bIsFromEmpty;
        m_ssPrefix = inPrefix;
        m_spTypeOrProperty = std::static_pointer_cast<IXamlSchemaObject>(inProperty);
        m_spTypeNamespace = inNamespace;
    }

    void InitText(
                const XamlLineInfo& inLineInfo, 
                const std::shared_ptr<XamlText>& inText)
    {
        m_NodeType = XamlScannerNode::sntText;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = FALSE;
        m_spText = inText;
    }

    void InitPrefixDefinition(
                const XamlLineInfo& inLineInfo,
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const std::shared_ptr<XamlText>& inAttributeText)
    {
        m_NodeType = XamlScannerNode::sntPrefixDefinition;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = FALSE;
        m_ssPrefix = inPrefix;
        m_spTypeNamespace = inNamespace;
        m_spText = inAttributeText;
    }

    void InitDirective(
                const XamlLineInfo& inLineInfo,
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlProperty>& inPropertyAttribute,
                const std::shared_ptr<XamlText>& inAttributeText)
    {
        m_NodeType = XamlScannerNode::sntDirective;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = FALSE;
        m_ssPrefix = inPrefix;      
        m_spTypeOrProperty = inPropertyAttribute;
        m_spText = inAttributeText;
    }

    void InitAttribute(
                const XamlLineInfo& inLineInfo,
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlProperty>& inPropertyAttribute,
                const std::shared_ptr<XamlText>& inAttributeText)
    {
        m_NodeType = XamlScannerNode::sntAttribute;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = FALSE;
        m_ssPrefix = inPrefix;      
        m_spTypeOrProperty = inPropertyAttribute;
        m_spText = inAttributeText;
    }

    void InitElement(
                const XamlLineInfo& inLineInfo,
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const std::shared_ptr<XamlType>& inType)
    {
        m_NodeType = XamlScannerNode::sntElement;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = FALSE;
        m_ssPrefix = inPrefix;   
        m_spTypeNamespace = inNamespace;
        m_spTypeOrProperty = inType;
    }

    void InitEmptyElement(
                const XamlLineInfo& inLineInfo,
                _In_ const xstring_ptr& inPrefix,
                const std::shared_ptr<XamlNamespace>& inNamespace,
                const std::shared_ptr<XamlType>& inType)
    {
        m_NodeType = XamlScannerNode::sntEmptyElement;
        m_LineInfo = inLineInfo;
        m_bIsEmptyTag = TRUE;
        m_ssPrefix = inPrefix;   
        m_spTypeNamespace = inNamespace;
        m_spTypeOrProperty = inType;
    }



    const ScannerNodeType& get_NodeType() const 
    { 
        return m_NodeType; 
    }                
    
    const std::shared_ptr<XamlType>& get_Type() const 
    { 
        ASSERT(     
                    (m_NodeType == XamlScannerNode::sntEmptyElement) 
                || (m_NodeType == XamlScannerNode::sntElement));
        return GET_VOID_SP(XamlType); 
    }

    const std::shared_ptr<XamlNamespace>& get_TypeNamespace() const 
    { 
        ASSERT(
                    (m_NodeType == XamlScannerNode::sntPropertyElement)
                ||  (m_NodeType == XamlScannerNode::sntPrefixDefinition)
                ||  (m_NodeType == XamlScannerNode::sntElement)
                ||  (m_NodeType == XamlScannerNode::sntEmptyElement));

        return m_spTypeNamespace; 
    }

    const std::shared_ptr<XamlProperty>& get_PropertyAttribute() const 
    { 
        ASSERT(
                    (m_NodeType == XamlScannerNode::sntDirective)
                ||  (m_NodeType == XamlScannerNode::sntPrefixDefinition)
                ||  (m_NodeType == XamlScannerNode::sntAttribute));
        return GET_VOID_SP(XamlProperty);
    }

    const std::shared_ptr<XamlText>& get_PropertyAttributeText() const 
    { 
        ASSERT(
                    (m_NodeType == XamlScannerNode::sntDirective)
                ||  (m_NodeType == XamlScannerNode::sntPrefixDefinition)
                ||  (m_NodeType == XamlScannerNode::sntAttribute));
        return m_spText; 
    }

    const std::shared_ptr<XamlProperty>& get_PropertyElement() const 
    { 
        // TODO: Collapse with get_PropertyAttribute
        ASSERT(m_NodeType == XamlScannerNode::sntPropertyElement);
        return GET_VOID_SP(XamlProperty);
    }

    const bool& get_IsEmptyTag() const 
    { 
        return m_bIsEmptyTag; 
    }

    const std::shared_ptr<XamlText>& get_TextContent() const 
    { 
        // TODO: Collapse with property attrib text
        ASSERT(m_NodeType == XamlScannerNode::sntText);
        return m_spText; 
    }

    const xstring_ptr& get_Prefix() const 
    { 
        return m_ssPrefix; 
    }

    const XamlLineInfo& get_LineInfo() const 
    { 
        return m_LineInfo; 
    }

private:
    std::shared_ptr<IXamlSchemaObject> m_spTypeOrProperty;
    std::shared_ptr<XamlText> m_spText;
    xstring_ptr m_ssPrefix;
    std::shared_ptr<XamlNamespace> m_spTypeNamespace;
    ScannerNodeType m_NodeType;
    bool m_bIsEmptyTag;
    XamlLineInfo m_LineInfo;
};

#undef GET_VOID_SP

