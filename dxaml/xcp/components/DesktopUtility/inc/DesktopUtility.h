// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DesktopUtility {

bool IsOnDesktop();
// Windows 11 still reports version 10.0; build 22000 is the boundary.
// AppCompat shims can report an older version, so callers must tolerate a
// true result on Windows 11. A failed version query returns false.
bool IsPriorToWindows11() noexcept;
void DeleteIsOnDesktopCache();
} // namespace
