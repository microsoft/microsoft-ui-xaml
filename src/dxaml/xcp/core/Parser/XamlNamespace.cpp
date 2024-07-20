// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlNamespace.h"
#include "XamlPredicateService.h"
#include "XamlPredicateHelpers.h"

//------------------------------------------------------------------------
//
//  Method:   get_TargetNamespace
//
//  Synopsis:
//      Gets the Target Namespace name that this XamlNamespace represents.
//      The string that the namespace represents is specific to the Class 
//      derived from XamlNamespace.
//
//------------------------------------------------------------------------
//
xstring_ptr XamlNamespace::get_TargetNamespace() const
{
    return get_TargetNamespaceCore();
}


//------------------------------------------------------------------------
//
//  Method:   GetTypeExtensionName
//
//  Synopsis:
//      This is for general MarkupExtension handling:  It just takes 
//      the type-name and appends "Extension" to it.
//
//------------------------------------------------------------------------
//
_Check_return_ HRESULT XamlNamespace::GetTypeExtensionName(
    _In_ const xstring_ptr& inTypeName, 
    _Out_ xstring_ptr* pstrOutName)
{
    IFC_RETURN(xstring_ptr::Concatenate(
            inTypeName, 0,
            XSTRING_PTR_EPHEMERAL(L"Extension"), 0,
            pstrOutName));

    return S_OK;
}

// Resolves the conditional predicate string associated with this XamlNamespace into
// a concrete XamlType and arguments string
void XamlNamespace::ResolveConditionalPredicate(_In_ const std::shared_ptr<XamlParserContext>& xamlContext)
{
    if (!m_conditionalPredicateString.IsNullOrEmpty() && !m_resolvedConditionalPredicate)
    {
        std::shared_ptr<XamlType> predicateType;
        xstring_ptr predicateArgs;

        Parser::XamlPredicateService::CrackConditionalPredicate(
            xamlContext,
            m_conditionalPredicateString,
            predicateType,
            predicateArgs);

        m_xamlPredicateAndArgs = std::make_shared<Parser::XamlPredicateAndArgs>(predicateType, predicateArgs);

        m_resolvedConditionalPredicate = true;
    }
}