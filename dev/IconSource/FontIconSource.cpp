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
