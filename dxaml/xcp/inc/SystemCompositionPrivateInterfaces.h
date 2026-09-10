// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// XAML targets the Windows 10 RS5 contract, but its Windows 11 implementation
// consumes and implements newer system Composition ABI. Expose the current SDK
// Composition declarations without raising the contract level for other APIs.
#pragma push_macro("WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION")
#undef WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION
#define WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION 0xf0000
#include <Windows.UI.Composition.h>
#pragma pop_macro("WINDOWS_FOUNDATION_UNIVERSALAPICONTRACT_VERSION")
