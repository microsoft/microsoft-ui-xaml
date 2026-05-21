// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

//  Utility class that allows to set a reg key during a test and
//  restore or delete it when the class instance is destroyed.

#pragma once

namespace Microsoft { namespace UI { namespace Xaml { namespace Tests {
    namespace Common {

        class RegKeyHelper
        {
        public:
            RegKeyHelper()
            {
                m_canInitialize = true;
                Initialize(nullptr, nullptr, static_cast<DWORD>(0));
            }

            RegKeyHelper(Platform::String^ keyPath, Platform::String^ keyName, bool defaultValue, bool newValue)
            {
                m_canInitialize = true;
                Initialize(keyPath, keyName, defaultValue, newValue);
            }

            ~RegKeyHelper()
            {
                RestoreValue();
            }

            void Initialize(Platform::String^ keyPath, Platform::String^ keyName, bool defaultValue, bool newValue)
            {
                Initialize(keyPath, keyName, defaultValue);
                SetBoolValue(newValue, true /*useCurrentValueAsRestoreValue*/);
            }

            void Initialize(Platform::String^ keyPath, Platform::String^ keyName, bool defaultValue)
            {
                Initialize(keyPath, keyName, static_cast<DWORD>(defaultValue ? 1 : 0));
            }

            void Initialize(Platform::String^ keyPath, Platform::String^ keyName, DWORD defaultValue)
            {
                if (!m_canInitialize)
                {
                    throw ref new Platform::FailureException;
                }
                m_keyPath = keyPath;
                m_keyName = keyName;
                m_defaultValue = defaultValue;
                m_restoreValue = static_cast<DWORD>(-1);
                m_needsRestore = false;
                m_needsDeletion = false;
                m_wasWritten = false;
            }

            // Returns True if the reg key defined by m_keyPath & m_keyName exists.
            bool Exists()
            {
                if (m_keyPath == nullptr || m_keyName == nullptr)
                {
                    throw ref new Platform::FailureException;
                }

                HKEY hkey = nullptr;
                HRESULT hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_keyPath->Data(), 0, KEY_READ, &hkey);
                if (hr == ERROR_SUCCESS)
                {
                    DWORD data = 0;
                    DWORD dwSize = sizeof(DWORD);
                    hr = RegQueryValueEx(hkey, m_keyName->Data(), 0, nullptr, reinterpret_cast<LPBYTE>(&data), &dwSize);
                    RegCloseKey(hkey);
                    return hr == ERROR_SUCCESS;
                }
                return false;
            }

            bool GetBoolValue()
            {
                return ReadValue() != 0;
            }

            DWORD GetDWORDValue()
            {
                return ReadValue();
            }

            // Pass in useCurrentValueAsRestoreValue==True if you want the current
            // regkey value to become the restore value used by RestoreValue().
            void SetBoolValue(bool value, bool useCurrentValueAsRestoreValue)
            {
                if (useCurrentValueAsRestoreValue)
                {
                    m_restoreValue = ReadValue();
                }

                WriteValue(value ? 1 : 0);

                if (useCurrentValueAsRestoreValue)
                {
                    m_needsRestore = true;
                    m_canInitialize = false;
                }
            }

            // Pass in useCurrentValueAsRestoreValue==True if you want the current
            // regkey value to become the restore value used by RestoreValue().
            void SetDWORDValue(DWORD value, bool useCurrentValueAsRestoreValue)
            {
                if (useCurrentValueAsRestoreValue)
                {
                    m_restoreValue = ReadValue();
                }

                WriteValue(value);

                if (useCurrentValueAsRestoreValue)
                {
                    m_needsRestore = true;
                    m_canInitialize = false;
                }
            }

            // Restores the regkey value that was set when SetBoolValue or
            // SetDWORDValue was called with useCurrentValueAsRestoreValue==True.
            void RestoreValue()
            {
                if (m_needsRestore)
                {
                    if (m_needsDeletion)
                    {
                        DeleteValue();
                    }
                    else
                    {
                        WriteValue(m_restoreValue);
                    }
                    m_restoreValue = static_cast<DWORD>(-1);
                    m_needsRestore = false;
                    m_canInitialize = true;
                }
            }

            void DeleteValue()
            {
                if (m_keyPath == nullptr || m_keyName == nullptr)
                {
                    throw ref new Platform::FailureException;
                }

                test_infra::TestServices::Utilities->DeleteRegKey(m_keyPath, m_keyName, false /*current user*/);
            }

        private:
            // Returns the current regkey value if it exists, or m_defaultValue otherwise.
            DWORD ReadValue()
            {
                if (m_keyPath == nullptr || m_keyName == nullptr)
                {
                    throw ref new Platform::FailureException;
                }

                DWORD data = 0;
                HKEY hkey = nullptr;
                HRESULT hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, m_keyPath->Data(), 0, KEY_READ, &hkey);
                if (hr == ERROR_SUCCESS)
                {
                    DWORD dwSize = sizeof(DWORD);
                    hr = RegQueryValueEx(hkey, m_keyName->Data(), 0, nullptr, reinterpret_cast<LPBYTE>(&data), &dwSize);
                    RegCloseKey(hkey);
                }
                return (hr == ERROR_SUCCESS) ? data : m_defaultValue;
            }

            void WriteValue(DWORD value)
            {
                if (m_keyPath == nullptr || m_keyName == nullptr)
                {
                    throw ref new Platform::FailureException;
                }

                bool nameExisted = false;
                test_infra::TestServices::Utilities->SetRegKey(m_keyPath, m_keyName, value, &nameExisted, false/*current user*/);

                if (!m_wasWritten)
                {
                    // This is the first time the regkey was written. If it did not exist, the RestoreValue() method
                    // will have to delete it.
                    m_needsDeletion = !nameExisted;
                    m_wasWritten = true;
                }
            }

        private:
            Platform::String^ m_keyPath;
            Platform::String^ m_keyName;
            bool m_canInitialize;
            bool m_needsRestore;
            bool m_needsDeletion;
            bool m_wasWritten;
            DWORD m_defaultValue;
            DWORD m_restoreValue;
        };
    }
} } } }
