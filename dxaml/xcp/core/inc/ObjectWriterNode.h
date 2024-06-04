// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ObjectWriterNodeType.h"
#include "LineInfo.h"
#include "SubObjectWriterResult.h"

struct XamlQualifiedObject;
class CustomWriterRuntimeData;
class XamlProperty;
class XamlNamespace;
class XamlType;
class XamlTextSyntax;
class IXamlSchemaObject;
class ObjectWriterNodeList;
class XamlBinaryFormatSubReader2;
class DirectiveProperty;
class CValue;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// Nodes representing the commands from the ObjectWriter.
// Each node information is self contained is a description
// of the operation and schema/value parameters that is used
// to reconstruct the operation.
//
// This is a composite object as a perf optimization so that 
// we can prevent a node type hierarchy which would need
// to be allocated on the heap to allow virtual dispatch.
class ObjectWriterNode
{
public:
    ObjectWriterNode() WI_NOEXCEPT;
    ~ObjectWriterNode() WI_NOEXCEPT;

    #pragma region Stack Manipulation
    static ObjectWriterNode MakePushScopeNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakePopScopeNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;
    #pragma endregion

    #pragma region Tree Creation
    static ObjectWriterNode MakeAddNamespaceNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strNamespacePrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeWithInitialValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueTypeConvertedConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueTypeConvertedResolvedTypeNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlType>& spTypeProxy) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueTypeConvertedResolvedPropertyNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT;

    static ObjectWriterNode MakeGetValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT;

    static ObjectWriterNode MakeBeginInitNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeTypeConvertValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter) WI_NOEXCEPT;

    static ObjectWriterNode MakePushConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakePushResolvedTypeNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakePushResolvedPropertyNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT;

    static ObjectWriterNode MakeEndInitNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeEndInitPopScopeNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeEndInitProvideValuePopScopeNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeAddToCollectionNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeAddToDictionaryNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeAddToDictionaryWithKeyNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT;
    #pragma endregion

    #pragma region Type Specific Operations
    static ObjectWriterNode MakeSetConnectionIdNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetNameNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spNameValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeProvideValueNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    static ObjectWriterNode MakeProvideStaticResourceValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeProvideThemeResourceValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeProvideTemplateBindingValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueFromStaticResourceNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueFromThemeResourceNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueFromTemplateBindingNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlProperty>& spPropertyProxy) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetValueFromMarkupExtensionNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetDirectivePropertyNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<DirectiveProperty>& spProperty) WI_NOEXCEPT;

    static ObjectWriterNode MakeGetResourcePropertyBagNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKeyValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeCheckPeerTypeNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strClassName) WI_NOEXCEPT;
    #pragma endregion

    #pragma region Custom Data Blob
    static ObjectWriterNode MakeSetCustomRuntimeData(
        _In_ const XamlLineInfo& lineInfo,
        _In_ std::shared_ptr<CustomWriterRuntimeData> customWriterData,
        _In_ std::shared_ptr<SubObjectWriterResult> customWriterNodeStream) WI_NOEXCEPT;
    
    static ObjectWriterNode MakeSetResourceDictionaryItems(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;
    
    static ObjectWriterNode MakeSetDeferredPropertyNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<ObjectWriterNodeList>& spNodeList,
        _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetDeferredPropertyNodeWithValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) WI_NOEXCEPT;

    static ObjectWriterNode MakeSetDeferredPropertyNodeWithReader(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlBinaryFormatSubReader2>& spReader,
        _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT;

    static ObjectWriterNode MakeStreamOffsetMarker(
        _In_ const UINT32 tokenIndex) WI_NOEXCEPT;

    #pragma endregion

    #pragma region Conditional Xaml

    static ObjectWriterNode MakeBeginConditionalScope(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) WI_NOEXCEPT;

    static ObjectWriterNode MakeEndConditionalScope(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;

    #pragma endregion

    #pragma region Aggregate Operations
    static ObjectWriterNode MakePushScopeAddNamespaceNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strNamespacePrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeGetValueNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlProperty>& spProperty) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeWithConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeWithConstantBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeWithTypeConvertedConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakePushScopeCreateTypeWithTypeConvertedConstantBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeWithConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeWithConstantBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeWithTypeConvertedConstantNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

    static ObjectWriterNode MakeCreateTypeWithTypeConvertedConstantBeginInitNode(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntax,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT;

#pragma endregion

    #pragma region Misc
    static ObjectWriterNode MakeEndOfStreamNode(
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT;
    #pragma endregion

    #pragma region Getters
    const ObjectWriterNodeType GetNodeType() const WI_NOEXCEPT;
    
    const std::shared_ptr<XamlType> GetXamlType() const WI_NOEXCEPT;

    const std::shared_ptr<XamlType> GetXamlTypeProxy() const WI_NOEXCEPT;

    const std::shared_ptr<XamlProperty> GetXamlProperty() const WI_NOEXCEPT;

    const std::shared_ptr<XamlProperty> GetXamlPropertyProxy() const WI_NOEXCEPT;

    const std::shared_ptr<XamlNamespace> GetNamespace() const WI_NOEXCEPT;

    const std::shared_ptr<XamlQualifiedObject>& GetValue() const WI_NOEXCEPT;

    const xstring_ptr& GetStringValue() const WI_NOEXCEPT;

    const std::shared_ptr<XamlTextSyntax>& GetTypeConverter() const WI_NOEXCEPT;
    
    const std::shared_ptr<ObjectWriterNodeList>& GetNodeList() const WI_NOEXCEPT;

    const std::shared_ptr<XamlBinaryFormatSubReader2>& GetSubReader() const WI_NOEXCEPT;

    const std::vector<std::pair<bool, xstring_ptr>>& GetListOfReferences() const WI_NOEXCEPT;

    const std::shared_ptr<CustomWriterRuntimeData>& GetCustomWriterData() const WI_NOEXCEPT;

    const std::shared_ptr<SubObjectWriterResult>& GetCustomWriterNodeStream() const WI_NOEXCEPT;

    const std::shared_ptr<Parser::XamlPredicateAndArgs> GetXamlPredicateAndArgs() const WI_NOEXCEPT;

    const XamlLineInfo& GetLineInfo() const WI_NOEXCEPT;
    #pragma endregion

    #pragma region Helper Functions
    const bool RequiresNewScope() const WI_NOEXCEPT;

    const bool RequiresScopeToEnd() const WI_NOEXCEPT;
    
    const bool ProvidesCustomBinaryData() const WI_NOEXCEPT;
    #pragma endregion

    #pragma region Debugging Helpers

    _Check_return_ HRESULT GetLineInfoAsString(_Out_ xstring_ptr& strValue) const;

    _Check_return_ HRESULT ToString(_Out_ xstring_ptr& strValue) const;

    xstring_ptr ToStringCValueHelper(_In_ const CValue& inValue) const;

    #pragma endregion

protected:
    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spTextSyntaxConverter(spConverter)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spValue(spValue)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spValue(spValue)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared2) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spShared2(spShared2)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spValue(spValue)
        , m_spTextSyntaxConverter(spConverter)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared2,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spShared2(spShared2)
        , m_spTextSyntaxConverter(spConverter)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const UINT32 intValue) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_intValue(intValue)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strValue,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_strValue(strValue)
        , m_spShared(spShared)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strValue) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_strValue(strValue)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<CustomWriterRuntimeData>& customWriterData,
        _In_ const std::shared_ptr<SubObjectWriterResult>& customWriterNodeStream) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_CustomWriterData(customWriterData)
        , m_CustomWriterNodeStream(customWriterNodeStream)
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<ObjectWriterNodeList>& spNodeList,
        _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spNodeList(spNodeList)
        , m_vecResourceList(std::move(vecResourceList))
    {
    }

    ObjectWriterNode(
        _In_ const ObjectWriterNodeType nodeType,
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<IXamlSchemaObject>& spShared,
        _In_ const std::shared_ptr<XamlBinaryFormatSubReader2>& spReader,
        _In_ std::vector<std::pair<bool, xstring_ptr>>&& vecResourceList) WI_NOEXCEPT
        : m_NodeType(nodeType)
        , m_LineInfo(lineInfo)
        , m_spShared(spShared)
        , m_spSubReader(spReader)
        , m_vecResourceList(std::move(vecResourceList))
    {
    }

private:
    ObjectWriterNodeType m_NodeType;
    XamlLineInfo m_LineInfo;

    std::shared_ptr<CustomWriterRuntimeData> m_CustomWriterData;
    std::shared_ptr<SubObjectWriterResult> m_CustomWriterNodeStream;
    // TODO: Merge with SubObjectWriterResult
    std::shared_ptr<ObjectWriterNodeList> m_spNodeList;

    std::shared_ptr<IXamlSchemaObject> m_spShared;
    std::shared_ptr<IXamlSchemaObject> m_spShared2;
    std::shared_ptr<XamlQualifiedObject> m_spValue;
    std::shared_ptr<XamlTextSyntax> m_spTextSyntaxConverter;
    std::shared_ptr<XamlBinaryFormatSubReader2> m_spSubReader;
    xstring_ptr m_strValue;
    UINT32 m_intValue;
    std::vector<std::pair<bool,xstring_ptr>> m_vecResourceList;
};

