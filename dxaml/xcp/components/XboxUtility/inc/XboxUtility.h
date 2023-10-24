// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace XboxUtility {

bool IsOnXbox();
void DeleteIsOnXboxCache();

bool IsGamepadNavigationInput(_In_ wsy::VirtualKey key);

bool IsGamepadNavigationDirection(_In_ wsy::VirtualKey key);
bool IsGamepadPageNavigationDirection(_In_ wsy::VirtualKey key);

bool IsGamepadNavigationRight(_In_ wsy::VirtualKey key);
bool IsGamepadNavigationLeft(_In_ wsy::VirtualKey key);
bool IsGamepadNavigationUp(_In_ wsy::VirtualKey key);
bool IsGamepadNavigationDown(_In_ wsy::VirtualKey key);

bool IsGamepadNavigationAccept(_In_ wsy::VirtualKey key);
bool IsGamepadNavigationCancel(_In_ wsy::VirtualKey key);

xaml_input::FocusNavigationDirection GetNavigationDirection(_In_ wsy::VirtualKey key);
xaml_input::FocusNavigationDirection GetPageNavigationDirection(_In_ wsy::VirtualKey key);
} // namespace
