// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides logical and directional navigation between focusable objects.

#include "precomp.h"
#include "KeyboardNavigation.h"
#include "ButtonBase.g.h"

using namespace DirectUI;

// Given a key, returns the appropriate navigation action.
/*static*/ void KeyboardNavigation::TranslateKeyToKeyNavigationAction(
    _In_ xaml::FlowDirection flowDirection,
    _In_ wsy::VirtualKey key,
    _Out_ xaml_controls::KeyNavigationAction* pNavAction,
    _Out_ bool* pIsValidKey)
{
    *pIsValidKey = true;
    *pNavAction = xaml_controls::KeyNavigationAction_Up;

    bool bInvertForRTL = (flowDirection == xaml::FlowDirection_RightToLeft);

    switch (key)
    {
    case wsy::VirtualKey_PageUp:
        *pNavAction = xaml_controls::KeyNavigationAction_Previous;
        break;

    case wsy::VirtualKey_PageDown:
        *pNavAction = xaml_controls::KeyNavigationAction_Next;
        break;

    case wsy::VirtualKey_Down:
        *pNavAction = xaml_controls::KeyNavigationAction_Down;
        break;

    case wsy::VirtualKey_Up:
        *pNavAction = xaml_controls::KeyNavigationAction_Up;
        break;

    case wsy::VirtualKey_Left:
        *pNavAction = (bInvertForRTL
            ? xaml_controls::KeyNavigationAction_Right
            : xaml_controls::KeyNavigationAction_Left);
        break;

    case wsy::VirtualKey_Right:
        *pNavAction = (bInvertForRTL
            ? xaml_controls::KeyNavigationAction_Left
            : xaml_controls::KeyNavigationAction_Right);
        break;

    case wsy::VirtualKey_Home:
        *pNavAction = xaml_controls::KeyNavigationAction_First;
        break;

    case wsy::VirtualKey_End:
        *pNavAction = xaml_controls::KeyNavigationAction_Last;
        break;

    default:
        *pIsValidKey = false;
        break;
    }
}
