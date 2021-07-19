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

    bitmapIcon.ShowAsMonochrome(ShowAsMonochrome());

    if (const auto newForeground = Foreground())
    {
        bitmapIcon.Foreground(newForeground);
    }
    return bitmapIcon;
}

winrt::DependencyProperty BitmapIconSource::GetIconElementPropertyCore(winrt::DependencyProperty sourceProperty)
{
    if (sourceProperty == s_ShowAsMonochromeProperty)
    {
        return winrt::BitmapIcon::ShowAsMonochromeProperty();
    }
    else if (sourceProperty == s_UriSourceProperty)
    {
        return winrt::BitmapIcon::UriSourceProperty();
    }

    return __super::GetIconElementPropertyCore(sourceProperty);
}
