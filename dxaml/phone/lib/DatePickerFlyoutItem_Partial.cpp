// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

_Check_return_ HRESULT
DatePickerFlyoutItem::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyObjectFactory> spInnerFactory;
    wrl::ComPtr<xaml::IDependencyObject> spInnerInstance;
    wrl::ComPtr<IInspectable> spInnerInspectable;

    IFC(DatePickerFlyoutItemGenerated::InitializeImpl());
    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_DependencyObject).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IDatePickerFlyoutItem*>(this),
        &spInnerInspectable,
        &spInnerInstance));

    IFC(SetComposableBasePointers(
        spInnerInspectable.Get(),
        spInnerFactory.Get()));

Cleanup:
    return hr;
}

_Check_return_ HRESULT
DatePickerFlyoutItem::GetCustomPropertyImpl(_In_ HSTRING name, _Outptr_ xaml::Data::ICustomProperty** returnValue)
{
    UNREFERENCED_PARAMETER(name);
    ARG_VALIDRETURNPOINTER(returnValue);
    *returnValue = nullptr;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
DatePickerFlyoutItem::GetIndexedPropertyImpl(_In_ HSTRING name, _In_ wxaml_interop::TypeName type, _Outptr_ xaml::Data::ICustomProperty** returnValue)
{
    UNREFERENCED_PARAMETER(name);
    UNREFERENCED_PARAMETER(type);
    ARG_VALIDRETURNPOINTER(returnValue);
    *returnValue = nullptr;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
DatePickerFlyoutItem::GetStringRepresentationImpl(_Out_ HSTRING* plainText)
{
    HRESULT hr = S_OK;
    wrl_wrappers::HString primaryText;
    wrl_wrappers::HString primaryTextWithSpace;
    wrl_wrappers::HString secondaryText;
    wrl_wrappers::HString outputText;

    IFC(get_PrimaryText(primaryText.GetAddressOf()));
    IFC(get_SecondaryText(secondaryText.GetAddressOf()));

    if (primaryText.Get() != nullptr && secondaryText.Get() != nullptr)
    {
        IFC(primaryText.Concat(wrl_wrappers::HStringReference(L" "), primaryTextWithSpace));
        IFC(primaryTextWithSpace.Concat(secondaryText, outputText));
    }
    else if (primaryText.Get() != nullptr)
    {
        IFC(primaryText.CopyTo(outputText.ReleaseAndGetAddressOf()));
    }
    else if (secondaryText.Get() != nullptr)
    {
        IFC(secondaryText.CopyTo(outputText.ReleaseAndGetAddressOf()));
    }
    else
    {
        IFC(wrl_wrappers::HStringReference(L"").CopyTo(outputText.ReleaseAndGetAddressOf()));
    }

    outputText.CopyTo(plainText);

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
DatePickerFlyoutItem::get_TypeImpl(_Out_ wxaml_interop::TypeName* type)
{
    HRESULT hr = S_OK;
    type->Kind = wxaml_interop::TypeKind_Primitive;
    IFC(wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_DatePickerFlyoutItem).CopyTo(&type->Name));

Cleanup:
    RRETURN(hr);
}

} } } } XAML_ABI_NAMESPACE_END
