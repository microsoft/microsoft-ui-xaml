// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DesktopUtility {

bool IsOnDesktop();
bool IsOnWindows10() noexcept;
void DeleteIsOnDesktopCache();
} // namespace
