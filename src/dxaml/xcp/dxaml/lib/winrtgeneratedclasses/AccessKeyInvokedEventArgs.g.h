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


#define __AccessKeyInvokedEventArgs_GUID "68c26118-fc64-4621-a7b0-6bbdb9a3179b"

namespace DirectUI
{
    class AccessKeyInvokedEventArgs;

    class __declspec(novtable) __declspec(uuid(__AccessKeyInvokedEventArgs_GUID)) AccessKeyInvokedEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::IAccessKeyInvokedEventArgs,
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.AccessKeyInvokedEventArgs");

        BEGIN_INTERFACE_MAP(AccessKeyInvokedEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(AccessKeyInvokedEventArgs, ABI::Microsoft::UI::Xaml::Input::IAccessKeyInvokedEventArgs)
        END_INTERFACE_MAP(AccessKeyInvokedEventArgs, DirectUI::EventArgs)

    public:
        AccessKeyInvokedEventArgs();
        ~AccessKeyInvokedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_Handled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Handled)(BOOLEAN value) override;

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


