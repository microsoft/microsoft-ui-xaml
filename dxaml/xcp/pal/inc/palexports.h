// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

//  Abstract:
//      Contains the static methods that provide access to the PAL interfaces

#ifndef __INCLUDE_PAL_EXPORTS__
#define __INCLUDE_PAL_EXPORTS__

struct IPALDebuggingServices;
struct IPALMemoryServices;
struct IPALURIServices;
struct IPALMathServices;
struct IPALPrintIOServices;
struct IPALThreadingServices;
struct IPALTextServices;
struct IPALCoreServices;

//
// These functions must be implemented but the respective platform
// PAL implementation.
//

// Extenal defintions as we need stuff from SL PAL for this to work.
extern IPALDebuggingServices* __stdcall GetPALDebuggingServices();
extern IPALMemoryServices* __stdcall GetPALMemoryServices();
extern IPALURIServices* __stdcall GetPALURIServices();
extern IPALMathServices* __stdcall GetPALMathServices();
extern IPALPrintIOServices* __stdcall GetPALPrintIOServices();
extern IPALThreadingServices* __stdcall GetPALThreadingServices();
extern IPALTextServices* __stdcall GetPALTextServices();
extern IPALCoreServices* __stdcall GetPALCoreServices();

//------------------------------------------------------------------------
//
//  Method:   XcpDebugSetLeakDetectionFlag
//
//  Synopsis:
//      Mark LeakDetection flag in individual object that was allocated
//      through XcpDebugAllocate.
//
//------------------------------------------------------------------------

extern void XcpDebugSetLeakDetectionFlag(
    _In_ void *pAddress,
    _In_ bool fDisableLeakDetection
    );

#endif // __INCLUDE_PAL_CORE__

