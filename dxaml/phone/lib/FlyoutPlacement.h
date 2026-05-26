// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#pragma region XamlControlsExt Exports

HRESULT
XamlControlsCalculateFlyoutPlacement(
    _In_ IInspectable* pFlyoutToPlace,
    _In_ UINT32 preferredPlacement,
    _In_ BOOLEAN allowFallbacks,
    _In_ IInspectable* placementTargetBoundsRect,
    _In_ IInspectable* controlSize,
    _In_ IInspectable* containerRect,
    _Out_ UINT32* pFinalPlacement,
    _Outptr_ IInspectable** ppControlRect
    );

#pragma endregion XamlControlsExt Exports


namespace Private {

class FlyoutPlacementHelper
{

public:

    static _Check_return_ HRESULT CalculatePlacement(
        _In_ xaml_primitives::IFlyoutBase* pFlyoutToPlace,
        _In_ BOOLEAN allowFallbacks,
        _In_ wf::Rect placementTargetBounds,
        _In_ wf::Size controlSize,
        _In_ wf::Rect containerRect,
        _Inout_ xaml_primitives::FlyoutPlacementMode* pPlacement,
        _Out_ wf::Rect* pControlRect);

private:

    static _Check_return_ HRESULT PerformPhonePlacementForPortrait(
            _Inout_ xaml_primitives::FlyoutPlacementMode* pPlacement,
            _In_ BOOLEAN allowFallbacks,
            _In_ const wf::Rect& placementTargetBounds,
            _In_ const wf::Size& controlSize,
            _In_ const wf::Rect& containerRect,
            _Out_ wf::Rect* pControlRect,
            _Out_ BOOLEAN* doesFit);

};

}
