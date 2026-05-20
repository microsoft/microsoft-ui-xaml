// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#define WEX_STRING(fmt, ...)      WEX::Common::NoThrowString().Format(fmt, __VA_ARGS__)

#define LOG_OUTPUT(fmt, ...)      WEX::Logging::Log::Trace(WEX::Logging::CommentTrace(WEX_STRING(fmt, __VA_ARGS__)))
#define LOG_OUTPUT_SC(fmt, ...)   WEX::Logging::Log::Trace(WEX::Logging::CommentTrace(WEX_STRING(fmt, __VA_ARGS__)).WithAttachment(WEX::Logging::LogTraceAttachment::ScreenCapture))

#define LOG_WARNING(fmt, ...)     WEX::Logging::Log::Trace(WEX::Logging::WarningTrace(WEX_STRING(fmt, __VA_ARGS__)))
#define LOG_WARNING_SC(fmt, ...)  WEX::Logging::Log::Trace(WEX::Logging::WarningTrace(WEX_STRING(fmt, __VA_ARGS__)).WithAttachment(WEX::Logging::LogTraceAttachment::ScreenCapture))

#define LOG_ERROR(fmt, ...)       WEX::Logging::Log::Trace(WEX::Logging::ErrorTrace(WEX_STRING(fmt, __VA_ARGS__)))
#define LOG_ERROR_SC(fmt, ...)    WEX::Logging::Log::Trace(WEX::Logging::ErrorTrace(WEX_STRING(fmt, __VA_ARGS__)).WithAttachment(WEX::Logging::LogTraceAttachment::ScreenCapture))

#ifdef Trace
#undef Trace
#endif

#define XCPW(x) L##x

#define LogThrow_IfFailed(expr)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (FAILED(__expr__)) LOG_ERROR(L"(%s) returned failed HR(0x%x),%s(%d)", XCPW(#expr), __expr__, __WFILE__, __LINE__); \
        WEX::Common::Throw::IfFailed(__expr__);\
    }

#define LogThrow_IfFailedWithMessage(expr, msg)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (FAILED(__expr__)) LOG_ERROR(L"(%s) returned failed HR(0x%x),%s(%d)", XCPW(#expr), __expr__, __WFILE__, __LINE__); \
        WEX::Common::Throw::IfFailed(__expr__, msg);\
    }

#define LogThrow_LastErrorIf(expr)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (__expr__) LOG_ERROR(L"%s is true, %s(%d)", XCPW(#expr), __WFILE__, __LINE__); \
        WEX::Common::Throw::LastErrorIf(__expr__);\
    }

#define LogThrow_LastErrorIfFalse(expr)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (!__expr__) LOG_ERROR(L"%s is false, %s(%d)", XCPW(#expr), __WFILE__, __LINE__); \
        WEX::Common::Throw::LastErrorIfFalse(__expr__);\
    }

#define LogThrow_If(expr, errorcode, ...)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (__expr__) LOG_ERROR(L"%s is true, %s(%d)", XCPW(#expr), __WFILE__, __LINE__); \
        WEX::Common::Throw::If(__expr__, errorcode);\
    }

#define LogThrow_IfFalse(expr, errorcode, ...)\
    {\
        const auto __expr__ = expr; \
        __pragma(warning(suppress:4127)) \
        if (!__expr__) LOG_ERROR(L"%s is false, %s(%d)", XCPW(#expr), __WFILE__, __LINE__); \
        WEX::Common::Throw::IfFalse(__expr__, errorcode);\
    }

