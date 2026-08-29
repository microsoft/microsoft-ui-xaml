// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#pragma region XamlControlsExt Exports

extern "C"
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
    )
{
    HRESULT hr = S_OK;

    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutToPlace;
    wrl::ComPtr<wf::IPropertyValue> spPlacementTargetBoundsRect;
    wrl::ComPtr<wf::IPropertyValue> spControlSize;
    wrl::ComPtr<wf::IPropertyValue> spContainerRect;
    wrl::ComPtr<wf::IPropertyValue> spControlRect;
    xaml_primitives::FlyoutPlacementMode placement = xaml_primitives::FlyoutPlacementMode_Top;
    wf::Rect extractedPlacementTargetBoundsRect = { };
    wf::Size extractedControlSize = { };
    wf::Rect extractedContainerRect = { };
    wf::Rect controlRect = { };

    // Convert the arguments to the appropriate types
    placement = static_cast<xaml_primitives::FlyoutPlacementMode>(preferredPlacement);
    IFC(pFlyoutToPlace->QueryInterface<xaml_primitives::IFlyoutBase>(&spFlyoutToPlace));
    IFC(placementTargetBoundsRect->QueryInterface<wf::IPropertyValue>(&spPlacementTargetBoundsRect));
    IFC(controlSize->QueryInterface<wf::IPropertyValue>(&spControlSize));
    IFC(containerRect->QueryInterface<wf::IPropertyValue>(&spContainerRect));
    IFC(spPlacementTargetBoundsRect->GetRect(&extractedPlacementTargetBoundsRect));
    IFC(spControlSize->GetSize(&extractedControlSize));
    IFC(spContainerRect->GetRect(&extractedContainerRect));

    IFC(Private::FlyoutPlacementHelper::CalculatePlacement(
        spFlyoutToPlace.Get(),
        allowFallbacks,
        extractedPlacementTargetBoundsRect,
        extractedControlSize,
        extractedContainerRect,
        &placement,
        &controlRect));

    *pFinalPlacement = placement;
    IFC(Private::ValueBoxer::CreateRect(controlRect, &spControlRect));
    IFC(spControlRect.CopyTo(ppControlRect));

Cleanup:
    RRETURN(hr);
}

#pragma endregion XamlControlsExt Exports

namespace Private
{

_Check_return_ HRESULT FlyoutPlacementHelper::CalculatePlacement(
    _In_ xaml_primitives::IFlyoutBase* pFlyoutToPlace,
    _In_ BOOLEAN allowFallbacks,
    _In_ wf::Rect placementTargetBounds,
    _In_ wf::Size controlSize,
    _In_ wf::Rect containerRect,
    _Inout_ xaml_primitives::FlyoutPlacementMode* pPlacement,
    _Out_ wf::Rect* pControlRect)
{
    HRESULT hr = S_OK;
    wrl::ComPtr<xaml_primitives::IFlyoutBase> spFlyoutToPlace(pFlyoutToPlace);
    wrl::ComPtr<xaml_controls::IMenuFlyout> spMenuFlyoutToPlace;

    hr = spFlyoutToPlace.As(&spMenuFlyoutToPlace);
    if (SUCCEEDED(hr))
    {
        // Special case placement logic for MenuFlyout

        if (containerRect.Width > containerRect.Height)
        {
            // Landscape
            pControlRect->Y = containerRect.Y;
            pControlRect->Width = containerRect.Height;
            pControlRect->Height = containerRect.Height;


            if (*pPlacement == xaml_primitives::FlyoutPlacementMode_Left)
            {
                pControlRect->X = containerRect.X;
            }
            else
            {
                *pPlacement = xaml_primitives::FlyoutPlacementMode_Right;
                pControlRect->X = containerRect.X + (containerRect.Width - containerRect.Height);
            }
        }
        else
        {
            // Portrait
            BOOLEAN doesFit;
            if (*pPlacement != xaml_primitives::FlyoutPlacementMode_Top)
            {
                *pPlacement = xaml_primitives::FlyoutPlacementMode_Bottom;
            }

            IFC(FlyoutPlacementHelper::PerformPhonePlacementForPortrait(
                pPlacement,
                allowFallbacks,
                placementTargetBounds,
                controlSize,
                containerRect,
                pControlRect,
                &doesFit));
        }
    }
    else
    {
        hr = S_OK;
        // Placement logic for all other types of flyouts

        if (*pPlacement == xaml_primitives::FlyoutPlacementMode_Full)
        {
            pControlRect->X = containerRect.X;
            pControlRect->Y = containerRect.Y;
            pControlRect->Width = containerRect.Width;
            pControlRect->Height = containerRect.Height;
        }
        else
        {
            *pPlacement = xaml_primitives::FlyoutPlacementMode_Top;

            pControlRect->X = containerRect.X;
            pControlRect->Y = containerRect.Y;
            pControlRect->Width = containerRect.Width;
            pControlRect->Height = min(controlSize.Height, containerRect.Height);
        }
    }

Cleanup:
    RRETURN(hr);
}

_Check_return_ HRESULT FlyoutPlacementHelper::PerformPhonePlacementForPortrait(
    _In_ xaml_primitives::FlyoutPlacementMode* pPlacement,
    _In_ BOOLEAN allowFallbacks,
    _In_ const wf::Rect& placementTargetBounds,
    _In_ const wf::Size& controlSize,
    _In_ const wf::Rect& containerRect,
    _Out_ wf::Rect* pControlRect,
    _Out_ BOOLEAN* doesFit)
{
    HRESULT hr = S_OK;
    BOOLEAN preferredPlacementFits = FALSE;

    ASSERT(*pPlacement == xaml_primitives::FlyoutPlacementMode_Top ||
           *pPlacement == xaml_primitives::FlyoutPlacementMode_Bottom);

    *doesFit = FALSE;


    if (*pPlacement == xaml_primitives::FlyoutPlacementMode_Top)
    {
        preferredPlacementFits = placementTargetBounds.Y - containerRect.Y >= controlSize.Height;

        if (!preferredPlacementFits && allowFallbacks)
        {
            *pPlacement = xaml_primitives::FlyoutPlacementMode_Bottom;
            IFC(FlyoutPlacementHelper::PerformPhonePlacementForPortrait(
                pPlacement,
                FALSE,
                placementTargetBounds,
                controlSize,
                containerRect,
                pControlRect,
                doesFit));
        }

        if (!(*doesFit))
        {
            // Either the preferred placement fits, or the fallback doesn't
            *pPlacement = xaml_primitives::FlyoutPlacementMode_Top;
            pControlRect->Y = max(placementTargetBounds.Y - controlSize.Height, containerRect.Y);
        }
    }
    else
    {
        float bottomTarget = placementTargetBounds.Y + placementTargetBounds.Height;
        float bottomContainer = containerRect.Y + containerRect.Height;
        preferredPlacementFits = bottomContainer - bottomTarget >= controlSize.Height;

        if (!preferredPlacementFits && allowFallbacks)
        {
            *pPlacement = xaml_primitives::FlyoutPlacementMode_Top;
            IFC(FlyoutPlacementHelper::PerformPhonePlacementForPortrait(
                pPlacement,
                FALSE,
                placementTargetBounds,
                controlSize,
                containerRect,
                pControlRect,
                doesFit));
        }

        if (!(*doesFit))
        {
            // Either the preferred placement fits, or the fallback doesn't

            *pPlacement = xaml_primitives::FlyoutPlacementMode_Bottom;
            pControlRect->Y = max(containerRect.Y, min(bottomTarget, bottomContainer - controlSize.Height));
        }
    }

    pControlRect->X = containerRect.X;
    pControlRect->Width = containerRect.Width;
    pControlRect->Height = min(controlSize.Height, containerRect.Height);
    *doesFit |= preferredPlacementFits;

Cleanup:
    RRETURN(hr);
}

}
