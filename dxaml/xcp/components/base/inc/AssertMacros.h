// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

// Common ASSERT macro definitions

#pragma once

// We are deprecating the use ntassert.h, however, we still use other external dependency headers that
// include this.  Which means it gets included for all our stuff too.  Undef anything that might have been
// included from ntassert.h so that we can catch (at least most) uses.
#undef _DBGRAISEASSERTIONFAILURE_
#undef BREAK_DEBUG_BASE
#undef ASSERT_BREAKPOINT
#undef NT_ANALYSIS_ASSUME
#undef NT_ASSERT_ACTION
#undef NT_ASSERTMSG_ACTION
#undef NT_ASSERTMSGW_ACTION
#undef NT_ASSERT_ASSUME
#undef NT_ASSERTMSG_ASSUME
#undef NT_ASSERTMSGW_ASSUME
#undef NT_ASSERT_NOASSUME
#undef NT_ASSERTMSG_NOASSUME
#undef NT_ASSERTMSGW_NOASSUME
#undef NT_VERIFY
#undef NT_VERIFYMSG
#undef NT_VERIFYMSGW
#undef NT_FRE_ASSERT
#undef NT_FRE_ASSERTMSG
#undef NT_FRE_ASSERTMSGW
#undef NT_ASSERT
#undef NT_ASSERTMSG
#undef NT_ASSERTMSGW
#undef WIN_ASSERT
#undef WIN_ASSERTMSG
#undef WIN_ASSERTMSGW
#undef WIN_VERIFY
#undef WIN_VERIFYMSG
#undef WIN_VERIFYMSGW

//  Unfortunately, we also include several external dependency includes that reference NT_ASSERTxxx macros
//  but don't include ntassert.h, just assumming that it is being included elsewhere.  Known violators of this
//  are:
//     dxaml\external\private\inc\internal\onecoreuapshell\inc\PropVariant.h
//     dxaml\external\private\inc\internal\minwin\priv_sdk\inc\telemetry\MicrosoftTelemetryAssert.h
//
//  Another issue is that NT_ASSERTxxx macros are written in a manner that they can be used in expressions (hence
//  included in other assert macros like the telemetry asserts).  Our Asserts are not written that way, so we can't
//  just delegate to them.
#if DBG
#if defined(_PREFAST_)
#define NT_ASSERT(cond) (!(cond) ? (_Analysis_assume_(!!(cond)), __annotation(L"Debug", L"AssertFail", (cond)), DbgRaiseAssertionFailure() : (void) 0))
#else   // defined(_PREFAST_)
#define NT_ASSERT(cond) (!(cond) ? DbgRaiseAssertionFailure() : (void) 0)
#endif //  defined(_PREFAST_)
#define NT_ASSERTMSG(msg,cond) NT_ASSERT(cond)
#define NT_VERIFY(cond) NT_ASSERT(cond)
#else // if DBG
#define NT_ASSERT(cond) ((void) 0)
#define NT_ASSERTMSG(msg,cond) ((void) 0)
#define NT_VERIFY(cond) ((!!cond) ? TRUE : FALSE)
#endif // if DBG


#if DBG

#define XCPW(x) L##x

// Following the condition, these accept a format string and susbtitutions in
// the same way as wprintf. It doesn't look like the format string is currently
// functional though, so avoid using it.

// The __annotation and DbgRaiseAssertionFailure() (in FAIL_ASSERT_HERE())
// must be inline to the function "calling" ASSERT(), to let WER/Watson know what function is failing an
// assert.
// These macros incorrectly apply _Analysis_assume_ to their conditions. Since in FRE builds
// they are not evaluated this is actually disabling some static analysis.

#define VERIFY(cond, ...) ASSERT(cond, _VA_ARGS_)
#define VERIFYHR(hr, ...) ASSERTSUCCEEDED(hr, _VA_ARGS_)

#if !defined(EXP_CLANG)
#define VERIFY_COND(expr, cond, ...) ASSERT(expr ## cond, _VA_ARGS_)
#else
// Debug version of this macro is not portable as it relies on MSVC, non-conformant way of macro expansion.
#define VERIFY_COND(expr, cond, ...) (expr)
#endif // EXP_CLANG

// We need to disable warning 4127 (Constant expression in controlling statement)
// as sometimes the expression in the IF or the WHILE is actually a constant

#define ASSERT(cond, ...) \
    do { \
        _Analysis_assume_(!!(cond)); \
        __pragma(warning(suppress:4127)) \
        if(!(cond)) \
        { \
            FAIL_ASSERT_HERE(XCPW(#cond)); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

#define ASSERTFN(fn, ...) \
    do { \
        _Analysis_assume_(!(fn)); \
        __pragma(warning(suppress:4127)) \
        if((fn)) \
        { \
            FAIL_ASSERT_HERE((XCPW(#fn) L" == 0")); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)


#define ASSERTSUCCEEDED(hr, ...) \
    do { \
        _Analysis_assume_(SUCCEEDED(hr)); \
        __pragma(warning(suppress:4127)) \
        if(!SUCCEEDED(hr)) \
        { \
            FAIL_ASSERT_HERE((L"SUCCEEDED(" XCPW(#hr) L")")); \
        } \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)

#else // #if DBG

// _Analysis_assume_ resolves to nothing if we are not running under prefast, so
// so the comma operator is not valid.
#if defined(_PREFAST_)
#define VERIFY(cond, ...) (_Analysis_assume_(!!cond), ((cond) ? TRUE : FALSE))
#define VERIFYHR(hr, ...) (_Analysis_assume_(SUCCEEDED(hr)), ((hr) ? TRUE : FALSE))
#define ASSERT(cond, ...) _Analysis_assume_(!!(cond))
#else
#define VERIFY(cond, ...) (!!(cond) ? TRUE : FALSE)
#define VERIFYHR(hr, ...) (SUCCEEDED(hr) ? TRUE : FALSE)
#define ASSERT(cond, ...) ((void) 0)
#endif // end #if _PREFAST

#define VERIFY_COND(expr, cond, ...) (expr)

#define ASSERTFN(fn, ...)
#define ASSERTSUCCEEDED(hr, ...)

#endif // end #if DBG

// FAIL_ASSERT_HERE must be defined after the assert macros, so that it can be
// overridden in some compile units.  For example, core XAML overrides this to
// FAILFAST unless under a debugger (see minerror.h).
#define FAIL_ASSERT_HERE(x) \
    do { \
        __annotation(L"Debug", L"AssertFail", (x)); \
        DbgRaiseAssertionFailure(); \
    } \
    __pragma(warning(suppress:4127)) \
    while(0)


