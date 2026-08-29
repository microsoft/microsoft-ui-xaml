// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xstring_ptr.h"
#include "ObjectWriterCommonRuntime.h"
#include "LineInfo.h"

struct XamlQualifiedObject;
class XamlProperty;
class DirectiveProperty;
class XamlNamespace;
class XamlType;
class XamlTextSyntax;
class IXamlSchemaObject;
class XamlServiceProviderContext;
class ObjectWriterContext;
enum class ResourceReferenceType;

class SubObjectWriterResult;
class CustomWriterRuntimeData;

// Implementation of Object Graph realization by executing runtime operations against the Xaml OM.
class ObjectWriterRuntime : public ObjectWriterCommonRuntime
{
public:
    ObjectWriterRuntime(
        _In_ const std::shared_ptr<ObjectWriterContext>& spContext,
        _In_ const std::shared_ptr<ObjectWriterErrorService>& spErrorService,
        _In_ const bool isValidatorEnabled);

    _Check_return_ HRESULT AddNamespacePrefixImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const xstring_ptr& strNamespacePrefix,
        _In_ const std::shared_ptr<XamlNamespace>& spNamespace) override;

    _Check_return_ HRESULT PushConstantImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) override;

    _Check_return_ HRESULT SetDirectivePropertyImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT PushScopeImpl(
        _In_ const XamlLineInfo& lineInfo) override;

    _Check_return_ HRESULT PopScopeImpl(
        _In_ const XamlLineInfo& lineInfo) override;

    _Check_return_ HRESULT CreateTypeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT GetValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue) override;

    _Check_return_ HRESULT CreateTypeWithInitialValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
        _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT TypeConvertValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ bool fIsPropertyAssignment,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT SetValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) override;

    _Check_return_ HRESULT CheckPeerTypeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue) override;

    _Check_return_ HRESULT AddToCollectionImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType) override;

    _Check_return_ HRESULT AddToDictionaryImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey) override;

    _Check_return_ HRESULT SetConnectionIdImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget) override;

    _Check_return_ HRESULT GetXBindConnectorImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector) override;

    _Check_return_ HRESULT SetNameImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const xref_ptr<INameScope>& spNamescope) override;

    _Check_return_ HRESULT GetResourcePropertyBagImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
        _Out_ SP_MapPropertyToQO& spPropertyBag) override;

    _Check_return_ HRESULT ProvideValueImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue) override;

    _Check_return_ HRESULT BeginInitImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT EndInitImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT SetCustomRuntimeDataImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ std::shared_ptr<CustomWriterRuntimeData> customRuntimeData,
        _In_ std::shared_ptr<SubObjectWriterResult> subObjectWriterResult,
        _In_ xref_ptr<INameScope> nameScope,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
        const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback) override;

    _Check_return_ HRESULT DeferResourceDictionaryItemsImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ ObjectWriter* pObjectWriter,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _In_ const bool fIsDictionaryWithKeyProperty,
        _Out_ bool& hasDeferred) override;

    _Check_return_ HRESULT DeferTemplateContentImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent) override;

    _Check_return_ HRESULT GetStreamOffsetTokenImpl(
        _Out_ UINT32 *pTokenIndex) override;

    _Check_return_ HRESULT BeginConditionalScopeImpl(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs) override;

    _Check_return_ HRESULT EndConditionalScopeImpl(
        _In_ const XamlLineInfo& lineInfo) override;

private:
    #pragma region Helpers
    _Check_return_ HRESULT AddToCollectionImplHelper(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT AddToDictionaryImplHelper(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
        _In_ const std::shared_ptr<XamlType>& spValueType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey);

    _Check_return_ HRESULT GetResourcePropertyBagImplHelper(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
        _In_ const xref_ptr<IPALUri> spBaseUri,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _Out_ SP_MapPropertyToQO& spPropertyBag);

    _Check_return_ HRESULT ProvideValueImplHelper(
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT SetGuardImplHelper(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ bool fStartGuard);

    _Check_return_ HRESULT SetUriBaseImplHelper(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
        _In_ xref_ptr<IPALUri> spBaseUri);

    _Check_return_ HRESULT RaiseTemplateOwnerEventImplHelper(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance);

    _Check_return_ HRESULT StoreSourceInformationHelper(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance);

    _Check_return_ HRESULT AddDeferredProxy(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy);

    static void RemoveObjectReferencesFromStack(
        _In_ const std::shared_ptr<ObjectWriterStack>& spObjectWriterStack);

    void TraceElementSourceInfo(
        _In_ const XamlLineInfo& lineInfo,
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance
    );

    #pragma endregion

    // Cache the xaml diagnostics runtime feature so that this instance
    // won't require multiple calls during parse.
    bool m_shouldStoreSourceInformation;

    xstring_ptr m_resourceUri;
};

