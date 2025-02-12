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
#include "UIElement.g.h"

#define __RightTappedRoutedEventArgs_GUID "867d84ea-98f5-4132-bd14-a7bb5972e156"

namespace DirectUI
{
    class RightTappedRoutedEventArgs;

    class __declspec(novtable) __declspec(uuid(__RightTappedRoutedEventArgs_GUID)) RightTappedRoutedEventArgs :
        public ABI::Microsoft::UI::Xaml::Input::IRightTappedRoutedEventArgs,
        public DirectUI::RoutedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Input.RightTappedRoutedEventArgs");

        BEGIN_INTERFACE_MAP(RightTappedRoutedEventArgs, DirectUI::RoutedEventArgs)
            INTERFACE_ENTRY(RightTappedRoutedEventArgs, ABI::Microsoft::UI::Xaml::Input::IRightTappedRoutedEventArgs)
        END_INTERFACE_MAP(RightTappedRoutedEventArgs, DirectUI::RoutedEventArgs)

    public:
        RightTappedRoutedEventArgs();
        ~RightTappedRoutedEventArgs() override;

        // Properties.
        IFACEMETHOD(get_PointerDeviceType)(_Out_ ABI::Microsoft::UI::Input::PointerDeviceType* pValue) override;
        IFACEMETHOD(get_Handled)(_Out_ BOOLEAN* pValue) override;
        IFACEMETHOD(put_Handled)(BOOLEAN value) override;

        // Methods.
        IFACEMETHOD(GetPosition)(_In_opt_ ABI::Microsoft::UI::Xaml::IUIElement* pRelativeTo, _Out_ ABI::Windows::Foundation::Point* pReturnValue) override;

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;
        CEventArgs* CreateCorePeer() override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
    };
}


