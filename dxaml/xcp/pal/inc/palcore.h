// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Abstract:
//      Contains the types and methods provided by the core platform abstraction layer

#pragma once

struct ITextRange2;

#include "paltypes.h"
#include "paldebugging.h"
#include "palmemory.h"
#include "palexports.h"
#include "xstring_ptr.h"
#include "palfileuri.h"
#include "palthread.h"
#include "RefCounting.h"
#include "PalWorkItem.h"
#include "PalPrintingData.h"
#include "palgfx.h"
#include "palnetwork.h"
#include "paltext.h"
#include "PalPrintingServices.h"
#include "PalDirectManipulationCompositorService.h"
#include "PalDirectManipulationService.h"
#include "palInputPaneInteraction.h"
#include "paltouchservices.h"
#include "PALProcessCharacteristics.h"
#include "PalAutomationServices.h"
#include "xamlbehaviormode.h"

template <class T> class xvector;
#include <vadefs.h>

#ifndef __INCLUDE_PAL_CORE__
#define __INCLUDE_PAL_CORE__

//------------------------------------------------------------------------
//
//  Interface:  IPALApplicationSingleton
//
//  Synopsis:
//      PAL control object used in the construction/deconstruction
//  of singletons.
//
//------------------------------------------------------------------------

struct IPALApplicationSingleton
{
    virtual _Check_return_ XUINT32 Release() = 0;
};


//--------------------------------------------------------
//--------------------------------------------------------
//
// PAL Data Interfaces
//
//--------------------------------------------------------
//--------------------------------------------------------

//------------------------------------------------------------------------
//
//  Interface:  IPALClock
//
//  Synopsis:
//      PAL clock object
//
//------------------------------------------------------------------------
struct IPALClock : public IObject
{
protected:
    ~IPALClock(){}  // 'delete' not allowed, use 'Release' instead.

public:
    // Get an absolute time in seconds.  The base is arbitrary, but the delta between calls can be used to
    // measure elapsed time.
    virtual XDOUBLE GetAbsoluteTimeInSeconds() = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALRegion
//
//  Synopsis:
//      Provides an abstraction of region tracking
//
//------------------------------------------------------------------------
struct IPALRegion : public IObject
{
    virtual _Check_return_ HRESULT Add(_In_ const XRECT *pRect) = 0;
    virtual _Check_return_ HRESULT Remove(_In_ const XRECT *pRect) = 0;
    virtual _Check_return_ HRESULT Intersect(_In_ const XRECT *pRect) = 0;
    virtual _Check_return_ HRESULT GetRects(_Outptr_result_buffer_(*pRectCount) XRECT **ppRects, _Out_ XUINT32 *pRectCount) = 0;
    virtual _Check_return_ HRESULT GetInverseRects(_Inout_ xvector<XRECT> *pArray, _In_ const XRECT_RB *pBounds) = 0;
    virtual _Check_return_ HRESULT IsSubsetOf(_In_ IPALRegion *pOtherRegion, _Out_ bool *pIsSubset) = 0;
    virtual _Check_return_ HRESULT Contains(_In_ const XRECT *pRect, _Out_ bool *pContainsRect) = 0;
};


//--------------------------------------------------------
//--------------------------------------------------------
//
// PAL Service Interfaces
//
//--------------------------------------------------------
//--------------------------------------------------------


//------------------------------------------------------------------------
//
//  Interface:  IPlatformUtilities
//  Synopsis:
//       Utilities that needs to be implemented per platform
//
//------------------------------------------------------------------------
struct IPlatformUtilities
{
    virtual _Check_return_ __ecount_opt((sourceStringLen + 1)) WCHAR* Xstralloc(
        const _In_reads_opt_(sourceStringLen) WCHAR* sourceString,
        const _In_ XUINT32  sourceStringLen) = 0;

    virtual void Xstrfree(
        _In_z_ WCHAR* theString) = 0;

    virtual void * Xstrncpy(
        _Out_writes_(cChar) WCHAR *pTrg,
        _In_reads_(cChar) const WCHAR *pSrc,
        _In_ XUINT32 cChar) = 0;

    virtual XUINT32 Xstrlen(
        _Null_terminated_ const WCHAR *pString ) = 0;

    virtual XINT32 Xstrncmpi(
        const _In_z_ WCHAR *pTrg,
        const _In_z_ WCHAR *pSrc,
        _In_ XUINT32 cChar) = 0;

    virtual XINT32 Xstrncmp(
        const _In_z_ WCHAR *pTrg,
        const _In_z_ WCHAR *pSrc,
        _In_ XUINT32 cChar) = 0;

    virtual XUINT32 Xswprintf_s(
        _Out_writes_z_(cchBuf) WCHAR *pBuf,
        XUINT32 cchBuf,
        _In_z_ const WCHAR* pFormat,
        ...
        ) = 0;

    virtual XUINT32 vXswprintf_s(
        _Out_writes_z_(cchBuf) WCHAR *pBuf,
        XUINT32 cchBuf,
        _In_z_ const WCHAR* pFormat,
        _In_ va_list args
        ) = 0;

    virtual bool IsUILanguageRTL() = 0;
};


// DEPRECATED: Do not use IPALMathServices in new code. Use built-in C++ math
// functions directly.
struct IPALMathServices
{
    virtual _Check_return_ HRESULT CreateRegion(_Outptr_ IPALRegion **ppRegion) = 0;
};

// DEPRECATED: Do not use IPALPrintIOServices in new code. Use built-in C++ string
// functions directly.
struct IPALPrintIOServices
{
// Safe print functions

    virtual _Check_return_ HRESULT PrintStringCchW(
                                    _Out_writes_(cchDest) WCHAR *pszDest,
                                    _In_ XUINT32 cchDest,
                                    _In_z_ const WCHAR *pszFormat,
                                    ...) = 0;

    virtual _Check_return_ HRESULT PrintStringCchExW(
                                    _Out_writes_(cchDest) WCHAR *pszDest,
                                    _In_ XUINT32 cchDest,
                                    _Outptr_opt_result_buffer_(*pcchRemaining) WCHAR **ppszDestEnd,
                                    _Out_opt_ XUINT32 *pcchRemaining,
                                    _In_ XUINT32 dwFlags,
                                    _In_z_ const WCHAR *pszFormat,
                                    ...) = 0;

};

namespace ThemingData
{
    struct OpacitySplineTransform
    {
        XFLOAT startTime;       // The time in seconds when the transform starts
        XFLOAT durationTime;    // The duration of the segment
        XFLOAT startValue;      // Starting value of the opacity
        XFLOAT p1;              // First coefficient of the cubic Bezier curve
        XFLOAT p2;              // Second coefficient of the cubic Bezier curve
        XFLOAT endValue;        // Ending value of the opacity
        bool hasInitialValue;  // Flag to preset the initial value if there is a gap in start time
    };
};

struct IPALThemingServices
{
    virtual _Check_return_ HRESULT GetFadeInThemeAnimationData( _Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms ) = 0;
    virtual _Check_return_ HRESULT GetFadeOutThemeAnimationData( _Out_ XINT32 *pCount, _Outptr_result_buffer_(*pCount) ThemingData::OpacitySplineTransform **ppTransforms ) = 0;
};

//------------------------------------------------------------------------
//
//  Interface:  IPALCoreServices
//
//  Synopsis:
//      Provides an abstraction for core system support. This abstract interface
//      should be used to define the MINIMUM core functionality that we
//      wish to expose external to Silverlight.
//
//------------------------------------------------------------------------
struct IPALCoreServices
{
// File services

    virtual _Check_return_ HRESULT FileCreate(_In_ XUINT32 cName, _In_reads_(cName) const WCHAR *pName, XINT32 fileOptions, _Out_ IPALFile **ppFile) = 0;

// OS System Settings

    virtual _Check_return_ HRESULT GetPlatformUtilities(_Outptr_ IPlatformUtilities** ppInterface) = 0;

// Time services

    virtual               XUINT64 GetCPUTime() = 0;
    virtual               XUINT64 GetCPUMilliseconds() = 0;
    virtual _Check_return_ HRESULT CreateClock(_Out_ IPALClock **ppIClock) = 0;

// CPU Info
    virtual _Check_return_ HRESULT GetNumberOfCPUCores(_Out_ XUINT32 *pcProcessorCores) = 0;
    virtual                bool IsSupportedPlatform() = 0;

// Process wide control mechanisms
    virtual _Check_return_ HRESULT GetApplicationSingleton(_Out_ IPALApplicationSingleton **ppSingleton) = 0;
    virtual XUINT32 GenerateSecurityToken() = 0;

// Keyboard states
    virtual _Check_return_ HRESULT GetKeyboardModifiersState( _Out_ XUINT32 *pModifiers) = 0;

    virtual XUINT32 /* bool */ PerformanceFrequency(_Out_ XINT64_LARGE_INTEGER *lpFrequency) = 0;
    virtual XUINT32 /* bool */ PerformanceCounter(_Out_ XINT64_LARGE_INTEGER *lpPerformanceCount) = 0;

// COM initialization

    virtual _Check_return_ HRESULT CallCoInitializeMTA() = 0;
    virtual void CallCoUninitialize() = 0;

// Remote desktop
    virtual _Check_return_ HRESULT IsOnRemoteDesktopSession( _Out_ bool* pfOnRemoteDesktop ) = 0;
};

// Right now we define our own memory allocators for Jupiter in debug builds. This
// historically was done to support a feature called XcpLeakDetector. This feature
// tracks internally allocated memory and raises an error in debug builds if Jupiter
// exits with allocated memory.
//
// In fre build we also define our own memory allocator to keep track of current
// total allocation to report to the GC and to FAIL_FAST on failed small allocations.
//
// TODO: Our allocator can return null. A better SAL annotation for the return
// value (and the other new operator) is: _Ret_maybenull_ _Post_writable_byte_size_(_Size)
#if DBG && !defined(NO_XCP_NEW_AND_DELETE)

// In checked builds, memory allocation is done through the memory allocator
// in the PAL debugging service.
_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize);
_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new[](size_t cSize);

enum NoFailFastAllocationPolicy;
enum ZeroMemAllocationPolicy;

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, NoFailFastAllocationPolicy);
_Ret_notnull_ _Post_writable_byte_size_(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, NoFailFastAllocationPolicy);

_Ret_notnull_ _Post_writable_byte_size_(cSize) __declspec(allocator) void * __cdecl operator new(size_t cSize, ZeroMemAllocationPolicy);
_Ret_notnull_ _Post_writable_byte_size_(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, ZeroMemAllocationPolicy);

#pragma warning(push)
// Some header files have their own definition of this function and don't
// properly annotate it. We disable this warning to avoid inconsistent annotation
// errors for now.
#pragma warning(disable:28301)
void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress);
void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress);
#pragma warning(pop)

#elif !defined(NO_XCP_NEW_AND_DELETE)

enum NoFailFastAllocationPolicy;
enum ZeroMemAllocationPolicy;

// In fre builds we use the pal memory services to perform a failfast
// if a small allocation failed and report memory usage to the GC.

__declspec(allocator) void * __cdecl operator new(size_t cSize);
__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize);

__declspec(allocator) void * __cdecl operator new(size_t cSize, NoFailFastAllocationPolicy);
__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, NoFailFastAllocationPolicy);

__declspec(allocator) void * __cdecl operator new(size_t cSize, ZeroMemAllocationPolicy);
__bcount(cSize) XCP_FORCEINLINE __declspec(allocator) void * __cdecl operator new[](size_t cSize, ZeroMemAllocationPolicy);

#pragma warning(push)
// Some header files have their own definition of this function and don't
// properly annotate it. We disable this warning to avoid inconsistent annotation
// errors for now.
#pragma warning(disable:28301)
void __cdecl operator delete(_Frees_ptr_opt_ void *pAddress);
void __cdecl operator delete[](_Frees_ptr_opt_ void *pAddress);
#pragma warning(pop)

#endif // #if DBG && !defined(NO_XCP_NEW_AND_DELETE)

#include "PalResourceManager.h"
#include "xcontainers.h"
#include "PalNotify.h"

#endif // __INCLUDE_PAL_CORE__
