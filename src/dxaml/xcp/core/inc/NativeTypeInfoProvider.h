// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// Provide the XAML parser with type information necessary to parse native
// types.
class XamlNativeTypeInfoProvider final : public XamlTypeInfoProvider
{
public:
    XamlNativeTypeInfoProvider(
        _In_ const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext
        );

    _Check_return_ HRESULT ResolveAssembly(
        _In_ const xstring_ptr& inAssemblyName,
        _Out_ XamlAssemblyToken& outAssemblyToken
        ) override;

    _Check_return_ HRESULT GetTypeNamespace(
        _In_ const xstring_ptr& inTypeNamespace,
        _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
        ) override;


    _Check_return_ HRESULT GetTypeNamespace(
        _In_ const xstring_ptr& inAssemblyName,
        _In_ const xstring_ptr& inTypeNamespace,
        _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
        ) override;

    _Check_return_ HRESULT GetTypeNamespaceForType(
        _In_ const XamlTypeToken sXamlTypeToken,
        _Out_ XamlTypeNamespaceToken& rXamlTypeNamespaceToken,
        _Out_ xstring_ptr* pstrOutXamlTypeNamespaceName
        ) override;

    _Check_return_ HRESULT LookupTypeFlags(
        _In_ const XamlTypeToken sTypeToken,
        _In_ const XamlBitSet<BoolTypeBits>& btbLookupValues,
        _Out_ XamlBitSet<BoolTypeBits>& btbBitsChecked,
        _Out_ XamlBitSet<BoolTypeBits>& btbReturnValues
        ) override;

    _Check_return_ HRESULT LookupPropertyFlags(
        _In_ const XamlPropertyToken sPropertyToken,
        _In_ const XamlBitSet<BoolPropertyBits>& bpbLookupValues,
        _Out_ XamlBitSet<BoolPropertyBits>& bpbBitsChecked,
        _Out_ XamlBitSet<BoolPropertyBits>& bpbReturnValues
        ) override;

    _Check_return_ HRESULT ResolveTypeName(
        _In_ const XamlTypeNamespaceToken inNamespaceToken,
        _In_ const xstring_ptr& inTypeName,
        _Out_ XamlTypeToken& rXamlType
        ) override;

    _Check_return_ HRESULT ResolvePropertyName(
        _In_ const XamlTypeToken sTypeToken,
        _In_ const xstring_ptr& inPropertyName,
        _Out_ XamlPropertyToken& outProperty,
        _Out_ XamlTypeToken& outPropertyTypeToken
        ) override;

    _Check_return_ HRESULT ResolveDependencyPropertyName(
        _In_ const XamlTypeToken sTypeToken,
        _In_ const xstring_ptr& inPropertyName,
        _Out_ XamlPropertyToken& outProperty,
        _Out_ XamlTypeToken& outPropertyTypeToken
        ) override
    {
        return ResolvePropertyName(sTypeToken, inPropertyName, outProperty, outPropertyTypeToken);
    }

    _Check_return_ HRESULT GetTypeName(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ xstring_ptr* pstrOutTypeName
        ) override;

    // Get the full name of the type (including its CLR namespace).
    _Check_return_ HRESULT GetTypeFullName(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ xstring_ptr* pstrOutTypeFullName
        ) override;

    _Check_return_ HRESULT GetPropertyName(
        _In_ const XamlPropertyToken sPropertyToken,
        _Out_ xstring_ptr* pstrOutPropertyName
        ) override;

    _Check_return_ HRESULT GetBaseType(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlTypeToken& rBaseTypeToken
        ) override;

    // Gets the type that declares this property.
    _Check_return_ HRESULT GetDeclaringType(
        _In_ const XamlPropertyToken sPropertyToken,
        _Out_ XamlTypeToken& rDeclaringTypeToken
        ) override;

    // if DerivedType is derived from BaseType, then return success, otherwise fail
    _Check_return_ HRESULT IsAssignableFrom(
        _In_ const XamlTypeToken sDerivedTypeToken,
        _In_ const XamlTypeToken rxtBaseType,
        _Out_ bool& bOut
        ) override;

    _Check_return_ HRESULT GetContentProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rContentPropertyTypeToken
        ) override;

    // Collection types may optionally provide a type used to wrap literal
    // content from the parser before inserting into the collection.  The parser
    // won't actually wrap the content, but merely knows it's safe to pass
    // literal content to the collection which will handle it.
    _Check_return_ HRESULT GetContentWrapper(
        _In_ const XamlTypeToken sTypeToken,
        _Out_opt_ XamlTypeToken& rContentWrapperTypeToken
        ) override;

    _Check_return_ HRESULT GetRuntimeNameProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) override;

    _Check_return_ HRESULT GetDictionaryKeyProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) override;

    _Check_return_ HRESULT GetXmlLangProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) override;

    _Check_return_ HRESULT GetCollectionItemType(
        _In_ const XamlTypeToken sCollectionTypeToken,
        _Out_ XamlTypeToken& rCollectionItemTypeToken
        ) override;

    _Check_return_ HRESULT GetTextSyntaxForType(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlTypeToken& sTextSyntaxTypeToken
        ) override;

    _Check_return_ HRESULT GetTextSyntaxForProperty(
        _In_ const XamlPropertyToken sTypeToken,
        _Out_ XamlTypeToken& sTextSyntaxTypeToken
        ) override;

protected:
    _Check_return_ HRESULT GetCore(
        _Outptr_ CCoreServices **ppCore
        );

    _Check_return_ HRESULT GetSchemaContext(
        _Out_ std::shared_ptr<XamlSchemaContext>& outSchemaContext
        );

    _Check_return_ HRESULT IsMarkupExtension(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ bool& outbIsMarkupExtension
        );

    _Check_return_ HRESULT IsTypeConstructible(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ bool& outbIsConstructible
        );

    _Check_return_ HRESULT IsTypePublic(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ bool& outbIsPublic
        );

    // Determine whether the type token represents a System.String type.
    _Check_return_ HRESULT IsString(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ bool& outbIsString
        );

    _Check_return_ HRESULT IsPropertyEvent(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ bool& outbIsEvent
        );

    _Check_return_ HRESULT IsPropertyReadOnly(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ bool& outbIsReadOnly
        );

    _Check_return_ HRESULT IsPropertyStatic(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ bool& outbIsStatic
        );

    _Check_return_ HRESULT IsPropertyPublic(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ bool& outbIsPublic
        );

    _Check_return_ HRESULT IsPropertyAttachable(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ bool& outbIsAttachable
        );

    _Check_return_ HRESULT IsTemplateType(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ bool& outbIsTemplateType
        );

private:
    _Check_return_ HRESULT GetFullClassNameByIndex(
        _In_ const KnownTypeIndex nClassIndex,
        _Out_ xstring_ptr* pstrFullName
        );

    _Check_return_ HRESULT GetAllTypeNamespaces(
        _In_ const xstring_ptr& strXmlNamespace,
        _Outptr_result_buffer_(*pcTypeNamespaceIds) const KnownNamespaceIndex **ppuiTypeNamespaceIds,
        _Out_ XUINT32 *pcTypeNamespaceIds
        );

private:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    CCoreServices *m_pCore;
};


