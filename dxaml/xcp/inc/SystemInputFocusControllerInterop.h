// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace SystemInputFocusControllerInterop
{
    struct IFocusNavigationRequestEventArgs;

    MIDL_INTERFACE("3b0ba728-c435-5239-9b03-8280aa934270")
    INavigateFocusRequestedHandler : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE Invoke(
            ABI::Microsoft::UI::Input::IInputFocusController* sender,
            IFocusNavigationRequestEventArgs* args) = 0;
    };

    MIDL_INTERFACE("7830558f-1fcc-53ad-910e-3c8411d4d05f")
    IInputFocusController2 : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE DepartFocus(
            ABI::Microsoft::UI::Input::FocusNavigationReason reason,
            ABI::Microsoft::UI::Input::FocusNavigationResult* result) = 0;

        virtual HRESULT STDMETHODCALLTYPE add_NavigateFocusRequested(
            INavigateFocusRequestedHandler* handler,
            EventRegistrationToken* token) = 0;

        virtual HRESULT STDMETHODCALLTYPE remove_NavigateFocusRequested(
            EventRegistrationToken token) = 0;
    };

    MIDL_INTERFACE("2c2d2c7d-891b-510e-a786-be76af3aabcf")
    IFocusNavigationRequestEventArgs : public IInspectable
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE get_Reason(
            ABI::Microsoft::UI::Input::FocusNavigationReason* value) = 0;

        virtual HRESULT STDMETHODCALLTYPE put_Reason(
            ABI::Microsoft::UI::Input::FocusNavigationReason value) = 0;

        virtual HRESULT STDMETHODCALLTYPE get_Result(
            ABI::Microsoft::UI::Input::FocusNavigationResult* value) = 0;

        virtual HRESULT STDMETHODCALLTYPE put_Result(
            ABI::Microsoft::UI::Input::FocusNavigationResult value) = 0;
    };
}
