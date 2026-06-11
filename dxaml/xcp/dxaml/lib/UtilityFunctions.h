// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace DirectUI
{
    // Positional arguments (%1, %2 vs %s) for strings are very important for localization.

    template<int len> inline
    DWORD FormatMsg(wchar_t const (&buffer)[len], wchar_t const * pszFormat, ...)
    {
        va_list args = nullptr;
        va_start(args, pszFormat);

        DWORD result = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, pszFormat, 0, 0 ,
            const_cast<wchar_t*>(&buffer[0]), len, &args);

        va_end(args);
        return result;
    }

    // Allows for better inlining decisions of TraceApiFunctionCallStart/TraceApiFunctionCallStop to reduce DLL size
    // when adding ETW events to code gen.
    void ApiEtwStart(void* id, KnownMethodIndex knownMethodIndex);
    void ApiEtwStop(void* id, HRESULT hr);

    _Check_return_ HRESULT DefaultStrictApiCheck(_In_ DependencyObject* object);
    _Check_return_ HRESULT StrictOnlyApiCheck(_In_ DependencyObject* object, const WCHAR* apiName);
    _Check_return_ HRESULT NonStrictOnlyApiCheck(_In_ DependencyObject* object, const WCHAR* apiName);
}
