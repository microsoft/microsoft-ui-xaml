// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Provides logical and directional navigation between focusable objects.

#pragma once

namespace DirectUI
{
    class ButtonBase;

    // Provides logical and directional navigation between focusable objects.
    class KeyboardNavigation
    {
        private:
            KeyboardNavigation()
            {
            }

        public:

            // Given a key, returns the appropriate navigation action.
            static void TranslateKeyToKeyNavigationAction(
                _In_ xaml::FlowDirection flowDirection,
                _In_ wsy::VirtualKey key,
                _Out_ xaml_controls::KeyNavigationAction* pNavAction,
                _Out_ bool* pIsValidKey);

    };
}
