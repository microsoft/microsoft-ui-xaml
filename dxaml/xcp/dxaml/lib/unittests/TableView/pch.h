// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <Windows.h>
#include <ole2.h>
#include <string>
#define wil_details_GetKernelBaseModuleHandle() GetModuleHandleW(L"kernelbase.dll")
