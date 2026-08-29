// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{


_Check_return_ HRESULT
PickerFlyoutPresenter::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IContentControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IContentControl> spDelegatingInner;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;

    IFC(PickerFlyoutPresenterGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_ContentControl).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<IPickerFlyoutPresenter*>(this)),
        &spNonDelegatingInnerInspectable,
        &spDelegatingInner));

    IFC(SetComposableBasePointers(
            spNonDelegatingInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spNonDelegatingInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.PickerFlyoutPresenter"));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyoutPresenter::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;
    wrl::ComPtr<xaml_controls::IBorder> spBackgroundBorder;
    wrl::ComPtr<xaml::IDependencyObject> spTitlePresenterAsDO;

    _tpTitlePresenter.Clear();

    IFC(PickerFlyoutPresenterGenerated::OnApplyTemplateImpl());

    IFC(GetComposableBase().As(&spControlProtected));

    IFC(spControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(L"TitlePresenter").Get(),
        &spTitlePresenterAsDO));

    if (spTitlePresenterAsDO)
    {
        wrl::ComPtr<xaml_controls::ITextBlock> spTitlePresenter;
        IGNOREHR(spTitlePresenterAsDO.As(&spTitlePresenter));
        if (spTitlePresenter)
        {
            wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
            IFC(spTitlePresenter.As(&spPresenterAsUI));
            IFC(spPresenterAsUI->put_Visibility(_title.IsEmpty() ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
            IFC(spTitlePresenter->put_Text(_title.Get()));
            IFC(SetPtrValue(_tpTitlePresenter, spTitlePresenter.Get()));
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyoutPresenter::SetTitle(_In_ HSTRING title)
{
    HRESULT hr = S_OK;

    _title.Set(title);
    if (_tpTitlePresenter)
    {
        wrl::ComPtr<xaml::IUIElement> spPresenterAsUI;
        IFC(_tpTitlePresenter.As(&spPresenterAsUI));
        IFC(spPresenterAsUI->put_Visibility(_title.IsEmpty() ? xaml::Visibility_Collapsed : xaml::Visibility_Visible));
        IFC(_tpTitlePresenter->put_Text(_title));
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyoutPresenter::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_automation_peers::PickerFlyoutPresenterAutomationPeer> spPickerFlyoutPresenterAutomationPeer;

    IFC(wrl::MakeAndInitialize<xaml_automation_peers::PickerFlyoutPresenterAutomationPeer>
            (&spPickerFlyoutPresenterAutomationPeer, this));

    IFC(spPickerFlyoutPresenterAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

}}}} XAML_ABI_NAMESPACE_END
