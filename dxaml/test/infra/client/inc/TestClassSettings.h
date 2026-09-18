// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <windows.h>
#include <string>

namespace Private::Infrastructure::TestClassSettings {

    inline HRESULT SetValue(const wchar_t* name, const wchar_t* value)
    {
        return ::SetEnvironmentVariableW(name, value) ? S_OK : HRESULT_FROM_WIN32(::GetLastError());
    }

    template<size_t BufferSize>
    HRESULT GetValue(const wchar_t* name, std::wstring& value)
    {
        value.clear();
        wchar_t buffer[BufferSize] = {};
        ::SetLastError(ERROR_SUCCESS);
        const DWORD written = ::GetEnvironmentVariableW(name, buffer, ARRAYSIZE(buffer));
        if (written == 0)
        {
            const DWORD error = ::GetLastError();
            return error == ERROR_ENVVAR_NOT_FOUND ? S_FALSE
                : (error == ERROR_SUCCESS ? E_INVALIDARG : HRESULT_FROM_WIN32(error));
        }
        if (written >= ARRAYSIZE(buffer))
        {
            return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        }
        value = buffer;
        return S_OK;
    }
}
