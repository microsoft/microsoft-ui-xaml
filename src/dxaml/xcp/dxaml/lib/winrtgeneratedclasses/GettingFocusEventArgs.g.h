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

#define __GettingFocusEventArgs_GUID "36548ba1-034c-4ba6-b387-619bdf9972ff"

namespace DirectUI
{
    class GettingFocusEventArgs;

    class __declspec(novtable) __declspec(uuid(__GettingFocusEventArgs_GUID)) GettingFocusEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::IGettingFocusEventArgs,
        public DirectUI::RoutedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.GettingFocusEventArgs");

        BEGIN_INTERFACE_MAP(GettingFocusEventArgs, DirectUI::RoutedEventArgs)
            INTERFACE_ENTRY(GettingFocusEventArgs, ABI::Microsoft::UI::Xaml::Input::IGettingFocusEventArgs)
        END_INTERFACE_MAP(GettingFocusEventArgs, DirectUI::RoutedEventArgs)

    public:
        GettingFocusEventArgs();
        ~GettingFocusEventArgs() override;

        // Properties.
        IFACEMETHOD(get_OldFocusedElement)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppValue) override;
        IFACEMETHOD(get_NewFocusedElement)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::IDependencyObject** ppValue) override;
        IFACEMETHOD(put_NewFocusedElement)(_In_opt_ ABI::Microsoft::UI::Xaml::IDependencyObject* pValue) override;
        IFACEMETHOD(get_FocusState)(_Out_ ABI::Microsoft::UI::Xaml::FocusState* pValue) override;
        IFACEMETHOD(get_Direction)(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusNavigationDirection* pValue) override;
        IFACEMETHOD(get_Handled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Handled)(BOOLEAN value) override;
        IFACEMETHOD(get_InputDevice)(_Out_ ABI::Microsoft::UI::Xaml::Input::FocusInputDeviceKind* pValue) override;
        IFACEMETHOD(get_Cancel)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Cancel)(BOOLEAN value) override;
        IFACEMETHOD(get_CorrelationId)(_Out_ GUID* pValue) override;

        // Methods.
        IFACEMETHOD(TryCancel)(_Out_ BOOLEAN* pReturnValue) override;
        IFACEMETHOD(TrySetNewFocusedElement)(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pReturnValue) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.
         _Check_return_ HRESULT TryCancelImpl(_Out_ BOOLEAN* pReturnValue); 
         _Check_return_ HRESULT TrySetNewFocusedElementImpl(_In_ ABI::Microsoft::UI::Xaml::IDependencyObject* pElement, _Out_ BOOLEAN* pReturnValue); 

        // Fields.
    };
}


