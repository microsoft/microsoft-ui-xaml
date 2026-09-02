// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include "pch.h"
#include "common.h"

#include <mutex>
#include <string>
#include <vector>

namespace TableViewDetails
{
    // Culture-aware integer formatting shared by the group-key stringifier (TableViewRow) and
    // the group-header count (TableViewGroupInfo).
    //
    // Only the *locale resolution* is cached -- GetUserDefaultLocaleName + Language::IsWellFormed
    // + GlobalizationPreferences::HomeGeographicRegion were previously re-run per call, on the UI
    // thread during measure. The DecimalFormatter itself is NOT shared: callers mutate
    // FractionDigits, so a shared instance would let one call site's digits leak into another's
    // output (and would race across XAML threads).
    struct GroupingLocale
    {
        std::vector<winrt::hstring> Languages;
        winrt::hstring Region;
        bool HasLanguages{ false };
    };

    inline GroupingLocale const& GetGroupingLocale() noexcept
    {
        static GroupingLocale s_locale;
        static std::once_flag s_onceFlag;

        std::call_once(
            s_onceFlag,
            []() noexcept
            {
                try
                {
                    WCHAR currentLocale[LOCALE_NAME_MAX_LENGTH] = {};
                    if (GetUserDefaultLocaleName(currentLocale, LOCALE_NAME_MAX_LENGTH) != 0)
                    {
                        // Strip any sort-order suffix (e.g. de-DE_phoneb), which is not a valid tag.
                        if (WCHAR* underscore = wcschr(currentLocale, L'_'))
                        {
                            *underscore = L'\0';
                        }

                        if (winrt::Language::IsWellFormed(currentLocale))
                        {
                            s_locale.Languages.push_back(winrt::hstring(currentLocale));
                            s_locale.Region = winrt::GlobalizationPreferences::HomeGeographicRegion();
                            s_locale.HasLanguages = true;
                        }
                    }
                }
                catch (...)
                {
                    s_locale.Languages.clear();
                    s_locale.HasLanguages = false;
                }
            });

        return s_locale;
    }

    // Returns a fresh formatter each call: the caller owns its FractionDigits/IntegerDigits.
    inline winrt::DecimalFormatter CreateCurrentCultureDecimalFormatter() noexcept
    {
        try
        {
            auto const& locale = GetGroupingLocale();

            auto formatter = locale.HasLanguages
                ? winrt::DecimalFormatter(locale.Languages, locale.Region)
                : winrt::DecimalFormatter();

            formatter.IntegerDigits(1);
            return formatter;
        }
        catch (...)
        {
            return nullptr;
        }
    }

    inline winrt::hstring FormatIntegerForCurrentCulture(int64_t value) noexcept
    {
        try
        {
            if (auto const formatter = CreateCurrentCultureDecimalFormatter())
            {
                formatter.FractionDigits(0);
                return formatter.FormatInt(value);
            }
        }
        catch (...)
        {
        }

        return winrt::hstring{ std::to_wstring(value) };
    }
}
