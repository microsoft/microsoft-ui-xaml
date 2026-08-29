// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MetadataAPI.h"
#include "XamlNamespace.h"
#include "XamlServiceProviderContext.h"
#include "XamlType.h"

//-------------------------------------------------------------------------
//
//  Function:   CType::FromString
//
//  Synopsis:   This function will translate the string representation
//              of a type into its managed form
//
//-------------------------------------------------------------------------
_Check_return_ HRESULT CType::FromString(_In_ CREATEPARAMETERS *pCreate)
{
    std::shared_ptr<XamlServiceProviderContext> spServiceProviderContext = pCreate->m_spServiceProviderContext;
    xstring_ptr spQName;
    std::shared_ptr<XamlType> spXamlType;

    IFCEXPECT_RETURN(spServiceProviderContext);
    spQName = pCreate->m_value.AsString();

    IFC_RETURN(spServiceProviderContext->ResolveXamlType(spQName, spXamlType));
    // TODO: Report a specific error message (WPF uses "Type reference cannot find public type named 'TypeNameWithoutXmlnsPrefixHere'." though it also does so via x:Type )
    IFCEXPECT_RETURN(spXamlType);

    IFC_RETURN(FromXamlType(spXamlType));

    return S_OK;
}

_Check_return_ HRESULT
CType::FromXamlType(_In_ const std::shared_ptr<XamlType>& spXamlType)
{
    m_nTypeIndex = spXamlType->get_TypeToken().GetHandle();
    return S_OK;
}
