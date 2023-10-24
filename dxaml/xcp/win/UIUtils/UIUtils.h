// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:  Common functions used by features with Windows UI such as
//             the Updater and the EULA prompt

#pragma once

#include <windows.h>

HRSRC FindResourceExWithFallback(
    HINSTANCE hModule,
    LPCTSTR  lpType,
    LPCTSTR  lpName,
    LANGID wLanguage);