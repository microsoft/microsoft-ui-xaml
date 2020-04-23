// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

#include <codecvt>
#include <string>

namespace StringUtil
{
    /// <summary>
    /// Formats a given string with the desired parameter list.
    /// </summary>
    /// <param name="formatString">The format string.</param>
    /// <param name="...">The parameter list of format args.</param>
    /// <returns>The formatted string.</returns>
    winrt::hstring FormatString(std::wstring_view formatString, ...);

    std::wstring Utf8ToUtf16(const std::string_view& utf8Str);
    std::string Utf16ToUtf8(const std::wstring_view& utf16Str);
}

class VisualStateUtil
{
public:
    static winrt::VisualStateGroup GetVisualStateGroup(const winrt::FrameworkElement& control, const std::wstring_view& groupName);
    static bool VisualStateGroupExists(const winrt::FrameworkElement& control, const std::wstring_view& groupName);
    static void GotToStateIfGroupExists(const winrt::Control& control, const std::wstring_view& groupName, const std::wstring_view& stateName, bool useTransitions);
};

namespace LayoutUtils
{
    template<class UIElementType>
    inline float MeasureAndGetDesiredWidthFor(UIElementType element, winrt::Size const& availableSize)
    {
        float desiredWidth = 0;
        if (element)
        {
            element.Measure(availableSize);
            desiredWidth = element.DesiredSize().Width;
        }
        return desiredWidth;
    }

    template<class UIElementType>
    inline double GetActualWidthFor(UIElementType element)
    {
        return (element ? element.ActualWidth() : 0);
    }
}

namespace Util
{
    inline winrt::Visibility constexpr VisibilityFromBool(bool visible) 
    { 
        return visible ? winrt::Visibility::Visible : winrt::Visibility::Collapsed;
    }
}
