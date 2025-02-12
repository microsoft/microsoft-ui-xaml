// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.
//------------------------------------------------------------------------
//
//  Abstract:
//
//      XAML types.
//      NOTE: This file was generated by a tool.
//
//------------------------------------------------------------------------

#pragma once

#include "RoutedEventArgs.g.h"

#define __NoFocusCandidateFoundEventArgs_GUID "590cf413-aa3d-45ab-a558-cbdf4a034e6b"

namespace DirectUI
{
    class NoFocusCandidateFoundEventArgs;

    class __declspec(novtable) __declspec(uuid(__NoFocusCandidateFoundEventArgs_GUID)) NoFocusCandidateFoundEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::INoFocusCandidateFoundEventArgs,
        public DirectUI::RoutedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.NoFocusCandidateFoundEventArgs");

        BEGIN_INTERFACE_MAP(NoFocusCandidateFoundEventArgs, DirectUI::RoutedEventArgs)
            INTERFACE_ENTRY(NoFocusCandidateFoundEventArgs, ABI::Microsoft::UI::Xaml::Input::INoFocusCandidateFoundEventArgs)
        END_INTERFACE_MAP(NoFocusCandidateFoundEventArgs, DirectUI::RoutedEventArgs)

    public:
        NoFocusCandidateFoundEventArgs();
        ~NoFocusCandidateFoundEventArgs() override;

        // Properties.
        IFACEMETHOD(get_Direction)(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusNavigationDirection* pValue) override;
        IFACEMETHOD(get_Handled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Handled)(BOOLEAN value) override;
        IFACEMETHOD(get_InputDevice)(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusInputDeviceKind* pValue) override;

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


