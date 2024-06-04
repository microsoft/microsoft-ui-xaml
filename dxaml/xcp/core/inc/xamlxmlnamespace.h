// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <XamlNamespace.h>
#include <XamlSchemaContext.h>
#include <XamlTypeNamespace.h>
#include <XamlProperty.h>
#include <XamlType.h>

class XamlXmlNamespace 
    : public XamlNamespace
{
public:
    XamlXmlNamespace() = default;

    ~XamlXmlNamespace() override
    {
    }

    XamlXmlNamespace(
        const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext,
        _In_ const xstring_ptr& inXmlNamespaceUri,
        _In_ const xstring_ptr& inConditionalPredicateString,
        size_t runtimeIndex)
        : XamlNamespace(runtimeIndex, inConditionalPredicateString)
        , m_spXamlSchemaContext(inXamlSchemaContext)
        , m_spXmlNamespaceUri(inXmlNamespaceUri)
        , m_bIsImplicitlyResolved(FALSE)
        , m_bIsImplicitlyResolvedComputed(FALSE)
    {
    }

    XamlXmlNamespace(
        const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext,
        _In_ const xstring_ptr& inXmlNamespaceUri,
        _In_ const xstring_ptr& inConditionalPredicateString)
        : XamlNamespace(inConditionalPredicateString)
        , m_spXamlSchemaContext(inXamlSchemaContext)
        , m_spXmlNamespaceUri(inXmlNamespaceUri)
        , m_bIsImplicitlyResolved(FALSE)
        , m_bIsImplicitlyResolvedComputed(FALSE)
    {
    }

    _Check_return_ HRESULT AddTypeNamespace(const std::shared_ptr<XamlTypeNamespace> inTypeNamespace);

    bool get_IsResolved() const override;

    _Check_return_ HRESULT GetXamlType(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ std::shared_ptr<XamlType>& outType) override;

    _Check_return_ HRESULT GetDirectiveType(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ std::shared_ptr<XamlType>& outType) override;


    _Check_return_ HRESULT GetDirectiveProperty(
        _In_ const xstring_ptr_view& inPropertyName, 
        _Out_ std::shared_ptr<DirectiveProperty>& ppOut) override;

    XamlXmlNamespace* AsXamlXmlNamespace() override { return static_cast<XamlXmlNamespace*>(this); }

    std::shared_ptr<XamlNamespace> get_OriginalXamlXmlNamespace() override
    {
        return shared_from_this();
    }

    _Check_return_ HRESULT GetIsImplicitlyResolved(_Out_ bool* pbIsImplicitlyResolved);

    std::shared_ptr<XamlNamespace> Clone() const override;

protected:

    xstring_ptr get_TargetNamespaceCore() const override;

private:
    _Check_return_ HRESULT GetXamlTypeImpl(
        _In_ const xstring_ptr& inTypeName, 
        _Out_ std::shared_ptr<XamlType>& outType);
    
    bool m_bIsImplicitlyResolved : 1;
    bool m_bIsImplicitlyResolvedComputed : 1;

protected:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    xstring_ptr m_spXmlNamespaceUri;
    std::vector<std::shared_ptr<XamlTypeNamespace>> m_typeNamespaceList;
    containers::vector_map<xstring_ptr, std::shared_ptr<XamlType>> m_mapNameToXamlType;

    // m_mapKnownNotFoundTypes:  this is a list of types that were previously
    // looked up, and not found.  Keeping track of these cuts down in unneeded
    // reflection in cases where we have to look up a type by multiple names
    // (e.g.: Foo/FooExtension for MarkupExtension).
    containers::vector_set<xstring_ptr> m_mapKnownNotFoundTypes;
};

