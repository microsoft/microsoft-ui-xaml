// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "ImageIconSource.h"

winrt::IconElement ImageIconSource::CreateIconElementCore()
{
    winrt::ImageIcon imageIcon;
    if (const auto imageSource = ImageSource())
    {
        imageIcon.Source(imageSource);
    }
    if (const auto newForeground = Foreground())
    {
        imageIcon.Foreground(newForeground);
    }
    return imageIcon;
}
