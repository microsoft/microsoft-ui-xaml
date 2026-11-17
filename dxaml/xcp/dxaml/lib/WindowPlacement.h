// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// See docs\design-notes\window-placement.md for behavior and the private storage format.

namespace DirectUI
{
    class WindowPlacement final
    {
    public:
        // The HRESULT reports failures. These flags describe additional outcomes of the operation.
        enum class PlacementResult
        {
            None = 0,
            LoadedAndApplied = 0x1, // TryLoadAndApply successfully loaded and applied the placement.
            HashCollision = 0x2,    // A record with a colliding hash was detected
        };

        _Check_return_ HRESULT GetId(_Out_ HSTRING* value);
        _Check_return_ HRESULT SetId(_In_opt_ HSTRING value);

        _Check_return_ HRESULT TryLoadAndApply(HWND hwnd, _Out_ PlacementResult* result);
        _Check_return_ HRESULT Save(HWND hwnd, _Out_ PlacementResult* result);

    private:
        // Returns success without initializing storage when the app has no package identity.
        _Check_return_ HRESULT EnsureStorage();

        wrl_wrappers::HString m_id;
        wrl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> m_settings;
    };

    DEFINE_ENUM_FLAG_OPERATORS(WindowPlacement::PlacementResult);
}
