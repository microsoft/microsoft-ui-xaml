// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlUnknownXmlNamespace.h"

bool XamlUnknownXmlNamespace::get_IsResolved() const
{
    return false;
}

HRESULT XamlUnknownXmlNamespace::GetXamlType(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ std::shared_ptr<XamlType>& outType
    )
{
    std::shared_ptr<XamlSchemaContext> schemaContext;

    IFC_RETURN(GetSchemaContext(schemaContext));
    IFC_RETURN(UnknownType::Create(schemaContext, shared_from_this(), inTypeName, outType));

    return S_OK;

}

HRESULT XamlUnknownXmlNamespace::GetDirectiveType(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ std::shared_ptr<XamlType>& outType
    )
{
    HRESULT hr = S_OK;
    
    // TODO: we can't return E_NOTIMPL
    ////IFC(E_NOTIMPL);
    
////Cleanup:
    RRETURN(hr);    
}

HRESULT XamlUnknownXmlNamespace::GetDirectiveProperty(
    _In_ const xstring_ptr_view& inPropertyName, 
    _Out_ std::shared_ptr<DirectiveProperty>& ppOut
    )
{
    // TODO: I think this can remain not impl.
    RRETURN(S_OK);
}

xstring_ptr XamlUnknownXmlNamespace::get_TargetNamespaceCore() const
{
    return m_spXmlNamespaceUri;
}


HRESULT XamlUnknownXmlNamespace::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    return S_OK;
}

std::shared_ptr<XamlNamespace> XamlUnknownXmlNamespace::Clone() const
{
    auto clone = std::make_shared<XamlUnknownXmlNamespace>();
    clone->m_xamlPredicateAndArgs = m_xamlPredicateAndArgs;
    clone->m_conditionalPredicateString = m_conditionalPredicateString;
    clone->m_uiRuntimeIndex = m_uiRuntimeIndex;
    clone->m_resolvedConditionalPredicate = m_resolvedConditionalPredicate;

    clone->m_spXamlSchemaContext = m_spXamlSchemaContext;
    clone->m_spXmlNamespaceUri = m_spXmlNamespaceUri;
    clone->m_typeNamespaceList = m_typeNamespaceList;
    clone->m_mapNameToXamlType = m_mapNameToXamlType;
    clone->m_mapKnownNotFoundTypes = m_mapKnownNotFoundTypes;

    return clone;
}