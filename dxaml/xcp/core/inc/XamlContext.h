// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class XamlNamespace;
class XamlSchemaContext;
class XamlName;
class XamlType;
class XamlProperty;
class ParserErrorReporter;

class XamlContext 
{
public:
    enum UsageMode 
    { 
        umParser, 
        umBuilder, 
        umSymbolTable 
    };
  
    virtual HRESULT AddNamespacePrefix(
                _In_ const xstring_ptr& inPrefix, 
                const std::shared_ptr<XamlNamespace>& inXamlNamespace) = 0;

    virtual std::shared_ptr<XamlNamespace> FindNamespaceByPrefix(
                _In_ const xstring_ptr& inPrefix) = 0;

    HRESULT get_SchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext);

    
    HRESULT GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService);

    HRESULT GetXamlType(
                const std::shared_ptr<XamlName>& inTypeName, 
                std::shared_ptr<XamlType>& outXamlType);

    HRESULT GetXamlType(
                const std::shared_ptr<XamlName>& inTypeName, 
                std::shared_ptr<XamlNamespace>& outXamlNamespace, 
                std::shared_ptr<XamlType>& outXamlType);

    HRESULT GetXamlProperty(
                _In_ const std::shared_ptr<XamlType>& inXamlType, 
                _In_ const xstring_ptr& inPropertyName, 
                _Out_ std::shared_ptr<XamlProperty>& outXamlProperty);

    HRESULT GetXamlAttachableProperty(
                _In_ const std::shared_ptr<XamlType>& inXamlType, 
                _In_ const xstring_ptr& inPropertyName, 
                _Out_ std::shared_ptr<XamlProperty>& outXamlProperty);

protected:

    XamlContext(const std::shared_ptr<XamlSchemaContext>& inSchemaContext)
        : m_SchemaContext(inSchemaContext)
    {

    }

    virtual ~XamlContext()
    {
    }

    HRESULT PopulateDefaultNamespaces();

    HRESULT GetXamlType(_In_ const xstring_ptr& prefix,
                                _In_ const xstring_ptr& name,
                                std::shared_ptr<XamlNamespace>& outXamlNamespace,
                                std::shared_ptr<XamlType>& outXamlType);
                                

private:
    std::weak_ptr<XamlSchemaContext> m_SchemaContext;
};

