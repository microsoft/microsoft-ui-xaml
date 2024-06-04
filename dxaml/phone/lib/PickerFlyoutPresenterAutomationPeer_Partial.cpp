// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Automation { namespace Peers
{

_Check_return_ HRESULT
PickerFlyoutPresenterAutomationPeer::InitializeImpl(
    _In_ xaml_controls::IPickerFlyoutPresenter* pOwner)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeerFactory> spInnerFactory;
    wrl::ComPtr<xaml::Automation::Peers::IFrameworkElementAutomationPeer> spInnerInstance;
    wrl::ComPtr<xaml::IFrameworkElement> spPickerFlyoutPresenterAsFE;
    wrl::ComPtr<xaml_controls::IPickerFlyoutPresenter> spOwner(pOwner);
    wrl::ComPtr<IInspectable> spInnerInspectable;

    ARG_NOTNULL(pOwner, "pOwner");

    IFC(PickerFlyoutPresenterAutomationPeerGenerated::InitializeImpl(pOwner));

    IFC(wf::GetActivationFactory(
          wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Automation_Peers_FrameworkElementAutomationPeer).Get(),
          &spInnerFactory));

    IFC((static_cast<IInspectable*>(pOwner))->QueryInterface<xaml::IFrameworkElement>(
        &spPickerFlyoutPresenterAsFE));

    IFC(spInnerFactory->CreateInstanceWithOwner(
            spPickerFlyoutPresenterAsFE.Get(),
            static_cast<IPickerFlyoutPresenterAutomationPeer*>(this),
            &spInnerInspectable,
            &spInnerInstance));

    IFC(SetComposableBasePointers(
            spInnerInspectable.Get(),
            spInnerFactory.Get()));

Cleanup:
    RRETURN(hr);
}

#pragma region IAutomationPeerOverrides

_Check_return_ HRESULT
PickerFlyoutPresenterAutomationPeer::GetAutomationControlTypeCoreImpl(
    _Out_ xaml::Automation::Peers::AutomationControlType *returnValue)
{

    *returnValue = xaml::Automation::Peers::AutomationControlType_Pane;
    RRETURN(S_OK);
}

_Check_return_ HRESULT
PickerFlyoutPresenterAutomationPeer::GetClassNameCoreImpl(
    _Out_ HSTRING *returnValue)
{
    HRESULT hr = S_OK;

    IFC(wrl_wrappers::HStringReference(L"PickerFlyoutPresenter").CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyoutPresenterAutomationPeer::GetNameCoreImpl(
    _Out_ HSTRING *returnValue)
{
    HRESULT hr = S_OK;

    IFC(Private::FindStringResource(
        UIA_AP_PICKERFLYOUT_NAME,
        returnValue));

Cleanup:
    RRETURN(hr);
}

#pragma endregion

} } } } } XAML_ABI_NAMESPACE_END

