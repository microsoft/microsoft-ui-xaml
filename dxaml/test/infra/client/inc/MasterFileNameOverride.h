// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestClassSettings.h>

namespace Private::Infrastructure::MasterFiles {

    inline constexpr const wchar_t* c_classNameEnvVar = L"XAML_TEST_MASTER_FILE_CLASS";

    inline bool IsValidClassName(const wchar_t* name)
    {
        if (!name || !name[0])
        {
            return false;
        }
        for (size_t i = 0; name[i]; ++i)
        {
            const wchar_t character = name[i];
            if (i >= 255 || !(character == L'_' || (character >= L'A' && character <= L'Z')
                || (character >= L'a' && character <= L'z') || (i > 0 && character >= L'0' && character <= L'9')))
            {
                return false;
            }
        }
        return true;
    }

    inline HRESULT SetClassName(const wchar_t* name)
    {
        if (name && !IsValidClassName(name))
        {
            return E_INVALIDARG;
        }
        return TestClassSettings::SetValue(c_classNameEnvVar, name);
    }

    inline HRESULT ApplyClassName(std::wstring& testName)
    {
        std::wstring className;
        const HRESULT result = TestClassSettings::GetValue<256>(c_classNameEnvVar, className);
        if (result != S_OK)
        {
            return result == S_FALSE ? S_OK : result;
        }
        if (!IsValidClassName(className.c_str()))
        {
            return E_INVALIDARG;
        }

        const size_t methodSeparator = testName.rfind(L"::");
        if (methodSeparator == std::wstring::npos || methodSeparator == 0)
        {
            return E_INVALIDARG;
        }
        const size_t classSeparator = testName.rfind(L"::", methodSeparator - 1);
        const size_t classStart = classSeparator == std::wstring::npos ? 0 : classSeparator + 2;
        if (classStart == methodSeparator)
        {
            return E_INVALIDARG;
        }
        testName.replace(classStart, methodSeparator - classStart, className);
        return S_OK;
    }
}
