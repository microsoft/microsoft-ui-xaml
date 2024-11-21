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


#define __WindowVisibilityChangedEventArgs_GUID "159060e1-bae1-45bd-b686-67cac50dcb27"

namespace DirectUI
{
    class WindowVisibilityChangedEventArgs;

    class __declspec(novtable) __declspec(uuid(__WindowVisibilityChangedEventArgs_GUID)) WindowVisibilityChangedEventArgs:
        public ctl::WeakReferenceSource
        , public ABI::Microsoft::UI::Xaml::IWindowVisibilityChangedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.WindowVisibilityChangedEventArgs");

        BEGIN_INTERFACE_MAP(WindowVisibilityChangedEventArgs, ctl::WeakReferenceSource)
            INTERFACE_ENTRY(WindowVisibilityChangedEventArgs, ABI::Microsoft::UI::Xaml::IWindowVisibilityChangedEventArgs)
        END_INTERFACE_MAP(WindowVisibilityChangedEventArgs, ctl::WeakReferenceSource)

    public:
        WindowVisibilityChangedEventArgs();
        ~WindowVisibilityChangedEventArgs() override;

        // Event source typedefs.


        // Properties.
        IFACEMETHOD(get_Handled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Handled)(BOOLEAN value) override;
        IFACEMETHOD(get_Visible)(_Out_ BOOLEAN* pValue) override;
        _Check_return_ HRESULT put_Visible(BOOLEAN value);

        // Events.

        // Methods.


    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        BOOLEAN m_handled;
        BOOLEAN m_visible;
    };
}


