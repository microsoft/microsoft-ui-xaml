// Copyright (c) Microsoft Corporation and Contributors.
// Licensed under the MIT License.

// This is a duplicate of the WinAppSDK repo file: dev\common\WindowsAppSdk-ProductInfo.h
// Everything should be kept in sync with the exception of potentially the major/minor
// release versions if WInUI3 has forked prior to WinAppSDK.

#ifndef __WINDOWSAPPSDK_PRODUCTINFO_H__
#define __WINDOWSAPPSDK_PRODUCTINFO_H__

#ifndef STR2
#define STR1(x) #x
#define VERSION_DELIMIMITER STR1(.)
#define STR2(a,b)     STR1(a) VERSION_DELIMIMITER STR1(b)
#endif

#define WINDOWSAPPSDK_RELEASE_MAJOR                         1
#define WINDOWSAPPSDK_RELEASE_MINOR                         5

#define WINDOWSAPPSDK_PRODUCT_VERSION    WINDOWSAPPSDK_RELEASE_MAJOR, WINDOWSAPPSDK_RELEASE_MINOR
#define WINDOWSAPPSDK_PRODUCT_VERSION_STRING   STR2(WINDOWSAPPSDK_RELEASE_MAJOR, WINDOWSAPPSDK_RELEASE_MINOR)
#define WINDOWSAPPSDK_COMPANY_NAME    "Microsoft Corporation"
#define WINDOWSAPPSDK_LEGAL_COPYRIGHT    "Copyright (c) Microsoft Corporation. All rights reserved."
#define WINDOWSAPPSDK_PRODUCT_NAME    "Windows App SDK"

#endif // __WINDOWSAPPSDK_PRODUCTINFO_H__
