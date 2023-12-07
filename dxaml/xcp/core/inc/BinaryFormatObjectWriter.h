// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "ObjectWriterSettings.h"

class ObjectWriterCommonRuntime;
class ObjectWriterContext;
class ObjectWriterErrorService;
class ObjectWriterNode;
class XamlLineInfo;
class XamlTextSyntax;

class BinaryFormatObjectWriter final
{
public:
    BinaryFormatObjectWriter()
    {
    }

public:
    ~BinaryFormatObjectWriter()
    {
        // m_qoLastInstance is explicitly reset here because its CValue, m_value, may be pointing to a non-ref-counted CDependencyObject.
        // This prevents the XamlQualifiedObject accessing a destructed CDependencyObject in its destructor.
        m_qoLastInstance.reset();
    }

    static
    _Check_return_ HRESULT Create(
                _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                _In_ const ObjectWriterSettings& settings,
                _Out_ std::shared_ptr<BinaryFormatObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSavedContext> spSavedContext;
        return Create(spSchemaContext, spSavedContext, settings, rspObjectWriter);
    }

    static
    _Check_return_ HRESULT Create(
                _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
                _Out_ std::shared_ptr<BinaryFormatObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        return Create(spSchemaContext, spSavedContext, ObjectWriterSettings(), rspObjectWriter);
    }

    static
    _Check_return_ HRESULT Create(
                _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
                _In_ const ObjectWriterSettings& settings,
                _Out_ std::shared_ptr<BinaryFormatObjectWriter>& rspObjectWriter)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        return Create(spSchemaContext, spSavedContext, settings, rspObjectWriter);
    }

    static
    _Check_return_ HRESULT Create(
                _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
                _In_ const std::shared_ptr<XamlSavedContext>& spSavedContext,
                _In_ const ObjectWriterSettings& settings,
                _Out_ std::shared_ptr<BinaryFormatObjectWriter>& rspObjectWriter);

    _Check_return_ HRESULT WriteNode(_In_ const ObjectWriterNode& inNode);

    std::shared_ptr<XamlQualifiedObject> get_Result() const;

    void SetActiveObject(_In_ std::shared_ptr<XamlQualifiedObject> object);

    _Check_return_ HRESULT ProvideStaticResourceReference(
        _In_ const ObjectWriterNode& node,
        _In_opt_ CStyle* optimizedStyleParent = nullptr,
        _In_ KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty);

    _Check_return_ HRESULT ProvideThemeResourceValue(
        _In_ const ObjectWriterNode& node,
        _In_opt_ CStyle* optimizedStyleParent = nullptr,
        _In_ KnownPropertyIndex stylePropertyIndex = KnownPropertyIndex::UnknownType_UnknownProperty);

    _Check_return_ HRESULT ProvideValueForMarkupExtension(
        _In_ const ObjectWriterNode& node);

private:
    _Check_return_ HRESULT PushScopeIfRequired(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT PopScopeIfRequired(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT AddNamespacePrefix(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT CreateInstanceFromType(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT BeginInitOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT EndInitOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT GetValueFromMemberOnParentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT GetResourcePropertyBagForCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT SetNameOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT SetConnectionIdOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT SetDeferredPropertyOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT PushConstant(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT SetValueOnCurrentInstance(
        _In_ const ObjectWriterNode& node,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spValue);

    _Check_return_ HRESULT TypeConvertValue(
        _In_ const ObjectWriterNode& node,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
        _In_ const bool createValueTypeAsObject,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance);

    _Check_return_ HRESULT TypeConvertResolvedType(
        _In_ const ObjectWriterNode& node,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
        _In_ const bool createValueTypeAsObject,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance);

    _Check_return_ HRESULT TypeConvertResolvedProperty(
        _In_ const ObjectWriterNode& node,
        _In_ const std::shared_ptr<XamlType>& spType,
        _In_ const std::shared_ptr<XamlTextSyntax>& spConverter,
        _In_ const bool createValueTypeAsObject,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spConvertedInstance);

    _Check_return_ HRESULT SetTemplateBindingOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT AddToCollectionOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT AddToDictionaryOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT SetCustomRuntimeDataOnCurrentInstance(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT BeginConditionalScope(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT EndConditionalScope(
        _In_ const ObjectWriterNode& node);

    _Check_return_ HRESULT PerformDuplicatePropertyAssignmentCheck(
        _In_ const std::shared_ptr<XamlProperty>& spProperty,
        _In_ const XamlLineInfo& lineInfo);

    bool IsSkippingForConditionalScope() const;

private:
    std::shared_ptr<XamlQualifiedObject> m_qoLastInstance;
    std::shared_ptr<XamlQualifiedObject> m_rootObjectCreated;
    std::shared_ptr<XamlType> m_qoLastType;
    std::shared_ptr<XamlQualifiedObject> m_qoRootObjectInstance;
    std::shared_ptr<XamlQualifiedObject> m_qoEventRoot;
    std::shared_ptr<XamlQualifiedObject> m_qoXBindConnector;
    std::shared_ptr<XamlQualifiedObject> m_qoParentXBindConnector;
    std::shared_ptr<ObjectWriterContext> m_spContext;
    std::shared_ptr<ObjectWriterCallbacksDelegate> m_spObjectWriterCallbacks;
    std::shared_ptr<ObjectWriterCommonRuntime> m_spRuntime;
    std::shared_ptr<ObjectWriterErrorService> m_spErrorService;
    bool m_replacingPropertyValues = false;
};