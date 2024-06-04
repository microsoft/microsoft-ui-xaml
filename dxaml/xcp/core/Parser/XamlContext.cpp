// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

HRESULT XamlContext::PopulateDefaultNamespaces()
{
    std::shared_ptr<XamlNamespace> xmlDirectiveNamespace;
    std::shared_ptr<XamlSchemaContext> schemaContext;

    // TODO: Sets a local assembly?
    ////    ctx._localAssembly = localAssembly;

    // TODO: Hardcoded string - is this the right uri? (XmlDirectives.Uri)
    schemaContext = m_SchemaContext.lock();
    if (!schemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    IFC_RETURN(schemaContext->GetXamlXmlNamespace(c_strXmlUri, xmlDirectiveNamespace));
    IFC_RETURN(AddNamespacePrefix(c_strXmlPrefix, xmlDirectiveNamespace));

    return S_OK;
}

HRESULT XamlContext::get_SchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_SchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

HRESULT XamlContext::GetErrorService(std::shared_ptr<ParserErrorReporter>& outErrorService)
{
    std::shared_ptr<XamlSchemaContext> schemaContext;
    
    IFC_RETURN(get_SchemaContext(schemaContext));
    IFC_RETURN(schemaContext->GetErrorService(outErrorService));

    return S_OK;
}

HRESULT XamlContext::GetXamlProperty(
            _In_ const std::shared_ptr<XamlType>& inXamlType, 
            _In_ const xstring_ptr& inPropertyName, 
            _Out_ std::shared_ptr<XamlProperty>& outXamlProperty)
{
    RRETURN(inXamlType->GetProperty(inPropertyName, outXamlProperty));
}
HRESULT XamlContext::GetXamlAttachableProperty(
            _In_ const std::shared_ptr<XamlType>& inXamlType, 
            _In_ const xstring_ptr& inPropertyName, 
            _Out_ std::shared_ptr<XamlProperty>& outXamlProperty)
{
    RRETURN(inXamlType->GetAttachableProperty(inPropertyName, outXamlProperty));
}

HRESULT XamlContext::GetXamlType(const std::shared_ptr<XamlName>& inTypeName, std::shared_ptr<XamlType>& outXamlType)
{
    std::shared_ptr<XamlNamespace> xamlNamespace;

    IFC_RETURN(GetXamlType(inTypeName, xamlNamespace, outXamlType));

    return S_OK;
}

HRESULT XamlContext::GetXamlType(const std::shared_ptr<XamlName>& inTypeName, std::shared_ptr<XamlNamespace>& outXamlNamespace, std::shared_ptr<XamlType>& outXamlType)
{
    RRETURN(GetXamlType(inTypeName->get_Prefix(), inTypeName->get_Name(), outXamlNamespace, outXamlType));
}

HRESULT XamlContext::GetXamlType(_In_ const xstring_ptr& inPrefix,
                            _In_ const xstring_ptr& inName,
                            std::shared_ptr<XamlNamespace>& outXamlNamespace,
                            std::shared_ptr<XamlType>& outXamlType)
{
    // Returns S_FALSE and null in the out params on failure.
    outXamlNamespace = FindNamespaceByPrefix(inPrefix);

    if (!outXamlNamespace)
    {
        return S_FALSE;
    }
    else
    {
        IFC_RETURN(outXamlNamespace->GetXamlType(inName, outXamlType));
    }

    return S_FALSE;
}



