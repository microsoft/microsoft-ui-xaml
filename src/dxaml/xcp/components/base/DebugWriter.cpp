// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"
#include <DebugWriter.h>
#include <DependencyLocator.h>
#include <ErrorHandlerSettings.h>
#include <TraceLoggingInterop.h>
#include <XamlTraceLogging.h>

const unsigned int MaxString = 200;
const unsigned int MaxFileNameString = 100;

// ".cpp\0"
const unsigned int MaxExtString = 5;

DECLARE_TRACELOGGING_CLASS(XamlTraceLogging, "Microsoft-Windows-XAML", (0x531A35AB, 0x63CE, 0x4BCF, 0xAA, 0x98, 0xF8, 0x8C, 0x7A, 0x89, 0xE4, 0x55));

class DebugOutputTraceLogging final : public TelemetryBase
{
    IMPLEMENT_TELEMETRY_CLASS(DebugOutputTraceLogging, XamlTraceLogging);

public:
    DEFINE_TRACELOGGING_EVENT_PARAM1(ErrorAdditionalInformation,
        PCWSTR, Message,
        TraceLoggingLevel(WINEVENT_LEVEL_ERROR));
};

void __stdcall WILResultMessageCallback(_Inout_ wil::FailureInfo* pFailure, _Inout_updates_opt_z_(cchDest) PWSTR pszDest, _Pre_satisfies_(cchDest > 0) size_t cchDest) noexcept
{
    // Debug:
    // %FILENAME(%LINE): %Caller_MSG [%CODE(%FUNCTION)] %HRESULT - %SystemMessage 
    //
    // Specific example:
    //   v:\T\microsoft-ui-xaml-lift\dxaml\xcp\components\dependencyObject\PropertySystem.cpp(3181): Msg:[ValidateFloatValue(pDP->GetIndex(), value.AsFloat())] [CDependencyObject::ValidateCValue] 80070057 - E_INVALIDARG
    //
    //
    // Release:
    // %MODULE(%RETURN): %HRESULT - %SystemMessage
    //
    // Specific example:
    //   Microsoft.UI.Xaml.dll!00007FFFB250A520: 80070057 - E_INVALIDARG

    PWSTR dest = pszDest;
    PCWSTR destEnd = (pszDest + cchDest);

    if (pFailure->pszFile != nullptr)
    {
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%hs(%u): ", pFailure->pszFile, pFailure->uLineNumber);
    }
    else
    {
        // Release builds only have the module name, return address, and HRESULT.
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%hs!%p: ", pFailure->pszModule, pFailure->returnAddress);
    }

    if ((pFailure->pszMessage != nullptr) || (pFailure->pszCallContext != nullptr) || (pFailure->pszFunction != nullptr))
    {
        if (pFailure->pszMessage != nullptr)
        {
            dest = wil::details::LogStringPrintf(dest, destEnd, L"Msg:[%ws] ", pFailure->pszMessage);
        }
        if (pFailure->pszCallContext != nullptr)
        {
            dest = wil::details::LogStringPrintf(dest, destEnd, L"CallContext:[%hs] ", pFailure->pszCallContext);
        }

        if (pFailure->pszCode != nullptr)
        {
            dest = wil::details::LogStringPrintf(dest, destEnd, L"[%hs(%hs)] ", pFailure->pszFunction, pFailure->pszCode);
        }
        else if (pFailure->pszFunction != nullptr)
        {
            dest = wil::details::LogStringPrintf(dest, destEnd, L"[%hs] ", pFailure->pszFunction);
        }
    }

    dest = wil::details::LogStringPrintf(dest, destEnd, L"%08X - ", pFailure->hr);

    switch (pFailure->hr)
    {
    case 0x802b000a: // E_XAMLPARSEFAILED
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_XAMLPARSEFAILED\n");
        break;
    case 0x802b0014: // E_LAYOUTCYCLE
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_LAYOUTCYCLE\n");
        break;
    case 0x88000fa8: // AG_E_LAYOUT_CYCLE
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"AG_E_LAYOUT_CYCLE\n");
        break;
    case 0x800f1000: // E_NER_INVALID_OPERATION
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_NER_INVALID_OPERATION\n");
        break;
    case 0x800f1001: // E_NER_ARGUMENT_EXCEPTION
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_NER_ARGUMENT_EXCEPTION\n");
        break;
    case 0x8001010E: // RPC_E_WRONG_THREAD
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"RPC_E_WRONG_THREAD\n");
        break;
    case 0x80070057: // E_INVALIDARG
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_INVALIDARG\n");
        break;
    case 0x8000000B: // E_BOUNDS
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_BOUNDS\n");
        break;
    case 0x80000019: // E_ASYNC_OPERATION_NOT_STARTED
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_ASYNC_OPERATION_NOT_STARTED\n");
        break;
    case 0x800710DD: // E_INVALID_OPERATION
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_INVALID_OPERATION\n");
        break;
    case 0x8007000E: // E_OUTOFMEMORY
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_OUTOFMEMORY\n");
        break;
    case 0x8000ffff: // E_UNEXPECTED
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", L"E_UNEXPECTED\n");
        break;
    default:
    {
        wchar_t szErrorText[256]{};
        szErrorText[0] = L'\0';
        FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, pFailure->hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), szErrorText, ARRAYSIZE(szErrorText), nullptr);
        dest = wil::details::LogStringPrintf(dest, destEnd, L"%ws", szErrorText);
        break;
    }
    }
}

// ExtString + FileNameString + len('[') + len(']') - 
//     An Extra Null Terminator + 4 digits for line number 
const unsigned int MaxSourcePositionString = MaxExtString + MaxFileNameString + 2 - 1 + 4;

void DisplayDebugMessage(
    DebugWriter::DebugWriterType type,
    ErrorHandling::LoggingLevel level,
    _In_opt_z_ const WCHAR* pFileName,
    INT32 iLine,
    INT32 iValue, // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR* pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR* pMessage, // Message text
    ...
)
{
    va_list(vargs);
    va_start(vargs, pMessage);
    return DisplayDebugMessageV(type, level, pFileName, iLine, iValue, pTestString, pMessage, vargs);
}

void DisplayDebugMessageV(
    DebugWriter::DebugWriterType type,
    ErrorHandling::LoggingLevel level,
    _In_opt_z_ const WCHAR* pFileName,
    INT32 iLine,
    INT32 iValue, // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR* pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR* pMessage, // Message text
    void* pVArgs // va_list of args with pMessage as first arg.
)
{
    WCHAR message[MaxString];
    WCHAR outputMessage[400];

    // If we have a message and it's not just a null terminator we will attempt
    // to format it using the passed in pointer to va_args, otherwise we just
    // use an empty message.
    if (pMessage && *pMessage)
    {
        va_list vargs = static_cast<va_list>(pVArgs);
        const WCHAR* pFormat = va_arg(vargs, WCHAR*);
        StringCchVPrintf(message, ARRAY_SIZE(message), pFormat, vargs);
        message[ARRAY_SIZE(message)-1] = 0;
    }
    else
    {
        message[0] = L'\0';
    }

    // Fixed length buffer for filename + line number
    WCHAR sourcePosition[MaxSourcePositionString]; 
    WCHAR ext[MaxExtString]; 
    WCHAR fileName[MaxFileNameString];
    _wsplitpath_s(pFileName, nullptr, 0, nullptr, 0, fileName, ARRAY_SIZE(fileName), ext, ARRAY_SIZE(ext));

    // Format the source position as the filename (without path) plus the line number
    StringCchPrintf(
        sourcePosition,
        ARRAY_SIZE(sourcePosition),
        L"%s%s[%d]",
        fileName,
        ext,
        iLine
    );
    // Guarantee termination
    sourcePosition[ARRAY_SIZE(sourcePosition)-1] = 0; 

    // Format the overall message
    StringCchPrintf(
        outputMessage, ARRAY_SIZE(outputMessage),
        L"%s: Source '%s', Value %d, Message %s\n",
        sourcePosition,
        pTestString,
        iValue,
        message 
    );

    // Guarantee termination
    outputMessage[ARRAY_SIZE(outputMessage)-1] = 0; 

    OutputDebugString(outputMessage);

    if (type == DebugWriter::DebugWriterType::UseLoggerIfPresent)
    {
        static auto errorHandlerSettings = ErrorHandling::GetErrorHandlingSettings();
        if (errorHandlerSettings)
        {
            errorHandlerSettings->LogMessage(outputMessage, level);
        }
    }
}

void DisplayReleaseMessage(_In_ const WCHAR* message)
{
    // Log to debugger output
    OutputDebugString(message);
    OutputDebugString(L"\n");

    // Log to test infrastructure error handler (if present)
    static auto errorHandlerSettings = ErrorHandling::GetErrorHandlingSettings();
    if (errorHandlerSettings)
    {
        errorHandlerSettings->LogMessage(message, ErrorHandling::LoggingLevel::Warning);
    }

    // Log to tracelogging so this appears in ETL traces
    DebugOutputTraceLogging::ErrorAdditionalInformation(message);
}