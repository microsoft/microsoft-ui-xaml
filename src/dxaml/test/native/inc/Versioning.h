// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

// To help TAEF deploy the customized AppxManifest for compatibility tests

// this file defines the versions for managed test.
// please make a corresponding change to dxaml/test/managed/common/Versions.cs when you update this file.

#define APPXMANIFEST_WINDOWS_VERSION_CURRENT             L"AppXManifest.native.current.xml"     // Windows 19H1
#define APPXMANIFEST_WINDOWS_VERSION_CURRENT_DM          L"AppXManifest.DesignMode.xml"         // Windows 19H1 with DesignMode support
#define APPXMANIFEST_WINDOWS_VERSION_CURRENT_CENTENNIAL  L"AppXManifest.Centennial.xml"         // Windows Centennial Latest

#define WINDOWS_OS_VERSION_20H2                         L"19042"
#define WINDOWS_OS_VERSION_19H1                         L"18362"
#define WINDOWS_OS_VERSION_RS5                          L"17763"
#define WINDOWS_OS_VERSION_RS4                          L"17134"
#define WINDOWS_OS_VERSION_22H2                         L"22621"