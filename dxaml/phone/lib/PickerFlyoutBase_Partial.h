// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <algorithm>

XAML_ABI_NAMESPACE_BEGIN namespace Microsoft { namespace UI { namespace Xaml { namespace Controls { namespace Primitives
{
    class PickerFlyoutBase :
        public PickerFlyoutBaseGenerated
    {

    public:

        PickerFlyoutBase();

        // Simple helper to get the current frame
        template <class T>
        static _Check_return_ HRESULT GetCurrentWindowContents(_Outptr_ T** ppContents)
        {
            HRESULT hr = S_OK;
            wrl::ComPtr<xaml::IWindowStatics> spWindowStatics;
            wrl::ComPtr<xaml::IWindow> spCurrentWindow;
            wrl::ComPtr<xaml::IUIElement> spWindowContentsAsUI;

            *ppContents = NULL;

            IFC(wf::GetActivationFactory(
                wrl_wrappers::HStringReference(RuntimeClass_Microsoft_UI_Xaml_Window).Get(),
                &spWindowStatics));

            IFC(spWindowStatics->get_Current(&spCurrentWindow));
            IFC(spCurrentWindow->get_Content(&spWindowContentsAsUI));

            hr = spWindowContentsAsUI.CopyTo(ppContents);

        Cleanup:
            RRETURN(hr);
        }

    protected:

        ~PickerFlyoutBase();

        _Check_return_ HRESULT InitializeImpl(_In_opt_ IInspectable* outer) override;

    private:

        // AppBar support
        _Check_return_ HRESULT OnCancelButtonClick(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        _Check_return_ HRESULT OnAcceptButtonClick(
            _In_ IInspectable* pSender,
            _In_ xaml::IRoutedEventArgs* pArgs);

        // We need to either show a confirmation app bar, or hide any existing one.
        // We hide by registering a hidden (i.e. opacity=0) command bar.
        // This method will ensure the existance of and return the appropriate
        // command bar to register based on the AreSelectionOptionsRequired property.
        _Check_return_ HRESULT GetCommandBarToRegister(_Outptr_ xaml_controls::ICommandBar** ppCommandBar);

        _Check_return_ HRESULT Hide();

        // IFlyoutBaseOverrides Impl

        _Check_return_ HRESULT CreatePresenterImpl(_Outptr_ xaml_controls::IControl** returnValue) override
        {
            // CreatePresenter is abstract
            *returnValue = nullptr;
            return E_NOTIMPL;
        }

    public:
        // Customized methods

        _Check_return_ HRESULT OnConfirmedImpl();

        _Check_return_ HRESULT ShouldShowConfirmationButtonsImpl(_Out_ BOOLEAN* returnValue)
        {
            // AreSelectionOptionsRequired is abstract
            *returnValue = FALSE;
            return E_NOTIMPL;
        }

    private:
        Private::TrackerPtr<xaml_controls::ICommandBar> _tpPickerCommandBar; // app bar registered to show accept/cancel options
        Private::TrackerPtr<xaml_controls::ICommandBar> _tpHiddenCommandBar; // app bar registered to hide all app bars
    };

    ActivatableClassWithFactory(PickerFlyoutBase, PickerFlyoutBaseFactory);

} } } } } XAML_ABI_NAMESPACE_END
