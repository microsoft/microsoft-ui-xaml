// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeTokens.h"
#include "TypeBits.h"

// Abstract interface for type information that can be implemented by any
// platform that will be supported by the XAML parser.
class XamlTypeInfoProvider
{
protected:
    XamlTypeInfoProvider() {}

public:
    
    virtual _Check_return_ HRESULT ResolveAssembly(
        _In_ const xstring_ptr& inAssemblyName,
        _Out_ XamlAssemblyToken& outAssemblyToken
        ) = 0;

    virtual _Check_return_ HRESULT GetTypeNamespace(
        _In_ const xstring_ptr& inTypeNamespace,
        _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
        ) = 0;
    
    virtual _Check_return_ HRESULT GetTypeNamespace(
        _In_ const xstring_ptr& inAssemblyName,
        _In_ const xstring_ptr& inTypeNamespace,
        _Out_ XamlTypeNamespaceToken& outTypeNamespaceToken
        ) = 0;

    virtual _Check_return_ HRESULT GetTypeNamespaceForType(
        _In_ const XamlTypeToken sXamlTypeToken,
        _Out_ XamlTypeNamespaceToken& rXamlTypeNamespaceToken,
        _Out_ xstring_ptr* pstrOutXamlTypeNamespaceName
        ) = 0;
        
    virtual _Check_return_ HRESULT LookupTypeFlags( 
        _In_ const XamlTypeToken sTypeToken, 
        _In_ const XamlBitSet<BoolTypeBits>& btbLookupValues, 
        _Out_ XamlBitSet<BoolTypeBits>& btbBitsChecked, 
        _Out_ XamlBitSet<BoolTypeBits>& btbReturnValues
        ) = 0;
    
    virtual _Check_return_ HRESULT LookupPropertyFlags( 
        _In_ const XamlPropertyToken sPropertyToken, 
        _In_ const XamlBitSet<BoolPropertyBits>& bpbLookupValues, 
        _Out_ XamlBitSet<BoolPropertyBits>& bpbBitsChecked, 
        _Out_ XamlBitSet<BoolPropertyBits>& bpbReturnValues
        ) = 0;
    
    virtual _Check_return_ HRESULT  ResolveTypeName(
        _In_ const XamlTypeNamespaceToken inNamespaceToken,
        _In_ const xstring_ptr& inTypeName,
        _Out_ XamlTypeToken& rXamlType      
        ) = 0;

    virtual _Check_return_ HRESULT ResolvePropertyName(
        _In_ const XamlTypeToken sTypeToken,
        _In_ const xstring_ptr& inPropertyName,
        _Out_ XamlPropertyToken& outProperty,
        _Out_ XamlTypeToken& outPropertyTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT ResolveDependencyPropertyName(
        _In_ const XamlTypeToken sTypeToken,
        _In_ const xstring_ptr& inPropertyName,
        _Out_ XamlPropertyToken& outProperty,
        _Out_ XamlTypeToken& outPropertyTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetTypeName(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ xstring_ptr* pstrOutTypeName
        ) = 0;

    // Get the full name of the type (including its CLR namespace).
    virtual _Check_return_ HRESULT GetTypeFullName(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ xstring_ptr* pstrOutTypeFullName
        ) = 0;

    virtual _Check_return_ HRESULT GetPropertyName(
        _In_ const XamlPropertyToken sPropertyToken,
        _Out_ xstring_ptr* pstrOutPropertyName
        ) = 0;
    
    // if the passed type is derived directly or indirectly from a core
    // Silverlight type, then return the name of that type.
    virtual _Check_return_ HRESULT GetBaseType(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlTypeToken& rBaseTypeToken
        ) = 0;

    // Gets the type that declares this property.
    virtual _Check_return_ HRESULT GetDeclaringType(
        _In_ const XamlPropertyToken sPropertyToken,
        _Out_ XamlTypeToken& rDeclaringTypeToken
        ) = 0;
    
    // if DerivedType is derived from BaseType, then return success, otherwise fail
    virtual _Check_return_ HRESULT IsAssignableFrom(
        _In_ const XamlTypeToken sDerivedTypeToken,
        _In_ const XamlTypeToken rxtBaseType,
        _Out_ bool& bOut
        ) = 0;
    
    virtual _Check_return_ HRESULT GetContentProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rContentPropertyTypeToken
        ) = 0;

    // Collection types may optionally provide a type used to wrap literal
    // content from the parser before inserting into the collection.  The parser
    // won't actually wrap the content, but merely knows it's safe to pass
    // literal content to the collection which will handle it.
    virtual _Check_return_ HRESULT GetContentWrapper(
        _In_ const XamlTypeToken sTypeToken,
        _Out_opt_ XamlTypeToken& rContentWrapperTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetRuntimeNameProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) = 0;
    
    virtual _Check_return_ HRESULT GetDictionaryKeyProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetXmlLangProperty(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlPropertyToken& rPropertyToken,
        _Out_ XamlTypeToken& rPropertyTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetCollectionItemType(
        _In_ const XamlTypeToken sCollectionTypeToken,
        _Out_ XamlTypeToken& rCollectionItemTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetTextSyntaxForType(
        _In_ const XamlTypeToken sTypeToken,
        _Out_ XamlTypeToken& sTextSyntaxTypeToken
        ) = 0;

    virtual _Check_return_ HRESULT GetTextSyntaxForProperty(
        _In_ const XamlPropertyToken sTypeToken,
        _Out_ XamlTypeToken& sTextSyntaxTypeToken
        ) = 0;
};

