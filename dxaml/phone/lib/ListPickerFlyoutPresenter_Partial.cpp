// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

_Check_return_ HRESULT
ListPickerFlyoutPresenter::InitializeImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlFactory> spInnerFactory;
    wrl::ComPtr<xaml_controls::IControl> spDelegatingInner;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    wrl::ComPtr<xaml::IDataTemplate> spListViewTemplate;
    wrl::ComPtr<xaml::IDependencyObject> spItemsHostAsI;
    wrl::ComPtr<xaml_controls::IListView> spItemsHost;
    wrl::ComPtr<xaml_controls::IListViewBase> spItemsHostAsLVB;
    wrl::ComPtr<xaml_primitives::IFlyoutBasePrivate> spFlyoutBasePrivate;

    IFC(ListPickerFlyoutPresenterGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Control).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IInspectable*>(static_cast<IListPickerFlyoutPresenter*>(this)),
        &spNonDelegatingInnerInspectable,
        &spDelegatingInner));

    IFC(SetComposableBasePointers(
            spNonDelegatingInnerInspectable.Get(),
            spInnerFactory.Get()));

    IFC(Private::SetDefaultStyleKey(
            spNonDelegatingInnerInspectable.Get(),
            L"Microsoft.UI.Xaml.Controls.ListPickerFlyoutPresenter"));

    IFC(Private::ApplicationResourceHelpers::GetApplicationResource(
        wrl_wrappers::HStringReference(L"ListPickerFlyoutPresenterContentTemplate").Get(),
        spListViewTemplate.GetAddressOf()));

    IFC(spListViewTemplate->LoadContent(&spItemsHostAsI));
    IFC(spItemsHostAsI.As(&spItemsHostAsLVB));
    IFC(put_ItemsHost(spItemsHostAsLVB.Get()));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
ListPickerFlyoutPresenter::OnApplyTemplateImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::IControlProtected> spControlProtected;
    wrl::ComPtr<xaml::IDependencyObject> spItemsHostPanelAsDO;
    wrl::ComPtr<xaml::IDependencyObject> spTitlePresenterAsDO;
    wrl::ComPtr<xaml_controls::IListViewBase> spItemsHost;
    wrl::ComPtr<xaml::IUIElement> spItemsHostAsUI;
    wrl::ComPtr<wfc::IVector<xaml::UIElement*>> spItemsHostPanelChildren;

    _tpTitlePresenter.Clear();
    if (_tpItemsHostPanel)
    {
        // Clean the ItemsHost out of the previous template
        IFC(_tpItemsHostPanel->get_Children(&spItemsHostPanelChildren));
        IFC(spItemsHostPanelChildren->Clear());
        spItemsHostPanelChildren.Reset();
        _tpItemsHostPanel.Clear();
    }

    IFC(ListPickerFlyoutPresenterGenerated::OnApplyTemplateImpl());

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

    IFC(spControlProtected->GetTemplateChild(
        wrl_wrappers::HStringReference(L"ItemsHostPanel").Get(),
        &spItemsHostPanelAsDO));
    if (spItemsHostPanelAsDO)
    {
        wrl::ComPtr<xaml_controls::IPanel> spItemsHostPanel;
        IGNOREHR(spItemsHostPanelAsDO.As(&spItemsHostPanel));
        IFC(SetPtrValue(_tpItemsHostPanel, spItemsHostPanel.Get()));
    }

    IFC(get_ItemsHost(&spItemsHost));
    if (_tpItemsHostPanel && spItemsHost)
    {
        // Inject the ItemsHost into the template
        IFC(spItemsHost.As(&spItemsHostAsUI));
        IFC(_tpItemsHostPanel->get_Children(&spItemsHostPanelChildren));
        IFC(spItemsHostPanelChildren->Append(spItemsHostAsUI.Get()));
    }

Cleanup:
    RRETURN(hr);
}


_Check_return_ HRESULT
ListPickerFlyoutPresenter::SetTitle(_In_ HSTRING title)
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
ListPickerFlyoutPresenter::OnCreateAutomationPeerImpl(
    _Outptr_ xaml::Automation::Peers::IAutomationPeer **returnValue)
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_automation_peers::ListPickerFlyoutPresenterAutomationPeer> spListPickerFlyoutPresenterAutomationPeer;

    IFC(wrl::MakeAndInitialize<xaml_automation_peers::ListPickerFlyoutPresenterAutomationPeer>
            (&spListPickerFlyoutPresenterAutomationPeer, this));

    IFC(spListPickerFlyoutPresenterAutomationPeer.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}

}}}} XAML_ABI_NAMESPACE_END
