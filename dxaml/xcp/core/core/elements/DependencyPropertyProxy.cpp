// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "MetadataAPI.h"
#include "XamlServiceProviderContext.h"
#include "XamlQualifiedObject.h"
#include "XamlSchemaContext.h"
#include "XamlType.h"
#include "XamlProperty.h"

// This function will translate the string representation
// of a type into its managed form
_Check_return_ HRESULT CDependencyPropertyProxy::FromString(CREATEPARAMETERS *pCreate)
{
    std::shared_ptr<XamlType> spXamlType;
    std::shared_ptr<XamlProperty> spXamlProperty;
    xstring_ptr spPropertyName;
    std::shared_ptr<XamlSchemaContext> spSchemaContext;
    auto spServiceProviderContext = pCreate->m_spServiceProviderContext;

    xstring_ptr strPropertyName = pCreate->m_value.AsString();

    IFCEXPECT_RETURN(spServiceProviderContext);
    IFC_RETURN(spServiceProviderContext->GetSchemaContext(spSchemaContext));

    // Get the property name and optionally resolve the type if the value was of
    // the form "type.property".
    auto ichDot = strPropertyName.FindChar(L'.');
    if (ichDot != xstring_ptr_view::npos)
    {
        xstring_ptr spTypeName;

        IFC_RETURN(strPropertyName.SubString(0, ichDot, &spTypeName));
        IFC_RETURN(strPropertyName.SubString(ichDot + 1, strPropertyName.GetCount(), &spPropertyName));

        IFC_RETURN(spServiceProviderContext->ResolveXamlType(spTypeName, spXamlType));

        // If the type cannot be resolved then this is a bad property name
        // fail and use the generic type conversion failure error
        if (!spXamlType)
        {
            IFC_RETURN(E_FAIL);
        }
    }
    else
    {
        spPropertyName = strPropertyName;
    }

    // If the type name wasn't included with the property name, we'll check to
    // see if there are any ambient TargetType values (i.e. if we're contained
    // within a Style or ControlTemplate that defined a TargetType property)
    if (!spXamlType)
    {
        KnownTypeIndex typeIndex = KnownTypeIndex::UnknownType;
        std::shared_ptr<XamlQualifiedObject> qoValue;
        IFC_RETURN(spServiceProviderContext->GetAmbientTargetType(qoValue));
        if (!qoValue)
        {
            // Fail and use the use the generic type conersion failure error
            IFC_RETURN(E_FAIL);
        }

        if (qoValue->GetCValuePtr()->GetType() == valueObject)
        {
            CType* type = nullptr;
            IFC_RETURN(DoPointerCast<CType>(type, qoValue->GetValue()));
            if (!type)
            {
                // Fail and use the use the generic type conersion failure error
                IFC_RETURN(E_FAIL);
            }

            typeIndex = type->GetReferencedTypeIndex();
        }
        else if (qoValue->GetCValuePtr()->GetType() == valueTypeHandle)
        {
            typeIndex = qoValue->GetCValuePtr()->AsTypeHandle();
        }
        else
        {
            IFC_RETURN(E_UNEXPECTED);
        }

        if (!spXamlType)
        {
            XamlTypeToken typeToken = XamlTypeToken::FromType(DirectUI::MetadataAPI::GetClassInfoByIndex(typeIndex));
            IFC_RETURN(spSchemaContext->GetXamlType(typeToken, spXamlType));
        }
    }

    // Once we've resolved the type, we can resolve the property name with
    // respect to it.
    if (spXamlType)
    {
        IFC_RETURN(spXamlType->GetDependencyProperty(spPropertyName, spXamlProperty));

        // Report an error like "The property '%0' was not found in type '%1'."
        if (!spXamlProperty)
        {
            xstring_ptr spFullTypeName;
            IFC_RETURN(spXamlType->get_FullName(&spFullTypeName));
            IFC_RETURN(CErrorService::OriginateInvalidOperationError(
                    GetContext(),
                    AG_E_PARSER2_UNKNOWN_PROP_ON_TYPE,
                    spPropertyName,
                    spFullTypeName));
        }

        IFC_RETURN(FromXamlProperty(spXamlProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT
CDependencyPropertyProxy::FromXamlProperty(_In_ const std::shared_ptr<XamlProperty>& spXamlProperty)
{
    // Populate the DependencyPropertyProxy from the XamlProperty
    XamlPropertyToken propertyToken = spXamlProperty->get_PropertyToken();
    if ((propertyToken.GetProviderKind() == tpkNative) || (propertyToken.GetProviderKind() == tpkManaged))
    {
        SetPropertyIndex(propertyToken.GetHandle());
    }
    else
    {
        // Fail and use the use the generic type conersion failure error
        IFC_RETURN(E_FAIL);
    }

    return S_OK;
}

bool CDependencyPropertyProxy::IsEqual(CDependencyPropertyProxy *pDependencyPropertyProxy)
{
    return (GetPropertyIndex() == pDependencyPropertyProxy->GetPropertyIndex());
}

const CDependencyProperty* CDependencyPropertyProxy::GetDP()
{
    const CDependencyProperty* property = DirectUI::MetadataAPI::GetDependencyPropertyByIndex(GetPropertyIndex());

    return property;
}

void CDependencyPropertyProxy::SetDP(_In_ const CDependencyProperty* pDependencyProperty)
{
    SetPropertyIndex(pDependencyProperty->GetIndex());
}
