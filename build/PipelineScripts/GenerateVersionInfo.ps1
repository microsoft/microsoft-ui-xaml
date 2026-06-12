# Copyright (c) Microsoft Corporation and Contributors.
# Licensed under the MIT License.

# This script overrides AssemblyInfo.cs and AssemblyInfo.ver in build/VersionInfo.
# The ProductMajor and ProductMinor parameters are used for the ProductVersion field.

# AssemblyInfo.ver is used in an rc file to add VersionInfo to a dll.
# VERSIONINFO_FILENAME must be defined when used.
# VERSIONINFO_FILEVERSION and VERSIONINFO_FILEVERSION_STRING are optional.

# If VERSIONINFO_FILEVERSION and VERSIONINFO_FILEVERSION_STRING are not defined,
# ProductMajor and ProductMinor are used.

[CmdLetBinding()]
Param(
    [ValidateRange(0, 65535)]
    [int]$ProductMajor,

    [ValidateRange(0, 64)]
    [int]$ProductMinor,
    [string]$VersionInfoPath
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = 'Stop'

$utf8NoBomEncoding = New-Object System.Text.UTF8Encoding $False

$scriptFullPath =  (Split-Path -Parent $MyInvocation.MyCommand.Definition)
Write-Verbose "scriptFullPath: $scriptFullPath"

$companyName = "Microsoft Corporation"
$productName = "Windows App SDK"
$copyright = "Copyright (c) Microsoft Corporation. All rights reserved."

$productInfo = @"
// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

// This is a duplicate of the WinAppSDK repo file: dev\common\WindowsAppSdk-ProductInfo.h
// Everything should be kept in sync with the exception of potentially the major/minor
// release versions if WinUI has forked prior to WinAppSDK.

#ifndef __WINDOWSAPPSDK_PRODUCTINFO_H__
#define __WINDOWSAPPSDK_PRODUCTINFO_H__

#ifndef STR2
#define STR1(x) #x
#define VERSION_DELIMIMITER STR1(.)
#define STR2(a,b)     STR1(a) VERSION_DELIMIMITER STR1(b)
#endif

// If the specific version information has not been defined, then define it now.
// The default for WINUI is the WINUI major version, WINAPPSDK major version,
// WINAPPSDK minor version. This means that we ignore the WINUI_RELEASE_MINOR and
// WINUI_RELEASE_PATCH variables.

#ifndef WINUI_RELEASE_MAJOR
#define WINUI_RELEASE_MAJOR                                 3
#endif
#ifndef WINUI_BUILD_VERSION
#define WINUI_BUILD_VERSION                                 0
#endif

#define WINDOWSAPPSDK_RELEASE_MAJOR                         $ProductMajor
#define WINDOWSAPPSDK_RELEASE_MINOR                         $ProductMinor

#define WINDOWSAPPSDK_PRODUCT_VERSION    WINDOWSAPPSDK_RELEASE_MAJOR, WINDOWSAPPSDK_RELEASE_MINOR
#define WINDOWSAPPSDK_PRODUCT_VERSION_STRING   STR2(WINDOWSAPPSDK_RELEASE_MAJOR, WINDOWSAPPSDK_RELEASE_MINOR)
#define WINDOWSAPPSDK_COMPANY_NAME              "$companyName"
#define WINDOWSAPPSDK_LEGAL_COPYRIGHT           "$copyright"
#define WINDOWSAPPSDK_PRODUCT_NAME              "$productName"

#endif // __WINDOWSAPPSDK_PRODUCTINFO_H__
"@

Write-Host $productInfo
Write-Host "Writing $VersionInfoPath..."
[System.IO.File]::WriteAllLines($VersionInfoPath, $productInfo, $utf8NoBomEncoding)
