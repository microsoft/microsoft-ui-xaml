#pragma once
// Note: Intended to be included by User32Utils.h.

#include <windows.h>

#include <string>
#include <vector>

//
// Helpers for reading and writing values to app-specific registry keys,
// used to persist data like window positions.
//

namespace RegistryHelpers
{

// Reads a REG_DWORD value. Missing or malformed values return defaultValue.
inline DWORD ReadDword(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName,
    DWORD defaultValue = 0)
{
    DWORD result = defaultValue;
    DWORD cbData = sizeof(result);

    RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_DWORD,
        nullptr,
        &result,
        &cbData);

    return result;
}

// HKCU convenience overload for app settings under a per-app subkey.
inline DWORD ReadDword(
    PCWSTR subKey,
    PCWSTR valueName,
    DWORD defaultValue = 0)
{
    return ReadDword(
        HKEY_CURRENT_USER,
        subKey,
        valueName,
        defaultValue);
}

// Creates the app key if needed and writes a REG_DWORD value.
inline void WriteDword(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName,
    DWORD value)
{
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(
            rootKey,
            subKey,
            0,
            nullptr,
            0,
            KEY_SET_VALUE,
            nullptr,
            &hKey,
            nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(
            hKey,
            valueName,
            0,
            REG_DWORD,
            reinterpret_cast<const BYTE*>(&value),
            sizeof(value));

        RegCloseKey(hKey);
    }
}

// HKCU convenience overload for writing a REG_DWORD value.
inline void WriteDword(
    PCWSTR subKey,
    PCWSTR valueName,
    DWORD value)
{
    WriteDword(
        HKEY_CURRENT_USER,
        subKey,
        valueName,
        value);
}

// Reads a REG_SZ value using a size query first; missing values return empty.
inline std::wstring ReadString(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName)
{
    DWORD cb = 0;
    LSTATUS result = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_SZ,
        nullptr,
        nullptr,
        &cb);

    if ((result != ERROR_SUCCESS) ||
        (cb < sizeof(wchar_t)) ||
        ((cb % sizeof(wchar_t)) != 0))
    {
        return {};
    }

    std::wstring value(cb / sizeof(wchar_t), L'\0');
    result = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_SZ,
        nullptr,
        value.data(),
        &cb);

    if (result != ERROR_SUCCESS)
    {
        return {};
    }

    while (!value.empty() && (value.back() == L'\0'))
    {
        value.pop_back();
    }

    return value;
}

// HKCU convenience overload for reading a REG_SZ value.
inline std::wstring ReadString(
    PCWSTR subKey,
    PCWSTR valueName)
{
    return ReadString(
        HKEY_CURRENT_USER,
        subKey,
        valueName);
}

// Creates the app key if needed and writes a null-terminated REG_SZ value.
inline void WriteString(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName,
    const std::wstring& value)
{
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(
            rootKey,
            subKey,
            0,
            nullptr,
            0,
            KEY_SET_VALUE,
            nullptr,
            &hKey,
            nullptr) == ERROR_SUCCESS)
    {
        const DWORD cb =
            static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));

        RegSetValueExW(
            hKey,
            valueName,
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(value.c_str()),
            cb);

        RegCloseKey(hKey);
    }
}

// HKCU convenience overload for writing a REG_SZ value.
inline void WriteString(
    PCWSTR subKey,
    PCWSTR valueName,
    const std::wstring& value)
{
    WriteString(
        HKEY_CURRENT_USER,
        subKey,
        valueName,
        value);
}

// Reads a REG_BINARY payload containing unsigned int values.
inline std::vector<unsigned int> ReadUintArray(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName)
{
    DWORD size = 0;
    LSTATUS result = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_BINARY,
        nullptr,
        nullptr,
        &size);

    if ((result != ERROR_SUCCESS) ||
        (size == 0) ||
        ((size % sizeof(unsigned int)) != 0))
    {
        return {};
    }

    std::vector<unsigned int> values(size / sizeof(unsigned int));
    DWORD cb = size;
    result = RegGetValueW(
        rootKey,
        subKey,
        valueName,
        RRF_RT_REG_BINARY,
        nullptr,
        values.data(),
        &cb);

    if ((result == ERROR_SUCCESS) && (cb == size))
    {
        return values;
    }

    return {};
}

// HKCU convenience overload for reading a REG_BINARY uint array.
inline std::vector<unsigned int> ReadUintArray(
    PCWSTR subKey,
    PCWSTR valueName)
{
    return ReadUintArray(
        HKEY_CURRENT_USER,
        subKey,
        valueName);
}

// Creates the app key if needed and writes unsigned ints as REG_BINARY.
inline void WriteUintArray(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName,
    const std::vector<unsigned int>& values)
{
    if (values.empty())
    {
        return;
    }

    HKEY hKey = nullptr;
    if (RegCreateKeyExW(
            rootKey,
            subKey,
            0,
            nullptr,
            0,
            KEY_SET_VALUE,
            nullptr,
            &hKey,
            nullptr) == ERROR_SUCCESS)
    {
        RegSetValueExW(
            hKey,
            valueName,
            0,
            REG_BINARY,
            reinterpret_cast<const BYTE*>(values.data()),
            static_cast<DWORD>(values.size() * sizeof(unsigned int)));

        RegCloseKey(hKey);
    }
}

// HKCU convenience overload for writing a REG_BINARY uint array.
inline void WriteUintArray(
    PCWSTR subKey,
    PCWSTR valueName,
    const std::vector<unsigned int>& values)
{
    WriteUintArray(
        HKEY_CURRENT_USER,
        subKey,
        valueName,
        values);
}

// Deletes one value from an app key, creating the key if it does not exist.
inline void DeleteValue(
    HKEY rootKey,
    PCWSTR subKey,
    PCWSTR valueName)
{
    HKEY hKey = nullptr;
    if (RegCreateKeyExW(
            rootKey,
            subKey,
            0,
            nullptr,
            0,
            KEY_SET_VALUE,
            nullptr,
            &hKey,
            nullptr) == ERROR_SUCCESS)
    {
        RegDeleteValueW(
            hKey,
            valueName);

        RegCloseKey(hKey);
    }
}

// HKCU convenience overload for deleting one value.
inline void DeleteValue(
    PCWSTR subKey,
    PCWSTR valueName)
{
    DeleteValue(
        HKEY_CURRENT_USER,
        subKey,
        valueName);
}

} // namespace RegistryHelpers

// Legacy wrapper: opens or creates an HKCU app key. Caller closes the handle.
inline HKEY GetAppRegKey(PCWSTR appName)
{
    HKEY hKey = nullptr;

    LONG openRes = RegOpenKeyExW(
        HKEY_CURRENT_USER,
        appName,
        0,
        KEY_ALL_ACCESS,
        &hKey);

    if (openRes != ERROR_SUCCESS)
    {
        RegCreateKeyExW(
            HKEY_CURRENT_USER,
            appName,
            0,
            nullptr,
            0,
            KEY_WRITE,
            nullptr,
            &hKey,
            nullptr);
    }

    return hKey;
}

// Legacy wrapper for reading an HKCU REG_SZ app value.
inline std::wstring ReadStringRegKey(PCWSTR appName, PCWSTR keyName)
{
    return RegistryHelpers::ReadString(
        appName,
        keyName);
}

// Legacy wrapper for writing an HKCU REG_SZ app value.
inline void WriteStringRegKey(
    PCWSTR appName,
    PCWSTR keyName,
    std::wstring keyValue)
{
    RegistryHelpers::WriteString(
        appName,
        keyName,
        keyValue);
}

// Legacy wrapper for deleting one HKCU app value.
inline void DeleteRegValue(PCWSTR appName, PCWSTR keyName)
{
    RegistryHelpers::DeleteValue(
        appName,
        keyName);
}

// Legacy wrapper for reading an HKCU REG_DWORD app value.
inline DWORD ReadDwordRegKey(
    PCWSTR appName,
    PCWSTR keyName,
    DWORD dwDefault)
{
    return RegistryHelpers::ReadDword(
        appName,
        keyName,
        dwDefault);
}

// Legacy wrapper for writing an HKCU REG_DWORD app value.
inline void WriteDwordRegKey(
    PCWSTR appName,
    PCWSTR keyName,
    DWORD dwValue)
{
    RegistryHelpers::WriteDword(
        appName,
        keyName,
        dwValue);
}
