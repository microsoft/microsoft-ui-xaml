// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlNamespace.h"
#include "XamlSchemaContext.h"
#include "XamlXmlNamespace.h"

class XamlAssembly;

class XamlTypeNamespace final
    : public XamlNamespace
{
public:
    XamlTypeNamespace() = default;

    XamlTypeNamespace(
        const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext,
        _In_ const xstring_ptr& inTypeNamespaceString,
        const XamlTypeNamespaceToken inTypeNamespaceToken,
        const std::shared_ptr<XamlAssembly>& inOwningAssembly,
        const std::shared_ptr<XamlNamespace>& originalXamlXmlNamespace)
    : m_spXamlSchemaContext(inXamlSchemaContext)
    , m_spTypeNamespaceString(inTypeNamespaceString)
    , m_spOwningAssembly(inOwningAssembly)
    , m_XamlTypeNamespaceToken(inTypeNamespaceToken)
    , m_originalXamlXmlNamespace(originalXamlXmlNamespace)
    {
    }

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

    _Check_return_ HRESULT GetOwningAssembly(std::shared_ptr<XamlAssembly>& outOwningAssembly);

    const XamlTypeNamespaceToken& get_TypeNamespaceToken() const
    {
        return m_XamlTypeNamespaceToken;
    }

    std::shared_ptr<XamlNamespace> get_OriginalXamlXmlNamespace() override
    {
        return m_originalXamlXmlNamespace.lock();
    }

    void set_OriginalXamlXmlNamespace(std::shared_ptr<XamlNamespace> originalXamlXmlNamespace)
    {
        m_originalXamlXmlNamespace = originalXamlXmlNamespace;
    }

    std::shared_ptr<XamlNamespace> Clone() const override;

protected:

    xstring_ptr get_TargetNamespaceCore() const override;

    _Check_return_ HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);

private:
    // TODO: I make spOwningAssembly weak_ptr but it may not even be
    // needed, seems the typeinfoproviderkind it gets from it
    // is also in the namespace token.

    _Check_return_ HRESULT GetTypeInfoProviderKind(XamlTypeInfoProviderKind& outTypeInfoProviderKind);
private:
    std::weak_ptr<XamlSchemaContext> m_spXamlSchemaContext;
    std::weak_ptr<XamlAssembly> m_spOwningAssembly;
    std::weak_ptr<XamlNamespace> m_originalXamlXmlNamespace;
    xstring_ptr m_spTypeNamespaceString;
    XamlTypeNamespaceToken m_XamlTypeNamespaceToken;
};