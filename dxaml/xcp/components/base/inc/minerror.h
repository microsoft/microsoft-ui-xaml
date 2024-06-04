// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// A bare set of error handling macros needed to work with classes
// fundemental to the XAML framework (xref_ptr, xstring, etc). This include file
// is designed to be included from unit tests and other places where we'd
// like to build isolated objects.

#pragma once
#include "ErrorContext.h"
#include "DebugWriter.h"
#include "AssertMacros.h"
#include <wil/common.h> // wil::verify_hresult
#include <wil/result_macros.h>

#define UNCONDITIONAL_EXPR(expr) (0, expr)
typedef struct _STOWED_EXCEPTION_INFORMATION_V2 *PSTOWED_EXCEPTION_INFORMATION_V2;
void __stdcall FailFastWithStowedExceptions(HRESULT hrError, ULONG cStowedExceptions, _In_reads_opt_(cStowedExceptions) PSTOWED_EXCEPTION_INFORMATION_V2 aStowedExceptionPointers[]) noexcept;

// FAIL_ASSERT_HERE() and XAML_FAIL_FAST() are implemented as macros to allow the debugger
// and telemetry tools to point directly at the function which triggered the assert or
// fail-fast.
//
// Assert failures with debugger attached won't pack up the thread's error context info,
// but in the debugger (or in full dumps), the error context info will be reachable.
// See g_dwErrorContextTlsIndex for a starting point to manually use the debugger to get
// to the stowed error contexts for the current thread, if any.
//
// When a user mode debugger is not present, pack up stowed exceptions to maximize the
// info available from dumps that don't have all the heap.
//
// This overrides the FAIL_ASSERT_HERE macro that is defined in AssertMacros.h.  This allows
// us to run our own our own fail-fast code to package up custom error information and
// included it in the crashdump.
#undef FAIL_ASSERT_HERE
#define FAIL_ASSERT_HERE(x) \
    do { \
        __annotation(L"Debug", L"AssertFail", (x)); \
        DbgRaiseAssertionFailure(); \
        if (IsDebuggerPresent() == FALSE) \
        { \
            XAML_FAIL_FAST(); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

// This macro is used to fail-fast on internal failures only.
// Do not use this to fail-fast when an error originated in app code.
#define XAML_FAIL_FAST() \
    do { \
        PSTOWED_EXCEPTION_INFORMATION_V2* ppStowedExceptions; \
        unsigned int cStowedExceptions; \
        TraceForFailFast(E_UNEXPECTED); \
        OnNewFailureEncountered(E_UNEXPECTED, nullptr, nullptr); \
        GetStowedExceptionsForFailFast(&ppStowedExceptions, &cStowedExceptions); \
        FailFastWithStowedExceptions(E_UNEXPECTED, cStowedExceptions, ppStowedExceptions); \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

// This macro will cause a FAIL_FAST in retail and debug builds when the
// given condition is not met.
#define FAIL_FAST_ASSERT(condition) \
    do { \
    __pragma(warning(suppress:4127)) \
        if (condition) \
        { \
        } \
        else \
        { \
            XAML_FAIL_FAST(); \
        } \
    } while(0)

// We've temporarily removed these telemetry asserts, later we'll switch them to use the public APIs
// Task 29627489: Lifted XAML: Use public telemetry APIs in place of calls to private OS telemetry APIs (which we've disabled)
#define MICROSOFT_TELEMETRY_ASSERT_DISABLED(expr) ASSERT(expr)
#define MICROSOFT_TELEMETRY_ASSERT_MSG_DISABLED(expr, msg) ASSERT(expr)

#if DBG
#define MICROSOFT_TELEMETRY_ASSERT_HR(x) \
    { \
        HRESULT _hr_ = (x); \
        MICROSOFT_TELEMETRY_ASSERT_DISABLED(SUCCEEDED(_hr_)); \
    }
#else
#define MICROSOFT_TELEMETRY_ASSERT_HR(x) (x)
#endif

// This macro is used to fail-fast without generating a new error context at the
// point of the fail-fast.
//
// This should be used for failing fast due to an error that orignated
// somewhere else and propagated to the point of the fail-fast.
//
// If the error has an associated error info object on the thread, it's critical
// that the HRESULT passed here matches the error code of the thread's error info
// object. If they don't match, RoFailFast will ignore the thread's error info
// and it won't be included in the crash dump.
//
// If the error that caused the fail-fast doesn't have an associated error info
// object on the thread, then you can pass anything for the HRESULT here.
#define FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(hr) \
    do { \
        PSTOWED_EXCEPTION_INFORMATION_V2* ppStowedExceptions; \
        unsigned int cStowedExceptions; \
        TraceForFailFast(hr); \
        GetStowedExceptionsForFailFast(&ppStowedExceptions, &cStowedExceptions); \
        FailFastWithStowedExceptions(hr, cStowedExceptions, ppStowedExceptions); \
    } \
    __pragma(warning(suppress:4127)) \
    while (0)

#ifndef WIDEN
#define WIDEN2(x) L ## x
#define WIDEN(x) WIDEN2(x)
#endif

#ifndef __WFILE__
#define __WFILE__ WIDEN(__FILE__)
#endif

#define WIDEN_VA(x,y) WIDEN(#y)

#pragma region IFC Macros

#if DBG
// DEPRECATED: Consider using LOG for OutputWindow-style debugging. For more sophisticated
// tracing consider using ETW markers.
#define TRACE(flag, ...) \
    do { \
        DisplayDebugMessage(DebugWriter::DebugWriterType::DebugOutputOnly, ErrorHandling::LoggingLevel::Info, __WFILE__, __LINE__, 0, NULL, WIDEN_VA(,#__VA_ARGS__), ##__VA_ARGS__); \
    } \
    __pragma(warning(suppress:4127)) \
    while(UNCONDITIONAL_EXPR(0))

// Prints the given text to the screen in DBG builds, accepts a printf-style variable list of
// arguments.
#define LOG(...) \
    do { \
        DisplayDebugMessage(DebugWriter::DebugWriterType::DebugOutputOnly, ErrorHandling::LoggingLevel::Info, __WFILE__, __LINE__, 0, NULL, WIDEN_VA(,#__VA_ARGS__), ##__VA_ARGS__); \
    } \
    __pragma(warning(suppress:4127)) \
    while(UNCONDITIONAL_EXPR(0))

#define LOG_INFO_EX(...) \
    do { \
        DisplayDebugMessage(DebugWriter::DebugWriterType::UseLoggerIfPresent, ErrorHandling::LoggingLevel::Info, __WFILE__, __LINE__, 0, NULL, WIDEN_VA(,#__VA_ARGS__), ##__VA_ARGS__); \
    } \
    __pragma(warning(suppress:4127)) \
    while(UNCONDITIONAL_EXPR(0))

#define LOG_WARNING_EX(...) \
    do { \
        DisplayDebugMessage(DebugWriter::DebugWriterType::UseLoggerIfPresent, ErrorHandling::LoggingLevel::Warning, __WFILE__, __LINE__, 0, NULL, WIDEN_VA(,#__VA_ARGS__), ##__VA_ARGS__); \
    } \
    __pragma(warning(suppress:4127)) \
    while(UNCONDITIONAL_EXPR(0))

#define LOG_LEAK_EX(...) \
    do { \
        DisplayDebugMessage(DebugWriter::DebugWriterType::UseLoggerIfPresent, ErrorHandling::LoggingLevel::Leak, __WFILE__, __LINE__, 0, NULL, WIDEN_VA(,#__VA_ARGS__), ##__VA_ARGS__); \
    } \
    __pragma(warning(suppress:4127)) \
    while(UNCONDITIONAL_EXPR(0))

// DISPLAY_DEBUG calls out to WIL instead of using DisplayDebugMessage.  This ensures that errors are logged to WIL and are also output to the console.
#define DISPLAY_DEBUG(hr, testString) LOG_HR_MSG((hr), (testString));

#define DISPLAY_DIAGNOSTICS_RELEASE(diagnosticMessage) DisplayReleaseMessage((diagnosticMessage));

#define LOG_ASSERT(cond, ...) \
    do { \
        _Analysis_assume_(!!(cond)); \
        __pragma(warning(suppress:4127)) \
        if(!(cond)) \
        { \
            LOG_WARNING_EX(XCPW(#cond)); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

#else
#define TRACE(flag, ...)
#define LOG(...)
#define LOG_INFO_EX(...)
#define LOG_WARNING_EX(...)
#define LOG_LEAK_EX(...)
#define DISPLAY_DEBUG(hr, testString)
#define DISPLAY_DIAGNOSTICS_RELEASE(diagnosticMessage) DisplayReleaseMessage((diagnosticMessage));
#define LOG_ASSERT(cond, ...)
#endif

// Define two distinct types that we can use to detect mismatches
namespace Details
{
    struct ifc_return_type {};
    struct ifc_goto_type   {};
}

#define CHECK_MISMATCH_GOTO                    \
    __if_exists(active_ifc_pattern) {          \
        static_assert(                         \
            std::is_same<active_ifc_pattern,   \
            ::Details::ifc_goto_type>::value,  \
            "Don't mix goto/return IFCs!");    \
    }                                          \
    typedef ::Details::ifc_goto_type active_ifc_pattern;

#define CHECK_MISMATCH_RETURN                    \
    __if_exists(active_ifc_pattern) {            \
        static_assert(                           \
            std::is_same<active_ifc_pattern,     \
            ::Details::ifc_return_type>::value,  \
            "Don't mix goto/return IFCs!");      \
    }                                            \
    typedef ::Details::ifc_return_type active_ifc_pattern;

#define IFC(x) \
    { \
        hr = (x); \
        wil::verify_hresult(hr); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(hr)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(hr); \
            DISPLAY_DEBUG(hr, #x) \
            goto Cleanup; \
        } \
    } \
    CHECK_MISMATCH_GOTO

#define IFC_EXTRA_INFO(x, extraInfo) \
    { \
        hr = (x); \
        wil::verify_hresult(hr); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(hr)) \
        { \
        } \
        else \
        { \
            OnFailureWithExtraInfo<__COUNTER__>(hr, extraInfo); \
            DISPLAY_DEBUG(hr, #x) \
            if (extraInfo) \
            { \
                for (const auto& _extraInfoEntry_ : *extraInfo) \
                { \
                    DISPLAY_DIAGNOSTICS_RELEASE(_extraInfoEntry_.c_str()) \
                } \
            } \
            goto Cleanup; \
        } \
    } \
    CHECK_MISMATCH_GOTO

#define IFC_ALLOW_INVALIDARG(x) \
    { \
        hr = (x); \
        wil::verify_hresult(hr); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(hr) || (hr == E_INVALIDARG)) \
        { \
            hr = S_OK; \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(hr); \
            DISPLAY_DEBUG(hr, #x) \
            goto Cleanup; \
        } \
    } \
    CHECK_MISMATCH_GOTO

#define IFC_RETURN(x) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(_hr_)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(_hr_); \
            DISPLAY_DEBUG(_hr_, #x) \
            return _hr_; \
        } \
    } \
    CHECK_MISMATCH_RETURN

#define IFC_RETURN_EXTRA_INFO(x, extraInfo) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(_hr_)) \
        { \
        } \
        else \
        { \
            OnFailureWithExtraInfo<__COUNTER__>(_hr_, extraInfo); \
            DISPLAY_DEBUG(_hr_, #x) \
            if (extraInfo) \
            { \
                for (const auto& _extraInfoEntry_ : *extraInfo) \
                { \
                    DISPLAY_DIAGNOSTICS_RELEASE(_extraInfoEntry_.c_str()) \
                } \
            } \
            return _hr_; \
        } \
    } \
    CHECK_MISMATCH_RETURN

#define IFC_RETURN_ALLOW_INVALIDARG(x) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(_hr_) || (_hr_ == E_INVALIDARG)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(_hr_); \
            DISPLAY_DEBUG(_hr_, #x) \
            return _hr_; \
        } \
    } \
    CHECK_MISMATCH_RETURN

#define IFC_RETURN_ALLOW(x, y) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(_hr_) || (_hr_ == (y))) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(_hr_); \
            DISPLAY_DEBUG(_hr_, #x) \
            return _hr_; \
        } \
    } \
    CHECK_MISMATCH_RETURN

#define IFCOOM(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        hr = E_OUTOFMEMORY; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCOOM_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        constexpr HRESULT _hr_ = E_OUTOFMEMORY; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCPTR(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        hr = E_POINTER; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCPTR_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        constexpr HRESULT _hr_ = E_POINTER; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCHNDL(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != NULL) \
    { \
    } \
    else \
    { \
        hr = E_HANDLE; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCHNDL_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) != NULL) \
    { \
    } \
    else \
    { \
        constexpr HRESULT _hr_ = E_HANDLE; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCCHECK(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        hr = E_FAIL; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCCHECK_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        constexpr HRESULT _hr_ = E_FAIL; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCCATASTROPHIC(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        ASSERT(!"Internal Error"); \
        hr = E_UNEXPECTED; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCCATASTROPHIC_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        ASSERT(!"Internal Error"); \
        constexpr HRESULT _hr_ = E_UNEXPECTED; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCEXPECT(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        hr = E_UNEXPECTED; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCEXPECT_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        constexpr HRESULT _hr_ = E_UNEXPECTED; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN

#define IFCEXPECTRC(x, y) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        hr = y; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCEXPECTRC_RETURN(x, y) \
    __pragma(warning(suppress:4127)) \
    if (!!(x)) \
    { \
    } \
    else \
    { \
        const HRESULT _hr_ = y; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCPTRRC(x, y) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        hr = y; \
        OnNewFailure<__COUNTER__>(hr); \
        DISPLAY_DEBUG(hr, #x) \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCPTRRC_RETURN(x, y) \
    __pragma(warning(suppress:4127)) \
    if ((x) != nullptr) \
    { \
    } \
    else \
    { \
        const HRESULT _hr_ = y; \
        OnNewFailure<__COUNTER__>(_hr_); \
        DISPLAY_DEBUG(_hr_, #x) \
        return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


#define IFCONTINUE(x) \
    { \
        hr = (x); \
        wil::verify_hresult(hr); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(hr)) \
        { \
        } \
        else \
        { \
            hrTemp = hr; \
            DISPLAY_DEBUG(hr, #x) \
        } \
    }

// IFCW32 macros handle the return value from a Win32 function.

#define IFCW32(x)                                                           \
    __pragma(warning(suppress:4127))                                        \
    if ((x) == FALSE)                                                       \
    {                                                                       \
        hr = HRESULT_FROM_WIN32(GetLastError());                            \
        if (SUCCEEDED(hr)) hr = E_FAIL;                                     \
        OnNewFailure<__COUNTER__>(hr);                                      \
        DISPLAY_DEBUG(hr, #x)                                              \
        goto Cleanup;                                                       \
    }                                                                       \
    CHECK_MISMATCH_GOTO

#define IFCW32_RETURN(x)                                                    \
    __pragma(warning(suppress:4127))                                        \
    if ((x) == FALSE)                                                       \
    {                                                                       \
        HRESULT _hr_ = HRESULT_FROM_WIN32(GetLastError());                  \
        if (SUCCEEDED(_hr_)) _hr_ = E_FAIL;                                 \
        OnNewFailure<__COUNTER__>(_hr_);                                    \
        DISPLAY_DEBUG(_hr_, #x)                                            \
        return _hr_;                                                        \
    }                                                                       \
    CHECK_MISMATCH_RETURN

// These versions of the IFCW32 macros will clear the error code prior to
// making the call.

#define IFCW32SLE(x)                                                        \
    {                                                                       \
        ::SetLastError(ERROR_SUCCESS);                                      \
        IFCW32(x);                                                          \
    }

#define IFCW32SLE_RETURN(x)                                                 \
    {                                                                       \
        ::SetLastError(ERROR_SUCCESS);                                      \
        IFCW32_RETURN(x);                                                   \
    }

#define IFCOOMFAILFAST(x) \
    { \
        __pragma(warning(suppress:4127)) \
        if ((x) != nullptr) \
        { \
        } \
        else \
        { \
            IFCFAILFAST(E_OUTOFMEMORY); \
        } \
    }

#define IFCFAILFAST(x) \
    { \
        __pragma(warning(suppress:26498)) \
        const HRESULT __HR = (x); \
        wil::verify_hresult(__HR); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(__HR)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(__HR); \
            DISPLAY_DEBUG(__HR, #x) \
            FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(__HR); \
        } \
    }

#define IFCFAILFAST_ALLOW_INVALIDARG(x) \
    { \
        const HRESULT __HR = (x); \
        wil::verify_hresult(__HR); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(__HR) || (__HR == E_INVALIDARG)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(__HR); \
            DISPLAY_DEBUG(__HR, #x) \
            FAIL_FAST_USING_EXISTING_ERROR_CONTEXT(__HR); \
        } \
    }

#define IFCW32FAILFAST(x) \
{ \
    if ((x) == FALSE) { IFCFAILFAST(HRESULT_FROM_WIN32(GetLastError())); } \
}

// This macro will cause a FAIL_FAST in retail and debug builds when the
// given condition is not met.
#define XCP_FAULT_ON_FAILURE(condition) FAIL_FAST_ASSERT(condition)

#define IGNOREHR(hr) (void) (hr)

// DEPRECATED: APIs should not return incorrect HRESULTs when operating
// normally. If this is an error condition please use the tracing version
// of this macro.
#define IFC_NOTRACE(x) \
    { \
        hr = (x); \
        wil::verify_hresult(hr); \
        __pragma(warning(suppress:4127)) \
        if (FAILED(hr)) \
            goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFC_NOTRACE_RETURN(x) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (FAILED(_hr_)) \
            return _hr_; \
    } \
    CHECK_MISMATCH_RETURN


// DEPRECATED: APIs should not return incorrect HRESULTs when operating
// normally. If this is an error condition please use the tracing version
// of this macro..
#define IFCPTR_NOTRACE(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) == NULL) \
    { \
        hr = E_POINTER; \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCPTR_NOTRACE_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if ((x) == NULL) \
    { \
        return E_POINTER; \
    } \
    CHECK_MISMATCH_RETURN


// DEPRECATED: APIs should not return incorrect HRESULTs when operating
// normally. If this is an error condition please use the tracing version
// of this macro.
#define IFCCHECK_NOTRACE(x) \
    __pragma(warning(suppress:4127)) \
    if (!(x)) \
    { \
        hr = E_FAIL; \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCCHECK_NOTRACE_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if (!(x)) \
    { \
        return E_FAIL; \
    } \
    CHECK_MISMATCH_RETURN


// DEPRECATED: APIs should not return incorrect HRESULTs when operating
// normally. If this is an error condition please use the tracing version
// of this macro.
#define IFCEXPECT_NOTRACE(x) \
    __pragma(warning(suppress:4127)) \
    if (!(x)) \
    { \
        hr = E_UNEXPECTED; \
        goto Cleanup; \
    } \
    CHECK_MISMATCH_GOTO

#define IFCEXPECT_NOTRACE_RETURN(x) \
    __pragma(warning(suppress:4127)) \
    if (!(x)) \
    { \
        return E_UNEXPECTED; \
    } \
    CHECK_MISMATCH_RETURN


// DEPRECATED: APIs should not return incorrect HRESULTs when operating
// normally. If this is an error condition please use the tracing version
// of this macro.
#define TRACE_HR(x) \
    hr = (x); \
    wil::verify_hresult(hr); \
    __pragma(warning(suppress:4127)) \
    if (SUCCEEDED(hr)) \
    { \
    } \
    else \
    { \
        OnNewFailure<__COUNTER__>(hr); \
        ClearErrorContext(); \
        DisplayDebugMessage(DebugWriter::DebugWriterType::DebugOutputOnly, ErrorHandling::LoggingLevel::Info, __WFILE__, __LINE__, hr, L#x, NULL); \
    }

#define TRACE_HR_NORETURN(x) \
    { \
        const HRESULT _hr_ = (x); \
        wil::verify_hresult(_hr_); \
        __pragma(warning(suppress:4127)) \
        if (SUCCEEDED(_hr_)) \
        { \
        } \
        else \
        { \
            OnFailure<__COUNTER__>(_hr_); \
            ClearErrorContext(); \
            DisplayDebugMessage(DebugWriter::DebugWriterType::DebugOutputOnly, ErrorHandling::LoggingLevel::Info, __WFILE__, __LINE__, _hr_, L#x, NULL); \
        } \
    } \
    CHECK_MISMATCH_RETURN

// Task: https://microsoft.visualstudio.com/DefaultCollection/OS/_workitems/edit/20605780
// To remove the dependency on win32errorhelpers.h, we implement this method as an inline
// function here. We need to change its name to avoid conflict of a existing version of
// this method in \internal\mincore\priv_sdk\inc\win32errorhelpers.h
//
// If HRESULT_FROM_WIN32(GetLastError()) is not an error, make it a generic error.
// This should only be called when you already know the function call failed, because
// it will turn success into failure.
inline HRESULT HResultFromKnownLastError()
{
    const HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
    return (SUCCEEDED(hr) ? E_FAIL : hr);
}


#pragma endregion

