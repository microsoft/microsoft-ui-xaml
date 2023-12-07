// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "XamlIsland.g.h"
#include <fwd/Microsoft.UI.Xaml.hosting.h>
#include <Microsoft.UI.Content.h>
#include <Microsoft.UI.Input.h>
#include "XAMLIslandRoot_Partial.h"

namespace DirectUI
{
    class WindowsXamlManager;

    PARTIAL_CLASS(XamlIsland)
    {
        friend class XamlIslandGenerated;

    public:
        XamlIsland();
        ~XamlIsland() override;

        // IClosable
        IFACEMETHOD(Close)() override;

        _Check_return_ HRESULT get_ContentImpl(_Outptr_ xaml::IUIElement** ppValue);
        _Check_return_ HRESULT put_ContentImpl(_In_opt_ xaml::IUIElement* pValue);

        _Check_return_ HRESULT get_ContentIslandImpl(_Outptr_ ixp::IContentIsland **ppValue);

        _Check_return_ xaml_hosting::IXamlIslandRoot *GetXamlIslandRootNoRef();

    protected:
        _Check_return_ HRESULT Initialize() override;
        _Check_return_ HRESULT QueryInterfaceImpl(_In_ REFIID iid, _Outptr_ void** ppObject) override;

    private:
        bool m_bClosed = false;

        ctl::ComPtr<WindowsXamlManager> m_spXamlCore;
        ctl::ComPtr<xaml_hosting::IXamlIslandRoot> m_spXamlIsland;
        XamlIslandRoot * m_xamlIsland;
        CXamlIslandRoot * m_pXamlIslandCore;
        ctl::ComPtr<ABI::Microsoft::UI::Input::IInputFocusController2> m_inputFocusController2;
        EventRegistrationToken m_focusNavigationRequestedToken = {};
    };
}

