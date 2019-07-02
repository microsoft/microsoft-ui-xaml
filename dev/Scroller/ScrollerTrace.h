// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "common.h"
#include "TraceLogging.h"
#include "Utils.h"
#include "MUXControlsTestHooks.h"

inline bool IsScrollerTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollerVerboseTracingEnabled()
{
    return g_IsLoggingProviderEnabled &&
        g_LoggingProviderLevel >= WINEVENT_LEVEL_VERBOSE &&
        (g_LoggingProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_LoggingProviderMatchAnyKeyword == 0);
}

inline bool IsScrollerPerfTracingEnabled()
{
    return g_IsPerfProviderEnabled &&
        g_PerfProviderLevel >= WINEVENT_LEVEL_INFO &&
        (g_PerfProviderMatchAnyKeyword & KEYWORD_SCROLLER || g_PerfProviderMatchAnyKeyword == 0);
}

#define SCROLLER_TRACE_INFO_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollerTrace::TraceInfo(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLER_TRACE_INFO(sender, message, ...) \
if (IsScrollerTracingEnabled()) \
{ \
    SCROLLER_TRACE_INFO_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollerTrace::s_IsDebugOutputEnabled || ScrollerTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLER_TRACE_INFO_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLER_TRACE_VERBOSE_ENABLED(includeTraceLogging, sender, message, ...) \
ScrollerTrace::TraceVerbose(includeTraceLogging, sender, message, __VA_ARGS__); \

#define SCROLLER_TRACE_VERBOSE(sender, message, ...) \
if (IsScrollerVerboseTracingEnabled()) \
{ \
    SCROLLER_TRACE_VERBOSE_ENABLED(true /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \
else if (ScrollerTrace::s_IsVerboseDebugOutputEnabled) \
{ \
    SCROLLER_TRACE_VERBOSE_ENABLED(false /*includeTraceLogging*/, sender, message, __VA_ARGS__); \
} \

#define SCROLLER_TRACE_PERF(info) \
if (IsScrollerPerfTracingEnabled()) \
{ \
    ScrollerTrace::TracePerfInfo(info); \
} \

class ScrollerTrace
{
public:
    static bool s_IsDebugOutputEnabled;
    static bool s_IsVerboseDebugOutputEnabled;

    static void TraceInfo(bool includeTraceLogging, const winrt::IInspectable& sender, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[384]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ScrollerInfo" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_INFO),
                    TraceLoggingKeyword(KEYWORD_SCROLLER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"Scroller") >= WINEVENT_LEVEL_INFO || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_INFO))
            {
                globalTestHooks->LogMessage(sender, buffer, false /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }

    static void TraceVerbose(bool includeTraceLogging, const winrt::IInspectable& sender, PCWSTR message, ...) noexcept
    {
        va_list args;
        va_start(args, message);
        WCHAR buffer[384]{};
        if (SUCCEEDED(StringCchVPrintfW(buffer, ARRAYSIZE(buffer), message, args)))
        {
            if (includeTraceLogging)
            {
                // TraceViewers
                // http://toolbox/pef 
                // http://fastetw/index.aspx
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ScrollerVerbose" /* eventName */,
                    TraceLoggingLevel(WINEVENT_LEVEL_VERBOSE),
                    TraceLoggingKeyword(KEYWORD_SCROLLER),
                    TraceLoggingWideString(buffer, "Message"));
            }

            if (s_IsDebugOutputEnabled || s_IsVerboseDebugOutputEnabled)
            {
                OutputDebugStringW(buffer);
            }

            com_ptr<MUXControlsTestHooks> globalTestHooks = MUXControlsTestHooks::GetGlobalTestHooks();

            if (globalTestHooks &&
                (globalTestHooks->GetLoggingLevelForType(L"Scroller") >= WINEVENT_LEVEL_VERBOSE || globalTestHooks->GetLoggingLevelForInstance(sender) >= WINEVENT_LEVEL_VERBOSE))
            {
                globalTestHooks->LogMessage(sender, buffer, true /*isVerboseLevel*/);
            }
        }
        va_end(args);
    }

    static void TracePerfInfo(PCWSTR info) noexcept
    {
        // TraceViewers
        // http://toolbox/pef 
        // http://fastetw/index.aspx
        TraceLoggingWrite(
            g_hPerfProvider,
            "ScrollerPerf" /* eventName */,
            TraceLoggingLevel(WINEVENT_LEVEL_INFO),
            TraceLoggingKeyword(KEYWORD_SCROLLER),
            TraceLoggingWideString(info, "Info"));
    }
};

class DebugTracingHelper
{
template <typename T>
static inline PCWSTR GetFormatCode(bool* convertToString);

template <>
static inline PCWSTR GetFormatCode<wchar_t const*>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<wchar_t const*>\n");
    return L"%s";
}

template <>
static inline PCWSTR GetFormatCode<float>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<float>\n");
    *convertToString = false;
    return L"%f";
}

template <>
static inline PCWSTR GetFormatCode<double>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<double>\n");
    *convertToString = false;
    return L"%lf";
}

template <>
static inline PCWSTR GetFormatCode<uint16_t>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<uint16_t>\n");
    *convertToString = false;
    return L"%uh";
}

template <>
static inline PCWSTR GetFormatCode<int16_t>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<int16_t>\n");
    *convertToString = false;
    return L"%hd";
}

template <>
static inline PCWSTR GetFormatCode<int32_t>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<int32_t>\n");
    *convertToString = false;
    return L"%d";
}

template <>
static inline PCWSTR GetFormatCode<struct winrt::Windows::Foundation::Size>(bool* convertToString)
{
    OutputDebugStringW(L"GetFormatCode<struct winrt::Windows::Foundation::Size>\n");
    *convertToString = true;
    return L"";
}


template <typename T>
static inline HRESULT GetStringConversion(
    PWSTR stringConversion,
    size_t cchStringConversion,
    T value)
{
    return E_NOTIMPL;
}

template <>
static inline HRESULT GetStringConversion<struct winrt::Windows::Foundation::Size>(
    PWSTR stringConversion,
    size_t cchStringConversion,
    struct winrt::Windows::Foundation::Size value)
{
    OutputDebugStringW(L"GetStringConversion<struct winrt::Windows::Foundation::Size>\n");
    return StringCchPrintfW(stringConversion, cchStringConversion, L"Size(Width: %f, Height: %f)", value.Width, value.Height);
}


public:

static inline void DebugPrintArgs(PWSTR output, size_t cchOuput)
{
    OutputDebugStringW(L"DebugPrintArgs()\n");
}

template<typename T, typename... Args>
static inline void DebugPrintArgs(PWSTR output, size_t cchOuput, T value, Args... args)
{
    bool convertToString = false;
    PCWSTR code = GetFormatCode<T>(&convertToString);
    size_t cbOuput = cchOuput * sizeof(wchar_t);

    if (cchOuput != 1024)
    {
        StringCbCatExW(
            output,
            cbOuput,
            L", ",
            &output,
            &cbOuput,
            STRSAFE_IGNORE_NULLS);
    }

    if (convertToString)
    {
        WCHAR stringConversion[384]{};
        GetStringConversion<T>(stringConversion, ARRAYSIZE(stringConversion), value);
        OutputDebugStringW(L"DebugPrintArgs<T> stringConversion=");
        OutputDebugStringW(stringConversion);

        StringCbCatExW(
            output,
            cbOuput,
            stringConversion,
            &output,
            &cbOuput,
            STRSAFE_IGNORE_NULLS);
    }
    else
    {
        OutputDebugStringW(L"DebugPrintArgs<T> code=");
        OutputDebugStringW(code);

        StringCchPrintfExW(
            output,
            cbOuput,
            &output,
            &cbOuput,
            STRSAFE_IGNORE_NULLS,
            code,
            value);
    }
    OutputDebugStringW(L"\n");

    DebugPrintArgs(output, cbOuput / sizeof(wchar_t) /*cchOuput*/, args...);
}


template <typename T>
static inline void DebugPrintArg(PWSTR output, size_t cchOuput, char const* name, T value)
{
}

template <>
static inline void DebugPrintArg<wchar_t const*>(PWSTR output, size_t cchOuput, char const* name, wchar_t const* value)
{
    OutputDebugStringW(L"DebugPrintArg<wchar_t const*>\n");

    size_t cbOuput = cchOuput * sizeof(wchar_t);

    StringCbCatExW(
        output,
        cbOuput,
        StringUtil::Utf8ToUtf16(name).c_str(),
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCbCatExW(
        output,
        cbOuput,
        L": ",
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCbCatW(
        output,
        cbOuput,
        value);
}

template <>
static inline void DebugPrintArg<float>(PWSTR output, size_t cchOuput, char const* name, float value)
{
    OutputDebugStringW(L"DebugPrintArg<float>\n");

    size_t cbOuput = cchOuput * sizeof(wchar_t);

    StringCbCatExW(
        output,
        cbOuput,
        StringUtil::Utf8ToUtf16(name).c_str(),
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCbCatExW(
        output,
        cbOuput,
        L": ",
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCchPrintfW(
        output,
        cbOuput,
        L"%f",
        value);
}


template <>
static inline void DebugPrintArg<int32_t>(PWSTR output, size_t cchOuput, char const* name, int32_t value)
{
    OutputDebugStringW(L"DebugPrintArg<int32_t>\n");

    size_t cbOuput = cchOuput * sizeof(wchar_t);

    StringCbCatExW(
        output,
        cbOuput,
        StringUtil::Utf8ToUtf16(name).c_str(),
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCbCatExW(
        output,
        cbOuput,
        L": ",
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCchPrintfW(
        output,
        cbOuput,
        L"%d",
        value);
}

template <>
static inline void DebugPrintArg<struct winrt::Windows::Foundation::Size>(PWSTR output, size_t cchOuput, char const* name, struct winrt::Windows::Foundation::Size value)
{
    OutputDebugStringW(L"DebugPrintArg<struct winrt::Windows::Foundation::Size>\n");

    size_t cbOuput = cchOuput * sizeof(wchar_t);

    StringCbCatExW(
        output,
        cbOuput,
        StringUtil::Utf8ToUtf16(name).c_str(),
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    StringCbCatExW(
        output,
        cbOuput,
        L": ",
        &output,
        &cbOuput,
        STRSAFE_IGNORE_NULLS);

    WCHAR stringConversion[384]{};

    GetStringConversion<struct winrt::Windows::Foundation::Size>(
        stringConversion,
        ARRAYSIZE(stringConversion),
        value);

    StringCbCatW(
        output,
        cbOuput,
        stringConversion);
}

static inline void DebugPrint(PCWSTR methodName, PWSTR message)
{
    OutputDebugStringW(L"methodName=");
    OutputDebugStringW(methodName);
    OutputDebugStringW(L"\n");
    OutputDebugStringW(L"message=");
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
}

static inline void DebugPrintArguments(PCWSTR methodName, uint8_t argumentsCount, ...)
{
    MUX_ASSERT(argumentsCount >= 1);
    MUX_ASSERT(argumentsCount <= 5);

    OutputDebugStringW(L"methodName=");
    OutputDebugStringW(methodName);
    OutputDebugStringW(L"\n");

    va_list args;
    va_start(args, argumentsCount);

    size_t cchMessage = argumentsCount * 1024;
    PWSTR message = new WCHAR[argumentsCount * 1024];
    message[0] = '\0';

    WCHAR formatBuffer[19]{};
    PWSTR format = formatBuffer;
    size_t cbFormat = ARRAYSIZE(formatBuffer) * sizeof(wchar_t);

    StringCbCatExW(
        format,
        cbFormat,
        L"%s",
        &format,
        &cbFormat,
        STRSAFE_IGNORE_NULLS);

    for (uint8_t argument = 1; argument < argumentsCount; argument++)
    {
        StringCbCatExW(
            format,
            cbFormat,
            L", %s",
            &format,
            &cbFormat,
            STRSAFE_IGNORE_NULLS);
    }

    StringCchVPrintfW(message, cchMessage, formatBuffer, args);

    OutputDebugStringW(L"message=");
    OutputDebugStringW(message);
    OutputDebugStringW(L"\n");
    delete[] message;
    va_end(args);
}

//inline void DebugPrint3Args(PCWSTR methodName, PWSTR mgsArg1, PWSTR mgsArg2, PWSTR mgsArg3)
//{
//    OutputDebugStringW(L"methodName=");
//    OutputDebugStringW(methodName);
//    OutputDebugStringW(L"\n");
//
//    WCHAR messageBuffer[3076]{};
//    PWSTR message = messageBuffer;
//    size_t cchMessage = ARRAYSIZE(messageBuffer);
//
//    //PWSTR newMessage = nullptr;
//    //size_t cchNewMessage = 0;
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg1,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//    //message = newMessage;
//    //cchMessage = cchNewMessage;
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        L", ",
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg2,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        L", ",
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg3,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    OutputDebugStringW(L"message=");
//    OutputDebugStringW(messageBuffer);
//    OutputDebugStringW(L"\n");
//}

//inline void DebugPrint4Args(PCWSTR methodName, PWSTR mgsArg1, PWSTR mgsArg2, PWSTR mgsArg3, PWSTR mgsArg4)
//{
//    OutputDebugStringW(L"methodName=");
//    OutputDebugStringW(methodName);
//    OutputDebugStringW(L"\n");
//
//    WCHAR messageBuffer[4102]{};
//    PWSTR message = messageBuffer;
//    size_t cchMessage = ARRAYSIZE(messageBuffer);
//
//    //PWSTR newMessage = nullptr;
//    //size_t cchNewMessage = 0;
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg1,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//    //message = newMessage;
//    //cchMessage = cchNewMessage;
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        L", ",
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg2,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        L", ",
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg3,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        L", ",
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    StringCbCatExW(
//        message,
//        cchMessage,
//        mgsArg4,
//        &message,
//        &cchMessage,
//        STRSAFE_IGNORE_NULLS);
//
//    OutputDebugStringW(L"message=");
//    OutputDebugStringW(messageBuffer);
//    OutputDebugStringW(L"\n");
//}
};

#define DEBUG_PRINT(methodName, ...) \
WCHAR message[1024]{}; \
DebugTracingHelper::DebugPrintArgs(message, ARRAYSIZE(message), __VA_ARGS__); \
DebugTracingHelper::DebugPrint(methodName, message); \

#define DEBUG_PRINT3ARGS(methodName, arg1, arg2, arg3) \
{ \
WCHAR mgsArg1[1022]{}; \
WCHAR mgsArg2[1022]{}; \
WCHAR mgsArg3[1022]{}; \
DebugTracingHelper::DebugPrintArg(mgsArg1, ARRAYSIZE(mgsArg1), #arg1, arg1); \
DebugTracingHelper::DebugPrintArg(mgsArg2, ARRAYSIZE(mgsArg2), #arg2, arg2); \
DebugTracingHelper::DebugPrintArg(mgsArg3, ARRAYSIZE(mgsArg3), #arg3, arg3); \
DebugTracingHelper::DebugPrintArguments(methodName, 3, mgsArg1, mgsArg2, mgsArg3); \
} \

//DebugPrint3Args(methodName, mgsArg1, mgsArg2, mgsArg3); \

#define DEBUG_PRINT4ARGS(methodName, arg1, arg2, arg3, arg4) \
{ \
WCHAR mgsArg1[1022]{}; \
WCHAR mgsArg2[1022]{}; \
WCHAR mgsArg3[1022]{}; \
WCHAR mgsArg4[1022]{}; \
DebugTracingHelper::DebugPrintArg(mgsArg1, ARRAYSIZE(mgsArg1), #arg1, arg1); \
DebugTracingHelper::DebugPrintArg(mgsArg2, ARRAYSIZE(mgsArg2), #arg2, arg2); \
DebugTracingHelper::DebugPrintArg(mgsArg3, ARRAYSIZE(mgsArg3), #arg3, arg3); \
DebugTracingHelper::DebugPrintArg(mgsArg4, ARRAYSIZE(mgsArg4), #arg4, arg4); \
DebugTracingHelper::DebugPrintArguments(methodName, 4, mgsArg1, mgsArg2, mgsArg3, mgsArg4); \
} \

//DebugPrint4Args(methodName, mgsArg1, mgsArg2, mgsArg3, mgsArg4); \

#define DEBUG_PRINT5ARGS(methodName, arg1, arg2, arg3, arg4, arg5) \
{ \
WCHAR mgsArg[5][1022]{}; \
DebugTracingHelper::DebugPrintArg(mgsArg[0], ARRAYSIZE(mgsArg[0]), #arg1, arg1); \
DebugTracingHelper::DebugPrintArg(mgsArg[1], ARRAYSIZE(mgsArg[1]), #arg2, arg2); \
DebugTracingHelper::DebugPrintArg(mgsArg[2], ARRAYSIZE(mgsArg[2]), #arg3, arg3); \
DebugTracingHelper::DebugPrintArg(mgsArg[3], ARRAYSIZE(mgsArg[3]), #arg4, arg4); \
DebugTracingHelper::DebugPrintArg(mgsArg[4], ARRAYSIZE(mgsArg[4]), #arg5, arg5); \
DebugTracingHelper::DebugPrintArguments(methodName, 5, mgsArg[0], mgsArg[1], mgsArg[2], mgsArg[3], mgsArg[4]); \
} \
