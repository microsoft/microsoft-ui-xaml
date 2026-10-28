// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// See docs\design-notes\window-placement.md for behavior and the private storage format.

namespace DirectUI
{
    class WindowPlacement final
    {
    public:
        _Check_return_ HRESULT GetId(_Out_ HSTRING* value);
        _Check_return_ HRESULT SetId(_In_opt_ HSTRING value);
        bool TryRestore(HWND hwnd);
        _Check_return_ HRESULT Save(HWND hwnd);

    private:
        _Check_return_ HRESULT EnsureStorage();

        wrl_wrappers::HString m_id;
        wrl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> m_settings;
        wrl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> m_record;
    };
}
