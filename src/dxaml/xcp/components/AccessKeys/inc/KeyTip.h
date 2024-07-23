// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <CDependencyObject.h>
#include <UIElement.h>
#include <CControl.h>
#include <Framework.h>
#include <Popup.h>

namespace KeyTipHelper
{
    struct VisualProperties;
}

struct KeyTip
{
    KeyTip() = delete;

    KeyTip(_In_ CDependencyObject* obj)
        : Object(obj)
        , State(NeedsPopup) { }

    KeyTip(_In_ XRECTF_RB& objBounds, _In_ XRECTF_RB& ktBounds)
    {
        ObjectBounds = objBounds;
        KeytipBounds = ktBounds;
    }

    KeyTip(_In_ CDependencyObject* obj,
        _In_ DirectUI::KeyTipPlacementMode placementMode,
        _In_ const bool placementModeInitialized)
        : Object(obj)
        , State(NeedsPopup),
        PlacementMode(placementMode),
        PlacementModeInitialized(placementModeInitialized)
        {  }

    ~KeyTip();

    // This type is moveable and not copyable to make sure when objects of this type are in a vector
    // the dtor only executes when the object is removed from the vector
    KeyTip(const KeyTip& other) = delete;
    KeyTip& operator=(const KeyTip& other) = delete;

    KeyTip(_Inout_ KeyTip&& other) noexcept
    {
        *this = std::move(other);
    }
    KeyTip& operator=(_Inout_ KeyTip&& other) noexcept
    {
        Popup = std::move(other.Popup);
        Object = std::move(other.Object);
        State = other.State;
        PlacementMode = other.PlacementMode;
        ObjectBounds = other.ObjectBounds;
        KeytipBounds = other.KeytipBounds;
        HorizontalOffset = other.HorizontalOffset;
        VerticalOffset = other.VerticalOffset;
        ManuallyPositioned = other.ManuallyPositioned;
        IsRightToLeft = other.IsRightToLeft;
        PlacementModeInitialized = other.PlacementModeInitialized;
        AlignedWithManuallyPositioned = other.AlignedWithManuallyPositioned;
        return *this;
    }

    void Reset();

    _Check_return_ HRESULT CreatePopup(
        _Inout_ KeyTipHelper::VisualProperties& visualProperties,
        _In_ unsigned int keysPressed);

    xref_ptr<CDependencyObject> Object;
    xref_ptr<CPopup> Popup;

    enum
    {
        Normal,
        NeedsPopup,
        NeedsHidePopup
    } State = Normal;

    XRECTF_RB ObjectBounds = {};
    XRECTF_RB KeytipBounds = {};
    DirectUI::KeyTipPlacementMode PlacementMode = DirectUI::KeyTipPlacementMode::Auto;
    float HorizontalOffset = 0;
    float VerticalOffset = 0;
    bool ManuallyPositioned = false;
    bool IsRightToLeft = false;
    bool PlacementModeInitialized = false;
    bool AlignedWithManuallyPositioned = false;
};

