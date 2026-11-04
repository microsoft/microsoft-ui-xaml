// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "Utils.h"

#include <locale.h>
#include <memory>

winrt::hstring StringUtil::FormatDouble(double value)
{
    static const auto cLocale = []
    {
        std::unique_ptr<std::remove_pointer_t<_locale_t>, decltype(&_free_locale)> locale(
            _create_locale(LC_NUMERIC, "C"), &_free_locale);
        if (!locale)
        {
            winrt::throw_hresult(E_OUTOFMEMORY);
        }
        return locale;
    }();

    wchar_t buffer[32];
    // Keep ordinary values readable, increasing printf's default precision only when
    // necessary to round-trip. The dynamically linked UCRT avoids STL conversion tables.
    for (int precision = 6; precision <= std::numeric_limits<double>::max_digits10; ++precision)
    {
        const int length = _swprintf_s_l(buffer, ARRAYSIZE(buffer), L"%.*g", cLocale.get(), precision, value);
        if (length < 0)
        {
            winrt::throw_hresult(E_UNEXPECTED);
        }
        if (!std::isfinite(value) || _wcstod_l(buffer, nullptr, cLocale.get()) == value)
        {
            return winrt::hstring{ buffer, static_cast<uint32_t>(length) };
        }
    }

    winrt::throw_hresult(E_UNEXPECTED);
}

winrt::hstring StringUtil::FormatString(std::wstring_view formatString, ...)
{
    va_list pArgs;
    va_start(pArgs, formatString);
    
    LPVOID formattedString = nullptr;

    // Format the string
    const DWORD formattedLength = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_STRING,
        formatString.data(),
        0,
        0,
        (LPTSTR)&formattedString,
        0,
        &pArgs);

    va_end(pArgs);

    if (formattedLength == 0 || formattedString == nullptr)
    {
        LocalFree(formattedString);
        return {};
    }

    winrt::hstring result((LPTSTR)formattedString);
    LocalFree(formattedString);

    return result;
}

std::wstring StringUtil::Utf8ToUtf16(const std::string_view& utf8Str)
{
    std::wstring converted;
    if (utf8Str.size() > 0)
    {
        const int length = MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), nullptr, 0);
        if (length > 0)
        {
            converted.resize(length);
            if (MultiByteToWideChar(CP_UTF8, 0, utf8Str.data(), (int)utf8Str.size(), converted.data(), (int)converted.size()) == 0)
            {
                winrt::throw_last_error();
            }
        }
        else
        {
            winrt::throw_last_error();
        }
    }

    return converted;
}

std::string StringUtil::Utf16ToUtf8(const std::wstring_view& utf16Str)
{
    std::string converted;
    if (utf16Str.size() > 0)
    {
        const int length = WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), (int)utf16Str.size(), nullptr, 0, nullptr, nullptr);
        if (length > 0)
        {
            converted.resize(length);
            if (WideCharToMultiByte(CP_UTF8, 0, utf16Str.data(), (int)utf16Str.size(), converted.data(), (int)converted.size(), nullptr, nullptr) == 0)
            {
                winrt::throw_last_error();
            }
        }
        else
        {
            winrt::throw_last_error();
        }
    }

    return converted;
}

winrt::VisualStateGroup VisualStateUtil::GetVisualStateGroup(const winrt::FrameworkElement& control, const std::wstring_view& groupName)
{
    winrt::VisualStateGroup group{};
    auto visualStateGroups = winrt::VisualStateManager::GetVisualStateGroups(control);
    for (auto const& visualStateGroup : visualStateGroups)
    {
        if (visualStateGroup.Name() == groupName)
        {
            group = visualStateGroup;
            return group;
        }
    }
    return group;
}

bool VisualStateUtil::VisualStateGroupExists(const winrt::FrameworkElement& control, const std::wstring_view& groupName)
{
    return static_cast<bool>(GetVisualStateGroup(control, groupName));
}

void VisualStateUtil::GoToStateIfGroupExists(const winrt::Control& control, const std::wstring_view& groupName, const std::wstring_view& stateName, bool useTransitions)
{
    auto visualStateGroup = GetVisualStateGroup(control, groupName);
    if (visualStateGroup)
    {
        winrt::VisualStateManager::GoToState(control, stateName, useTransitions);
    }
}
