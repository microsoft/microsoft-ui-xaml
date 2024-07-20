// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlTypeTokens.h"
#include "xstring_ptr.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <StableXbfIndexes.g.h>

class CCoreServices;
class ParserErrorReporter;
class XamlNamespace;
class XamlProperty;
class DirectiveProperty;
class ImplicitProperty;
class XamlTextSyntax;
class XamlTypeNamespace;
class XamlAssembly;
class XamlRuntime;
class XamlTypeInfoProvider;
class XamlNativeTypeInfoProvider;
class XamlManagedTypeInfoProvider;
class XamlNativeRuntime;
class XamlType;
class XamlManagedRuntime;
class XamlXmlNamespace;
class IParserCoreServices;
struct XamlQualifiedObject;

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Parser {
    class XamlSchemaContextUnitTests;
    class XamlOptimizedNodeListUnitTests;
} } } } }

// Resolves xmlns Uri string to XamlXmlNamespace, and caches type
// information that is not subject to change for the lifetime of
// the Core.
class XamlSchemaContext final
    : public std::enable_shared_from_this<XamlSchemaContext>
{
    friend class Microsoft::UI::Xaml::Tests::Parser::XamlSchemaContextUnitTests;
    friend class Microsoft::UI::Xaml::Tests::Parser::XamlOptimizedNodeListUnitTests;

public:
    XamlSchemaContext() = default;
#pragma region Creation Methods

    static
    _Check_return_ HRESULT Create(
        _In_ IParserCoreServices *pCore,
        _Out_ std::shared_ptr<XamlSchemaContext>& outXamlSchemaContext
        );

    static
    _Check_return_ HRESULT Create(
        _In_ IParserCoreServices *pCore,
        _In_ const std::shared_ptr<ParserErrorReporter>& inErrorReporter,
        _Out_ std::shared_ptr<XamlSchemaContext>& outXamlSchemaContext
        );

#pragma endregion

#pragma region Schema Context APIs for Assembly, Namespace, TypeNamespace, Type and Property

    xstring_ptr GetSourceAssembly() const;

    _Check_return_ HRESULT PushSourceAssembly(
        _In_ const xstring_ptr& inssSourceAssembly
        );

    void PopSourceAssembly();

    xstring_ptr GetDefaultAssemblyName() const;

    _Check_return_ HRESULT AddAssembly(
        _In_ const XamlAssemblyToken& tokAssembly,
        _In_ const xstring_ptr& strAssemblyName
        );

    _Check_return_ HRESULT GetXamlAssembly(
        _In_ const XamlAssemblyToken& inAssemblyToken,
        _In_ const xstring_ptr& inAssemblyName,
        _Out_ std::shared_ptr<XamlAssembly>& outXamlAssembly
        );

    _Check_return_ HRESULT GetXamlAssembly(
        _In_ const XamlAssemblyToken& inAssemblyToken,
        _Out_ std::shared_ptr<XamlAssembly>& outXamlAssembly
        );

    // Helper method that validates a passed-in namespace string of the form
    // "using:<code namespace>" and returns the code namespace component.
    // Sets 'isValidXmlns' parameter to false if input is not of this form.
    _Check_return_ HRESULT CrackXmlns(
        _In_ const xstring_ptr& strXmlns,
        _Out_ xstring_ptr& strNamespace,
        _Out_ bool& isValidXmlns);

    _Check_return_ HRESULT AddAssemblyXmlnsDefinition(
        _In_ const XamlAssemblyToken& tokAssembly,
        _In_ const xstring_ptr& strXmlnsUri,
        _In_ const XamlTypeNamespaceToken& inTypeNamespaceToken,
        _In_ const xstring_ptr& strTypeNamespace
        );

    _Check_return_ HRESULT GetXamlXmlNamespace(
        _In_ const xstring_ptr& inXmlns,
        _Out_ std::shared_ptr<XamlNamespace>& outNamespace
        );

    const std::shared_ptr<XamlNamespace>& GetXamlNamespaceFromRuntimeIndex(
        _In_ size_t uiIndex
        ) const;

    _Check_return_ HRESULT GetXamlTypeNamespace(
        _In_ const XamlTypeNamespaceToken& inTypeNamespaceToken,
        _In_ const xstring_ptr& inTypeNamespaceName,
        _In_ const std::shared_ptr<XamlAssembly>& inXamlAssembly,
        _In_ const std::shared_ptr<XamlXmlNamespace>& xamlXmlNamespace,
        _Out_ std::shared_ptr<XamlTypeNamespace>& outXamlTypeNamespace
        );

    _Check_return_ HRESULT GetXamlType(
        _In_ const XamlTypeToken& inTypeToken,
        _Out_ std::shared_ptr<XamlType>& outXamlType
        );

    _Check_return_ HRESULT GetXamlType(
        _In_ const XamlTypeToken& inTypeToken,
        _In_ const std::shared_ptr<XamlNamespace>& inNamespace,
        _In_ const xstring_ptr& inTypeName,
        _Out_ std::shared_ptr<XamlType>& outXamlType
        );

    _Check_return_ HRESULT GetXamlType(
        _In_ const XamlTypeToken& inTypeToken,
        _In_ const std::shared_ptr<XamlNamespace>& inNamespace,
        _Out_ std::shared_ptr<XamlType>& outXamlType
        );

    _Check_return_ const std::shared_ptr<XamlType>& GetXamlTypeFromRuntimeIndex(
        _In_ size_t uiIndex) const;

    _Check_return_ HRESULT GetXamlTypeFromStableXbfIndex(
        _In_ Parser::StableXbfTypeIndex index,
        _Out_ std::shared_ptr<XamlType>& outXamlType
        );

    _Check_return_ HRESULT RegisterXamlProperty(
        _In_ const std::shared_ptr<XamlProperty>& spXamlProperty
        );

    _Check_return_ HRESULT GetXamlProperty(
        _In_ const XamlPropertyToken& inPropertyToken,
        _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
        );

    _Check_return_ HRESULT GetXamlProperty(
        _In_ const XamlPropertyToken& inPropertyToken,
        _In_ const XamlTypeToken& inPropertyTypeToken,
        _In_ const xstring_ptr& inPropertyName,
        _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
        );

    _Check_return_ HRESULT GetXamlProperty(
        _In_ const XamlPropertyToken& inPropertyToken,
        _In_ const XamlTypeToken& inPropertyTypeToken,
        _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
        );

    _Check_return_ HRESULT GetXamlProperty(
        _In_ const xstring_ptr& spFullyQualifiedPropertyName,
        _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
        );

    _Check_return_ HRESULT GetXamlDirective(
        _In_ const std::shared_ptr<XamlNamespace>& inXamlNamespace,
        _In_ const xstring_ptr_view& inssName,
        _Out_ std::shared_ptr<DirectiveProperty>& outXamlProperty
        );

    _Check_return_ const std::shared_ptr<XamlProperty>& GetXamlPropertyFromRuntimeIndex(
        _In_ size_t uiIndex) const;

    _Check_return_ HRESULT GetXamlPropertyFromStableXbfIndex(
        _In_ Parser::StableXbfPropertyIndex index,
        _Out_ std::shared_ptr<XamlProperty>& outXamlProperty
        );

    _Check_return_ HRESULT GetXamlTextSyntax(
        _In_ const XamlTypeToken& sTypeConverterTypeToken,
        _Out_ std::shared_ptr<XamlTextSyntax>& outXamlTextSyntax
        );

#pragma endregion

#pragma region Schema Context APIs for retrieving TypeInfoProvider and Runtime based on Token.

    _Check_return_ HRESULT GetTypeInfoProvider(
        _In_ XamlTypeInfoProviderKind tpk,
        _Out_ std::shared_ptr<XamlTypeInfoProvider>& outTypeInfoProvider
        );

    _Check_return_ HRESULT GetRuntime(
        _In_ XamlFactoryKind fk,
        _Out_ std::shared_ptr<XamlRuntime>& rspXamlRuntime
        );

#pragma endregion

#pragma region Cached Schema Properties and Types

    _Check_return_ HRESULT get_X_KeyProperty(
        _Out_ std::shared_ptr<DirectiveProperty>& rspOut
        );

    _Check_return_ HRESULT get_X_NameProperty(
        _Out_ std::shared_ptr<DirectiveProperty>& rspOut
        );

    _Check_return_ HRESULT get_StringXamlType(
        _Out_ std::shared_ptr<XamlType>& spStringType
        );

    _Check_return_ HRESULT get_Int32XamlType(
        _Out_ std::shared_ptr<XamlType>& spInt32Type
        );

    _Check_return_ HRESULT get_StringSyntax(
        _Out_ std::shared_ptr<XamlTextSyntax>& outStringSyntax
        );

    _Check_return_ HRESULT get_Int32Syntax(
        _Out_ std::shared_ptr<XamlTextSyntax>& outStringSyntax
        );

    _Check_return_ HRESULT get_InitializationProperty(
        _Out_ std::shared_ptr<ImplicitProperty>& outInitializationProperty
        );

    _Check_return_ HRESULT get_ItemsProperty(
        _In_ const std::shared_ptr<XamlType>& inCollectionType,
        _Out_ std::shared_ptr<ImplicitProperty>& outItemsProperty
        );

    std::shared_ptr<XamlQualifiedObject> get_NullKeyedResourceXQO();

#pragma endregion

#pragma region External Services: Core, ErrorService

    // DEPRECATED: Returns a via a deref_out pointer but doesn't add a ref on the
    // core. Use GetCore() instead.
    _Check_return_ HRESULT GetCore(
        _Outptr_ CCoreServices **ppCore
        );

    CCoreServices* GetCore();

    _Check_return_ HRESULT GetErrorService(
        _Out_ std::shared_ptr<ParserErrorReporter>& outErrorService
        );

    _Check_return_ HRESULT SetErrorService(
        _In_ const std::shared_ptr<ParserErrorReporter>& inErrorService
        );

#pragma endregion

#pragma region Miscellaneous

    // TODO: Move into the XamlRuntime
    _Check_return_ HRESULT CheckPeerType(
        _In_ const std::shared_ptr<XamlQualifiedObject>& inQO,
        _In_ const xstring_ptr& inssClassName,
        _In_ const bool bIsExactMatchRequired
        );

#pragma endregion

private:
    _Check_return_ HRESULT Initialize();

    _Check_return_ HRESULT FlushXamlNodeStreamCacheManager();

    void InitializeSpecialXmlNamespaceMap();

    _Check_return_ HRESULT InitializeXamlNamespaceDefinitions();

    static
    _Check_return_ HRESULT ExtractTypeNamespace(
        _In_ const xstring_ptr& pstrXmlNs,
        _Out_ xstring_ptr& pstrOutTypeNamespace
        );

    _Check_return_ HRESULT GetFullyQualifiedNameParts(
        _In_ const xstring_ptr& spFullyQualifiedName,
        _Out_ xstring_ptr* pstrOutNamespaceUri,
        _Out_ xstring_ptr* pstrOutTypeName,
        _Out_ xstring_ptr* pstrOutPropertyName
        );

    void set_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty(const bool newValue)
    {
        m_testHook_UseEmptyXamlTypeNamespaceInGetXamlProperty = newValue;
    }

    bool get_TestHook_UseEmptyXamlTypeNamespaceInGetXamlProperty() const
    {
        return m_testHook_UseEmptyXamlTypeNamespaceInGetXamlProperty;
    }

private:
    std::shared_ptr<XamlNativeTypeInfoProvider>  m_spNativeTypeInfoProvider;
    std::shared_ptr<XamlManagedTypeInfoProvider> m_spManagedTypeInfoProvider;

    std::shared_ptr<XamlNativeRuntime>  m_spNativeRuntime;
    std::shared_ptr<XamlManagedRuntime> m_spManagedRuntime;

    std::shared_ptr<XamlType>       m_spStringType;
    std::shared_ptr<XamlType>       m_spInt32Type;
    std::shared_ptr<XamlType>       m_spUnknownType;
    std::shared_ptr<XamlTextSyntax> m_spStringSyntax;
    std::shared_ptr<XamlTextSyntax> m_spInt32Syntax;
    std::shared_ptr<ImplicitProperty>   m_spInitializationProperty;
    std::shared_ptr<ImplicitProperty>   m_spItemsProperty;
    std::shared_ptr<DirectiveProperty>   m_spXKeyProperty;
    std::shared_ptr<DirectiveProperty>   m_spXNameProperty;
    std::shared_ptr<XamlQualifiedObject> m_spNullKeyedResourceXQO;

    std::stack < xstring_ptr, std::vector<xstring_ptr>> m_SourceAssemblies;

    containers::vector_map<xstring_ptr, std::shared_ptr<XamlXmlNamespace>>  m_mapUriToXmlNamespace;

    // map to cache repeatedly referenced fully-qualifed property names.
    containers::vector_map<xstring_ptr, std::shared_ptr<XamlProperty>>  m_mapFullyQualifiedProperty;

    // caches for UnknownTypes/Properties we create when a requested StableXbfIndex is
    // outside the range of indices known to this version of the framework
    containers::vector_map<std::underlying_type<Parser::StableXbfTypeIndex>::type, std::shared_ptr<XamlType>> m_unknownStableXbfTypeIndexes;
    containers::vector_map<std::underlying_type<Parser::StableXbfPropertyIndex>::type, std::shared_ptr<XamlProperty>> m_unknownStableXbfPropertyIndexes;

    // maps of tokens to XamlXxxxx instances

    std::unordered_map<XamlTypeToken, std::shared_ptr<XamlType>> m_mapMasterTypeList;
    std::unordered_map<XamlPropertyToken, std::shared_ptr<XamlProperty>> m_mapMasterMemberList;
    std::unordered_map<XamlTypeNamespaceToken, std::shared_ptr<XamlTypeNamespace>> m_mapMasterTypeNamespaceList;
    std::unordered_map<XamlAssemblyToken, std::shared_ptr<XamlAssembly>> m_mapMasterAssemblyList;

    std::vector< std::shared_ptr<XamlType> >      m_typeVector;
    std::vector< std::shared_ptr<XamlProperty> >  m_memberVector;
    std::vector< std::shared_ptr<XamlNamespace> > m_xmlNamespaceVector;

    IParserCoreServices *m_pCore = nullptr;
    std::shared_ptr<ParserErrorReporter> m_ErrorService;

    bool m_testHook_UseEmptyXamlTypeNamespaceInGetXamlProperty = false;
};

