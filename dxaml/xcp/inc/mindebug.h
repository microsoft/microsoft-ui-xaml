// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A bare set of debug headers needed to work effectively with the XAML 
// framework. This include fileis designed to be included from unit tests 
// and other places where we'd like to build isolated classes.

#pragma once

// Devices note: CE does not have some of the symbol lookup mechanism so the debugging utility does not work currently
//  This is being turned off for CE for the following reasons:
//  1. This increases code size on debug builds which is already hitting limits of VM on downlevel CE devices
//  2. The leak detection infrastructure needs work and potentially we can do this on an as needed basis.

#ifdef _PREFAST_
    #define XCP_MONITOR 0
#else
    #ifndef XCP_MONITOR
        #if DBG
            #define XCP_MONITOR 1
        #else
            #define XCP_MONITOR 0
        #endif
    #endif
#endif

#if XCP_MONITOR

#ifdef __cplusplus
extern "C"
#endif
void XcpMarkWeakPointer(
_In_ void* thisPtr,
_In_ void* ptrToWeakPtr);

#ifdef __cplusplus
extern "C"
#endif
void XcpMarkStrongPointer(
_In_ void* thisPtr,
_In_ void* ptrToStrongPtr);

#ifdef __cplusplus
extern "C"
#endif
void XcpMarkPartTimeStrongPointer(
_In_ void* thisPtr,
_In_ void* ptrToPartTimeStrongPtr,
_Out_ void** context);

#ifdef __cplusplus
extern "C"
#endif
void XcpIncPartTimeStrongPointer(
_In_ void* context);

#ifdef __cplusplus
extern "C"
#endif
void XcpDecPartTimeStrongPointer(
_In_ void* context);


#ifdef __cplusplus
extern "C"
#endif
void XcpMarkHardToTrack(
_In_ void* hardToTrackBlock);

#else

#define XcpMarkWeakPointer(thisPtr, ptrToWeakPtr)
#define XcpMarkStrongPointer(thisPtr, ptrToStrongPtr)
#define XcpMarkPartTimeStrongPointer(thisPtr, ptrToPartTimeStrongPtr, context)
#define XcpIncPartTimeStrongPointer(context)
#define XcpDecPartTimeStrongPointer(context)
#define XcpMarkHardToTrack(hardToTrackBlock)

#endif

// Save some typing for the common case of marking members of "this" as weak or strong:
#define XCP_WEAK(ptrToWeakPtr) XcpMarkWeakPointer(this, ptrToWeakPtr)
#define XCP_STRONG(ptrToStrongPtr) XcpMarkStrongPointer(this, ptrToStrongPtr)

// The base regkey where various XAML debug flags are stored
#define XAML_ROOT_KEY L"Software\\Microsoft\\WinUI\\XAML"
