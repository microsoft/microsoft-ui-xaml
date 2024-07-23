// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "ObjectWriterCommonRuntime.h"
#include "ObjectWriterNodeList.h"
#include "ObjectWriterErrorService.h"
#include <ObjectWriterContext.h>
#include <SubObjectWriterResult.h>
#include <CustomWriterRuntimeData.h>
#include <XamlPredicateHelpers.h>
#include "XamlProperty.h"

const bool ObjectWriterCommonRuntime::IsValidatorEnabled() const
{
    return m_isValidatorEnabled;
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::AddNamespacePrefix(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const xstring_ptr& strNamespacePrefix,
    _In_ const std::shared_ptr<XamlNamespace>& spNamespace)
{
    IFC_RETURN(m_spContext->AddNamespacePrefix(strNamespacePrefix, spNamespace));
    IFC_RETURN(AddNamespacePrefixImpl(lineInfo, strNamespacePrefix, spNamespace));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::PushConstant(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    m_spContext->Current().set_Value(spValue);
    IFC_RETURN(PushConstantImpl(lineInfo, spValue));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::SetDirectiveProperty(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<DirectiveProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    IFC_RETURN(m_spContext->Parent().add_Directive(spProperty, spInstance));
    IFC_RETURN(SetDirectivePropertyImpl(lineInfo, spProperty, spInstance));

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::PushScope(_In_ const XamlLineInfo& lineInfo)
{
    m_spContext->PushScope();
    return PushScopeImpl(lineInfo);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::PopScope(_In_ const XamlLineInfo& lineInfo)
{
    m_spContext->PopScope();
    return PopScopeImpl(lineInfo);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::CreateType(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    // skip the validation of the root object during encoding
    // because when we instantiate the stream, an existing
    // root may be provided making this validation, incorrectly
    // error if the xaml type specified was an abstract non constructible type.
    //
    // eg. HomePage.xaml
    // <mypages:LayoutAwarePage ... x:Class="..." />
    //
    // only the x:Class should be honored and we should not test
    // constructibility of abstract LayoutAwarePage
    bool skipValidateCreate = (m_isEncoding && !m_isRootObjectCreated);
    if (IsValidatorEnabled() && !skipValidateCreate)
    {
        bool bIsConstructible = false;
        bool bIsTypePublic = true;

        // Make sure we can create an instance of the type
        IFC_RETURN(spType->IsConstructible(bIsConstructible));

        // For managed types, we will additionally verify that the type is public.
        if (spType->get_TypeToken().GetProviderKind() == tpkManaged)
        {
            IFC_RETURN(spType->IsPublic(bIsTypePublic));
        }

        if (!bIsConstructible || !bIsTypePublic)
        {
            xstring_ptr spTypeName;
            IFC_RETURN(spType->get_FullName(&spTypeName));

            IFC_RETURN(m_spErrorService->ReportError(AG_E_PARSER2_OW_NO_CTOR, lineInfo, spTypeName));
        }
    }

    IFC_RETURN(CreateTypeImpl(lineInfo, spType, spCallback, spRootObjectInstance, spInstance));

    m_isRootObjectCreated = true;

    return S_OK;
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::GetValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    return GetValueImpl(lineInfo, spInstance, spProperty, spValue);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::CreateTypeWithInitialValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<ObjectWriterCallbacksDelegate>& spCallback,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spRootObjectInstance,
    _Inout_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    return CreateTypeWithInitialValueImpl(lineInfo, spType, spCallback, spRootObjectInstance, spInstance);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::TypeConvertValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ std::shared_ptr<XamlServiceProviderContext> spContext,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlTextSyntax>& spTextSyntaxConverter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ bool fIsPropertyAssignment,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    return TypeConvertValueImpl(lineInfo, spContext, spType, spTextSyntaxConverter, spValue, fIsPropertyAssignment, spInstance);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::SetValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    return SetValueImpl(lineInfo, spType, spInstance, spProperty, spValue);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::CheckPeerType(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue)
{
    return CheckPeerTypeImpl(lineInfo, spInstance, spValue);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::AddToCollection(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spParentPropertyType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType)
{
    return AddToCollectionImpl(lineInfo, spParentPropertyType, spParentCollection, spValue, spValueType);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::AddToDictionary(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spValue,
    _In_ const std::shared_ptr<XamlType>& spValueType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey)
{
    return AddToDictionaryImpl(lineInfo, spParentCollection, spValue, spValueType, spKey);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::SetConnectionId(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget)
{
    return SetConnectionIdImpl(lineInfo, spComponentConnector, spConnectionId, spTarget);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::GetXBindConnector(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spComponentConnector,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spConnectionId,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spTarget,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spReturnConnector)
{
    return GetXBindConnectorImpl(lineInfo, spComponentConnector, spConnectionId, spTarget, spReturnConnector);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::SetName(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spName,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const xref_ptr<INameScope>& spNamescope)
{
    return SetNameImpl(lineInfo, spName, spInstance, spNamescope);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::GetResourcePropertyBag(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spKey,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spObject,
    _Out_ SP_MapPropertyToQO& spPropertyBag)
{
    return GetResourcePropertyBagImpl(lineInfo, spServiceProviderContext, spKey, spType, spObject, spPropertyBag);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::ProvideValue(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spMarkupExtension,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spValue)
{
    return ProvideValueImpl(lineInfo, spServiceProviderContext, spMarkupExtension, spValue);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::BeginInit(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    return BeginInitImpl(lineInfo, spXamlType, spInstance);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::EndInit(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spXamlType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance)
{
    return EndInitImpl(lineInfo, spXamlType, spInstance);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::SetCustomRuntimeData(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ std::shared_ptr<CustomWriterRuntimeData> customRuntimeData,
    _In_ std::shared_ptr<SubObjectWriterResult> subObjectWriterResult,
    _In_ xref_ptr<INameScope> nameScope,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spXBindConnector,
    const std::shared_ptr<ObjectWriterCallbacksDelegate>& callback)
{
    return SetCustomRuntimeDataImpl(
        lineInfo,
        spType,
        spInstance,
        std::move(customRuntimeData),
        std::move(subObjectWriterResult),
        std::move(nameScope),
        spXBindConnector,
        callback);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::DeferResourceDictionaryItems(
    _In_ const XamlLineInfo& lineInfo,
    _In_ ObjectWriter* pObjectWriter,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spCollection,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _In_ const bool fIsDictionaryWithKeyProperty,
    _Out_ bool& hasDeferred)
{
    return DeferResourceDictionaryItemsImpl(lineInfo, pObjectWriter, spCollection, spNodeList, fIsDictionaryWithKeyProperty, hasDeferred);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::DeferTemplateContent(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<XamlType>& spType,
    _In_ const std::shared_ptr<XamlProperty>& spProperty,
    _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance,
    _In_ const std::shared_ptr<XamlOptimizedNodeList>& spNodeList,
    _Out_ std::shared_ptr<XamlQualifiedObject>& spTemplateContent)
{
    return DeferTemplateContentImpl(lineInfo, spType, spProperty, spInstance, spNodeList, spTemplateContent);
}

_Check_return_ HRESULT ObjectWriterCommonRuntime::GetStreamOffsetToken(
    _Out_ UINT32 *pTokenIndex)
{
    return GetStreamOffsetTokenImpl(pTokenIndex);
}

_Check_return_ HRESULT
ObjectWriterCommonRuntime::BeginConditionalScope(
    _In_ const XamlLineInfo& lineInfo,
    _In_ const std::shared_ptr<Parser::XamlPredicateAndArgs>& xamlPredicateAndArgs)
{
    return BeginConditionalScopeImpl(lineInfo, xamlPredicateAndArgs);
}

_Check_return_ HRESULT
ObjectWriterCommonRuntime::EndConditionalScope(
    _In_ const XamlLineInfo& lineInfo)
{
    return EndConditionalScopeImpl(lineInfo);
}

std::shared_ptr<ObjectWriterNodeList> ObjectWriterCommonRuntime::GetNodeList()
{
    std::shared_ptr<ObjectWriterNodeList> spNodeList;

    return spNodeList;
}

