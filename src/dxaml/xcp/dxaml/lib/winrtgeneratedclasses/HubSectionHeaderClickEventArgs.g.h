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

#include "HubSection.g.h"

#define __HubSectionHeaderClickEventArgs_GUID "3b8c4614-a2a4-409e-8820-6de2ec88138e"

namespace DirectUI
{
    class HubSectionHeaderClickEventArgs;

    class __declspec(novtable) __declspec(uuid(__HubSectionHeaderClickEventArgs_GUID)) HubSectionHeaderClickEventArgs :
        public ABI::Microsoft::UI::Xaml::Controls::IHubSectionHeaderClickEventArgs,
        public DirectUI::EventArgs
    {

        INSPECTABLE_CLASS(L"Microsoft.UI.Xaml.Controls.HubSectionHeaderClickEventArgs");

        BEGIN_INTERFACE_MAP(HubSectionHeaderClickEventArgs, DirectUI::EventArgs)
            INTERFACE_ENTRY(HubSectionHeaderClickEventArgs, ABI::Microsoft::UI::Xaml::Controls::IHubSectionHeaderClickEventArgs)
        END_INTERFACE_MAP(HubSectionHeaderClickEventArgs, DirectUI::EventArgs)

    public:
        HubSectionHeaderClickEventArgs();
        ~HubSectionHeaderClickEventArgs() override;

        // Properties.
        IFACEMETHOD(get_Section)(_Outptr_result_maybenull_ ABI::Microsoft::UI::Xaml::Controls::IHubSection** ppValue) override;
        _Check_return_ HRESULT put_Section(_In_opt_ ABI::Microsoft::UI::Xaml::Controls::IHubSection* pValue);

        // Methods.

    protected:
        HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        // Customized properties.

        // Customized methods.

        // Fields.
        TrackerPtr<ABI::Microsoft::UI::Xaml::Controls::IHubSection> m_pSection;
    };
}


