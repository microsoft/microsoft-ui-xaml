// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xamlxmlnamespace.h"

class XamlSchemaContext;
class XamlTypeNamespace;
class XamlType;
class XamlProperty;

// Represents the special XML namespace
// (http://www.w3.org/XML/1998/namespace) and the markup XML namespace
// (http://schemas.microsoft.com/winfx/2006/xaml XAML markup).
class XamlSpecialXmlNamespace final
    : public XamlXmlNamespace
{
public:
    XamlSpecialXmlNamespace() = default;

    XamlSpecialXmlNamespace(
        const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext,
        _In_ const xstring_ptr& inXmlNamespaceUri)
        : XamlXmlNamespace(inXamlSchemaContext, inXmlNamespaceUri, xstring_ptr::EmptyString())
        , m_eSpecialXmlNamespaceKind(sxkNone)
    {
        if (XSTRING_PTR_FROM_STORAGE(c_strXmlUri).Equals(inXmlNamespaceUri))
        {
            m_eSpecialXmlNamespaceKind = sxkXml;
        }
    }

    ~XamlSpecialXmlNamespace() override
    {
    }

    _Check_return_ HRESULT AddTypeNamespace(
        const std::shared_ptr<XamlTypeNamespace> inTypeNamespace
        );

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

    std::shared_ptr<XamlNamespace> Clone() const override;

private:
    XamlSpecialXmlNamespaceKind m_eSpecialXmlNamespaceKind;
    containers::vector_map<xstring_ptr, std::shared_ptr<DirectiveProperty>> m_mapDirectiveProperties;
};

// Represents the http://schemas.microsoft.com/winfx/2006/xaml XAML markup
// namespace (typically prefixed with x:).
class XamlMarkupXmlNamespace final
    : public XamlXmlNamespace
{
public:
    XamlMarkupXmlNamespace() = default;

    XamlMarkupXmlNamespace(
        _In_ const std::shared_ptr<XamlSchemaContext>& spSchemaContext,
        _In_ const xstring_ptr& spNamespaceUri)
            : XamlXmlNamespace(spSchemaContext, spNamespaceUri, xstring_ptr::EmptyString())
    {
    }

    _Check_return_ HRESULT AddTypeNamespace(
        _In_ const std::shared_ptr<XamlTypeNamespace> spTypeNamespace);

    bool get_IsResolved() const override;

    _Check_return_ HRESULT GetXamlType(
        _In_ const xstring_ptr& spTypeName,
        _Out_ std::shared_ptr<XamlType>& spType) override;

    _Check_return_ HRESULT GetDirectiveType(
        _In_ const xstring_ptr& spTypeName,
        _Out_ std::shared_ptr<XamlType>& spType) override;

    _Check_return_ HRESULT GetDirectiveProperty(
        _In_ const xstring_ptr_view& spPropertyName,
        _Out_ std::shared_ptr<DirectiveProperty>& spProperty) override;

    std::shared_ptr<XamlNamespace> Clone() const override;

private:
    containers::vector_map<xstring_ptr, std::shared_ptr<XamlType>> m_mapDirectiveTypes;
    containers::vector_map<xstring_ptr, std::shared_ptr<DirectiveProperty>> m_mapDirectiveProperties;
};


