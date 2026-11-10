#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace profiler
{
    constexpr DWORD ProtocolVersion = 1;
    constexpr DWORD StartupTimeoutMs = 30000;
    constexpr size_t MaxPathCharacters = 32768;

    inline std::string Utf8(std::wstring_view value)
    {
        if (value.empty())
        {
            return {};
        }
        const int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
        if (!length)
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "UTF-8 conversion");
        }
        std::string result(static_cast<size_t>(length), '\0');
        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), length, nullptr, nullptr))
        {
            throw std::system_error(static_cast<int>(GetLastError()), std::system_category(), "UTF-8 conversion");
        }
        return result;
    }

    // A value as a JSON string for the result files and the redirect log, which are read as UTF-8. std::format
    // inserts values unescaped, and the log records paths, which have backslashes and can be non-ASCII.
    inline std::string Json(std::wstring_view value)
    {
        const auto text = Utf8(value);
        std::string result = "\"";
        constexpr char hex[] = "0123456789abcdef";
        for (unsigned char ch : text)
        {
            if (ch == '"' || ch == '\\')
            {
                result += '\\';
                result += static_cast<char>(ch);
            }
            else if (ch < 0x20)
            {
                result += "\\u00";
                result += hex[ch >> 4];
                result += hex[ch & 15];
            }
            else
            {
                result += static_cast<char>(ch);
            }
        }
        result += '"';
        return result;
    }

    inline bool WriteAll(HANDLE file, const std::string& text)
    {
        DWORD written = 0;
        if (!WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr))
        {
            return false;
        }
        if (written == text.size())
        {
            return true;
        }
        SetLastError(ERROR_WRITE_FAULT);
        return false;
    }

    inline std::wstring ModulePath(HMODULE module)
    {
        std::wstring result(MaxPathCharacters, L'\0');
        const DWORD length = GetModuleFileNameW(module, result.data(), static_cast<DWORD>(result.size()));
        if (!length || length >= result.size())
        {
            return {};
        }
        result.resize(length);
        return result;
    }

    inline std::wstring ParentPath(const std::wstring& path)
    {
        const auto index = path.find_last_of(L"\\/");
        return index == std::wstring::npos ? std::wstring{} : path.substr(0, index);
    }

    // A folder's full path without trailing separators, so that appending L"\\" and a file name gives the loader's form.
    inline std::wstring DirectoryPath(const std::wstring& path)
    {
        std::wstring result = path;
        const DWORD length = path.empty() ? 0 : GetFullPathNameW(path.c_str(), 0, nullptr, nullptr);
        if (length)
        {
            std::wstring full(length, L'\0');
            const DWORD copied = GetFullPathNameW(path.c_str(), length, full.data(), nullptr);
            if (copied && copied < length)
            {
                result.assign(full.data(), copied);
            }
        }
        while (!result.empty() && (result.back() == L'\\' || result.back() == L'/'))
        {
            result.pop_back();
        }
        return result;
    }

    inline std::wstring ShortPath(const std::wstring& path)
    {
        const DWORD length = GetShortPathNameW(path.c_str(), nullptr, 0);
        if (!length)
        {
            return path;
        }
        std::wstring result(length, L'\0');
        const DWORD copied = GetShortPathNameW(path.c_str(), result.data(), length);
        if (!copied || copied >= length)
        {
            return path;
        }
        result.resize(copied);
        return result;
    }

    // Detours names the helper in the app's import table, which the loader decodes with the system's ANSI code page
    // unless the app's manifest sets another. The 8.3 short path is ASCII where one exists; otherwise the path must
    // survive a round trip through the code page, so that a best-fit character cannot name a different file.
    inline std::string ImportPath(const std::wstring& path)
    {
        const auto shortPath = ShortPath(path);
        const int size = static_cast<int>(shortPath.size());
        const int length = WideCharToMultiByte(CP_ACP, 0, shortPath.data(), size, nullptr, 0, nullptr, nullptr);
        std::string result(static_cast<size_t>(length), '\0');
        std::wstring decoded(shortPath.size(), L'\0');
        if (length &&
            WideCharToMultiByte(CP_ACP, 0, shortPath.data(), size, result.data(), length, nullptr, nullptr) == length &&
            MultiByteToWideChar(CP_ACP, 0, result.data(), length, decoded.data(), size) == size && decoded == shortPath)
        {
            return result;
        }
        SetLastError(ERROR_NO_UNICODE_TRANSLATION);
        return {};
    }

    inline bool EqualPath(std::wstring_view left, std::wstring_view right)
    {
        if (left.starts_with(L"\\\\?\\"))
        {
            left.remove_prefix(4);
        }
        if (right.starts_with(L"\\\\?\\"))
        {
            right.remove_prefix(4);
        }
        return CompareStringOrdinal(left.data(), static_cast<int>(left.size()),
            right.data(), static_cast<int>(right.size()), TRUE) == CSTR_EQUAL;
    }

    inline std::wstring Environment(const wchar_t* name)
    {
        const DWORD length = GetEnvironmentVariableW(name, nullptr, 0);
        if (!length || length > MaxPathCharacters)
        {
            return {};
        }
        std::wstring result(length, L'\0');
        const DWORD copied = GetEnvironmentVariableW(name, result.data(), length);
        if (!copied || copied >= length)
        {
            return {};
        }
        result.resize(copied);
        return result;
    }

    inline std::wstring Quote(std::wstring_view value)
    {
        std::wstring result = L"\"";
        size_t slashes = 0;
        for (wchar_t ch : value)
        {
            if (ch == L'\\')
            {
                ++slashes;
                continue;
            }
            result.append(slashes * (ch == L'"' ? 2 : 1), L'\\');
            slashes = 0;
            if (ch == L'"')
            {
                result += L'\\';
            }
            result += ch;
        }
        result.append(slashes * 2, L'\\');
        return result + L'"';
    }

    class Handle
    {
    public:
        explicit Handle(HANDLE value = nullptr) noexcept : m_value(value) {}
        ~Handle()
        {
            if (m_value && m_value != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_value);
            }
        }
        Handle(const Handle&) = delete;
        Handle& operator=(const Handle&) = delete;
        void reset(HANDLE value)
        {
            if (m_value && m_value != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_value);
            }
            m_value = value;
        }
        HANDLE get() const noexcept { return m_value; }
        explicit operator bool() const noexcept { return m_value && m_value != INVALID_HANDLE_VALUE; }
    private:
        HANDLE m_value;
    };
}
