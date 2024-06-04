// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

_Check_return_ HRESULT 
XamlAssembly::GetTypeNamespace_ForBinaryXaml(
    const std::shared_ptr<XamlAssembly>& spAssembly,
    _In_ const xstring_ptr& inTypeNamespace,
    std::shared_ptr<XamlTypeNamespace>& outXamlTypeNamespace
    )
{
    std::shared_ptr<XamlSchemaContext> spXamlSchemaContext;
    std::shared_ptr<XamlTypeInfoProvider> spXamlTypeInfoProvider;
    XamlTypeNamespaceToken tokXamlTypeNamespace;

    IFC_RETURN(GetSchemaContext(spXamlSchemaContext));
    IFC_RETURN(spXamlSchemaContext->GetTypeInfoProvider(m_XamlAssemblyToken.GetProviderKind(), spXamlTypeInfoProvider));

    IFC_RETURN(spXamlTypeInfoProvider->GetTypeNamespace(m_spAssemblyName, inTypeNamespace, tokXamlTypeNamespace));
    IFC_RETURN(spXamlSchemaContext->GetXamlTypeNamespace(tokXamlTypeNamespace, inTypeNamespace, spAssembly, std::shared_ptr<XamlXmlNamespace>(), outXamlTypeNamespace));

    if (!outXamlTypeNamespace)
    {
        IFC_RETURN(spXamlSchemaContext->GetTypeInfoProvider(tpkManaged, spXamlTypeInfoProvider));
        IFC_RETURN(spXamlTypeInfoProvider->GetTypeNamespace(m_spAssemblyName, inTypeNamespace, tokXamlTypeNamespace));
        IFC_RETURN(spXamlSchemaContext->GetXamlTypeNamespace(tokXamlTypeNamespace, inTypeNamespace, spAssembly, std::shared_ptr<XamlXmlNamespace>(), outXamlTypeNamespace));
    }

    return S_OK;
}

_Check_return_ HRESULT 
XamlAssembly::get_Name(_Out_ xstring_ptr* pstrOutName)
{
    IFCEXPECT_RETURN(!m_spAssemblyName.IsNull());
    *pstrOutName = m_spAssemblyName;
    
    return S_OK;
}

_Check_return_ HRESULT 
XamlAssembly::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

