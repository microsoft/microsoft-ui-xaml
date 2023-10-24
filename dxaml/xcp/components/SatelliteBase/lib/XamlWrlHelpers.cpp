// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include <NamespaceAliases.h>
#include <Microsoft.UI.Xaml.h>

#include <XamlWrlHelpers.h>
#include <ValueBoxer.h>

namespace Private
{
    HRESULT
    InitializeDependencyProperty(
        _In_ LPCWSTR propertyNameString,
        _In_ LPCWSTR propertyTypeNameString,
        _In_ LPCWSTR ownerTypeNameString,
        bool isAttached,
        _In_opt_ IInspectable* defaultValue,
        _In_opt_ xaml::IPropertyChangedCallback* propertyChangedCallback,
        _COM_Outptr_ xaml::IDependencyProperty** returnValue)
    {
        wrl::ComPtr<IInspectable> propertyMetadataInner;
        wrl::ComPtr<xaml::IPropertyMetadata> propertyMetadata;
        wrl_wrappers::HString propertyName;

        wrl::ComPtr<xaml::IDependencyPropertyStatics> dependencyPropertyStatics;

        wxaml_interop::TypeName propertyType = {};
        wrl_wrappers::HString propertyTypeName;

        wxaml_interop::TypeName ownerType = {};
        wrl_wrappers::HString ownerTypeName;

        wrl::ComPtr<xaml::IPropertyMetadataFactory> propertyMetadataFactory;

        IFCPTR_RETURN(returnValue);
        *returnValue = nullptr;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_PropertyMetadata).Get(),
            &propertyMetadataFactory));

        IFC_RETURN(propertyMetadataFactory->CreateInstanceWithDefaultValueAndCallback(
            defaultValue,
            propertyChangedCallback,
            nullptr /* outer */,
            &propertyMetadataInner,
            &propertyMetadata));

        IFC_RETURN(propertyTypeName.Set(propertyTypeNameString));

        IFC_RETURN(ownerTypeName.Set(ownerTypeNameString));

        propertyType.Name = propertyTypeName.Get();
        propertyType.Kind = wxaml_interop::TypeKind_Metadata;

        ownerType.Name = ownerTypeName.Get();
        ownerType.Kind = wxaml_interop::TypeKind_Metadata;

        IFC_RETURN(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyProperty).Get(),
            &dependencyPropertyStatics));

        IFC_RETURN(propertyName.Set(propertyNameString));

        if (isAttached)
        {
            IFC_RETURN(dependencyPropertyStatics->RegisterAttached(
                propertyName.Get(),
                propertyType,
                ownerType,
                propertyMetadata.Get(),
                returnValue));
        }
        else
        {
            IFC_RETURN(dependencyPropertyStatics->Register(
                propertyName.Get(),
                propertyType,
                ownerType,
                propertyMetadata.Get(),
                returnValue));
        }

        return S_OK;
    }

    HRESULT
    SetDefaultStyleKey(
        _In_ IInspectable* pInspectable,
        _In_ LPCWSTR pwszDefaultStyleKey)
    {
        wrl::ComPtr<IInspectable> spInspectable(pInspectable);
        wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;
        wrl::ComPtr<IInspectable> spPropertyValue;

        IFCPTR_RETURN(pwszDefaultStyleKey);

        IFC_RETURN(spInspectable.As(&spControlProtected));
        if (nullptr != spControlProtected)
        {
            wrl_wrappers::HString value;

            IFC_RETURN(value.Set(pwszDefaultStyleKey));
            IFC_RETURN(ValueBoxer::CreateString(value.Get(), &spPropertyValue));

            IFC_RETURN(spControlProtected->put_DefaultStyleKey(spPropertyValue.Get()));
        }         

        return S_OK;
    }

    HRESULT
    GetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _Out_ wrl::ComPtr<wf::IPropertyValue>* propertyValue)
    {
        wrl::ComPtr<IInspectable> propertyValueAsInspectable;

        IFC_RETURN(GetDependencyPropertyValue(
            dependencyObjectAsInspectable,
            dependencyProperty,
            &propertyValueAsInspectable));

        // null IPropertyValue is incorrect on value-type DPs 
        // and mean the calling code is written incorrectly 
        IFC_RETURN(propertyValueAsInspectable.As(propertyValue));

        return S_OK;
    }

    HRESULT
    GetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _Out_ wrl::ComPtr<IInspectable>* propertyValue)
    {
        wrl::ComPtr<xaml::IDependencyObject> dependencyObject;
        wrl::ComPtr<IInspectable> inspectablePropertyValue;

        IFC_RETURN(dependencyObjectAsInspectable.As(
            &dependencyObject));

        IFC_RETURN(dependencyObject->GetValue(
            dependencyProperty.Get(),
            &inspectablePropertyValue));

        // In some cases GetValue will return a nullptr in
        // place of an actual IInspectable. CopyTo will set
        // propertyValue to null in these cases.
        IFC_RETURN(inspectablePropertyValue.CopyTo(propertyValue->GetAddressOf()));

        return S_OK;
    }

    HRESULT
    SetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _In_ wrl::ComPtr<wf::IPropertyValue> propertyValue)
    {
        wrl::ComPtr<IInspectable> propertyValueAsInspectable;
        IFC_RETURN(propertyValue.As(&propertyValueAsInspectable));
        IFC_RETURN(SetDependencyPropertyValue(
            dependencyObjectAsInspectable,
            dependencyProperty,
            propertyValueAsInspectable));

        return S_OK;
    }

    HRESULT
    SetDependencyPropertyValue(
        _In_ wrl::ComPtr<IInspectable> dependencyObjectAsInspectable,
        _In_ wrl::ComPtr<xaml::IDependencyProperty> dependencyProperty,
        _In_ wrl::ComPtr<IInspectable> propertyValue)
    {
        HRESULT hr = S_OK;
        wrl::ComPtr<xaml::IDependencyObject> dependencyObject;

        IFC(dependencyObjectAsInspectable.As(
            &dependencyObject));

        IFC(dependencyObject->SetValue(
            dependencyProperty.Get(),
            propertyValue.Get()))

    Cleanup:
        RRETURN(hr);
    }

}

