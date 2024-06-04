// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "xamlxmlnamespace.h"

class XamlSchemaContext;
class XamlUnknownXmlNamespace;
class XamlProperty;
class XamlType;

class XamlUnknownXmlNamespace final
    : public XamlXmlNamespace
{
public:
    XamlUnknownXmlNamespace() = default;

    XamlUnknownXmlNamespace(
        const std::shared_ptr<XamlSchemaContext>& inXamlSchemaContext,
        _In_ const xstring_ptr& inXmlNamespaceUri
        )
        : XamlXmlNamespace(inXamlSchemaContext, inXmlNamespaceUri, xstring_ptr::EmptyString())
    {
    }

    ~XamlUnknownXmlNamespace() override {}

    bool get_IsResolved() const override;

    // TODO: verify that we will be able to do without the typeArgs.  This seems to be used for Templates
    // virtual HRESULT GetXamlType(_In_ const xstring_ptr& inTypeName, XamlType[]typeArgs, _Out_ std::shared_ptr<XamlType& outType) = 0;
    _Check_return_ HRESULT GetXamlType(
        _In_ const xstring_ptr& inTypeName,
        _Out_ std::shared_ptr<XamlType>& outType) override;

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual HRESULT GetAllXamlTypes(_Out_ IEnumerable<XamlType>** ppOut) = 0;

    _Check_return_ HRESULT GetDirectiveType(
        _In_ const xstring_ptr& inTypeName,
        _Out_ std::shared_ptr<XamlType>& outType) override;

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual HRESULT GetAllDirectiveTypes(_Out_ IEnumerable<XamlType>** ppOut) = 0;

    _Check_return_ HRESULT GetDirectiveProperty(
        _In_ const xstring_ptr_view& inPropertyName,
        _Out_ std::shared_ptr<DirectiveProperty>& ppOut) override;

    // TODO: verify that this isn't needed.  Seems to be used for reflection
    // virtual HRESULT GetAllDirectiveProperties(_Out_ IEnumerable<XamlProperty>** ppOut) = 0;

    std::shared_ptr<XamlNamespace> Clone() const override;

protected:

    xstring_ptr get_TargetNamespaceCore() const override;

    HRESULT GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);
};


