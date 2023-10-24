// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "CompressedStackCacheHint.h"
#include "ParserTypedefs.h"

class XamlType;
class XamlSchemaContext;
class ObjectWriterContext;
class ObjectWriterFrame;
class CompressedObjectWriterStack;

// Stores context from the XAML parser to be used by type converters and
// markup extensions.
class XamlServiceProviderContext
{
private:
    std::shared_ptr<ObjectWriterContext> m_spObjectWriterContext;

public:
    XamlServiceProviderContext(_In_ const std::shared_ptr<ObjectWriterContext>& spObjectWriterContext);

    ~XamlServiceProviderContext() = default;

    _Check_return_ HRESULT GetSchemaContext(
        _Out_ std::shared_ptr<XamlSchemaContext>& rspSchemaContext);

    // Retrieve the root object instance of the parse context.
    //
    // Used by TemplateContent to grab the EventRoot and by Theme/StaticResourceExtension to
    // find the root with the dictionary to resolve resources against.
    std::shared_ptr<XamlQualifiedObject> GetRootObject();

    // Resolve a full type name using the parse context to resolve any namespaces.
    //
    // Used by PropertyPath to resolve string-based property names.
    _Check_return_ HRESULT ResolveXamlType(
        _In_ const xstring_ptr_view& spQName,
        _Out_ std::shared_ptr<XamlType>& rspType);

    // Resolve either a fully qualified property name or a property name using the target type
    _Check_return_ HRESULT ResolveDependencyProperty(
        _In_ const xstring_ptr& spQName,
        _In_ const std::shared_ptr<XamlType>& spTargetType,
        _Out_ std::shared_ptr<XamlProperty>& rspProperty);

    // Resolve a fully qualified property name
    _Check_return_ HRESULT ResolveProperty(
        _In_ const xstring_ptr_view& fullyQualifiedName,
        _Out_ std::shared_ptr<XamlProperty>& rspProperty);

    // Retrieve the base uri in the parse context.
    //
    // Currently used by ImageSource and ParserServiceProvider (implements IUriContext).
    xref_ptr<IPALUri> GetBaseUri();

    // Retrieve the top most target type from the parse context.
    //
    // Note: This isn't on the original interface. This is partly
    // an optimization because the types and properties can be cached on the
    // object writer context and partly to simplify the code.
    _Check_return_ HRESULT GetAmbientTargetType(
        _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue);

    // Resolve all instance objects that match the ambient search across the parser context
    // using either a type or property criteria.
    _Check_return_ HRESULT GetAllAmbientValues(
        _In_ std::shared_ptr<XamlType> spCeilingType,
        _In_ std::shared_ptr<XamlProperty> spProperty,
        _In_ std::shared_ptr<XamlType> spTypeToFind,
        _In_ CompressedStackCacheHint eCacheHint,
        _Outref_ AmbientValuesVector& rvecValues);

    // Resolve the first instance object that matches the ambient search across the parser context
    // using either a type or property criteria.
    _Check_return_ HRESULT GetAmbientValue(
        _In_ const std::shared_ptr<XamlType>& spCeilingType1,
        _In_ const std::shared_ptr<XamlType>& spCeilingType2,
        _In_ const std::shared_ptr<XamlProperty>& spProperty1,
        _In_ const std::shared_ptr<XamlProperty>& spProperty2,
        _In_ const std::shared_ptr<XamlType>& spTypeToFind,
        _In_ CompressedStackCacheHint cacheHint,
        _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue);

    // Returns the object that is the target of the current markup extension context.
    //
    // Used by MarkupExtension/TypeConverter.
    void GetMarkupExtensionTargetObject(_Out_ std::shared_ptr<XamlQualifiedObject>& spTargetObject);

    // Returns the property that is the target of the current markup extension context.
    // Note: this can be either a DependencyProperty or a non-DP property.
    //
    // Used by MarkupExtension/TypeConverter.
    void GetMarkupExtensionTargetProperty(_Out_ std::shared_ptr<XamlProperty>& spPropertyInfo);

private:
    std::shared_ptr<ObjectWriterContext> GetObjectWriterContext()
    {
        return m_spObjectWriterContext;
    }

    _Check_return_ HRESULT ExtractTargetTypeAndPropertyName(
        _In_ const xstring_ptr_view& propertyName,
        _Out_ std::shared_ptr<XamlType>& spType,
        _Out_ xstring_ptr* pPropertyName);
};


