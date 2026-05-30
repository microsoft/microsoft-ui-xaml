// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests { namespace Common {

    // Basic RAII for BSTR.
    class AutoBSTR
    {
    public:
        AutoBSTR()
        {
            m_bstr = nullptr;
        }

        AutoBSTR(const wchar_t *str)
        {
            m_bstr = ::SysAllocString(str);
        }

        ~AutoBSTR()
        {
            Release();
        }

        BSTR& Get()
        {
            return m_bstr;
        }

        void Release()
        {
            if (m_bstr)
            {
                ::SysFreeString(m_bstr);
                m_bstr = nullptr;
            }
        }

        BSTR* ReleaseAndGetAddressOf()
        {
            Release();
            return &m_bstr;
        }

        operator BSTR() const
        {
            return m_bstr;
        }

        operator bool() const
        {
            return m_bstr != nullptr;
        }

        static void VerifyAreEqual(const WCHAR *expectedString, AutoBSTR &actualString)
        {
            LOG_OUTPUT(L"Expected string = \"%s\", actual string = \"%s\".", expectedString, actualString.Get());
            VERIFY_IS_TRUE(!wcscmp(expectedString, actualString));
        }

        static void VerifyAreEqual(AutoBSTR &expectedString, AutoBSTR &actualString)
        {
            LOG_OUTPUT(L"Expected string = \"%s\", actual string = \"%s\".", expectedString.Get(), actualString.Get());
            VERIFY_IS_TRUE(!wcscmp(expectedString, actualString));
        }

    private:
        BSTR m_bstr;
    };

} } } } }
