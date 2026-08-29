// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <XamlBehaviorMode.h>


XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{

PickerFlyoutBase::PickerFlyoutBase()
{

}

PickerFlyoutBase::~PickerFlyoutBase()
{

}

_Check_return_ HRESULT
PickerFlyoutBase::InitializeImpl(_In_opt_ IInspectable* outer)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_primitives::IFlyoutBaseFactory> spInnerFactory;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spDelegatingInner;
    wrl::ComPtr<IInspectable> spNonDelegatingInnerInspectable;
    IInspectable* aggregateOuter = outer ? outer : static_cast<IPickerFlyoutBase*>(this);
    wrl::ComPtr<wf::IEventHandler<IInspectable*>> spOpeningEventHandler;
    wrl::ComPtr<wf::IEventHandler<IInspectable*>> spClosedEventHandler;

    IFC(PickerFlyoutBaseGenerated::InitializeImpl(aggregateOuter));

    IFC(wf::GetActivationFactory(
            wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_Primitives_FlyoutBase).Get(),
            &spInnerFactory));

    IFC(spInnerFactory->CreateInstance(
            aggregateOuter,
            &spNonDelegatingInnerInspectable,
            &spDelegatingInner));

    IFC(SetComposableBasePointers(
            spNonDelegatingInnerInspectable.Get(),
            spInnerFactory.Get()));

    // Care must be taken with any initialization after this point, as the outer object is not
    // finished fully initialized and any IInspectable operations on spDelegatingInner will
    // be delegated to the outer.

    // for PickerFlyoutBase, the default PlacementMode is Full.
    // as before Opening event raised,  FlyoutBase already cached the placementMode, so we have to set this before that.
    IFC(spDelegatingInner->put_Placement(
        xaml_primitives::FlyoutPlacementMode_Full));

    IFC(spDelegatingInner->put_ShouldConstrainToRootBounds(FALSE));

Cleanup:
    RRETURN(hr);
}

// -----
// App bar support
// -----

_Check_return_ HRESULT
PickerFlyoutBase::GetCommandBarToRegister(_Outptr_ xaml_controls::ICommandBar** ppCommandBar)
{
    HRESULT hr = S_OK;
    BOOLEAN areSelectionOptionsRequired = FALSE;

    *ppCommandBar = nullptr;
    IFC(ShouldShowConfirmationButtonsProtected(&areSelectionOptionsRequired));

    if (areSelectionOptionsRequired)
    {
        if (!_tpPickerCommandBar)
        {
            wrl::ComPtr<xaml_controls::ICommandBar> spPickerCommandBar;
            wrl::ComPtr<xaml_controls::IAppBarButton> spCancelButton;
            wrl::ComPtr<xaml_controls::IAppBarButton> spAcceptButton;
            wrl::ComPtr<xaml::IFrameworkElement> spCancelButtonAsFE;
            wrl::ComPtr<xaml::IFrameworkElement> spAcceptButtonAsFE;
            wrl::ComPtr<xaml_primitives::IButtonBase> spCancelButtonBase;
            wrl::ComPtr<xaml_primitives::IButtonBase> spAcceptButtonBase;
            wrl::ComPtr<xaml_controls::ICommandBarElement> spCancelButtonElement;
            wrl::ComPtr<xaml_controls::ICommandBarElement> spAcceptButtonElement;
            wrl::ComPtr<wfc::IVector<xaml_controls::ICommandBarElement*>> spPrimaryCommandsAsVector;
            wrl::ComPtr<wfc::IObservableVector<xaml_controls::ICommandBarElement*>> spPrimaryCommands;
            wrl::ComPtr<xaml_controls::ISymbolIcon> spAcceptButtonIcon;
            wrl::ComPtr<xaml_controls::ISymbolIcon> spCancelButtonIcon;
            wrl::ComPtr<xaml_controls::IIconElement> spAcceptButtonIconAsIconElement;
            wrl::ComPtr<xaml_controls::IIconElement> spCancelButtonIconAsIconElement;
            EventRegistrationToken cancelEventRegistrationToken;
            EventRegistrationToken acceptEventRegistrationToken;

            wrl_wrappers::HString acceptBtnLabel;
            wrl_wrappers::HString cancelBtnLabel;

            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_CommandBar).Get(),
                &spPickerCommandBar));

            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_AppBarButton).Get(),
                &spCancelButton));
            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_AppBarButton).Get(),
                &spAcceptButton));

            // Add localized lables to the AppBarButtons
            IFC(Private::FindStringResource(
                TEXT_PICKERFLYOUTBASE_ACCEPTBTNLABEL,
                acceptBtnLabel.GetAddressOf()));

            IFC(Private::FindStringResource(
                TEXT_PICKERFLYOUTBASE_CANCELBTNLABEL,
                cancelBtnLabel.GetAddressOf()));

            IFC(spAcceptButton->put_Label(acceptBtnLabel.Get()));
            IFC(spCancelButton->put_Label(cancelBtnLabel.Get()));

            IFC(spAcceptButton.As(&spAcceptButtonAsFE));
            IFC(spCancelButton.As(&spCancelButtonAsFE));
            IFC(spAcceptButtonAsFE->put_Name(wrl_wrappers::HStringReference(L"AcceptBtn").Get()));
            IFC(spCancelButtonAsFE->put_Name(wrl_wrappers::HStringReference(L"CancelBtn").Get()));

            // Create the icons
            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_SymbolIcon).Get(),
                &spAcceptButtonIcon));
            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_SymbolIcon).Get(),
                &spCancelButtonIcon));

            IFC(spAcceptButtonIcon->put_Symbol(xaml_controls::Symbol_Accept));
            IFC(spCancelButtonIcon->put_Symbol(xaml_controls::Symbol_Cancel));

            IFC(spAcceptButtonIcon.As(&spAcceptButtonIconAsIconElement));
            IFC(spCancelButtonIcon.As(&spCancelButtonIconAsIconElement));

            // Add the Icons to the AppBarButtons
            IFC(spAcceptButton->put_Icon(spAcceptButtonIconAsIconElement.Get()));
            IFC(spCancelButton->put_Icon(spCancelButtonIconAsIconElement.Get()));

            // Add event handlers
            IFC(spAcceptButton.As(&spAcceptButtonBase));
            IFC(spCancelButton.As(&spCancelButtonBase));

            IFC(spCancelButtonBase->add_Click(
                wrl::Callback<xaml::IRoutedEventHandler, PickerFlyoutBase, IInspectable*, IRoutedEventArgs*>(
                this, &PickerFlyoutBase::OnCancelButtonClick).Get(), &cancelEventRegistrationToken));

            IFC(spAcceptButtonBase->add_Click(
                wrl::Callback<xaml::IRoutedEventHandler, PickerFlyoutBase, IInspectable*, IRoutedEventArgs*>(
                this, &PickerFlyoutBase::OnAcceptButtonClick).Get(), &acceptEventRegistrationToken));

            // Add the new items to the app bar
            IFC(spAcceptButton.As(&spAcceptButtonElement));
            IFC(spCancelButton.As(&spCancelButtonElement));
            IFC(spPickerCommandBar->get_PrimaryCommands(&spPrimaryCommands));
            IFC(spPrimaryCommands.As(&spPrimaryCommandsAsVector));
            IFC(spPrimaryCommandsAsVector->Append(spAcceptButtonElement.Get()));
            IFC(spPrimaryCommandsAsVector->Append(spCancelButtonElement.Get()));

            wrl::ComPtr<xaml_controls::IAppBar> spPickerAppBar;
            IFC(spPickerCommandBar.As(&spPickerAppBar));
            IFC(spPickerAppBar->put_IsOpen(TRUE));

            IFC(SetPtrValue(_tpPickerCommandBar, spPickerCommandBar.Get()));
        }

        _tpPickerCommandBar.CopyTo(ppCommandBar);
    }
    else
    {
        if (!_tpHiddenCommandBar)
        {
            wrl::ComPtr<xaml_controls::ICommandBar> spHiddenCommandBar;
            wrl::ComPtr<xaml::IUIElement> spHiddenCommandBarAsUI;

            IFC(wf::ActivateInstance(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Controls_CommandBar).Get(),
                &spHiddenCommandBar));

            IFC(spHiddenCommandBar.As(&spHiddenCommandBarAsUI));
            IFC(spHiddenCommandBarAsUI->put_Visibility(xaml::Visibility_Collapsed));

            IFC(SetPtrValue(_tpHiddenCommandBar, spHiddenCommandBar.Get()));
        }

        _tpHiddenCommandBar.CopyTo(ppCommandBar);
    }

Cleanup:
    return hr;
}

_Check_return_ HRESULT
PickerFlyoutBase::OnAcceptButtonClick(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    RRETURN(OnConfirmedProtected());
}

_Check_return_ HRESULT
PickerFlyoutBase::OnCancelButtonClick(
    _In_ IInspectable* /*pSender*/,
    _In_ xaml::IRoutedEventArgs* /*pArgs*/)
{
    RRETURN(Hide());
}

_Check_return_ HRESULT
PickerFlyoutBase::OnConfirmedImpl()
{
    RRETURN(Hide());
}

_Check_return_ inline HRESULT PickerFlyoutBase::Hide()
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutBase;

    // Close the Flyout
    IFC(QueryInterface(__uuidof(xaml_primitives::IFlyoutBase), &spFlyoutBase));
    IFC(spFlyoutBase->Hide());

Cleanup:
    RRETURN(hr);
}

} } } } } XAML_ABI_NAMESPACE_END
