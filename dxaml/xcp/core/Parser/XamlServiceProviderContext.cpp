// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlServiceProviderContext.h>
#include <ObjectWriterContext.h>
#include "INamescope.h"

XamlServiceProviderContext::XamlServiceProviderContext(_In_ const std::shared_ptr<ObjectWriterContext>& spObjectWriterContext)
    : m_spObjectWriterContext(spObjectWriterContext)
{
}

HRESULT XamlServiceProviderContext::GetSchemaContext(
    _Out_ std::shared_ptr<XamlSchemaContext>& rspSchemaContext)
{
    IFC_RETURN(GetObjectWriterContext()->get_SchemaContext(rspSchemaContext));

    return S_OK;
}

std::shared_ptr<XamlQualifiedObject> XamlServiceProviderContext::GetRootObject()
{
    return GetObjectWriterContext()->get_RootInstance();
}

HRESULT XamlServiceProviderContext::ResolveXamlType(
    _In_ const xstring_ptr_view& spQName,
    _Out_ std::shared_ptr<XamlType>& rspType)
{
    IFC_RETURN(GetObjectWriterContext()->ServiceProvider_ResolveXamlType(spQName, rspType));

    return S_OK;
}

xref_ptr<IPALUri> XamlServiceProviderContext::GetBaseUri()
{
    return GetObjectWriterContext()->get_BaseUri();
}

HRESULT XamlServiceProviderContext::GetAmbientTargetType(
    _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue)
{
    IFC_RETURN(GetObjectWriterContext()->ServiceProvider_GetAmbientTargetType(rqoValue));

    return S_OK;
}

HRESULT XamlServiceProviderContext::GetAllAmbientValues(
    _In_ std::shared_ptr<XamlType> spCeilingType,
    _In_ std::shared_ptr<XamlProperty> spProperty,
    _In_ std::shared_ptr<XamlType> spTypeToFind,
    _In_ CompressedStackCacheHint eCacheHint,
    _Outref_ AmbientValuesVector& rvecValues)
{
    IFC_RETURN(GetObjectWriterContext()->ServiceProvider_GetAllAmbientValues(spCeilingType, spProperty, spTypeToFind, eCacheHint, rvecValues));

    return S_OK;
}

HRESULT XamlServiceProviderContext::GetAmbientValue(
    _In_ const std::shared_ptr<XamlType>& spCeilingType1,
    _In_ const std::shared_ptr<XamlType>& spCeilingType2,
    _In_ const std::shared_ptr<XamlProperty>& spProperty1,
    _In_ const std::shared_ptr<XamlProperty>& spProperty2,
    _In_ const std::shared_ptr<XamlType>& spTypeToFind,
    _In_ CompressedStackCacheHint cacheHint,
    _Out_ std::shared_ptr<XamlQualifiedObject>& rqoValue)
{
    IFC_RETURN(GetObjectWriterContext()->ServiceProvider_GetAmbientValue(
        spCeilingType1,
        spCeilingType2,
        spProperty1,
        spProperty2,
        spTypeToFind,
        cacheHint,
        rqoValue));

    return S_OK;
}

void XamlServiceProviderContext::GetMarkupExtensionTargetObject(
    // Reference to the property info of the property being parsed.
    _Out_ std::shared_ptr<XamlQualifiedObject>& spTargetObject)
{
    auto spObjectWriterContext = GetObjectWriterContext();

    if (spObjectWriterContext->get_Depth() > 1)
    {
        spTargetObject = spObjectWriterContext->Parent().get_Instance();
    }
    else
    {
        spTargetObject.reset();
    }
}

void XamlServiceProviderContext::GetMarkupExtensionTargetProperty(
    // Reference to the property info of the property being parsed.
    _Out_ std::shared_ptr<XamlProperty>& spPropertyInfo)
{
    auto spObjectWriterContext = GetObjectWriterContext();

    if (spObjectWriterContext->get_Depth() > 1)
    {
        spPropertyInfo = spObjectWriterContext->Parent().get_Member();
    }
    else
    {
        spPropertyInfo.reset();
    }
}

HRESULT XamlServiceProviderContext::ResolveDependencyProperty(
    _In_ const xstring_ptr& spQName,
    _In_ const std::shared_ptr<XamlType>& spTargetType,
    _Out_ std::shared_ptr<XamlProperty>& rspProperty)
{
    xstring_ptr spPropertyName;
    std::shared_ptr<XamlType> spXamlType;

    IFC_RETURN(ExtractTargetTypeAndPropertyName(spQName, spXamlType, &spPropertyName));

    // If the type name wasn't included with the property name, we'll check to
    // see if there are any ambient TargetType values (i.e. if we're contained
    // within a Style or ControlTemplate that defined a TargetType property)
    if (!spXamlType && !spTargetType)
    {
        // Fail and use the use the generic type conersion failure error
        IFC_RETURN(E_FAIL);
    }
    else if (!spXamlType)
    {
        spXamlType = spTargetType;
    }

    // Once we've resolved the type, we can resolve the property name with
    // respect to it.
    if (spXamlType)
    {
        // If we couldn't resolve a "type.property" name, then we must have a simple
        // property name with a supplied type
        if (spPropertyName.IsNullOrEmpty())
        {
            spPropertyName = spQName;
        }

        IFC_RETURN(spXamlType->GetDependencyProperty(spPropertyName, rspProperty));
    }

    return S_OK;
}

_Check_return_ HRESULT XamlServiceProviderContext::ExtractTargetTypeAndPropertyName(
    _In_ const xstring_ptr_view& propertyName,
    _Out_ std::shared_ptr<XamlType>& spType,
    _Out_ xstring_ptr* pPropertyName)
{
    // Get the property name and optionally resolve the type if the value was of
    // the form "type.property".
    auto ichDot = propertyName.FindChar(L'.');
    if (ichDot != xstring_ptr_view::npos)
    {
        xstring_ptr typeName;
        IFC_RETURN(propertyName.SubString(0, ichDot, &typeName));
        IFC_RETURN(propertyName.SubString(ichDot + 1, propertyName.GetCount(), pPropertyName));

        IFC_RETURN(ResolveXamlType(typeName, spType));

        // If the type cannot be resolved then this is a bad property name
        // fail and use the generic type conversion failure error
        IFCCHECK_RETURN(spType);
    }
    else
    {
        spType = nullptr;
        *pPropertyName = xstring_ptr();
    }

    return S_OK;
}

HRESULT XamlServiceProviderContext::ResolveProperty(
    _In_ const xstring_ptr_view& fullyQualifiedName,
    _Out_ std::shared_ptr<XamlProperty>& rspProperty)
{
    xstring_ptr propertyName;
    std::shared_ptr<XamlType> spXamlType;

    IFC_RETURN(ExtractTargetTypeAndPropertyName(fullyQualifiedName, spXamlType, &propertyName));
    IFCCHECK_RETURN(spXamlType);

    IFC_RETURN(spXamlType->GetProperty(propertyName, rspProperty));

    return S_OK;
}
