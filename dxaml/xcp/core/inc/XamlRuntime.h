// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeTokens.h"

class XamlType;
struct XamlQualifiedObject;
class XamlServiceProviderContext;
class XamlServiceProviderContext;

class XamlRuntime
{
public:
    virtual ~XamlRuntime() {}

    
    _Check_return_ virtual HRESULT CreateInstance(
        _In_ const XamlTypeToken& inXamlType, 
        // XamlQualifiedObject args, // TODO: do we need args?
        _Out_ std::shared_ptr<XamlQualifiedObject>& qo ) = 0;

    //CreateFromValue is expected to convert the provided value via any applicable converter (on property or type) or provide the original value if there is no converter
    _Check_return_ virtual HRESULT CreateFromValue(
        _In_ std::shared_ptr<XamlServiceProviderContext> inServiceContext,      // NOTE: This is deliberately no const-ref because is cast
        _In_ const XamlTypeToken& inTextSyntaxToken,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue, 
        _In_ const XamlPropertyToken& inProperty,          // used for event delegates
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoRootInstance,   // used for event delegates
        _In_ bool fIsPropertyAssignment, 
        _Out_ std::shared_ptr<XamlQualifiedObject>& qo
        ) = 0;

    _Check_return_ virtual HRESULT CreateFromXmlText(
        _In_ const XamlTypeToken& inXamlType, 
        _In_ const xstring_ptr& inText, 
        _Out_ std::shared_ptr<XamlQualifiedObject>& qo
        ) = 0;

    _Check_return_ virtual HRESULT GetValue(
        _In_ const XamlQualifiedObject& qoObj, 
        _In_ const XamlPropertyToken& inProperty, 
        _Out_ XamlQualifiedObject& outValue
        ) = 0;

    _Check_return_ virtual HRESULT GetAmbientValue(
        _In_ const XamlQualifiedObject& qoObj, 
        _In_ const XamlPropertyToken& inProperty, 
        _Out_ XamlQualifiedObject& outValue
        ) = 0;

    _Check_return_ virtual HRESULT SetValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& inObj, 
        _In_ const XamlPropertyToken& inProperty, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
        _In_ bool inBindTemplates
        ) = 0;

    _Check_return_ virtual HRESULT Add(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoCollection, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& inValue
        ) = 0;

    _Check_return_ virtual HRESULT AddToDictionary(
        _In_ const std::shared_ptr<XamlQualifiedObject>& collection, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& value, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& key
        ) = 0;

    _Check_return_ virtual HRESULT CallProvideValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoMarkupExtension, 
        _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext, 
        _Out_ std::shared_ptr<XamlQualifiedObject>& qo
        ) = 0;

    _Check_return_ virtual HRESULT InitializationGuard(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance, 
        _In_ bool fInitializing,
        _In_ const std::shared_ptr<XamlServiceProviderContext>& context
        ) = 0;

    _Check_return_ virtual HRESULT SetUriBase(
        _In_ const std::shared_ptr<XamlType>& spXamlType,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance, 
        _In_ xref_ptr<IPALUri> spBaseUri
        ) = 0;

    _Check_return_ virtual HRESULT SetConnectionId(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId, 
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget
        ) = 0;

    _Check_return_ virtual HRESULT GetXBindConnector(
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget,
        _In_ const std::shared_ptr<XamlQualifiedObject>& qoReturnConnector
        ) = 0;

    // Get a cached instance for shareable objects
    _Check_return_ virtual HRESULT TryGetCachedInstance(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    // Cache instance for shareable objects
    _Check_return_ virtual HRESULT CacheInstance(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) = 0;

    _Check_return_ virtual HRESULT AddDeferredProxy(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const XamlPropertyToken& inProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy) = 0;
};


