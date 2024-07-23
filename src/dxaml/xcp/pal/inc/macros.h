// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define DEBUG_BREAK

#if DBG
    #undef XCP_FORCEINLINE
    #define XCP_FORCEINLINE
#else
    #define XCP_FORCEINLINE
#endif

#define RRETURN(hr) return (hr)

template <typename T>
__forceinline void ReleaseInterface(T*& p) // defined as a function to avoid misuse on a smart pointer
{
    if (p)
    {
        (void)(p->Release());
        p = nullptr;
    }
}

template <typename T>
__forceinline void ReleaseInterfaceNoNULL(T* const p) // defined as a function to avoid misuse on a smart pointer
{
    if (p)
    {
        (void)(p->Release());
    }
}

template <typename T>
__forceinline void AddRefInterface(T* const p) // defined as a function to avoid misuse on a smart pointer
{
    if (p)
    {
        p->AddRef();
    }
}

template <typename T, typename S>
__forceinline void SetInterface(T*& dst, S* const src) // defined as a function to avoid misuse on a smart pointer
{
    AddRefInterface(src);
    dst = src;
}

template <typename T, typename S>
__forceinline void ReplaceInterface(T*& dst, S* const src) // defined as a function to avoid misuse on a smart pointer
{
    AddRefInterface(src);
    ReleaseInterfaceNoNULL(dst);
    dst = src;
}

// We need to disable warning 4127 (Constant expression in controlling statement)
// as sometimes the expression in the IF or the WHILE is actually a constant


#define SAFE_DELETE(p) \
    do \
    { \
        delete (p); \
        (p) = NULL; \
    } \
    __pragma(warning(suppress:4127)) \
    while (0);

#define SAFE_DELETE_ARRAY(p) \
    do \
    { \
        delete [] (p); \
        (p) = NULL; \
    } \
    __pragma(warning(suppress:4127)) \
    while (0);

// Warning: returns the element COUNT, not the SIZE in bytes, as the name might imply.
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// Return the number of elements in an array
// Leaving commented out because if there is an ARRAY_SIZE and an ARRAY_COUNT that implies
// that they are different.
// #define ARRAY_COUNT(A) (sizeof(A)/sizeof(A[0]))

// Return the number of characters in a null-terminated string (not including the null)
#define SZ_COUNT(S) (ARRAY_SIZE(S) - 1)

// An initializer helper where we have to put the length of the string followed by the string in
// an array, ex: instead of {AG_FOO, 3, L"Foo"} we can do {AG_FOO, STR(L"FOO")}
#define STR(S) SZ_COUNT(S), const_cast<WCHAR *>(S)
#define STR_LEN_PAIR(S) const_cast<WCHAR *>(S), SZ_COUNT(S)
#ifndef LEN_STR_PAIR
#define LEN_STR_PAIR(S) SZ_COUNT(S), const_cast<WCHAR *>(S)
#endif //LEN_STR_PAIR

#ifndef CHAR_LEN_PAIR
#define CHAR_LEN_PAIR(S) const_cast<char*>(S), SZ_COUNT(S)
#endif

#ifndef FASTCALL
#if DBG
    #define FASTCALL
#else
    #if defined(_X86_)
        #define FASTCALL _fastcall
    #else
        #define FASTCALL
    #endif
#endif
#endif

#define DYNCAST(targettype, pointer) static_cast<targettype>(pointer)

// Common macros

#ifndef FAILED
#define FAILED(hr)      (((HRESULT)(hr)) < 0)
#endif

#ifndef SUCCEEDED
#define SUCCEEDED(hr)   (((HRESULT)(hr)) >= 0)
#endif

// Success code

#ifndef S_OK
#define S_OK                        HRESULT(0x00000000L)
#endif

#ifndef S_FALSE
#define S_FALSE                     HRESULT(0x00000001L)
#endif

// HR and return value handling

#define IGNORERESULT(result) (void)result

#define IGNOREHR_UNTRIAGED(hr) (void) (hr)

// A macro used to record function failures without jumping to a
// Cleanup label on failure.
//
// Example:
// void SomeFunc() {
//     HRESULT hr = S_OK;
//     HRESULT recordHr = S_OK;
//     ...
//     RECORDFAILURE(OtherFunc1());
//     ...
//     IFC(OtherFunc2());
//     ...
//     IFC(recordHr);
// Cleanup:
//     ...
// }
//
// For an explanation of the unusual if / else at the beginning and end, see
// the C++ FAQ Lite "Miscellaneous technical issues" section.
#define RECORDFAILURE(x) \
    __pragma(warning(suppress:4127)) \
    if (1) { \
        HRESULT localHr = (x); \
        if (FAILED(localHr)) { \
            recordHr = localHr; \
        } \
    } \
    else (void)0
#define VERIFYRECORDFAILURE(x) \
    __pragma(warning(suppress:4127)) \
    if (1) { \
        HRESULT localHr = (x); \
        VERIFYHR(localHr); \
        if (FAILED(localHr)) { \
            recordHr = localHr; \
        } \
    } \
    else (void)0

#define CATCH_AND_IGNORE(exp) \
    do { \
        try \
        { \
            exp; \
        } \
        catch (...) \
        { } \
    } while (UNCONDITIONAL_EXPR(0))

// We define DUAL_NAMESPACE to be >>Microsoft<<::UI::Composition, and this must be done before any
// other header can define it to be >>Windows<<::UI::Composition.
#define DUAL_NAMESPACE Microsoft
