// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "Microsoft.UI.Windowing.h"

// See docs\design-notes\window-placement.md for the private storage format and
// docs\design-notes\window-placement-appwindow-alignment-scenarios.md for prototype behavior.

namespace DirectUI
{
    class WindowPlacement final
    {
    public:
        // The HRESULT reports failures. These flags describe additional outcomes of the operation.
        enum class PlacementResult
        {
            None = 0,
            Found = 0x1,            // TryLoad found a valid saved placement.
            HashCollision = 0x2,    // A record with a colliding hash was detected
        };

        _Check_return_ HRESULT GetId(_Out_ HSTRING* value);
        _Check_return_ HRESULT SetId(_In_opt_ HSTRING value);

        bool HasId() const { return !WindowsIsStringEmpty(m_id.Get()); }

        // Reads the saved placement for the current id. Returns null when there is no id, no package
        // identity, or no saved placement. The FullScreen flag is removed (WinUI never restores full
        // screen). Otherwise the details are returned as saved.
        _Check_return_ HRESULT TryLoad(
            _Outptr_result_maybenull_ ABI::Microsoft::UI::Windowing::IAppWindowPlacementDetails** details,
            _Out_ PlacementResult* result);

        // Writes details as the saved placement for the current id. Does nothing without an id or storage.
        _Check_return_ HRESULT Save(
            _In_ ABI::Microsoft::UI::Windowing::IAppWindowPlacementDetails* details,
            _Out_ PlacementResult* result);

    private:
        // Reopens m_settings. Leaves it null (and returns success) when the app has no package identity.
        _Check_return_ HRESULT EnsureStorage();

        wrl_wrappers::HString m_id;
        wrl::ComPtr<wfc::IMap<HSTRING, IInspectable*>> m_settings;
    };

    DEFINE_ENUM_FLAG_OPERATORS(WindowPlacement::PlacementResult);
}
