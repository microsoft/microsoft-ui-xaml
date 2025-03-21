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

#define __FocusEngagedEventArgs_GUID "b8f645fb-2ed2-4423-9a4f-0a518d0f05aa"

namespace DirectUI
{
    class FocusEngagedEventArgs;

    class __declspec(novtable) __declspec(uuid(__FocusEngagedEventArgs_GUID)) FocusEngagedEventArgs :
        public ABI::Microsoft::UI::Xaml::Controls::IFocusEngagedEventArgs,
        public DirectUI::RoutedEventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.FocusEngagedEventArgs");

        BEGIN_INTERFACE_MAP(FocusEngagedEventArgs, DirectUI::RoutedEventArgs)
            INTERFACE_ENTRY(FocusEngagedEventArgs, ABI::Microsoft::UI::Xaml::Controls::IFocusEngagedEventArgs)
        END_INTERFACE_MAP(FocusEngagedEventArgs, DirectUI::RoutedEventArgs)

    public:
        FocusEngagedEventArgs();
        ~FocusEngagedEventArgs() override;

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
        BOOLEAN m_handled;
    };
}


