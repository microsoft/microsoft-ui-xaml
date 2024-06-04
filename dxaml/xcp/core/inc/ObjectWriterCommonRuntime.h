// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xstring_ptr.h"
#include "LineInfo.h"
#include "ParserTypedefs.h"

struct XamlQualifiedObject;
class XamlProperty;
class DirectiveProperty;
class XamlNamespace;
class XamlType;
class XamlTextSyntax;
class IXamlSchemaObject;
class XamlServiceProviderContext;
class ObjectWriterContext;
class SubObjectWriterResult;
class CustomWriterRuntimeData;
class ObjectWriterErrorService;
class ObjectWriterNodeList;
class ObjectWriterCallbacksDelegate;
class INameScope;
class ObjectWriter;

namespace Parser
{
struct XamlPredicateAndArgs;
}

// Implementation of common ObjectWriter operations that can
// be shared between the ObjectWriterRuntimeEncoder and the
// ObjectWriterRuntime.
//
// * Stack Manipulation
// * Validation
//
// This class is not useful standalone, and hence the constructor
// is protected. Derive from this class and implement or override
// to provide a concrete implementation.
class ObjectWriterCommonRuntime
{
protected:
    ObjectWriterCommonRuntime(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
        _In_ const bool isValidatorEnabled,
        _In_ const bool isEncoding)
        : m_spContext(spContext)
        , m_spErrorService(spErrorService)
        , m_isValidatorEnabled(isValidatorEnabled)
        , m_isEncoding(isEncoding)
        , m_isRootObjectCreated(false)
    {}

public:
    _Check_return_ HRESULT AddNamespacePrefix(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strNamespacePrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace);

    _Check_return_ HRESULT PushConstant(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT SetDirectiveProperty(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT PushScope(
        _In_ const XamlLineInfo& lineInfo);

    _Check_return_ HRESULT PopScope(
        _In_ const XamlLineInfo& lineInfo);

    _Check_return_ HRESULT CreateType(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT GetValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT CreateTypeWithInitialValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT TypeConvertValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ bool fIsPropertyAssignment,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT SetValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT CheckPeerType(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT AddToCollection(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType);

    _Check_return_ HRESULT AddToDictionary(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey);

    _Check_return_ HRESULT SetConnectionId(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget);

    _Check_return_ HRESULT GetXBindConnector(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector);

    _Check_return_ HRESULT SetName(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const xref_ptr<INameScope>& spNameScope);

    _Check_return_ HRESULT GetResourcePropertyBag(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
        _Out_ SP_MapPropertyToQO& spPropertyBag);

    _Check_return_ HRESULT ProvideValue(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT BeginInit(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT EndInit(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT SetCustomRuntimeData(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ std::shared_ptr<CustomWriterRuntimeData> customRuntimeData,
        _In_ std::shared_ptr<SubObjectWriterResult> subObjectWriterResult,
        _In_ xref_ptr<INameScope> nameScope,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
        const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback);

    _Check_return_ HRESULT DeferResourceDictionaryItems(
        _In_ const XamlLineInfo& lineInfo,
        _In_ ObjectWriter* pObjectWriter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _In_ const bool fIsDictionaryWithKeyProperty,
        _Out_ bool& hasDeferred);

    _Check_return_ HRESULT DeferTemplateContent(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent);

    _Check_return_ HRESULT GetStreamOffsetToken(
        _Out_ UINT32 *pTokenIndex);

    _Check_return_ HRESULT BeginConditionalScope(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs);

    _Check_return_ HRESULT EndConditionalScope(
        _In_ const XamlLineInfo& lineInfo);

    virtual std::shared_ptr<ObjectWriterNodeList> GetNodeList();

protected:
    virtual _Check_return_ HRESULT AddNamespacePrefixImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strNamespacePrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace) = 0;

    virtual _Check_return_ HRESULT PushConstantImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) = 0;

    virtual _Check_return_ HRESULT SetDirectivePropertyImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT PushScopeImpl(
        _In_ const XamlLineInfo& lineInfo) = 0;

    virtual _Check_return_ HRESULT PopScopeImpl(
        _In_ const XamlLineInfo& lineInfo) = 0;

    virtual _Check_return_ HRESULT CreateTypeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT GetValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue) = 0;

    virtual _Check_return_ HRESULT CreateTypeWithInitialValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT TypeConvertValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ bool fIsPropertyAssignment,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT SetValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) = 0;

    virtual _Check_return_ HRESULT CheckPeerTypeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) = 0;

    virtual _Check_return_ HRESULT AddToCollectionImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType) = 0;

    virtual _Check_return_ HRESULT AddToDictionaryImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey) = 0;

    virtual _Check_return_ HRESULT SetConnectionIdImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget) = 0;

    virtual _Check_return_ HRESULT GetXBindConnectorImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector) = 0;

    virtual _Check_return_ HRESULT SetNameImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const xref_ptr<INameScope>& spNameScope) = 0;

    virtual _Check_return_ HRESULT GetResourcePropertyBagImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
        _Out_ SP_MapPropertyToQO& spPropertyBag) = 0;

    virtual _Check_return_ HRESULT ProvideValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue) = 0;

    virtual _Check_return_ HRESULT BeginInitImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT EndInitImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    virtual _Check_return_ HRESULT SetCustomRuntimeDataImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ std::shared_ptr<CustomWriterRuntimeData> customRuntimeData,
        _In_ std::shared_ptr<SubObjectWriterResult> subObjectWriterResult,
        _In_ xref_ptr<INameScope> nameScope,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
        const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback) = 0;

    virtual _Check_return_ HRESULT DeferResourceDictionaryItemsImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ ObjectWriter* pObjectWriter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _In_ const bool fIsDictionaryWithKeyProperty,
        _Out_ bool& hasDeferred) = 0;

    virtual _Check_return_ HRESULT DeferTemplateContentImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent) = 0;

    virtual _Check_return_ HRESULT GetStreamOffsetTokenImpl(
        _Out_ UINT32 *pTokenIndex) = 0;

    virtual _Check_return_ HRESULT BeginConditionalScopeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) = 0;

    virtual _Check_return_ HRESULT EndConditionalScopeImpl(
        _In_ const XamlLineInfo& lineInfo) = 0;

    const bool IsValidatorEnabled() const;

protected:
    std::shared_ptr<ObjectWriterContext> m_spContext;
    std::shared_ptr<ObjectWriterErrorService> m_spErrorService;
    bool m_isValidatorEnabled;
    bool m_isEncoding;
    bool m_isRootObjectCreated;
};

