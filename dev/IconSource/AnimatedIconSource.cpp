// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"
#include "common.h"

#include "IconSource.h"
#include "AnimatedIconSource.h"

winrt::IconElement AnimatedIconSource::CreateIconElementCore()
{
    winrt::AnimatedIcon animatedIcon;
    if (auto const source = Source())
    {
        animatedIcon.Source(source);
    }
    if (auto const fallbackIconSource = FallbackIconSource())
    {
        animatedIcon.FallbackIconSource(fallbackIconSource);
    }
    if (const auto newForeground = Foreground())
    {
        animatedIcon.Foreground(newForeground);
    }
    return animatedIcon;
}
