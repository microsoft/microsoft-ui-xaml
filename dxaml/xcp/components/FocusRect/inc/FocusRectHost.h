// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

class CDependencyObject;
class CUIElement;

struct FocusRectHost
{
    // Enum representing the reason we picked this element to be focus rect host.
    // See tree structures in CFocusRectManager::FindFocusRectHost for detail.
    enum class Type
    {
        None,
        Popup,
        ScrollContentPresenterChild,
        ScrollContentPresenterParent,
        ScrollContentPresenterPeer,
        LTE,
        Root,
        ElementContainsFocusRect
    };

    FocusRectHost();
    FocusRectHost(_In_ CDependencyObject* obj, _In_ FocusRectHost::Type type, _In_ CUIElement* scp = nullptr);

    CUIElement* Element;
    CUIElement* ScrollContentPresenter;
    FocusRectHost::Type type;

    bool HostedOutsideScpClip() const {
        return type == Type::ScrollContentPresenterParent || type == Type::ScrollContentPresenterPeer;
    }
};