// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "XamlTypeNamespace.h"

bool XamlTypeNamespace::get_IsResolved() const
{
    return true;
}


//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
// TODO: there may be some value in caching the type
// at this level, but there would really only be a benefit if the 
// same Type-Namespace showed up in multiple XmlnsDefinition Attributes
_Check_return_ HRESULT 
XamlTypeNamespace::GetXamlType(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ std::shared_ptr<XamlType>& outType)
{   
    std::shared_ptr<XamlTypeInfoProvider> spTypeInfoProvider;
    XamlTypeToken ttType;
    std::shared_ptr<XamlSchemaContext> schemaContext;
    
    XamlTypeInfoProviderKind typeInfoProviderKind;
    
    schemaContext = m_spXamlSchemaContext.lock();
    if (!schemaContext)
    {
        IFC_RETURN(E_FAIL);
    }
    IFC_RETURN(GetTypeInfoProviderKind(typeInfoProviderKind));
    IFC_RETURN(schemaContext->GetTypeInfoProvider(typeInfoProviderKind, spTypeInfoProvider));
    IFC_RETURN(spTypeInfoProvider->ResolveTypeName(m_XamlTypeNamespaceToken, inTypeName, ttType));

    if (ttType.GetHandle() != KnownTypeIndex::UnknownType)
    {
        IFC_RETURN(schemaContext->GetXamlType(ttType, shared_from_this(), inTypeName, outType));
    }
    else
    {
        outType.reset();
    }

    return S_OK;
}



//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT 
XamlTypeNamespace::GetDirectiveType(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ std::shared_ptr<XamlType>& outType)
{
    return E_NOTIMPL;
}



//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT 
XamlTypeNamespace::GetDirectiveProperty(
    _In_ const xstring_ptr_view& inPropertyName, 
    _Out_ std::shared_ptr<DirectiveProperty>& ppOut)
{
    return E_NOTIMPL;
}


// TODO: double-check that the using-namespace is the 
// correct value for the TargetNamespace property
//------------------------------------------------------------------------
//
//  Method:   Xxxx
//
//  Synopsis:
//      Xxx
//
//------------------------------------------------------------------------
//
xstring_ptr XamlTypeNamespace::get_TargetNamespaceCore() const
{
    return m_spTypeNamespaceString;
}


_Check_return_ HRESULT 
XamlTypeNamespace::GetTypeInfoProviderKind(XamlTypeInfoProviderKind& outTypeInfoProviderKind)
{
    // TODO: This may not even need to be done.
    // This might be able to get from namespace token.

    //std::shared_ptr<XamlAssembly> spOwningAssembly;
    //
    //IFC(GetOwningAssembly(spOwningAssembly));
    //   
    //outTypeInfoProviderKind = spOwningAssembly->GetTypeInfoProviderKind();

    outTypeInfoProviderKind = m_XamlTypeNamespaceToken.GetProviderKind();
    
    return S_OK;
}

_Check_return_ HRESULT
XamlTypeNamespace::GetOwningAssembly(std::shared_ptr<XamlAssembly>& outOwningAssembly)
{
    // TODO: expired() doesn't seem right here ... we're checking for the 
    // case where the the m_spXamlNamespace hasn't been initialized at all.
    if (m_spOwningAssembly.expired())
    {
        std::shared_ptr<XamlAssembly> spAssembly;
        
        if (!m_XamlTypeNamespaceToken.IsEmpty())
        {
            std::shared_ptr<XamlSchemaContext> spSchemaContext;
            XamlAssemblyToken sXamlAssemblyToken;
            
            IFC_RETURN(GetSchemaContext(spSchemaContext));
            sXamlAssemblyToken = XamlAssemblyToken(m_XamlTypeNamespaceToken.AssemblyToken.GetProviderKind(),
                                                   m_XamlTypeNamespaceToken.AssemblyToken.GetHandle());
    
            IFC_RETURN(spSchemaContext->GetXamlAssembly(sXamlAssemblyToken, spAssembly));
        }
        m_spOwningAssembly = spAssembly;
    }
    
    outOwningAssembly = m_spOwningAssembly.lock();
    if (!outOwningAssembly)
    {
        IFC_RETURN(E_FAIL);
    }
    
    return S_OK;
}

_Check_return_ HRESULT 
XamlTypeNamespace::GetSchemaContext(std::shared_ptr<XamlSchemaContext>& outSchemaContext)
{
    outSchemaContext = m_spXamlSchemaContext.lock();
    if (!outSchemaContext)
    {
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

std::shared_ptr<XamlNamespace> XamlTypeNamespace::Clone() const
{
    auto clone = std::make_shared<XamlTypeNamespace>();
    clone->m_xamlPredicateAndArgs = m_xamlPredicateAndArgs;
    clone->m_conditionalPredicateString = m_conditionalPredicateString;
    clone->m_uiRuntimeIndex = m_uiRuntimeIndex;
    clone->m_resolvedConditionalPredicate = m_resolvedConditionalPredicate;

    clone->m_spXamlSchemaContext = m_spXamlSchemaContext;
    clone->m_XamlTypeNamespaceToken = m_XamlTypeNamespaceToken;
    clone->m_spOwningAssembly = m_spOwningAssembly;
    clone->m_spTypeNamespaceString = m_spTypeNamespaceString;
    clone->m_originalXamlXmlNamespace = m_originalXamlXmlNamespace;

    return clone;
}