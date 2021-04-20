// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "FontIconSource.h"

winrt::IconElement FontIconSource::CreateIconElementCore()
{
    winrt::FontIcon fontIcon;

    fontIcon.Glyph(Glyph());
    fontIcon.FontSize(FontSize());
    if (const auto newForeground = Foreground())
    {
        fontIcon.Foreground(newForeground);
    }

    if (FontFamily())
    {
        fontIcon.FontFamily(FontFamily());
    }

    fontIcon.FontWeight(FontWeight());
    fontIcon.FontStyle(FontStyle());
    fontIcon.IsTextScaleFactorEnabled(IsTextScaleFactorEnabled());
    fontIcon.MirroredWhenRightToLeft(MirroredWhenRightToLeft());

    return fontIcon;
}

winrt::DependencyProperty FontIconSource::GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty)
{
    if (sourceProperty == s_FontFamilyProperty)
    {
        return winrt::FontIcon::FontFamilyProperty();
    }
    else if (sourceProperty == s_FontSizeProperty)
    {
        return winrt::FontIcon::FontSizeProperty();
    }
    else if (sourceProperty == s_FontStyleProperty)
    {
        return winrt::FontIcon::FontStyleProperty();
    }
    else if (sourceProperty == s_FontWeightProperty)
    {
        return winrt::FontIcon::FontWeightProperty();
    }
    else if (sourceProperty == s_GlyphProperty)
    {
        return winrt::FontIcon::GlyphProperty();
    }
    else if (sourceProperty == s_IsTextScaleFactorEnabledProperty)
    {
        return winrt::FontIcon::IsTextScaleFactorEnabledProperty();
    }
    else if (sourceProperty == s_MirroredWhenRightToLeftProperty)
    {
        return winrt::FontIcon::MirroredWhenRightToLeftProperty();
    }

    return __super::GetIconElementPropertyCore(sourceProperty);
}
