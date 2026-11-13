// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#pragma once

namespace TableViewDetails
{
    // Mirror the shipped XAML resources only when lookup fails or a column has no owner.
    inline constexpr double c_cellFontSizeFallback{ 14.0 };
    inline constexpr double c_headerFontSizeFallback{ 14.0 };
    inline constexpr double c_resizeGripperWidthFallback{ 8.0 };
    inline constexpr double c_sortIndicatorSizeFallback{ 16.0 };
    inline constexpr double c_defaultRowIndentSize{ 16.0 };
    inline constexpr double c_defaultRowExpanderSize{ 24.0 };
    inline constexpr double c_defaultGroupExpanderSize{ 12.0 };
    inline constexpr double c_defaultGroupExpanderSpacing{ 4.0 };

    struct DensityFallbackMetrics
    {
        double rowMinHeight;
        winrt::Thickness cellPadding;
        winrt::Thickness textPadding;
        winrt::Thickness groupHeaderPadding;
    };

    inline DensityFallbackMetrics const& GetDensityFallbackMetrics(
        winrt::TableViewDensity density = winrt::TableViewDensity::Standard)
    {
        static constexpr DensityFallbackMetrics medium{
            32.0, { 12, 6, 12, 6 }, { 12, 5, 12, 7 }, { 4, 6, 12, 6 } };
        static constexpr DensityFallbackMetrics large{
            40.0, { 12, 10, 12, 10 }, { 12, 9, 12, 11 }, { 4, 10, 12, 10 } };
        static constexpr DensityFallbackMetrics smallMetrics{
            24.0, { 12, 2, 12, 2 }, { 12, 1, 12, 3 }, { 4, 2, 12, 2 } };

        switch (density)
        {
        case winrt::TableViewDensity::Comfortable: return large;
        case winrt::TableViewDensity::Compact: return smallMetrics;
        default: return medium;
        }
    }
}
