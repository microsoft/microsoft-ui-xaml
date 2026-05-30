// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "XamlQualifiedObject.h"
#include "XamlServiceProviderContext.h"


_Check_return_ HRESULT 
CTemplateBindingExtension::ProvideValue(
            _In_ const std::shared_ptr<XamlServiceProviderContext>& spServiceProviderContext,
            _Out_ std::shared_ptr<XamlQualifiedObject>& qoValue)
{
    auto qo = std::make_shared<XamlQualifiedObject>(XamlTypeToken(tpkNative, KnownTypeIndex::TemplateBinding));
    IFC_RETURN(qo->SetDependencyObject(this));

    qoValue = std::move(qo);

    return S_OK;
}

_Check_return_ HRESULT 
CTemplateBindingExtension::SetTemplateBinding(
    _In_ CDependencyObject* pDependencyObject, 
    _In_ XamlPropertyToken tokTargetProperty)
{
    IFCEXPECT_RETURN(m_pSourceProperty);
    
    IFC_RETURN(pDependencyObject->SetTemplateBinding(
        DirectUI::MetadataAPI::GetDependencyPropertyByIndex(tokTargetProperty.GetHandle()),
        m_pSourceProperty->GetDP()));

    return S_OK;
}

void CTemplateBindingExtension::Reset()
{
    ASSERT(m_pSourceProperty == NULL || m_fTemplateBindingComplete);
    ReleaseInterface(m_pSourceProperty);
    m_fTemplateBindingComplete = FALSE; 
}

