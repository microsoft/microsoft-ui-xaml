// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once
#include "ErrorHandlerSettings.h"

namespace DebugWriter
{
    enum class DebugWriterType
    {
        DebugOutputOnly,
        UseLoggerIfPresent
    };
}

// This method overrides the error to log output functionality in WIL with our own.
void __stdcall WILResultMessageCallback(_Inout_ wil::FailureInfo* pFailure, _Inout_updates_opt_z_(cchDest) PWSTR pszDest, _Pre_satisfies_(cchDest > 0) size_t cchDest) noexcept;

// Formats a debug message in a standard way and outputs it
// to the debug console.
void DisplayDebugMessage(
    DebugWriter::DebugWriterType type,
    ErrorHandling::LoggingLevel level,
    _In_opt_z_ const WCHAR* pFileName,
    int iLine,
    int iValue, // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR* pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR* pMessage, // Message text
    ...
);

// Formats a debug message in a standard way and outputs it
// to the debug console, accepting a va_args array built from
// a standard call with variable arguments. 
void DisplayDebugMessageV(
    DebugWriter::DebugWriterType type,
    ErrorHandling::LoggingLevel level,
    _In_opt_z_ const WCHAR* pFileName,
    int iLine,
    int iValue, // E.g. assertion value, HRESULT
    _In_opt_z_ const WCHAR* pTestString, // E.g. Assertion string
    _In_opt_z_ const WCHAR* pMessage, // Message text
    void* pVArgs // va_list of args with pMessage as first arg.
);

// Prints a string to the debugger and logging sessions in both release and debug builds.  No fancy formatting because
// file/line will be unavailable in release builds.  These messages follow error printing so the HRESULT is not needed
// either.
void DisplayReleaseMessage(_In_ const WCHAR* message);