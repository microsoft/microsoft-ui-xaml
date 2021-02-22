// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "BitmapIconSource.h"

winrt::IconElement BitmapIconSource::CreateIconElementCore()
{
    winrt::BitmapIcon bitmapIcon;

    if (UriSource())
    {
        bitmapIcon.UriSource(UriSource());
    }

    if (winrt::ApiInformation::IsPropertyPresent(L"Windows.UI.Xaml.Controls.BitmapIcon", L"ShowAsMonochrome"))
    {
        bitmapIcon.ShowAsMonochrome(ShowAsMonochrome());
    }
    if (const auto newForeground = Foreground())
    {
        bitmapIcon.Foreground(newForeground);
    }
    return bitmapIcon;
}
