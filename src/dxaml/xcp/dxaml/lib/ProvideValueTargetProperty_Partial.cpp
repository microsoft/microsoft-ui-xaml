// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include "ParserServiceProvider.g.h"
#include "ProvideValueTargetProperty.g.h"
#include <MetadataApi.h>
#include <XamlType.h>
#include <XamlProperty.h>

using namespace DirectUI;

_Check_return_ HRESULT 
ProvideValueTargetProperty::get_DeclaringTypeImpl(_Out_ wxaml_interop::TypeName* pValue)
{
    std::shared_ptr<XamlType> propertyDeclaringType;

    IFC_RETURN(m_xamlProperty->get_DeclaringType(propertyDeclaringType));
    auto propertyDeclaringTypeInfo = MetadataAPI::GetClassInfoByIndex(propertyDeclaringType->get_TypeToken().GetHandle());
    IFC_RETURN(MetadataAPI::GetTypeNameByClassInfo(propertyDeclaringTypeInfo, pValue));

    return S_OK;
}

_Check_return_ HRESULT 
ProvideValueTargetProperty::get_NameImpl(_Out_ HSTRING* pValue)
{
    xstring_ptr propertyName;
    IFC_RETURN(m_xamlProperty->get_Name(&propertyName));

    xruntime_string_ptr propertyNamePromoted;
    IFC_RETURN(propertyName.Promote(&propertyNamePromoted));
    *pValue = propertyNamePromoted.DetachHSTRING();

    return S_OK;
}

_Check_return_ HRESULT 
ProvideValueTargetProperty::get_TypeImpl(_Out_ wxaml_interop::TypeName* pValue)
{
    std::shared_ptr<XamlType> propertyType;

    IFC_RETURN(m_xamlProperty->get_Type(propertyType));
    auto propertyTypeInfo = MetadataAPI::GetClassInfoByIndex(propertyType->get_TypeToken().GetHandle());
    IFC_RETURN(MetadataAPI::GetTypeNameByClassInfo(propertyTypeInfo, pValue));

    return S_OK;
}

_Check_return_ HRESULT 
ProvideValueTargetProperty::Initialize(std::shared_ptr<XamlProperty> xamlProperty)
{
    if (!xamlProperty)
    {
        return E_INVALIDARG;

    }
    m_xamlProperty = xamlProperty;

    return S_OK;
}