// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <RuntimeProfiler.h>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls
{

PickerFlyout::PickerFlyout() :
    _asyncOperationManager(FlyoutAsyncOperationManager<bool, PickerFlyout, PickerFlyoutShowAtAsyncOperationName>(Private::ReferenceTrackerHelper<PickerFlyout>(this)))
{
    __RP_Marker_ClassByName("PickerFlyout");
}

PickerFlyout::~PickerFlyout()
{

}

_Check_return_ HRESULT
PickerFlyout::InitializeImpl()
{
    HRESULT hr = S_OK;
    EventRegistrationToken openingToken = { };
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseFactory> spInnerFactory;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBase> spDelegatingInner;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    IFC(PickerFlyoutGenerated::InitializeImpl());

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
        static_cast<IPickerFlyout*>(this),
        &spNonDelegatingInnerInspectable,
        &spDelegatingInner));

    IFC(SetComposableBasePointers(
        spNonDelegatingInnerInspectable.Get(),
        spInnerFactory.Get()));

     IFC(QueryInterface(
        __uuidof(xaml_primitives::IFlyoutBase),
        &spFlyoutBase));

    IFC(spFlyoutBase->add_Opening(
        wrl::Callback<wf::IEventHandler<IInspectable*>>
        (this, &PickerFlyout::OnOpening).Get(),
        &openingToken));

    IFC(_asyncOperationManager.Initialize(
        spFlyoutBase.Get(),
        // Cancellation value provider function
        [] () -> bool
        {
            return false;
        }));

    IFC(spFlyoutBase->put_ShouldConstrainToRootBounds(TRUE));

Cleanup:
    RRETURN(hr);
}


// -----
// PickerFlyout Impl
// -----

_Check_return_ HRESULT
PickerFlyout::ShowAtAsyncImpl(
    _In_ xaml::IFrameworkElement* pTarget,
    _Outptr_ wf::IAsyncOperation<bool>** returnValue)
{
    HRESULT hr = S_OK;

    IFC(_asyncOperationManager.Start(pTarget, returnValue));

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyout::OnOpening(
    _In_ IInspectable* /* pSender */,
    _In_ IInspectable* /* pArgs */)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml::IDependencyObject> spThisAsDO;
    wrl::ComPtr<xaml_controls::IContentControl> spFlyoutPresenterAsCC;
    wrl::ComPtr<xaml_primitives::IPickerFlyoutBaseStatics> spPickerFlyoutBaseStatics;
    wrl::ComPtr<xaml::IUIElement> spContent;
    wrl_wrappers::HString title;

    ASSERT(_tpFlyoutPresenter, "Expected non-null presenter");

    IFC(wf::GetActivationFactory(
        wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_PickerFlyoutBase).Get(),
        &spPickerFlyoutBaseStatics));
    IFC(QueryInterface(__uuidof(xaml::IDependencyObject), &spThisAsDO));
    IFC(spPickerFlyoutBaseStatics->GetTitle(spThisAsDO.Get(), title.GetAddressOf()));
    IFC(get_Content(&spContent));

    IFC(_tpFlyoutPresenter.As(&spFlyoutPresenterAsCC));
    IFC(static_cast<PickerFlyoutPresenter*>(_tpFlyoutPresenter.Get())->SetTitle(title.Get()));
    IFC(spFlyoutPresenterAsCC->put_Content(spContent.Get()));

Cleanup:
    RRETURN(hr);
}

// -----
// IPickerFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
PickerFlyout::OnConfirmedImpl()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_controls::PickerConfirmedEventArgs> spArgs;

    IFC(_asyncOperationManager.Complete(true));
    IFC(wrl::MakeAndInitialize<xaml_controls::PickerConfirmedEventArgs>(&spArgs));
    IFC(m_ConfirmedEventSource.InvokeAll(this, spArgs.Get()));

    IFC(PickerFlyoutGenerated::OnConfirmedImpl());

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT
PickerFlyout::ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* result)
{
    RRETURN(get_ConfirmationButtonsVisible(result));
}

// -----
// IFlyoutBaseOverrides Impl
// -----

_Check_return_ HRESULT
PickerFlyout::CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue)
{
    HRESULT hr = S_OK;

    if (!_tpFlyoutPresenter)
    {
        wrl::ComPtr<IPickerFlyoutPresenter> spFlyoutPresenter;
        IFC(wrl::MakeAndInitialize<PickerFlyoutPresenter>(&spFlyoutPresenter));
        IFC(SetPtrValue(_tpFlyoutPresenter, spFlyoutPresenter.Get()));
    }
    IFC(_tpFlyoutPresenter.CopyTo(returnValue));

Cleanup:
    RRETURN(hr);
}



}}}} XAML_ABI_NAMESPACE_END
