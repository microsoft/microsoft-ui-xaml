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


#define __AccessKeyDisplayDismissedEventArgs_GUID "c9b22510-3344-4350-b7d2-60bf2d304b07"

namespace DirectUI
{
    class AccessKeyDisplayDismissedEventArgs;

    class __declspec(novtable) __declspec(uuid(__AccessKeyDisplayDismissedEventArgs_GUID)) AccessKeyDisplayDismissedEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::IAccessKeyDisplayDismissedEventArgs,
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.AccessKeyDisplayDismissedEventArgs");

        BEGIN_INTERFACE_MAP(AccessKeyDisplayDismissedEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(AccessKeyDisplayDismissedEventArgs, ABI::Microsoft::UI::Xaml::Input::IAccessKeyDisplayDismissedEventArgs)
        END_INTERFACE_MAP(AccessKeyDisplayDismissedEventArgs, DirectUI::EventArgs)

    public:
        AccessKeyDisplayDismissedEventArgs();
        ~AccessKeyDisplayDismissedEventArgs() override;

        // Properties.

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


