// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeTokens.h"
#include "XamlRuntime.h"

class XamlSchemaContext;
struct XamlQualifiedObject;
class XamlServiceProviderContext;
class CDependencyProperty;
class CCoreServices;

class XamlNativeRuntime final : public XamlRuntime
{
public:
    XamlNativeRuntime( _In_ const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext)
        : m_spXamlSchemaContext(inXamlSchemaContext), m_pCore(NULL)
    {
    }

    _Check_return_ HRESULT CreateInstance(
                _In_ const XamlTypeToken& inXamlType,
                // XamlQualifiedObject args, // TODO: do we need args?
                _Out_ std::shared_ptr<XamlQualifiedObject>& qo ) override;

    //CreateFromValue is expected to convert the provided value via any applicable converter (on property or type) or provide the original value if there is no converter
    _Check_return_ HRESULT CreateFromValue(
                _In_ std::shared_ptr<XamlServiceProviderContext> inServiceContext,          // NOTE: This is deliberately no const-ref because is cast
                _In_ const XamlTypeToken& inTextSyntaxToken,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoValue,
                _In_ const XamlPropertyToken& inProperty,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoRootInstance,
                _In_ bool fIsPropertyAssignment,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qo ) override;

    _Check_return_ HRESULT CreateFromXmlText(
                _In_ const XamlTypeToken& inXamlType,
                _In_ const xstring_ptr& inText,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qo ) override;

    _Check_return_ HRESULT GetValue(
                _In_ const XamlQualifiedObject& qoObj,
                _In_ const XamlPropertyToken& inProperty,
                _Out_ XamlQualifiedObject& outValue ) override;

    _Check_return_ HRESULT GetAmbientValue(
                _In_ const XamlQualifiedObject& qoObj,
                _In_ const XamlPropertyToken& inProperty,
                _Out_ XamlQualifiedObject& outValue ) override;

    _Check_return_ HRESULT SetValue(
                _In_ const std::shared_ptr<XamlQualifiedObject>& inObj,
                _In_ const XamlPropertyToken& inProperty,
                _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
                _In_ bool bBindTemplates ) override;

    _Check_return_ HRESULT Add(
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoCollection,
                _In_ const std::shared_ptr<XamlQualifiedObject>& inValue) override;

    _Check_return_ HRESULT AddToDictionary(
                _In_ const std::shared_ptr<XamlQualifiedObject>& collection,
                _In_ const std::shared_ptr<XamlQualifiedObject>& value,
                _In_ const std::shared_ptr<XamlQualifiedObject>& key) override;


    _Check_return_ HRESULT CallProvideValue(
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoMarkupExtension,
                _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
                _Out_ std::shared_ptr<XamlQualifiedObject>& qo ) override;

    _Check_return_ HRESULT InitializationGuard(
                _In_ const std::shared_ptr<XamlType>& spXamlType,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
                _In_ bool fInitializing,
                _In_ const std::shared_ptr<XamlServiceProviderContext>& context) override;

    _Check_return_ HRESULT SetUriBase(
                _In_ const std::shared_ptr<XamlType>& spXamlType,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoInstance,
                _In_ xref_ptr<IPALUri> spBaseUri ) override;

    _Check_return_ HRESULT SetConnectionId(
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget) override;

    _Check_return_ HRESULT GetXBindConnector(
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoComponentConnector,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoConnectionId,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoTarget,
                _In_ const std::shared_ptr<XamlQualifiedObject>& qoReturnConnector) override;

    // Get the XamlTypeToken for a dependency object.  Some entries in the type
    // table use the same type for text syntax (specifically enumerations and
    // things like booleans which are treated like enumerations on the native
    // side), so this method it used to obtain the correct type token for a
    // dependency object.
    static _Check_return_ HRESULT GetTypeTokenForDO(
        _In_ CDependencyObject* pdo,
        _Out_ XamlTypeToken& ttTypeToken );

    // Get a cached instance for shareable objects
    _Check_return_ HRESULT TryGetCachedInstance(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    // Cache instance for shareable objects
    _Check_return_ HRESULT CacheInstance(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spInstance) override;

    _Check_return_ HRESULT AddDeferredProxy(
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParent,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spParentCollection,
        _In_ const XamlPropertyToken& inProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& spProxy) override;

protected:
    _Check_return_ HRESULT GetCore( _Outptr_ CCoreServices **ppCore );
    _Check_return_ HRESULT GetSchemaContext( std::shared_ptr<XamlSchemaContext>& outSchemaContext );


private:
    _Check_return_ HRESULT CheckIsPropertyWriteable(
        _In_ CCoreServices *pCore,
        _In_ const CDependencyProperty *pdp);

    static _Check_return_ HRESULT CheckAssignableForSafety(
        _In_ const CDependencyProperty *pdp,
        _In_ CValue *pValue);


    _Check_return_ HRESULT GetAdjustedCValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
        _In_ const XamlPropertyToken& inProperty,
        _Out_ bool *pfShouldDelegateCall,
        _Outptr_ CValue **ppValue);

    _Check_return_ HRESULT DelegateSetValue(
        _In_ const std::shared_ptr<XamlQualifiedObject>& inObj,
        _In_ const XamlPropertyToken& inProperty,
        _In_ const std::shared_ptr<XamlQualifiedObject>& inValue,
        _In_ bool bBindTemplates
        );

private:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    CCoreServices *m_pCore;

    std::shared_ptr<XamlQualifiedObject> m_spTemplateBindingExtensionXamlInstance; // Cached instance corresponding to TemplateBindingExtension
    std::shared_ptr<XamlQualifiedObject> m_spStaticResourceExtensionXamlInstance; // Cached instance corresponding to StaticResourceExtension
    std::shared_ptr<XamlQualifiedObject> m_spThemeResourceExtensionXamlInstance; // Cached instance corresponding to ThemeResourceExtension
};

