// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//
//      Represents the special XML namespace
//      (http://www.w3.org/XML/1998/namespace) and the markup XML namespace
//      (http://schemas.microsoft.com/winfx/2006/xaml XAML markup).

#include "precomp.h"
#include "XamlSpecialXmlNamespace.h"

_Check_return_ HRESULT 
XamlSpecialXmlNamespace::AddTypeNamespace(
    const std::shared_ptr<XamlTypeNamespace> inTypeNamespace)
{
    return E_NOTIMPL;
}

bool XamlSpecialXmlNamespace::get_IsResolved() const
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
_Check_return_ HRESULT 
XamlSpecialXmlNamespace::GetXamlType(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ std::shared_ptr<XamlType>& outType)
{
    outType = std::shared_ptr<XamlType>();

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
XamlSpecialXmlNamespace::GetDirectiveType(
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
XamlSpecialXmlNamespace::GetDirectiveProperty(
    _In_ const xstring_ptr_view& inPropertyName, 
    _Out_ std::shared_ptr<DirectiveProperty>& outXamlProperty)
{
    // Initialize the m_mapDirectiveProperties mapping the first time that anyone
    // requests a directive property
    if (m_mapDirectiveProperties.size() == 0)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTextSyntax> spXamlTextSyntax;
        std::shared_ptr<XamlType> spTypeString;
        std::shared_ptr<DirectiveProperty> spNewDirective;
        std::shared_ptr<XamlNamespace> spXamlSpecialXmlNamespace;

        spSchemaContext = m_spXamlSchemaContext.lock();
        if (!spSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }
        spXamlSpecialXmlNamespace = shared_from_this();  
        
        IFC_RETURN(spSchemaContext->get_StringSyntax(spXamlTextSyntax));
        IFC_RETURN(spSchemaContext->get_StringXamlType(spTypeString));
        

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strLang, L"lang");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strLang, spTypeString, spXamlSpecialXmlNamespace, spXamlTextSyntax, xdLang, spNewDirective));
        m_mapDirectiveProperties[c_strLang] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));
        
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strBase, L"base");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strBase, spTypeString, spXamlSpecialXmlNamespace, spXamlTextSyntax, xdBase, spNewDirective));
        m_mapDirectiveProperties[c_strBase] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));
        
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strSpace, L"space");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strSpace, spTypeString, spXamlSpecialXmlNamespace, spXamlTextSyntax, xdSpace, spNewDirective));
        m_mapDirectiveProperties[c_strSpace] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));
    }

    // Lookup the directive in the map by name
    {
        auto itOutXamlProperty = m_mapDirectiveProperties.find(inPropertyName);
        if (itOutXamlProperty == m_mapDirectiveProperties.end())
        {
            outXamlProperty = nullptr;
            return S_FALSE;
        }
        else{
            outXamlProperty = itOutXamlProperty->second;
        }
    }

    return S_OK;
}

std::shared_ptr<XamlNamespace> XamlSpecialXmlNamespace::Clone() const
{
    auto clone = std::make_shared<XamlSpecialXmlNamespace>();
    clone->m_xamlPredicateAndArgs = m_xamlPredicateAndArgs;
    clone->m_conditionalPredicateString = m_conditionalPredicateString;
    clone->m_uiRuntimeIndex = m_uiRuntimeIndex;
    clone->m_resolvedConditionalPredicate = m_resolvedConditionalPredicate;

    clone->m_spXamlSchemaContext = m_spXamlSchemaContext;
    clone->m_spXmlNamespaceUri = m_spXmlNamespaceUri;
    clone->m_typeNamespaceList = m_typeNamespaceList;
    clone->m_mapNameToXamlType = m_mapNameToXamlType;
    clone->m_mapKnownNotFoundTypes = m_mapKnownNotFoundTypes;

    clone->m_eSpecialXmlNamespaceKind = m_eSpecialXmlNamespaceKind;
    clone->m_mapDirectiveProperties = m_mapDirectiveProperties;

    return clone;
}

//------------------------------------------------------------------------
//
//  Method:   XamlMarkupXmlNamespace::AddTypeNamespace
//
//  Synopsis:
//      This method is not implemented.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlMarkupXmlNamespace::AddTypeNamespace(
    const std::shared_ptr<XamlTypeNamespace> spTypeNamespace)
{
    return E_NOTIMPL;
}


//------------------------------------------------------------------------
//
//  Method:   XamlMarkupXmlNamespace::get_IsResolved
//
//  Synopsis:
//      Always returns TRUE becasue the markup namespace is always resolved.
//
//------------------------------------------------------------------------
bool XamlMarkupXmlNamespace::get_IsResolved() const
{
    return true;
}

//------------------------------------------------------------------------
//
//  Method:   XamlMarkupXmlNamespace::GetXamlType
//
//  Synopsis:
//      Attempt to resolve x:<primitive> types.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlMarkupXmlNamespace::GetXamlType(
    _In_ const xstring_ptr& spTypeName, 
    _Out_ std::shared_ptr<XamlType>& spType)
{
    // For now, this is only enabled in Jupiter for scheduling reasons.

    // Initialize the m_mapDirectiveTypes mapping the first time that anyone
    // requests a directive property.
    // We should refactor this into XamlSchemaContext if in the future we want to use 
    // different sets of types between products (Silverlight, Jupiter, ...).
    if (m_mapDirectiveTypes.size() == 0)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeNamespace> spXamlNamespace;
        std::shared_ptr<XamlType> spDirective;

        spSchemaContext = m_spXamlSchemaContext.lock();
        if (!spSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strBoolean, L"Boolean");
        IFC_RETURN(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::Boolean), spXamlNamespace, c_strBoolean, spDirective));
        m_mapDirectiveTypes[c_strBoolean] = spDirective;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strInt32, L"Int32");
        IFC_RETURN(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::Int32), spXamlNamespace, c_strInt32, spDirective));
        m_mapDirectiveTypes[c_strInt32] = spDirective;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strDouble, L"Double");
        IFC_RETURN(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::Double), spXamlNamespace, c_strDouble, spDirective));
        m_mapDirectiveTypes[c_strDouble] = spDirective;

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strString, L"String");
        IFC_RETURN(spSchemaContext->GetXamlType(XamlTypeToken(tpkNative, KnownTypeIndex::String), spXamlNamespace, c_strString, spDirective));
        m_mapDirectiveTypes[c_strString] = spDirective;
    }

    // Lookup the directive in the map by name
    {
        auto itSpType = m_mapDirectiveTypes.find(spTypeName);
        if (itSpType == m_mapDirectiveTypes.end())
        {
            spType = nullptr;
            return S_FALSE;
        }
        else{
            spType = itSpType->second;
        }
    }

    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   XamlMarkupXmlNamespace::GetDirectiveType
//
//  Synopsis:
//      Attempt to resolve directive types by name.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlMarkupXmlNamespace::GetDirectiveType(
    _In_ const xstring_ptr& spTypeName, 
    _Out_ std::shared_ptr<XamlType>& spType)
{
    if (spTypeName.Equals(XSTRING_PTR_EPHEMERAL(L"Null")))
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlTypeNamespace> spXamlNamespace;
        XamlTypeToken typeToken(tpkNative, KnownTypeIndex::NullExtension);
        
        spSchemaContext = m_spXamlSchemaContext.lock();
        if (!spSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }
        IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spXamlNamespace, spTypeName, spType));
        spType->SetIsDirective();
    }
    
    return S_OK;
}


//------------------------------------------------------------------------
//
//  Method:   XamlMarkupXmlNamespace::GetDirectiveProperty
//
//  Synopsis:
//      Attempt to resolve directive properties by name.
//
//------------------------------------------------------------------------
_Check_return_ HRESULT 
XamlMarkupXmlNamespace::GetDirectiveProperty(
    _In_ const xstring_ptr_view& spPropertyName, 
    _Out_ std::shared_ptr<DirectiveProperty>& spProperty)
{    
    // Initialize the m_mapDirectiveProperties mapping the first time that anyone
    // requests a directive property
    if (m_mapDirectiveProperties.size() == 0)
    {
        std::shared_ptr<XamlSchemaContext> spSchemaContext;
        std::shared_ptr<XamlNamespace> spNullNamespace; // TODO: The normal XamlProperty has a stubbed out namespace proeprty too.
        std::shared_ptr<DirectiveProperty> spNewDirective;
        std::shared_ptr<XamlTextSyntax> spXamlStringSyntax;
        std::shared_ptr<XamlTextSyntax> spXamlInt32Syntax;
        std::shared_ptr<XamlType> spTypeString;
        std::shared_ptr<XamlType> spTypeInt32;
        
        spSchemaContext = m_spXamlSchemaContext.lock();
        if (!spSchemaContext)
        {
            IFC_RETURN(E_FAIL);
        }
        IFC_RETURN(spSchemaContext->get_StringSyntax(spXamlStringSyntax));
        IFC_RETURN(spSchemaContext->get_StringXamlType(spTypeString));
        IFC_RETURN(spSchemaContext->get_Int32Syntax(spXamlInt32Syntax));
        IFC_RETURN(spSchemaContext->get_Int32XamlType(spTypeInt32));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strKey, L"Key");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strKey, spTypeString, spNullNamespace, spXamlStringSyntax, xdKey, spNewDirective));
        m_mapDirectiveProperties[c_strKey] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));
        
        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strName, L"Name");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strName, spTypeString, spNullNamespace, spXamlStringSyntax, xdName, spNewDirective));
        m_mapDirectiveProperties[c_strName] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strClass, L"Class");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strClass, spTypeString, spNullNamespace, spXamlStringSyntax, xdClass, spNewDirective));
        m_mapDirectiveProperties[c_strClass] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strUid, L"Uid");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strUid, spTypeString, spNullNamespace, spXamlStringSyntax, xdUid, spNewDirective));
        m_mapDirectiveProperties[c_strUid] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strDeferLoadStrategy, L"DeferLoadStrategy");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strDeferLoadStrategy, spTypeString, spNullNamespace, spXamlStringSyntax, xdDeferLoadStrategy, spNewDirective));
        m_mapDirectiveProperties[c_strDeferLoadStrategy] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strLoad, L"Load");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strLoad, spTypeString, spNullNamespace, spXamlStringSyntax, xdLoad, spNewDirective));
        m_mapDirectiveProperties[c_strLoad] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strClassModifier, L"ClassModifier");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strClassModifier, spTypeString, spNullNamespace, spXamlStringSyntax, xdClassModifier, spNewDirective));
        m_mapDirectiveProperties[c_strClassModifier] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strFieldModifier, L"FieldModifier");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strFieldModifier, spTypeString, spNullNamespace, spXamlStringSyntax, xdFieldModifier, spNewDirective));
        m_mapDirectiveProperties[c_strFieldModifier] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));

        DECLARE_CONST_STRING_IN_FUNCTION_SCOPE(c_strConnectionId, L"ConnectionId");
        IFC_RETURN(DirectiveProperty::Create(spSchemaContext, c_strConnectionId, spTypeInt32, spNullNamespace, spXamlInt32Syntax, xdConnectionId, spNewDirective));
        m_mapDirectiveProperties[c_strConnectionId] = spNewDirective;
        IFC_RETURN(spSchemaContext->RegisterXamlProperty(spNewDirective));
    }

    // Lookup the directive in the map by name
    {
        auto itSpProperty = m_mapDirectiveProperties.find(spPropertyName);
        if (itSpProperty == m_mapDirectiveProperties.end())
        {
            spProperty = nullptr;
            return S_FALSE;
        }
        else{
            spProperty = itSpProperty->second;
        }
    }

    return S_OK;
}

std::shared_ptr<XamlNamespace> XamlMarkupXmlNamespace::Clone() const
{
    auto clone = std::make_shared<XamlMarkupXmlNamespace>();
    clone->m_xamlPredicateAndArgs = m_xamlPredicateAndArgs;
    clone->m_conditionalPredicateString = m_conditionalPredicateString;
    clone->m_uiRuntimeIndex = m_uiRuntimeIndex;
    clone->m_resolvedConditionalPredicate = m_resolvedConditionalPredicate;

    clone->m_spXamlSchemaContext = m_spXamlSchemaContext;
    clone->m_spXmlNamespaceUri = m_spXmlNamespaceUri;
    clone->m_typeNamespaceList = m_typeNamespaceList;
    clone->m_mapNameToXamlType = m_mapNameToXamlType;
    clone->m_mapKnownNotFoundTypes = m_mapKnownNotFoundTypes;

    clone->m_mapDirectiveTypes = m_mapDirectiveTypes;
    clone->m_mapDirectiveProperties = m_mapDirectiveProperties;

    return clone;
}