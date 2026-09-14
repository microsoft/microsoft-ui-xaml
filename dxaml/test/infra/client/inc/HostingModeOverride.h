// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <TestClassSettings.h>

namespace Private { namespace Infrastructure { namespace Hosting {

    // The test DLL and infrastructure DLL must see the same class-declared mode.
    // It remains set through host teardown, including infrastructure destructors.
    inline constexpr const wchar_t* c_declaredHostingModeEnvVar = L"XAML_TEST_HOSTING_MODE";

    inline bool IsValidHostingMode(const wchar_t* mode)
    {
        return mode && (_wcsicmp(mode, L"WPF") == 0
            || _wcsicmp(mode, L"UAP") == 0
            || _wcsicmp(mode, L"WinForms") == 0
            || _wcsicmp(mode, L"Win32Explicit") == 0);
    }

    inline HRESULT SetDeclaredHostingMode(const wchar_t* mode)
    {
        if (mode && !IsValidHostingMode(mode))
        {
            return E_INVALIDARG;
        }
        return TestClassSettings::SetValue(c_declaredHostingModeEnvVar, mode);
    }

    inline HRESULT GetDeclaredHostingMode(std::wstring& mode)
    {
        const HRESULT result = TestClassSettings::GetValue<64>(c_declaredHostingModeEnvVar, mode);
        if (result != S_OK)
        {
            return result;
        }
        if (!IsValidHostingMode(mode.c_str()))
        {
            mode.clear();
            return E_INVALIDARG;
        }
        return S_OK;
    }

} } }
